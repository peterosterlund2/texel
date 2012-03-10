/*
 * enginecontrol.cpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#include "enginecontrol.hpp"
#include "random.hpp"
#include "searchparams.hpp"
#include "computerPlayer.hpp"
#include "textio.hpp"
#include "parameters.hpp"
#include "moveGen.hpp"

#include <iostream>
#include <memory>


EngineControl::SearchListener::SearchListener(std::ostream& os0)
    : os(os0)
{
}

void
EngineControl::SearchListener::notifyDepth(int depth) {
    os << "info depth " << depth << std::endl;
}

void
EngineControl::SearchListener::notifyCurrMove(const Move& m, int moveNr) {
    os << "info currmove " << moveToString(m) << " currmovenumber " << moveNr << std::endl;
}

void
EngineControl::SearchListener::notifyPV(int depth, int score, int time, U64 nodes, int nps, bool isMate,
                                        bool upperBound, bool lowerBound, const std::vector<Move>& pv) {
    std::string pvBuf;
    for (size_t i = 0; i < pv.size(); i++) {
        pvBuf += ' ';
        pvBuf += moveToString(pv[i]);
    }
    std::string bound;
    if (upperBound) {
        bound = " upperbound";
    } else if (lowerBound) {
        bound = " lowerbound";
    }
    os << "info depth " << depth << " score " << (isMate ? "mate " : "cp ")
       << score << bound << " time " << time << " nodes " << nodes
       << " nps " << nps << " pv" << pvBuf << std::endl;
}

void
EngineControl::SearchListener::notifyStats(U64 nodes, int nps, int time) {
    os << "info nodes " << nodes << " nps " << nps << " time " << time << std::endl;
}

EngineControl::EngineControl(std::ostream& o)
    : os(o),
      tt(8),
      hashSizeMB(16),
      ownBook(false),
      analyseMode(false),
      ponderMode(true),
      strength(1000),
      randomSeed(0)
{
    setupTT();
}

void
EngineControl::startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
    setupPosition(pos, moves);
    computeTimeLimit(sPar);
    ponder = false;
    infinite = (maxTimeLimit < 0) && (maxDepth < 0) && (maxNodes < 0);
    searchMoves = sPar.searchMoves;
    startThread(minTimeLimit, maxTimeLimit, maxDepth, maxNodes);
}

void
EngineControl::startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
    setupPosition(pos, moves);
    computeTimeLimit(sPar);
    ponder = true;
    infinite = false;
    startThread(-1, -1, -1, -1);
}

void
EngineControl::ponderHit() {
    std::shared_ptr<Search> mySearch;
#if 0
    synchronized (threadMutex) {
        mySearch = sc;
    }
#endif
    if (mySearch) {
        if (onePossibleMove) {
            if (minTimeLimit > 1) minTimeLimit = 1;
            if (maxTimeLimit > 1) maxTimeLimit = 1;
        }
        mySearch->timeLimit(minTimeLimit, maxTimeLimit);
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
    tt.clear();
}

/**
 * Compute thinking time for current search.
 */
void
EngineControl::computeTimeLimit(const SearchParams& sPar) {
    minTimeLimit = -1;
    maxTimeLimit = -1;
    maxDepth = -1;
    maxNodes = -1;
    if (sPar.infinite) {
        minTimeLimit = -1;
        maxTimeLimit = -1;
        maxDepth = -1;
    } else if (sPar.depth > 0) {
        maxDepth = sPar.depth;
    } else if (sPar.mate > 0) {
        maxDepth = sPar.mate * 2 - 1;
    } else if (sPar.moveTime > 0) {
        minTimeLimit = maxTimeLimit = sPar.moveTime;
    } else if (sPar.nodes > 0) {
        maxNodes = sPar.nodes;
    } else {
        int moves = sPar.movesToGo;
        if (moves == 0) {
            moves = 999;
        }
        moves = std::min(moves, 45); // Assume 45 more moves until end of game
        if (ponderMode) {
            const double ponderHitRate = 0.35;
            moves = (int)ceil(moves * (1 - ponderHitRate));
        }
        bool white = pos.whiteMove;
        int time = white ? sPar.wTime : sPar.bTime;
        int inc  = white ? sPar.wInc : sPar.bInc;
        const int margin = std::min(1000, time * 9 / 10);
        int timeLimit = (time + inc * (moves - 1) - margin) / moves;
        minTimeLimit = (int)(timeLimit * 0.85);
        maxTimeLimit = (int)(minTimeLimit * (std::max(2.5, std::min(4.0, moves / 2.0))));

        // Leave at least 1s on the clock, but can't use negative time
        minTimeLimit = clamp(minTimeLimit, 1, time - margin);
        maxTimeLimit = clamp(maxTimeLimit, 1, time - margin);
    }
}

int
EngineControl::clamp(int val, int min, int max) {
    if (val < min)
        return min;
    else if (val > max)
        return max;
    else
        return val;
}

void
EngineControl::startThread(int minTimeLimit, int maxTimeLimit, int maxDepth, int maxNodes) {
#if 0
    synchronized (threadMutex) {} // Must not start new search until old search is finished
    sc = std::make_shared<Search>(pos, posHashList, posHashListSize, tt);
    sc->timeLimit(minTimeLimit, maxTimeLimit);
    sc->setListener(std::make_shared<SearchListener>(os));
    sc->setStrength(strength, randomSeed);
    MoveGen::MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    if (searchMoves.size() > 0)
        moves.filter(searchMoves);
    MoveGen::MoveList srchMoves = moves;
    onePossibleMove = false;
    if ((srchMoves.size < 2) && !infinite) {
        onePossibleMove = true;
        if (!ponder)
            if ((maxDepth < 0) || (maxDepth > 2))
                maxDepth = 2;
    }
    tt.nextGeneration();
    const int srchmaxDepth = maxDepth;
    engineThread = new Thread(new Runnable() {
    public void run() {
        Move m;
        if (ownBook && !analyseMode) {
            Book book(false);
            m = book.getBookMove(pos);
        }
        if (m.isEmpty())
            m = sc.iterativeDeepening(srchMoves, srchmaxDepth, maxNodes, false);
        while (ponder || infinite) {
            // We should not respond until told to do so. Just wait until
            // we are allowed to respond.
            try {
                Thread.sleep(10);
            } catch (InterruptedException ex) {
                break;
            }
        }
        Move ponderMove = getPonderMove(pos, m);
        synchronized (threadMutex) {
            os << "bestmove " << moveToString(m);
            if (ponderMove != null)
                os << " ponder " << moveToString(ponderMove);
            os << std::endl;
            engineThread = null;
            sc = null;
        }
    }
    });
    engineThread.start();
#endif
}

void
EngineControl::stopThread() {
#if 0
    std::shared_ptr<Thread> myThread;
    std::shared_ptr<Search> mySearch;
    synchronized (threadMutex) {
        myThread = engineThread;
        mySearch = sc;
    }
    if (myThread) {
        mySearch->timeLimit(0, 0);
        infinite = false;
        ponder = false;
        myThread->join();
    }
#endif
}

void
EngineControl::setupTT() {
    int nEntries = hashSizeMB > 0 ? hashSizeMB * (1 << 20) / 24 : 1024;
    int logSize = (int) floor(log(nEntries) / log(2.0));
    tt.reSize(logSize);
}

void
EngineControl::setupPosition(Position pos, const std::vector<Move>& moves) {
    UndoInfo ui;
    posHashList.resize(200 + moves.size());
    posHashListSize = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        const Move& m = moves[i];
        posHashList[posHashListSize++] = pos.zobristHash();
        pos.makeMove(m, ui);
    }
    this->pos = pos;
}

/**
 * Try to find a move to ponder from the transposition table.
 */
Move
EngineControl::getPonderMove(Position pos, const Move& m) {
    Move ret;
    if (m.isEmpty())
        return ret;
    UndoInfo ui;
    pos.makeMove(m, ui);
    TranspositionTable::TTEntry ent;
    tt.probe(pos.historyHash(), ent);
    if (ent.type != TType::T_EMPTY) {
        ent.getMove(ret);
        MoveGen::MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool contains = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves.m[mi].equals(ret)) {
                contains = true;
                break;
            }
        if  (!contains)
            ret = Move();
    }
    pos.unMakeMove(m, ui);
    return ret;
}

std::string
EngineControl::moveToString(const Move& m) {
    if (m.isEmpty())
        return "0000";
    std::string ret = TextIO::squareToString(m.from());
    ret += TextIO::squareToString(m.to());
    switch (m.promoteTo()) {
    case Piece::WQUEEN:
    case Piece::BQUEEN:
        ret += 'q';
        break;
    case Piece::WROOK:
    case Piece::BROOK:
        ret += 'r';
        break;
    case Piece::WBISHOP:
    case Piece::BBISHOP:
        ret += 'b';
        break;
    case Piece::WKNIGHT:
    case Piece::BKNIGHT:
        ret += 'n';
        break;
    default:
        break;
    }
    return ret;
}

void
EngineControl::printOptions(std::ostream& os) {
    os << "option name Hash type spin default 16 min 1 max 2048" << std::endl;
    os << "option name OwnBook type check default false" << std::endl;
    os << "option name Ponder type check default true" << std::endl;
    os << "option name UCI_AnalyseMode type check default false" << std::endl;
    os << "option name UCI_EngineAbout type string default " << ComputerPlayer::engineName
            << " by Peter Osterlund, see http://web.comhem.se/petero2home/javachess/index.html" << std::endl;
    os << "option name Strength type spin default 1000 min 0 max 1000" << std::endl;


    std::vector<std::string> parNames;
    Parameters::instance().getParamNames(parNames);
    for (size_t i = 0; i < parNames.size(); i++) {
        std::string pName = parNames[i];
        std::shared_ptr<Parameters::ParamBase> p = Parameters::instance().getParam(pName);
        switch (p->type) {
        case Parameters::CHECK: {
            const Parameters::CheckParam& cp = static_cast<const Parameters::CheckParam&>(*p.get());
            os << "optionn name " << cp.name << " type check default "
                    << (cp.defaultValue?"true":"false") << std::endl;
            break;
        }
        case Parameters::SPIN: {
            const Parameters::SpinParam& sp = static_cast<const Parameters::SpinParam&>(*p.get());
            os << "option name " << sp.name << " type spin default "
                    << sp.defaultValue << " min " << sp.minValue
                    << " max " << sp.maxValue << std::endl;
            break;
        }
        case Parameters::COMBO: {
            const Parameters::ComboParam& cp = static_cast<const Parameters::ComboParam&>(*p.get());
            os << "option name " << cp.name << " type combo default " << cp.defaultValue;
            for (size_t i = 0; i < cp.allowedValues.size(); i++)
                os << " var " << cp.allowedValues[i];
            os << std::endl;
            break;
        }
        case Parameters::BUTTON:
            os << "option name " << p->name << " type button" << std::endl;
            break;
        case Parameters::STRING: {
            const Parameters::StringParam& sp = static_cast<const Parameters::StringParam&>(*p.get());
            os << "option name " << sp.name << " type string default "
                    << sp.defaultValue << std::endl;
            break;
        }
        }
    }
}

void
EngineControl::setOption(const std::string& optionName, const std::string& optionValue) {
    if (optionName == "hash") {
        str2Num(optionValue, hashSizeMB);
        setupTT();
    } else if (optionName == "ownbook") {
        ownBook = (toLowerCase(optionValue) == "true");
    } else if (optionName == "ponder") {
        ponderMode = (toLowerCase(optionValue) == "true");
    } else if (optionName == "uci_analysemode") {
        analyseMode = (toLowerCase(optionValue) == "true");
    } else if (optionName == "strength") {
        str2Num(optionValue, strength);
    } else {
        Parameters::instance().set(optionName, optionValue);
    }
}
