/*
    Texel - A UCI chess engine.
    Copyright (C) 2013  Peter Österlund, peterosterlund2@gmail.com

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

#include <memory>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>


class Search;
class SearchTreeInfo;

class ParallelData;
class SplitPoint;
class SplitPointMove;
class FailHighInfo;


class WorkerThread {
public:
    /** Constructor. Does not start new thread. */
    WorkerThread(int threadNo, ParallelData& pd, TranspositionTable& tt);

    /** Destructor. Waits for thread to terminate. */
    ~WorkerThread();

    /** Start thread. */
    void start();

    /** Tell thread to stop. */
    void stop(bool wait);

    /** Returns true if thread should stop searching. */
    bool shouldStop() const { return stopThread; }

    /** Return true if thread is running. */
    bool threadRunning() const { return thread != nullptr; }

private:
    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    /** Thread main loop. */
    void mainLoop();

    int threadNo;
    ParallelData& pd;
    std::shared_ptr<std::thread> thread;

    std::shared_ptr<Evaluate::EvalHashTables> et;
    std::shared_ptr<KillerTable> kt;
    std::shared_ptr<History> ht;
    TranspositionTable& tt;

    volatile bool stopThread;
};


/** Priority queue of pending search tasks. Handles thread safety. */
class WorkQueue {
    friend class ParallelTest;
public:
    /** Constructor. */
    WorkQueue(std::condition_variable& cv, FailHighInfo& fhInfo);

    /** Add SplitPoint to work queue. */
    void addWork(const std::shared_ptr<SplitPoint>& sp);

    /** Get best move for helper thread to work on. */
    std::shared_ptr<SplitPoint> getWork(int& moveNo);

    /** A helper thread stopped working on a move before it was finished. */
    void returnMove(const std::shared_ptr<SplitPoint>& sp, int moveNo);

    /** Set which move number the SplitPoint owner is currently searching. */
    void setOwnerCurrMove(const std::shared_ptr<SplitPoint>& sp, int moveNo);

    /** Cancel this SplitPoint and all children. */
    void cancel(const std::shared_ptr<SplitPoint>& sp);

    /** Set move to canceled after helper thread finished searching it. */
    void moveFinished(const std::shared_ptr<SplitPoint>& sp, int moveNo, bool cancelRemaining);

    /** Return probability that the best unstarted move needs to be searched.
     *  Also return the corresponding SplitPoint. */
    double getBestProbability(std::shared_ptr<SplitPoint>& bestSp) const;
    double getBestProbability() const;

private:
    /** Move sp to waiting if it has no unstarted moves. */
    void maybeMoveToWaiting(const std::shared_ptr<SplitPoint>& sp);

    /** Insert sp in queue. Notify condition variable if queue becomes non-empty. */
    void insertInQueue(const std::shared_ptr<SplitPoint>& sp);

    /** Recompute probabilities for "sp" and all children. Update "queue" and "waiting". */
    void updateProbabilities(const std::shared_ptr<SplitPoint>& sp);

    struct SplitPointCompare {
        bool operator()(const std::shared_ptr<SplitPoint>& a,
                        const std::shared_ptr<SplitPoint>& b) const;
    };

    /** Move sp and all its children from spSet to spVec. */
    static void removeFromSet(const std::shared_ptr<SplitPoint>& sp,
                              std::set<std::shared_ptr<SplitPoint>, SplitPointCompare>& spSet,
                              std::vector<std::shared_ptr<SplitPoint>>& spVec);

    /** Cancel "sp" and all children. Assumes mutex already locked. */
    void cancelInternal(const std::shared_ptr<SplitPoint>& sp);


    std::condition_variable& cv;
    FailHighInfo& fhInfo;
    mutable std::mutex mutex;

    // SplitPoints with unstarted SplitPointMoves
    std::set<std::shared_ptr<SplitPoint>, SplitPointCompare> queue;

    // SplitPoints with no unstarted SplitPointMoves
    std::set<std::shared_ptr<SplitPoint>, SplitPointCompare> waiting;
};


/** Fail high statistics, for estimating SplitPoint usefulness probabilities. */
class FailHighInfo {
public:
    /** Constructor. */
    FailHighInfo();

    /** Return probability that move moveNo needs to be searched.
     * @param parentMoveNo        Move number of move leading to this position.
     * @param currSearchedMoveNo  Move currently being searched.
     * @param moveNo              Move number to get probability for. */
    double getMoveNeededProbability(int parentMoveNo, int currMoveNo, int moveNo) const;

    /** Add fail high information.
     * @param parentMoveNo  Move number of move leading to this position.
     * @param nSearched     Number of moves searched at this node.
     * @param failHigh      True if the node failed high. */
    void addData(int parentMoveNo, int nSearched, bool failHigh);

    /** Rescale the counters, so that future updates have more weight. */
    void reScale();

private:
    void reScaleInternal(int factor);

    static const int NUM_STAT_MOVES = 15;
    mutable std::mutex mutex;

    int failHiCount[2][NUM_STAT_MOVES];   // [parentMoveNo>0?1:0][moveNo]
    int failLoCount[2];                   // [parentMoveNo>0?1:0]
    int totCount;                         // Sum of all counts
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


    // Notified when wq becomes non-empty and when search should stop
    std::condition_variable cv;

    FailHighInfo fhInfo;

    WorkQueue wq;

private:
    /** Vector of helper threads. Master thread not included. */
    std::vector<std::shared_ptr<WorkerThread>> threads;

    TranspositionTable& tt;
};


/** SplitPoint does not handle thread safety, WorkQueue must do that.  */
class SplitPoint {
    friend class ParallelTest;
public:
    /** Constructor. */
    SplitPoint(const std::shared_ptr<SplitPoint>& parentSp, int parentMoveNo,
               const Position& pos, const std::vector<U64>& posHashList,
               int posHashListSize, const SearchTreeInfo& sti,
               const KillerTable& kt, const History& ht,
               int alpha, int beta, int ply);

    /** Add a child SplitPoint */
    void addChild(const std::weak_ptr<SplitPoint>& child);

    /** Add a move to the SplitPoint. */
    void addMove(const SplitPointMove& spMove);

    /** Assign sequence number. */
    void setSeqNo();

    /** compute pSpUseful and pnextMoveUseful. */
    void computeProbabilities(const FailHighInfo& fhInfo);

    /** Get parent SplitPoint, or null for root SplitPoint. */
    std::shared_ptr<SplitPoint> getParent() const;

    /** Get children SplitPoints. */
    const std::vector<std::weak_ptr<SplitPoint>>& getChildren() const;


    U64 getSeqNo() const { return seqNo; }
    double getPSpUseful() const { return pSpUseful; }
    double getPNextMoveUseful() const { return pNextMoveUseful; }
    const History& getHistory() const { return ht; }
    const KillerTable& getKillerTable() const { return kt; }
    const SplitPointMove& getSpMove(int moveNo) const { return spMoves[moveNo]; }

    /** Return probability that moveNo needs to be searched. */
    double getPMoveUseful(const FailHighInfo& fhInfo, int moveNo) const;

    void getPos(Position& pos, const Move& move) const;
    void getPosHashList(const Position& pos, std::vector<U64>& posHashList,
                        int& posHashListSize) const;
    const SearchTreeInfo& getSearchTreeInfo() const { return searchTreeInfo; }
    int getAlpha() const { return alpha; }
    int getBeta() const { return beta; }
    int getPly() const { return ply; }


    /** Get index of first unstarted move. Mark move as being searched. */
    int getNextMove();

    /** A helper thread stopped working on a move before it was finished. */
    void returnMove(int moveNo);

    /** Set which move number the SplitPoint owner is currently searching. */
    void setOwnerCurrMove(int moveNo);

    /** Cancel this SplitPoint and all children. */
    void cancel();

    /** Set move to canceled after helper thread finished searching it. */
    void moveFinished(int moveNo, bool cancelRemaining);

    /** Return true if there are moves that have not been started to be searched. */
    bool hasUnStartedMove() const;

    /** Return true if there are moves that have not been finished (canceled) yet. */
    bool hasUnFinishedMove() const;

    /** Return true if this SplitPoint is an ancestor to "sp". */
    bool isAncestorTo(const SplitPoint& sp) const;

    /** Print object state to "os", for debugging. */
    void print(std::ostream& os, int level, const FailHighInfo& fhInfo) const;

private:
    /** Get index of first unstarted move, or -1 if there is no unstarted move. */
    int findNextMove() const;

    /** Remove null entries from children vector. */
    void cleanUpChildren();

    const Position pos;
    const std::vector<U64> posHashList; // List of hashes for previous positions up to the last "zeroing" move.
    const int posHashListSize;
    const SearchTreeInfo searchTreeInfo;   // For ply-1
    const KillerTable& kt;
    const History& ht;

    const int alpha;
    const int beta;
    const int ply;

    double pSpUseful;       // Probability that this SplitPoint is needed. 100% if parent is null.
    double pNextMoveUseful; // Probability that next unstarted move needs to be searched.

    const std::shared_ptr<SplitPoint> parent;
    const int parentMoveNo; // Move number in parent SplitPoint that generated this SplitPoint
    std::vector<std::weak_ptr<SplitPoint>> children;

    static U64 nextSeqNo;
    U64 seqNo;      // To break ties when two objects have same priority. Set by addWork() under lock
    int currMoveNo;
    std::vector<SplitPointMove> spMoves;
};


/** Represents one move at a SplitPoint. */
class SplitPointMove {
public:
    /** Constructor */
    SplitPointMove(const Move& move, int lmr, int depth,
                   int captSquare, bool inCheck);

    const Move& getMove() const { return move; }
    int getLMR() const { return lmr; }
    int getDepth() const { return depth; }
    int getRecaptureSquare() const { return captSquare; }
    bool getInCheck() const { return inCheck; }

    bool isCanceled() const { return canceled; }
    void setCanceled(bool value) { canceled = value; }

    bool isSearching() const { return searching; }
    void setSearching(bool value) { searching = value; }

private:
    Move move;      // Position defined by sp->pos + move
    int lmr;        // Amount of LMR reduction
    int depth;
    int captSquare; // Recapture square, or -1 if no recapture
    bool inCheck;   // True if side to move is in check

    bool canceled;  // Result is no longer needed
    bool searching; // True if currently searched by a helper thread
};


/** Handle a SplitPoint object using RAII. */
class SplitPointHolder {
public:
    /** Constructor. */
    SplitPointHolder(WorkQueue& wq, std::vector<std::shared_ptr<SplitPoint>>& spVec);

    /** Destructor. Cancel SplitPoint. */
    ~SplitPointHolder();

    /** Set the SplitPoint object. */
    void setSp(const std::shared_ptr<SplitPoint>& sp);

    /** Add a move to the SplitPoint. */
    void addMove(const SplitPointMove& spMove);

    /** Add SplitPoint to work queue. */
    void addToQueue();

private:
    SplitPointHolder(const SplitPointHolder&) = delete;
    SplitPointHolder operator=(const SplitPointHolder&) = delete;

    WorkQueue& wq;
    std::vector<std::shared_ptr<SplitPoint>>& spVec;
    std::shared_ptr<SplitPoint> sp;
    enum class State { EMPTY, CREATED, QUEUED } state;
};


inline bool
WorkQueue::SplitPointCompare::operator()(const std::shared_ptr<SplitPoint>& a,
                                         const std::shared_ptr<SplitPoint>& b) const {
    int pa = (int)(a->getPNextMoveUseful() * 65536);
    int pb = (int)(b->getPNextMoveUseful() * 65536);
    if (pa != pb)
        return pa > pb;
    if (a->getPly() != b->getPly())
        return a->getPly() < b->getPly();
    return a->getSeqNo() < b->getSeqNo();
}


#endif /* PARALLEL_HPP_ */
