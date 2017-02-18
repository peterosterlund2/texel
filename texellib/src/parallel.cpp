/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * parallel.cpp
 *
 *  Created on: Jul 9, 2013
 *      Author: petero
 */

#include "parallel.hpp"
#include "numa.hpp"
#include "search.hpp"
#include "tbprobe.hpp"
#include "textio.hpp"
#include "util/logger.hpp"

#include <cmath>
#include <cassert>

using namespace Logger;


void
Notifier::notify() {
    std::unique_lock<std::mutex> L(mutex);
    notified = true;
    cv.notify_all();
}

void
Notifier::wait() {
    std::unique_lock<std::mutex> L(mutex);
    while (!notified)
        cv.wait(L);
    notified = false;
}

// ----------------------------------------------------------------------------

Communicator::Communicator(Communicator* parent)
    : parent(parent) {
    if (parent)
        parent->addChild(this);
}

Communicator::~Communicator() {
    if (parent)
        parent->removeChild(this);
}

void
Communicator::addChild(Communicator* child) {
    std::unique_lock<std::mutex> L(mutex);
    children.push_back(child);
}

void
Communicator::removeChild(Communicator* child) {
    std::unique_lock<std::mutex> L(mutex);
    children.erase(std::remove(children.begin(), children.end(), child),
                   children.end());
}

void
Communicator::sendInitSearch(History& ht) {
    nodesSearched = 0;
    tbHits = 0;
    for (auto& c : children)
        c->doSendInitSearch(ht);
}

void
Communicator::sendStartSearch(int jobId, const Position& pos, int alpha, int beta,
                              int depth, KillerTable& kt) {
    this->jobId = jobId;
    for (auto& c : children)
        c->doSendStartSearch(jobId, pos, alpha, beta, depth, kt);
}

void
Communicator::sendStopSearch(int jobId) {
    for (auto& c : children)
        c->doSendStopSearch(jobId);
}


void
Communicator::sendReportResult(int jobId, int score) {
    if (parent)
        parent->doSendReportResult(jobId, score);
}

void
Communicator::sendReportStats(S64 nodesSearched, S64 tbHits) {
    if (parent) {
        retrieveStats(nodesSearched, tbHits);
        parent->doSendReportStats(nodesSearched, tbHits);
    } else {
        doSendReportStats(nodesSearched, tbHits);
    }
}

void
Communicator::poll(CommandHandler& handler) {
    if (parent)
        parent->doPoll();
    for (auto& c : children)
        c->doPoll();

    while (true) {
        std::unique_lock<std::mutex> L(mutex);
        if (cmdQueue.empty())
            break;
        Command cmd = cmdQueue.front();
        cmdQueue.pop_front();
        L.unlock();

        switch (cmd.type) {
        case CommandType::INIT_SEARCH:
            handler.initSearch(*ht);
            break;
        case CommandType::START_SEARCH: {
            Position pos;
            L.lock();
            pos.deSerialize(posData);
            KillerTable kt(this->kt);
            this->hasResult = false;
            L.unlock();
            jobId = cmd.jobId;
            handler.startSearch(cmd.jobId, pos, cmd.alpha, cmd.beta, cmd.depth, kt);
            break;
        }
        case CommandType::STOP_SEARCH:
            handler.stopSearch(cmd.jobId);
            break;
        case CommandType::REPORT_RESULT: {
            if (jobId == cmd.jobId) {
                L.lock();
                bool hasResult = this->hasResult;
                int resultScore = this->resultScore;
                L.unlock();
                if (hasResult)
                    handler.reportResult(cmd.jobId, resultScore);
            }
            break;
        }
        }
    }
}

// ----------------------------------------------------------------------------

ThreadCommunicator::ThreadCommunicator(Communicator* parent, Notifier& notifier)
    : Communicator(parent), notifier(notifier) {
}

void
ThreadCommunicator::doSendInitSearch(History& ht) {
    std::unique_lock<std::mutex> L(mutex);
    this->ht = &ht;
    cmdQueue.push_back(Command{CommandType::INIT_SEARCH, -1, -1, -1, -1});
    notifier.notify();
}

void
ThreadCommunicator::doSendStartSearch(int jobId, const Position& pos, int alpha, int beta,
                                      int depth, KillerTable& kt) {
    std::unique_lock<std::mutex> L(mutex);
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const Command& cmd) {
                                      return cmd.type == CommandType::START_SEARCH ||
                                             cmd.type == CommandType::STOP_SEARCH ||
                                             cmd.type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    pos.serialize(posData);
    this->kt = kt;
    cmdQueue.push_back(Command{CommandType::START_SEARCH, jobId, alpha, beta, depth});
    notifier.notify();
}

void
ThreadCommunicator::doSendStopSearch(int jobId) {
    std::unique_lock<std::mutex> L(mutex);
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const Command& cmd) {
                                      return cmd.type == CommandType::START_SEARCH ||
                                             cmd.type == CommandType::STOP_SEARCH ||
                                             cmd.type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    cmdQueue.push_back(Command{CommandType::STOP_SEARCH, jobId, -1, -1, -1});
    notifier.notify();
}

void
ThreadCommunicator::doSendReportResult(int jobId, int score) {
    std::unique_lock<std::mutex> L(mutex);
    if (hasResult)
        return;
    hasResult = true;
    resultScore = score;
    cmdQueue.push_back(Command{CommandType::REPORT_RESULT, jobId, -1, -1, -1});
    notifier.notify();
}

void
ThreadCommunicator::doSendReportStats(S64 nodesSearched, S64 tbHits) {
    std::unique_lock<std::mutex> L(mutex);
    this->nodesSearched += nodesSearched;
    this->tbHits += tbHits;
}

void
ThreadCommunicator::retrieveStats(S64& nodesSearched, S64& tbHits) {
    std::unique_lock<std::mutex> L(mutex);
    nodesSearched += this->nodesSearched;
    tbHits += this->tbHits;
    this->nodesSearched = 0;
    this->tbHits = 0;
}

// ----------------------------------------------------------------------------

WorkerThread::WorkerThread(int threadNo, Communicator* parentComm,
                           int numWorkers, TranspositionTable& tt)
    : threadNo(threadNo), numWorkers(numWorkers), terminate(false), tt(tt) {
    auto f = [this,parentComm]() {
        mainLoop(parentComm);
    };
    thread = make_unique<std::thread>(f);
}

WorkerThread::~WorkerThread() {
    children.clear();
    terminate = true;
    threadNotifier.notify();
    thread->join();
}

void
WorkerThread::createWorkers(int firstThreadNo, Communicator* parentComm,
                            int numWorkers, TranspositionTable& tt,
                            std::vector<std::shared_ptr<WorkerThread>>& children) {
    if (numWorkers <= 0) {
        children.clear();
        return;
    }

    const int maxChildren = 4;
    int numChildren = std::min(numWorkers, maxChildren);
    children.resize(numChildren);
    for (int i = 0; i < numChildren; i++) {
        int n = (numWorkers + numChildren - i - 1) / (numChildren - i);
        if (!children[i] || children[i]->getThreadNo() != firstThreadNo ||
                children[i]->getNumWorkers() != n)
            children[i] = std::make_shared<WorkerThread>(firstThreadNo, parentComm, n, tt);
        firstThreadNo += n;
        numWorkers -= n;
    }

    for (int i = 0; i < numChildren; i++)
        children[i]->waitInitialized();
}

void
WorkerThread::waitInitialized() {
    initialized.wait();
}

void
WorkerThread::mainLoop(Communicator* parentComm) {
    Numa::instance().bindThread(threadNo);

    comm = make_unique<ThreadCommunicator>(parentComm, threadNotifier);
    createWorkers(threadNo + 1, comm.get(), numWorkers - 1, tt, children);

    initialized.notify();

    class Handler : public Communicator::CommandHandler {
    public:
        Handler(WorkerThread& wt) : wt(wt) {}

        void initSearch(History& ht) override {
            wt.comm->sendInitSearch(ht);
            // FIXME!!
        }
        void startSearch(int jobId, const Position& pos, int alpha, int beta,
                         int depth, KillerTable& kt) override {
            wt.comm->sendStartSearch(jobId, pos, alpha, beta, depth, kt);
            // FIXME!!
        }
        void stopSearch(int jobId) override {
            wt.comm->sendStopSearch(jobId);
            // FIXME!!
        }
        void reportResult(int jobId, int score) override {
            wt.comm->sendReportResult(jobId, score);
            // FIXME!!
        }
    private:
        WorkerThread& wt;
    };
    Handler handler(*this);

    while (true) {
        threadNotifier.wait();
        if (terminate)
            break;
        comm->poll(handler);
    }
}

void
WorkerThread::sendReportStats(S64 nodesSearched, S64 tbHits) {
    comm->sendReportStats(nodesSearched, tbHits);
}

class ThreadStopHandler : public Search::StopHandler {
public:
    ThreadStopHandler(WorkerThread& wt, const Search& sc);

    /** Destructor. Report searched nodes to ParallelData object. */
    ~ThreadStopHandler();

    ThreadStopHandler(const ThreadStopHandler&) = delete;
    ThreadStopHandler& operator=(const ThreadStopHandler&) = delete;

    bool shouldStop() override;

private:
    /** Report searched nodes since last call. */
    void reportNodes();

    WorkerThread& wt;
    const Search& sc;
    int counter;             // Counts number of calls to shouldStop
    S64 lastReportedNodes;
    S64 lastReportedTbHits;
};

ThreadStopHandler::ThreadStopHandler(WorkerThread& wt, const Search& sc)
    : wt(wt), sc(sc), counter(0),
      lastReportedNodes(0), lastReportedTbHits(0) {
}

ThreadStopHandler::~ThreadStopHandler() {
    reportNodes();
}

bool
ThreadStopHandler::shouldStop() {
    if (wt.isStopped())
        return true;

    counter++;
    if (counter >= 100) {
        counter = 0;
        reportNodes();
    }

    return false;
}

void
ThreadStopHandler::reportNodes() {
    S64 totNodes = sc.getTotalNodesThisThread();
    S64 nodes = totNodes - lastReportedNodes;
    lastReportedNodes = totNodes;

    S64 totTbHits = sc.getTbHitsThisThread();
    S64 tbHits = totTbHits - lastReportedTbHits;
    lastReportedTbHits = totTbHits;

    wt.sendReportStats(nodes, tbHits);
}

#if 0
void
WorkerThread::mainLoop() {
//    const int minProbeDepth = TBProbe::tbEnabled() ? UciParams::minProbeDepth->getIntPar() : 100;

    //    log([&](std::ostream& os){os << "mainLoop, th:" << threadNo;});
    Numa::instance().bindThread(threadNo);
    if (!et)
        et = Evaluate::getEvalHashTables();
    if (!kt)
        kt = make_unique<KillerTable>();
    if (!ht)
        ht = make_unique<History>();

    TreeLogger logFile;
    logFile.open("/home/petero/treelog.dmp", threadNo);

    Position pos;
    for (int iter = 0; ; iter++) {
        if (isStopped())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const int depth = spMove.getDepth();
        if (sp != newSp) {
            sp = newSp;
            *ht = sp->getHistory();
            *kt = sp->getKillerTable();
        }
        Search::SearchTables st(tt, *kt, *ht, *et);
        sp->getPos(pos);
        const U64 rootNodeIdx = logFile.logPosition(pos);
        UndoInfo ui;
        pos.makeMove(spMove.getMove(), ui);
        std::vector<U64> posHashList;
        int posHashListSize;
        sp->getPosHashList(pos, posHashList, posHashListSize);
        Search sc(pos, posHashList, posHashListSize, st, pd, sp, logFile);
        sc.setThreadNo(threadNo);
        sc.setMinProbeDepth(minProbeDepth);
        const int alpha = sp->getAlpha();
        const int beta = sp->getBeta();
        const S64 nodes0 = pd.getNumSearchedNodes();
        auto stopHandler = make_unique<ThreadStopHandler>(*this, pd, *sp, spMove,
                                                          sc, alpha, nodes0, prio);
        sc.setStopHandler(std::move(stopHandler));
        const int ply = sp->getPly();
        const int lmr = spMove.getLMR();
        const int captSquare = spMove.getRecaptureSquare();
        const bool inCheck = spMove.getInCheck();
        sc.setSearchTreeInfo(ply, sp->getSearchTreeInfo(), spMove.getMove(), moveNo, lmr, rootNodeIdx);
        try {
            const bool smp = pd.numHelperThreads() > 1;
            int score = -sc.negaScout(smp, true, -(alpha+1), -alpha, ply+1,
                                      depth, captSquare, inCheck);
            if (((lmr > 0) && (score > alpha)) ||
                    ((score > alpha) && (score < beta))) {
                sc.setSearchTreeInfo(ply, sp->getSearchTreeInfo(), spMove.getMove(), moveNo, 0, rootNodeIdx);
                score = -sc.negaScout(smp, true, -beta, -alpha, ply+1,
                                      depth + lmr, captSquare, inCheck);
            }
            bool cancelRemaining = score >= beta;
            pd.wq.moveFinished(sp, moveNo, cancelRemaining, score);
            if (cancelRemaining)
                pd.setHelperFailHigh(sp->owningThread(), true);
        } catch (const Search::StopSearch&) {
            if (!spMove.isCanceled() && !pd.wq.isStopped())
                pd.wq.returnMove(sp, moveNo);
        }
    }
}
#endif
