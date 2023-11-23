/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * enginecontrol.cpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#define _GLIBCXX_USE_NANOSLEEP

#include "enginecontrol.hpp"
#include "uciprotocol.hpp"
#include "random.hpp"
#include "searchparams.hpp"
#include "search.hpp"
#include "book.hpp"
#include "textio.hpp"
#include "parameters.hpp"
#include "moveGen.hpp"
#include "logger.hpp"
#include "numa.hpp"
#include "cluster.hpp"
#include "clustertt.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <fstream>
#include <regex>
#include <climits>
#include <cmath>


EngineMainThread::EngineMainThread()
    : tt(256) {
    Communicator* clusterParent = Cluster::instance().createParentCommunicator(tt);
    comm = make_unique<ThreadCommunicator>(clusterParent, tt, notifier, true);
    Cluster::instance().createChildCommunicators(comm.get(), tt);
    Cluster::instance().connectAllReceivers(comm.get());
}

EngineMainThread::~EngineMainThread() {
}

void
EngineMainThread::mainLoop() {
    Numa::instance().bindThread(0);
    if (!Cluster::instance().isMasterNode()) {
        UciParams::hash->addListener([this]() {
            setupTT();
        });
        UciParams::clearHash->addListener([this]() {
            tt.clear();
        }, false);
        WorkerThread worker(0, nullptr, 1, tt);
        worker.mainLoopCluster(std::move(comm));
    } else {
        while (true) {
            notifierWait();
            if (quitFlag)
                break;
            setOptions();
            if (search) {
                doSearch();
                setOptions();
                {
                    std::lock_guard<std::mutex> L(mutex);
                    search = false;
                }
                searchStopped.notify_all();
            }
        }
        comm->sendQuit();
        class Handler : public Communicator::CommandHandler {
        public:
            explicit Handler(Communicator* comm) : comm(comm) {}
            void quitAck() override { comm->sendQuitAck(); }
        private:
            Communicator* comm;
        };
        Handler handler(comm.get());
        while (true) {
            comm->poll(handler);
            if (comm->hasQuitAck())
                break;
            notifierWait();
        }
    }
}

void
EngineMainThread::notifierWait() {
    if (Cluster::instance().isEnabled())
        notifier.wait(1);
    else
        notifier.wait();
}

void
EngineMainThread::quit() {
    std::lock_guard<std::mutex> L(mutex);
    quitFlag = true;
    notifier.notify();
}

void
EngineMainThread::setupTT() {
    int hashSizeMB = UciParams::hash->getIntPar();
    U64 nEntries = hashSizeMB > 0 ? ((U64)hashSizeMB) * (1 << 20) / sizeof(TranspositionTable::TTEntry)
                                  : (U64)1024;
    while (true) {
        try {
            if (nEntries < 1)
                break;
            tt.reSize(nEntries);
            break;
        } catch (const std::bad_alloc&) {
            nEntries /= 2;
        }
    }
}

void
EngineMainThread::startSearch(EngineControl* engineControl,
                              std::shared_ptr<Search>& sc, const Position& pos,
                              std::shared_ptr<MoveList>& moves,
                              bool ownBook, bool analyseMode,
                              int maxDepth, int maxNodes,
                              int maxPV, int minProbeDepth,
                              std::atomic<bool>& ponder, std::atomic<bool>& infinite) {
    int nThreads = UciParams::threads->getIntPar();
    if ((UciParams::strength->getIntPar() < 1000) ||
        (UciParams::maxNPS->getIntPar() > 0) ||
        UciParams::limitStrength->getBoolPar())
        nThreads = 1;
    int nThreadsThisNode;
    std::vector<int> nThreadsChildren;
    Cluster::instance().assignThreads(nThreads, nThreadsThisNode, nThreadsChildren);
    comm->sendAssignThreads(nThreadsThisNode, nThreadsChildren);
    WorkerThread::createWorkers(1, comm.get(), nThreadsThisNode - 1, tt, children);

    {
        std::lock_guard<std::mutex> L(mutex);
        this->engineControl = engineControl;
        this->sc = sc;
        this->pos = pos;
        this->moves = moves;
        this->ownBook = ownBook;
        this->analyseMode = analyseMode;
        this->maxDepth = maxDepth;
        this->maxNodes = maxNodes;
        this->maxPV = maxPV;
        this->minProbeDepth = minProbeDepth;
        this->ponder = &ponder;
        this->infinite = &infinite;
        search = true;
    }
    notifier.notify();
}

void
EngineMainThread::waitStop() {
    std::unique_lock<std::mutex> L(mutex);
    while (search)
        searchStopped.wait(L);
    sc.reset();
}

void
EngineMainThread::setOptionWhenIdle(const std::string& optionName,
                                    const std::string& optionValue) {
    {
        std::lock_guard<std::mutex> L(mutex);
        Parameters& params = Parameters::instance();
        if (params.getParam(optionName)) {
            pendingOptions[optionName] = optionValue;
            optionsSetFinished = false;
        }
    }
    notifier.notify();
}

void
EngineMainThread::waitOptionsSet() {
    std::unique_lock<std::mutex> L(mutex);
    while (!optionsSetFinished)
        optionsSet.wait(L);
}

void
EngineMainThread::doSearch() {
    Move m;
    if (ownBook && !analyseMode && !*infinite) {
        Book book(false);
        book.getBookMove(pos, m);
    }

    bool waitForStop = false;
    if (m.isEmpty()) {
        m = sc->iterativeDeepening(*moves, maxDepth, maxNodes, maxPV, false,
                                   minProbeDepth, clearHistory);
        waitForStop = true;
    }
    clearHistory = false;
    while (*ponder || *infinite) {
        // We should not respond until told to do so.
        // Just wait until we are allowed to respond.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    engineControl->finishSearch(pos, m);

    if (waitForStop) {
        comm->sendStopSearch();
        class Handler : public Communicator::CommandHandler {
        public:
            explicit Handler(Communicator* comm) : comm(comm) {}
            void stopAck() override { comm->sendStopAck(true); }
        private:
            Communicator* comm;
        };
        Handler handler(comm.get());
        comm->sendStopAck(false);
        while (true) {
            comm->poll(handler);
            if (comm->hasStopAck())
                break;
            notifierWait();
        }
        notifier.notify();
    }
}

void
EngineMainThread::setOptions() {
    while (true) {
        std::map<std::string, std::string> options;
        {
            std::lock_guard<std::mutex> L(mutex);
            options.swap(pendingOptions);
            if (options.empty()) {
                optionsSetFinished = true;
                optionsSet.notify_all();
                return;
            }
        }

        Parameters& params = Parameters::instance();
        for (auto& p : options) {
            const std::string& optionName = p.first;
            std::string optionValue = p.second;
            std::shared_ptr<Parameters::ParamBase> par = params.getParam(optionName);
            if (par && par->getType() == Parameters::STRING && optionValue == "<empty>")
                optionValue.clear();
            params.set(optionName, optionValue);
            comm->sendSetParam(optionName, optionValue);
        }
    }
}

// ----------------------------------------------------------------------------

EngineControl::EngineControl(std::ostream& o, EngineMainThread& engineThread0,
                             SearchListener& listener)
    : os(o), engineThread(engineThread0), listener(listener), randomSeed(0) {
    Numa::instance().bindThread(0);
    hashParListenerId = UciParams::hash->addListener([this]() {
        engineThread.setupTT();
    });
    clearHashParListenerId = UciParams::clearHash->addListener([this]() {
        engineThread.getTT().clear();
        ht.init();
        engineThread.setClearHistory();
    }, false);
    opponentParListenerId = UciParams::opponent->addListener([this]() {
        setOpponent();
    });
    contemptFileParListenerId = UciParams::contemptFile->addListener([this]() {
        setOpponent();
    }, false);

    et = Evaluate::getEvalHashTables();
}

EngineControl::~EngineControl() {
    UciParams::hash->removeListener(hashParListenerId);
    UciParams::clearHash->removeListener(clearHashParListenerId);
    UciParams::opponent->removeListener(opponentParListenerId);
    UciParams::contemptFile->removeListener(contemptFileParListenerId);
}

void
EngineControl::startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
    stopThread();
    setupPosition(pos, moves);
    computeTimeLimit(sPar);
    ponder = false;
    infinite = (maxTimeLimit < 0) && (maxDepth < 0) && (maxNodes < 0);
    searchMoves = sPar.searchMoves;
    startThread(minTimeLimit, maxTimeLimit, earlyStopPercentage, maxDepth, maxNodes);
}

void
EngineControl::startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
    stopThread();
    setupPosition(pos, moves);
    computeTimeLimit(sPar);
    ponder = true;
    infinite = false;
    startThread(-1, -1, -1, -1, -1);
}

void
EngineControl::ponderHit() {
    if (sc) {
        if (onePossibleMove) {
            if (minTimeLimit > 1) minTimeLimit = 1;
            if (maxTimeLimit > 1) maxTimeLimit = 1;
        }
        sc->timeLimit(minTimeLimit, maxTimeLimit, earlyStopPercentage);
    }
    infinite = (maxTimeLimit < 0) && (maxDepth < 0) && (maxNodes < 0);
    ponder = false;
}

void
EngineControl::stopSearch() {
    stopThread();
}

void
EngineControl::newGame() {
    randomSeed = Random().nextU64();
    setOption("Clear Hash", "");
}

void
EngineControl::computeTimeLimit(const SearchParams& sPar) {
    minTimeLimit = -1;
    maxTimeLimit = -1;
    earlyStopPercentage = -1;
    maxDepth = -1;
    maxNodes = -1;
    if (sPar.infinite) {
        minTimeLimit = -1;
        maxTimeLimit = -1;
        maxDepth = -1;
    } else {
        if (sPar.depth > 0)
            maxDepth = sPar.depth;
        if (sPar.mate > 0) {
            int md = sPar.mate * 2 - 1;
            maxDepth = maxDepth == -1 ? md : std::min(maxDepth, md);
        }
        if (sPar.nodes > 0)
            maxNodes = sPar.nodes;

        if (sPar.moveTime > 0) {
             minTimeLimit = maxTimeLimit = sPar.moveTime;
             earlyStopPercentage = 10000; // Don't stop search early if asked to search a fixed amount of time
        } else if (sPar.wTime || sPar.bTime) {
            int moves = sPar.movesToGo;
            if (moves == 0)
                moves = 999;
            moves = std::min(moves, static_cast<int>(timeMaxRemainingMoves)); // Assume at most N more moves until end of game
            bool white = pos.isWhiteMove();
            int time = white ? sPar.wTime : sPar.bTime;
            int inc  = white ? sPar.wInc : sPar.bInc;
            const int margin = std::min(static_cast<int>(bufferTime), time * 9 / 10);
            int timeLimit = (time + inc * (moves - 1) - margin) / moves;
            minTimeLimit = timeLimit;
            if (UciParams::ponder->getBoolPar()) {
                int oTime = white ? sPar.bTime : sPar.wTime;
                int oInc  = white ? sPar.bInc : sPar.wInc;
                double oTimeLimit = (oTime + oInc * (moves - 1) - margin) / moves;
                double k = timePonderHitRate * 0.01;
                minTimeLimit += (int)(std::min(oTimeLimit, timeLimit / (1 - k)) * k);
            }
            maxTimeLimit = (int)(minTimeLimit * clamp(moves * 0.5, 2.0, maxTimeUsage * 0.01));

            // Leave at least 1s on the clock, but can't use negative time
            minTimeLimit = clamp(minTimeLimit, 1, time - margin);
            maxTimeLimit = clamp(maxTimeLimit, 1, time - margin);
        }
    }
}

void EngineControl::setOpponent() {
    opponentBasedContempt = 0;
    std::string opponent = UciParams::opponent->getStringPar();
    std::ifstream is(UciParams::contemptFile->getStringPar());
    try {
        while (true) {
            std::string line;
            std::getline(is, line);
            if (!is || is.eof())
                break;
            if (startsWith(line, "#"))
                continue;
            size_t tabPos = line.find('\t');
            if (tabPos == std::string::npos)
                continue;

            std::regex re(line.substr(0, tabPos), std::regex::icase);
            if (std::regex_match(opponent, re)) {
                std::string sVal = trim(line.substr(tabPos+1));
                int val = 0;
                if (str2Num(sVal, val)) {
                    opponentBasedContempt = val;
                    return;
                }
            }
        }
    } catch (const std::regex_error&) {
        os << "info string error parsing contempt file" << std::endl;
    }
}

int EngineControl::getWhiteContempt(bool whiteMove) {
    if (UciParams::analyseMode->getBoolPar())
        return UciParams::analyzeContempt->getIntPar();
    int contempt;
    if (UciParams::autoContempt->getBoolPar()) {
        contempt = opponentBasedContempt;
    } else {
        contempt = UciParams::contempt->getIntPar();
    }
    int ret = whiteMove ? contempt : -contempt;
    return ret;
}

void
EngineControl::startThread(int minTimeLimit, int maxTimeLimit, int earlyStopPercentage,
                           int maxDepth, int maxNodes) {
    Communicator* comm = engineThread.getCommunicator();
    Search::SearchTables st(comm->getCTT(), kt, ht, *et);
    sc = std::make_shared<Search>(pos, posHashList, posHashListSize, st, *comm, treeLog);
    sc->setListener(listener);
    sc->setStrength(getStrength(), randomSeed, getMaxNPS());
    std::shared_ptr<MoveList> moves(std::make_shared<MoveList>());
    MoveGen::pseudoLegalMoves(pos, *moves);
    MoveGen::removeIllegal(pos, *moves);
    if (searchMoves.size() > 0)
        moves->filter(searchMoves);
    onePossibleMove = false;
    if ((moves->size < 2) && !infinite) {
        onePossibleMove = true;
        if (!ponder) {
            if (maxTimeLimit > 0) {
                maxTimeLimit = clamp(maxTimeLimit/100, 1, 100);
                minTimeLimit = clamp(minTimeLimit/100, 1, 100);
            } else {
                if ((maxDepth < 0) || (maxDepth > 2))
                    maxDepth = 2;
            }
        }
    }
    sc->timeLimit(minTimeLimit, maxTimeLimit, earlyStopPercentage);
    bool ownBook = UciParams::ownBook->getBoolPar();
    bool analyseMode = UciParams::analyseMode->getBoolPar();
    int maxPV = UciParams::multiPV->getIntPar();
    int minProbeDepth = UciParams::minProbeDepth->getIntPar();
    int whiteContempt = getWhiteContempt(pos.isWhiteMove());
    sc->setWhiteContempt(whiteContempt);
    if (analyseMode || infinite) {
        Position pos2(pos);
        auto et2 = Evaluate::getEvalHashTables();
        Evaluate eval(*et2);
        eval.connectPosition(pos2);
        eval.setWhiteContempt(whiteContempt);
        int evScore = eval.evalPosPrint() * (pos2.isWhiteMove() ? 1 : -1);
        std::stringstream ss;
        ss.precision(2);
        ss << std::fixed << (evScore / 100.0);
        os << "info string eval total  :" << ss.str() << std::endl;
        if (UciParams::analysisAgeHash->getBoolPar())
            engineThread.getTT().nextGeneration();
    } else {
        engineThread.getTT().nextGeneration();
    }
    engineThread.startSearch(this, sc, pos, moves, ownBook, analyseMode, maxDepth,
                             maxNodes, maxPV, minProbeDepth, ponder, infinite);
}

void
EngineControl::stopThread() {
    if (sc)
        sc->timeLimit(0, 0);
    infinite = false;
    ponder = false;
    engineThread.waitStop();
    engineThread.waitOptionsSet();
}

void
EngineControl::setupPosition(Position pos, const std::vector<Move>& moves) {
    UndoInfo ui;
    posHashList.clear();
    for (const Move& m : moves) {
        posHashList.push_back(pos.zobristHash());
        pos.makeMove(m, ui);
        if (pos.getHalfMoveClock() == 0)
            posHashList.clear();
    }
    if (posHashList.size() > 100) {
        // If more than 100 reversible moves have been played, a draw by the 50 move
        // rule can be claimed, so posHashList is not needed, since it is only used
        // to claim three-fold repetition draws.
        posHashList.clear();
    }
    posHashListSize = posHashList.size();
    posHashList.resize(posHashListSize + SearchConst::MAX_SEARCH_DEPTH * 2);
    this->pos = pos;
}

Move
EngineControl::getPonderMove(Position pos, const Move& m) {
    Move ret;
    if (m.isEmpty())
        return ret;
    UndoInfo ui;
    pos.makeMove(m, ui);
    TranspositionTable::TTEntry ent;
    engineThread.getTT().probe(pos.historyHash(), ent);
    if (ent.getType() != TType::T_EMPTY) {
        ent.getMove(ret);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool contains = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi] == ret) {
                contains = true;
                break;
            }
        if  (!contains)
            ret = Move();
    }
    pos.unMakeMove(m, ui);
    return ret;
}

void
EngineControl::printOptions(std::ostream& os) {
    std::vector<std::string> parNames;
    Parameters::instance().getParamNames(parNames);
    for (const auto& pName : parNames) {
        std::shared_ptr<Parameters::ParamBase> p = Parameters::instance().getParam(pName);
        switch (p->getType()) {
        case Parameters::CHECK: {
            const Parameters::CheckParam& cp = static_cast<const Parameters::CheckParam&>(*p.get());
            os << "option name " << cp.getName() << " type check default "
               << (cp.getDefaultValue() ? "true" : "false") << std::endl;
            break;
        }
        case Parameters::SPIN: {
            const Parameters::SpinParam& sp = static_cast<const Parameters::SpinParam&>(*p.get());
            os << "option name " << sp.getName() << " type spin default "
               << sp.getDefaultValue() << " min " << sp.getMinValue()
               << " max " << sp.getMaxValue() << std::endl;
            break;
        }
        case Parameters::COMBO: {
            const Parameters::ComboParam& cp = static_cast<const Parameters::ComboParam&>(*p.get());
            os << "option name " << cp.getName() << " type combo default " << cp.getDefaultValue();
            for (size_t i = 0; i < cp.getAllowedValues().size(); i++)
                os << " var " << cp.getAllowedValues()[i];
            os << std::endl;
            break;
        }
        case Parameters::BUTTON:
            os << "option name " << p->getName() << " type button" << std::endl;
            break;
        case Parameters::STRING: {
            const Parameters::StringParam& sp = static_cast<const Parameters::StringParam&>(*p.get());
            os << "option name " << sp.getName() << " type string default "
               << sp.getDefaultValue() << std::endl;
            break;
        }
        }
    }
}

void
EngineControl::setOption(const std::string& optionName, const std::string& optionValue) {
    engineThread.setOptionWhenIdle(optionName, optionValue);
}

void
EngineControl::waitReady() {
    if (!sc)
        engineThread.waitOptionsSet();
}

void
EngineControl::finishSearch(Position& pos, const Move& bestMove) {
    Move ponderMove = getPonderMove(pos, bestMove);
    listener.notifyPlayedMove(bestMove, ponderMove);
}

static int eloToStrength[][2] = {
    {  -625,   0 },
    {  -574,  15 },
    {  -458,  30 },
    {  -271,  45 },
    {   -57,  60 },
    {   140,  75 },
    {   416, 100 },
    {   618, 125 },
    {   749, 150 },
    {   965, 200 },
    {  1037, 250 },
    {  1188, 300 },
    {  1467, 350 },
    {  1778, 425 },
    {  1992, 500 },
    {  2245, 600 },
    {  2457, 700 },
    {  2629, 800 },
    {  2733, 875 },
    {  2838, 950 },
    {  2872, 975 },
    {  2900, 990 },
};

int
EngineControl::getStrength() const {
    if (!UciParams::limitStrength->getBoolPar())
        return UciParams::strength->getIntPar();

    const int elo = UciParams::elo->getIntPar();
    if (elo <= eloToStrength[0][0])
        return eloToStrength[0][1];
    const int n = COUNT_OF(eloToStrength);
    for (int i = 1; i < n; i++) {
        if (elo <= eloToStrength[i][0]) {
            double a  = eloToStrength[i-1][0];
            double b  = eloToStrength[i  ][0];
            double fa = eloToStrength[i-1][1];
            double fb = eloToStrength[i  ][1];
            return (int)round(fa + (elo - a) / (b - a) * (fb - fa));
        }
    }
    return eloToStrength[n-1][1];
}

int
EngineControl::getMaxNPS() const {
    int maxNPS = UciParams::maxNPS->getIntPar();
    int nps1 = maxNPS == 0 ? INT_MAX : maxNPS;
    int nps2 = nps1;
    if (UciParams::limitStrength->getBoolPar()) {
        int elo = UciParams::elo->getIntPar();
        if (elo < 1350)
            nps2 = 10000;
        else if (elo < 2100)
            nps2 = 100000;
        else
            nps2 = 750000;
    }
    int nps = std::min(nps1, nps2);
    return nps == INT_MAX ? 0 : nps;
}
