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
 * parallel.hpp
 *
 *  Created on: Jul 9, 2013
 *      Author: petero
 */

#ifndef PARALLEL_HPP_
#define PARALLEL_HPP_

#include "killerTable.hpp"
#include "history.hpp"
#include "transpositionTable.hpp"
#include "evaluate.hpp"
#include "searchUtil.hpp"
#include "constants.hpp"
#include "util/timeUtil.hpp"

#include <memory>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


class ParallelData;


class WorkerThread {
public:
    /** Constructor. Does not start new thread. */
    WorkerThread(int threadNo, ParallelData& pd, TranspositionTable& tt);

    /** Destructor. Waits for thread to terminate. */
    ~WorkerThread();

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    /** Start thread. */
    void start();

    /** Wait for thread to stop. */
    void join();

    /** Return true if thread is running. */
    bool threadRunning() const;

    /** Return thread number. The first worker thread is number 1. */
    int getThreadNo() const;

private:
    /** Thread main loop. */
    void mainLoop(int minProbeDepth);

    int threadNo;
    ParallelData& pd;
    std::unique_ptr<std::thread> thread;

    std::unique_ptr<Evaluate::EvalHashTables> et;
    std::unique_ptr<KillerTable> kt;
    std::unique_ptr<History> ht;
    TranspositionTable& tt;
};

/** Top-level parallel search data structure. */
class ParallelData {
public:
    /** Constructor. */
    ParallelData(TranspositionTable& tt);

    /** Create/delete worker threads so that there are numWorkers in total. */
    void addRemoveWorkers(int numWorkers);

    /** Start all worker threads. */
    void startAll();

    /** Stop all worker threads. */
    void stopAll();

    /** Get stopped flag. */
    bool isStopped() const;


    /** Return number of helper threads in use. */
    int numHelperThreads() const;

    /** Get number of nodes searched by all helper threads. */
    S64 getNumSearchedNodes() const;

    /** Get number of TB hits for all helper threads. */
    S64 getTbHits() const;

    /** Add nNodes to total number of searched nodes. */
    void addSearchedNodes(S64 nNodes);

    /** Add nTbHits to number of TB hits. */
    void addTbHits(S64 nTbHits);

private:
    /** Vector of helper threads. Master thread not included. */
    std::vector<std::unique_ptr<WorkerThread>> threads;
    std::atomic<bool> stopped;

    TranspositionTable& tt;

    std::atomic<S64> totalHelperNodes; // Number of nodes searched by all helper threads
    std::atomic<S64> helperTbHits;     // Number of TB hits for all helper threads
};


inline bool
WorkerThread::threadRunning() const {
    return thread != nullptr;
}

inline int
WorkerThread::getThreadNo() const {
    return threadNo;
}

inline ParallelData::ParallelData(TranspositionTable& tt0)
    : tt(tt0), totalHelperNodes(0), helperTbHits(0) {
    addRemoveWorkers(0);
}

inline int
ParallelData::numHelperThreads() const {
    return (int)threads.size();
}

inline S64
ParallelData::getNumSearchedNodes() const {
    return totalHelperNodes;
}

inline S64
ParallelData::getTbHits() const {
    return helperTbHits;
}

inline void
ParallelData::addSearchedNodes(S64 nNodes) {
    totalHelperNodes += nNodes;
}

inline void
ParallelData::addTbHits(S64 nTbHits) {
    helperTbHits += nTbHits;
}

#endif /* PARALLEL_HPP_ */
