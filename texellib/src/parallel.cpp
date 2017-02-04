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

// ----------------------------------------------------------------------------

WorkerThread::WorkerThread(int threadNo, ParallelData& pd,
                           TranspositionTable& tt)
    : threadNo(threadNo), pd(pd), tt(tt) {
}

WorkerThread::~WorkerThread() {
    assert(!thread);
}

void
WorkerThread::start() {
    assert(!thread);
    const int minProbeDepth = TBProbe::tbEnabled() ? UciParams::minProbeDepth->getIntPar() : 100;
    thread = make_unique<std::thread>([this,minProbeDepth](){ mainLoop(minProbeDepth); });
}

void
WorkerThread::join() {
    if (thread) {
        thread->join();
        thread.reset();
    }
}

class ThreadStopHandler : public Search::StopHandler {
public:
    ThreadStopHandler(WorkerThread& wt, ParallelData& pd,
                      const Search& sc);

    /** Destructor. Report searched nodes to ParallelData object. */
    ~ThreadStopHandler();

    ThreadStopHandler(const ThreadStopHandler&) = delete;
    ThreadStopHandler& operator=(const ThreadStopHandler&) = delete;

    bool shouldStop() override;

private:
    /** Report searched nodes since last call to ParallelData object. */
    void reportNodes();

    const WorkerThread& wt;
    ParallelData& pd;
    const Search& sc;
    int counter;             // Counts number of calls to shouldStop
    S64 lastReportedNodes;
    S64 lastReportedTbHits;
};

ThreadStopHandler::ThreadStopHandler(WorkerThread& wt, ParallelData& pd,
                                     const Search& sc)
    : wt(wt), pd(pd), sc(sc), counter(0),
      lastReportedNodes(0), lastReportedTbHits(0) {
}

ThreadStopHandler::~ThreadStopHandler() {
    reportNodes();
}

bool
ThreadStopHandler::shouldStop() {
    if (pd.isStopped())
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
    pd.addSearchedNodes(nodes);

    S64 totTbHits = sc.getTbHitsThisThread();
    S64 tbHits = totTbHits - lastReportedTbHits;
    lastReportedTbHits = totTbHits;
    pd.addTbHits(tbHits);
}

void
WorkerThread::mainLoop(int minProbeDepth) {
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
        if (pd.isStopped())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

#if 0
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
#endif
    }
}

// ----------------------------------------------------------------------------

void
ParallelData::addRemoveWorkers(int numWorkers) {
    while (numWorkers < (int)threads.size()) {
        assert(!threads.back()->threadRunning());
        threads.pop_back();
    }
    for (int i = threads.size(); i < numWorkers; i++)
        threads.push_back(make_unique<WorkerThread>(i+1, *this, tt));
}

void
ParallelData::startAll() {
    totalHelperNodes = 0;
    helperTbHits = 0;
    stopped = false;
    for (auto& thread : threads)
        thread->start();
}

void
ParallelData::stopAll() {
    stopped = true;
    for (auto& thread : threads)
        thread->join();
}

bool
ParallelData::isStopped() const {
    return stopped;
}
