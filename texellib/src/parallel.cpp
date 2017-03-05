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


void
Notifier::notify() {
    {
        std::lock_guard<std::mutex> L(mutex);
        notified = true;
    }
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
    std::lock_guard<std::mutex> L(mutex);
    children.push_back(child);
}

void
Communicator::removeChild(Communicator* child) {
    std::lock_guard<std::mutex> L(mutex);
    children.erase(std::remove(children.begin(), children.end(), child),
                   children.end());
}

void
Communicator::sendInitSearch(const Position& pos,
                             const std::vector<U64>& posHashList, int posHashListSize,
                             bool clearHistory) {
    nodesSearched = 0;
    tbHits = 0;
    for (auto& c : children)
        c->doSendInitSearch(pos, posHashList, posHashListSize, clearHistory);
}

void
Communicator::sendStartSearch(int jobId, const SearchTreeInfo& sti,
                              int alpha, int beta, int depth) {
    for (auto& c : children)
        c->doSendStartSearch(jobId, sti, alpha, beta, depth);
}

void
Communicator::sendStopSearch() {
    stopAckWaitSelf = true;
    stopAckWaitChildren = children.size();
    notifyThread();
    for (auto& c : children)
        c->doSendStopSearch();
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
Communicator::sendStopAck(bool child) {
    if (child) {
        stopAckWaitChildren--;
    } else {
        if (!stopAckWaitSelf)
            return;
        stopAckWaitSelf = false;
    }
    if (hasStopAck() && parent)
        parent->doSendStopAck();
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
        std::shared_ptr<Command> cmd = cmdQueue.front();
        cmdQueue.pop_front();
        L.unlock();

        switch (cmd->type) {
        case CommandType::INIT_SEARCH: {
            const InitSearchCommand* iCmd = static_cast<const InitSearchCommand*>(cmd.get());
            Position pos;
            pos.deSerialize(iCmd->posData);
            handler.initSearch(pos, iCmd->posHashList, iCmd->posHashListSize, iCmd->clearHistory);
            break;
        }
        case CommandType::START_SEARCH: {
            const StartSearchCommand*  sCmd = static_cast<const StartSearchCommand*>(cmd.get());
            handler.startSearch(sCmd->jobId, sCmd->sti, sCmd->alpha, sCmd->beta, sCmd->depth);
            break;
        }
        case CommandType::STOP_SEARCH:
            handler.stopSearch();
            break;
        case CommandType::REPORT_RESULT: {
            handler.reportResult(cmd->jobId, cmd->resultScore);
            break;
        case CommandType::STOP_ACK:
            handler.stopAck();
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
ThreadCommunicator::doSendInitSearch(const Position& pos,
                                     const std::vector<U64>& posHashList, int posHashListSize,
                                     bool clearHistory) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<InitSearchCommand>(pos, posHashList, posHashListSize, clearHistory));
    notifier.notify();
}

void
ThreadCommunicator::doSendStartSearch(int jobId, const SearchTreeInfo& sti,
                                      int alpha, int beta, int depth) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const std::shared_ptr<Command>& cmd) {
                                      return cmd->type == CommandType::START_SEARCH ||
                                             cmd->type == CommandType::STOP_SEARCH ||
                                             cmd->type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    cmdQueue.push_back(std::make_shared<StartSearchCommand>(jobId, sti, alpha, beta, depth));
    notifier.notify();
}

void
ThreadCommunicator::doSendStopSearch() {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const std::shared_ptr<Command>& cmd) {
                                      return cmd->type == CommandType::START_SEARCH ||
                                             cmd->type == CommandType::STOP_SEARCH ||
                                             cmd->type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    cmdQueue.push_back(std::make_shared<Command>(CommandType::STOP_SEARCH));
    notifier.notify();
}

void
ThreadCommunicator::doSendReportResult(int jobId, int score) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<Command>(CommandType::REPORT_RESULT, jobId, score));
    notifier.notify();
}

void
ThreadCommunicator::doSendReportStats(S64 nodesSearched, S64 tbHits) {
    std::lock_guard<std::mutex> L(mutex);
    this->nodesSearched += nodesSearched;
    this->tbHits += tbHits;
}

void
ThreadCommunicator::retrieveStats(S64& nodesSearched, S64& tbHits) {
    std::lock_guard<std::mutex> L(mutex);
    nodesSearched += this->nodesSearched;
    tbHits += this->tbHits;
    this->nodesSearched = 0;
    this->tbHits = 0;
}

void
ThreadCommunicator::doSendStopAck() {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<Command>(CommandType::STOP_ACK));
    notifier.notify();
}

void
ThreadCommunicator::notifyThread() {
    notifier.notify();
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
    std::vector<int> newChildren;
    children.resize(numChildren);
    for (int i = 0; i < numChildren; i++) {
        int n = (numWorkers + numChildren - i - 1) / (numChildren - i);
        if (!children[i] || children[i]->getThreadNo() != firstThreadNo ||
                children[i]->getNumWorkers() != n) {
            children[i] = std::make_shared<WorkerThread>(firstThreadNo, parentComm, n, tt);
            newChildren.push_back(i);
        }
        firstThreadNo += n;
        numWorkers -= n;
    }

    for (int i : newChildren)
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

    CommHandler handler(*this);

    while (true) {
        threadNotifier.wait();
        if (terminate)
            break;
        comm->poll(handler);
        if (jobId != -1)
            doSearch(handler);
        comm->sendStopAck(false);
    }
}

void
WorkerThread::CommHandler::initSearch(const Position& pos,
                                      const std::vector<U64>& posHashList, int posHashListSize,
                                      bool clearHistory) {
    wt.comm->sendInitSearch(pos, posHashList, posHashListSize, clearHistory);
    wt.pos = pos;
    wt.posHashList = posHashList;
    wt.posHashListSize = posHashListSize;
    wt.jobId = -1;

    wt.logFile = make_unique<TreeLogger>();
    wt.logFile->open("/home/petero/treelog.dmp", wt.threadNo);
    if (wt.kt)
        wt.kt->clear();
    if (wt.ht) {
        if (clearHistory)
            wt.ht->init();
        else
            wt.ht->reScale();
    }
}

void
WorkerThread::CommHandler::startSearch(int jobId, const SearchTreeInfo& sti,
                                       int alpha, int beta, int depth) {
    wt.comm->sendStartSearch(jobId, sti, alpha, beta, depth);
    wt.sti = sti;
    wt.jobId = jobId;
    wt.alpha = alpha;
    wt.beta = beta;
    wt.depth = depth;
    wt.hasResult = false;
}

void
WorkerThread::CommHandler::stopSearch() {
    wt.comm->sendStopSearch();
    wt.jobId = -1;
}

void
WorkerThread::CommHandler::reportResult(int jobId, int score) {
    wt.sendReportResult(jobId, score);
}

void
WorkerThread::CommHandler::stopAck() {
    wt.comm->sendStopAck(true);
}

void
WorkerThread::sendReportResult(int jobId, int score) {
    if (!hasResult && (this->jobId == jobId)) {
        comm->sendReportResult(jobId, score);
        hasResult = true;
    }
}

void
WorkerThread::sendReportStats(S64 nodesSearched, S64 tbHits) {
    comm->sendReportStats(nodesSearched, tbHits);
}

class ThreadStopHandler : public Search::StopHandler {
public:
    ThreadStopHandler(WorkerThread& wt, int jobId, const Search& sc,
                      Communicator::CommandHandler& commHandler);

    /** Destructor. Report searched nodes to ParallelData object. */
    ~ThreadStopHandler();

    ThreadStopHandler(const ThreadStopHandler&) = delete;
    ThreadStopHandler& operator=(const ThreadStopHandler&) = delete;

    bool shouldStop() override;

private:
    /** Report searched nodes since last call. */
    void reportNodes();

    WorkerThread& wt;
    const int jobId;
    const Search& sc;
    Communicator::CommandHandler& commHandler;
    int counter;             // Counts number of calls to shouldStop
    S64 lastReportedNodes;
    S64 lastReportedTbHits;
};

ThreadStopHandler::ThreadStopHandler(WorkerThread& wt, int jobId, const Search& sc,
                                     Communicator::CommandHandler& commHandler)
    : wt(wt), jobId(jobId), sc(sc), commHandler(commHandler), counter(0),
      lastReportedNodes(0), lastReportedTbHits(0) {
}

ThreadStopHandler::~ThreadStopHandler() {
    reportNodes();
}

bool
ThreadStopHandler::shouldStop() {
    wt.poll(commHandler);
    if (wt.shouldStop(jobId))
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

void
WorkerThread::doSearch(CommHandler& commHandler) {
    if (!et)
        et = Evaluate::getEvalHashTables();
    if (!kt)
        kt = make_unique<KillerTable>();
    if (!ht)
        ht = make_unique<History>();

    using namespace SearchConst;
    int initExtraDepth = threadNo & 1;
    for (int extraDepth = initExtraDepth; ; extraDepth++) {
        Search::SearchTables st(tt, *kt, *ht, *et);
        Position pos(this->pos);
        const U64 rootNodeIdx = logFile->logPosition(pos);

        UndoInfo ui;
        pos.makeMove(sti.currentMove, ui);

        posHashList[posHashListSize++] = pos.zobristHash();
        Search sc(pos, posHashList, posHashListSize, st, *comm, *logFile);
        posHashListSize--;
        sc.setThreadNo(threadNo);
        sc.initSearchTreeInfo();
        const int minProbeDepth = TBProbe::tbEnabled() ? UciParams::minProbeDepth->getIntPar() : MAX_SEARCH_DEPTH;
        sc.setMinProbeDepth(minProbeDepth);

        auto stopHandler = make_unique<ThreadStopHandler>(*this, jobId, sc, commHandler);
        sc.setStopHandler(std::move(stopHandler));

        int ply = 1;
        sc.setSearchTreeInfo(ply-1, sti, rootNodeIdx);
        int captSquare = -1;
        bool inCheck = MoveGen::inCheck(pos);
        try {
            int searchDepth = std::min(depth + extraDepth, MAX_SEARCH_DEPTH);
            int score = sc.negaScout(true, alpha, beta, ply, searchDepth, captSquare, inCheck);
            sendReportResult(jobId, score);
            if (searchDepth >= MAX_SEARCH_DEPTH) {
                jobId = -1;
                break;
            }
        } catch (const Search::StopSearch&) {
            break;
        }
    }
}
