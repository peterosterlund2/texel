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
 * parallel.cpp
 *
 *  Created on: Jul 9, 2013
 *      Author: petero
 */

#include "parallel.hpp"
#include "search.hpp"
#include "textio.hpp"

#include <cassert>

U64 SplitPoint::nextSeqNo = 0;

// ----------------------------------------------------------------------------

/** Sleep/wake timer. */
class SWTimer {
public:
    SWTimer(ParallelData& pd0, int threadNo0)
        : pd(pd0), threadNo(threadNo0),
          working(false), tSleep(0), tWork(0) {
        t0 = currentTime();
    }

    ~SWTimer() {
        pd.log([&](std::ostream& os){os << "timer th:" << threadNo << " total"
                                        << " s:" << tSleep << " w:" << tWork;});
    }

    void startSleep() {
        if (working) {
            double t1 = currentTime();
            tWork += t1 - t0;
//            pd.log([&](std::ostream& os){os << "timer th:" << threadNo << " worked:" << t1 - t0
//                                            << " s:" << tSleep << " w:" << tWork;});
            t0 = t1;
            working = false;
        }
    }

    void startWork() {
        if (!working) {
            double t1 = currentTime();
            tSleep += t1 - t0;
//            pd.log([&](std::ostream& os){os << "timer th:" << threadNo << " slept:" << t1 - t0
//                                            << " s:" << tSleep << " w:" << tWork;});
            t0 = t1;
            working = true;
        }
    }

private:
    ParallelData& pd;
    int threadNo;

    bool working;
    double t0;
    double tSleep, tWork;
};

// ----------------------------------------------------------------------------

WorkerThread::WorkerThread(int threadNo0, ParallelData& pd0,
                           TranspositionTable& tt0)
    : threadNo(threadNo0), pd(pd0), tt(tt0),
      stopThread(false) {
}

WorkerThread::~WorkerThread() {
    stop(true);
}

void
WorkerThread::start() {
    assert(!thread);
    stopThread = false;
    thread = std::make_shared<std::thread>([this](){ mainLoop(); });
}

void
WorkerThread::stop(bool wait) {
    stopThread = true;
    if (wait) {
        if (thread) {
            pd.cv.notify_all();
            thread->join();
            thread.reset();
            stopThread = false;
        }
    }
}

class ThreadStopHandler : public Search::StopHandler {
public:
    ThreadStopHandler(WorkerThread& wt, ParallelData& pd,
                      const SplitPoint& sp, const SplitPointMove& spm,
                      int moveNo, const Search& sc);

    /** Destructor. Report searched nodes to ParallelData object. */
    ~ThreadStopHandler();

    bool shouldStop();

private:
    ThreadStopHandler(const ThreadStopHandler&) = delete;
    ThreadStopHandler& operator=(const ThreadStopHandler&) = delete;

    /** Report searched nodes since last call to ParallelData object. */
    void reportNodes();

    const WorkerThread& wt;
    ParallelData& pd;
    const SplitPoint& sp;
    const SplitPointMove& spMove;
    const int moveNo;
    const Search& sc;
    int counter;             // Counts number of calls to shouldStop
    int nextProbCheck;       // Next time test for SplitPoint switch should be performed
    S64 lastReportedNodes;
};

ThreadStopHandler::ThreadStopHandler(WorkerThread& wt0, ParallelData& pd0,
                                     const SplitPoint& sp0, const SplitPointMove& spm0,
                                     int moveNo0, const Search& sc0)
    : wt(wt0), pd(pd0), sp(sp0), spMove(spm0), moveNo(moveNo0),
      sc(sc0), counter(0), nextProbCheck(1), lastReportedNodes(0) {
}

ThreadStopHandler::~ThreadStopHandler() {
    reportNodes();
}

bool
ThreadStopHandler::shouldStop() {
    if (wt.shouldStop() || spMove.isCanceled())
        return true;

    counter++;
    if (counter >= nextProbCheck) {
        nextProbCheck = counter + 1 + counter / 4;
        double myProb = sp.getPMoveUseful(pd.fhInfo, moveNo);
        std::shared_ptr<SplitPoint> bestSp;
        double bestProb = pd.wq.getBestProbability(bestSp);
        if ((bestProb > myProb + 0.01) && (bestProb >= (myProb + (1.0 - myProb) * 0.25)) &&
            (sp.owningThread() != wt.getThreadNo()))
            return true;
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
}

void
WorkerThread::mainLoop() {
//    pd.log([&](std::ostream& os){os << "mainLoop, th:" << threadNo;});
    if (!et)
        et = Evaluate::getEvalHashTables();
    if (!kt)
        kt = std::make_shared<KillerTable>();
    if (!ht)
        ht = std::make_shared<History>();

//    SWTimer timer(pd, threadNo);
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    Position pos;
    std::shared_ptr<SplitPoint> sp;
    while (!shouldStop()) {
        int moveNo = -1;
        std::shared_ptr<SplitPoint> newSp = pd.wq.getWork(moveNo);
        if (newSp) {
            const SplitPointMove& spMove = newSp->getSpMove(moveNo);
            int depth = spMove.getDepth();
            if (depth < 0) { // Move skipped by forward pruning or legality check
                pd.wq.moveFinished(newSp, moveNo, false);
                continue;
            }
//            timer.startWork();
            if (sp != newSp) {
                sp = newSp;
                *ht = sp->getHistory();
                *kt = sp->getKillerTable();
            }
            Search::SearchTables st(tt, *kt, *ht, *et);
            sp->getPos(pos, spMove.getMove());
            std::vector<U64> posHashList;
            int posHashListSize;
            sp->getPosHashList(pos, posHashList, posHashListSize);
            Search sc(pos, posHashList, posHashListSize, st, pd, sp);
            sc.setThreadNo(threadNo);
            auto stopHandler(std::make_shared<ThreadStopHandler>(*this, pd, *sp,
                                                                 spMove, moveNo,
                                                                 sc));
            sc.setStopHandler(stopHandler);
            int alpha = sp->getAlpha();
            int beta = sp->getBeta();
            int ply = sp->getPly();
            int lmr = spMove.getLMR();
            int captSquare = spMove.getRecaptureSquare();
            bool inCheck = spMove.getInCheck();
            sc.setSearchTreeInfo(ply-1, sp->getSearchTreeInfo());
            try {
//                pd.log([&](std::ostream& os){os << "th:" << threadNo << " seqNo:" << sp->getSeqNo() << " ply:" << ply
//                                                << " c:" << sp->getCurrMoveNo() << " m:" << moveNo
//                                                << " a:" << alpha << " d:" << depth/SearchConst::plyScale
//                                                << " p:" << sp->getPMoveUseful(pd.fhInfo, moveNo) << " start";});
                const bool smp = pd.numHelperThreads() > 1;
                int score = -sc.negaScout(smp, -beta, -alpha, ply+1,
                                          depth, captSquare, inCheck);
                if ((lmr > 0) && (score > alpha))
                    score = -sc.negaScout(smp, -beta, -alpha, ply+1,
                                          depth + lmr, captSquare, inCheck);
                bool cancelRemaining = score >= beta;
//                pd.log([&](std::ostream& os){os << "th:" << threadNo << " seqNo:" << sp->getSeqNo() << " ply:" << ply
//                                                << " c:" << sp->getCurrMoveNo() << " m:" << moveNo
//                                                << " a:" << alpha << " s:" << score
//                                                << " d:" << depth/SearchConst::plyScale << " n:" << sc.getTotalNodesThisThread();});
                pd.wq.moveFinished(sp, moveNo, cancelRemaining);
            } catch (const Search::StopSearch&) {
//                pd.log([&](std::ostream& os){os << "th:" << threadNo << " seqNo:" << sp->getSeqNo() << " m:" << moveNo
//                                                << " aborted n:" << sc.getTotalNodesThisThread();});
                if (!spMove.isCanceled() && !stopThread)
                    pd.wq.returnMove(sp, moveNo);
            }
        } else {
            sp.reset();
//            timer.startSleep();
            pd.cv.wait_for(lock, std::chrono::microseconds(1000));
        }
    }
//    pd.log([&](std::ostream& os){os << "~mainLoop, th:" << threadNo;});
}

// ----------------------------------------------------------------------------

WorkQueue::WorkQueue(std::condition_variable& cv0, FailHighInfo& fhInfo0)
    : cv(cv0), fhInfo(fhInfo0) {
}

void
WorkQueue::addWork(const std::shared_ptr<SplitPoint>& sp) {
    std::lock_guard<std::mutex> L(mutex);
    assert(queue.find(sp) == queue.end());
    sp->setSeqNo();
    std::shared_ptr<SplitPoint> parent = sp->getParent();
    if (parent) {
        if (parent->isCanceled())
            sp->cancel();
        else
            parent->addChild(sp);
    }
    if (sp->hasUnFinishedMove()) {
        sp->computeProbabilities(fhInfo);
        insertInQueue(sp);
    }
}

std::shared_ptr<SplitPoint>
WorkQueue::getWork(int& spMove) {
    std::lock_guard<std::mutex> L(mutex);
    if (queue.empty())
        return nullptr;
    std::shared_ptr<SplitPoint> ret = *queue.begin();
    spMove = ret->getNextMove();
    maybeMoveToWaiting(ret);
    updateProbabilities(ret);
    return ret;
}

void
WorkQueue::returnMove(const std::shared_ptr<SplitPoint>& sp, int moveNo) {
    std::lock_guard<std::mutex> L(mutex);
    sp->returnMove(moveNo);
    if (!sp->hasUnFinishedMove()) {
        waiting.erase(sp);
        queue.erase(sp);
    } else if (sp->hasUnStartedMove()) {
        if (waiting.find(sp) != waiting.end()) {
            waiting.erase(sp);
            insertInQueue(sp);
        }
    } else {
        if (queue.find(sp) != queue.end()) {
            queue.erase(sp);
            waiting.insert(sp);
        }
    }
    updateProbabilities(sp);
}

void
WorkQueue::setOwnerCurrMove(const std::shared_ptr<SplitPoint>& sp, int moveNo) {
    std::lock_guard<std::mutex> L(mutex);
    sp->setOwnerCurrMove(moveNo);
    maybeMoveToWaiting(sp);
    updateProbabilities(sp);
}

void
WorkQueue::cancel(const std::shared_ptr<SplitPoint>& sp) {
    std::lock_guard<std::mutex> L(mutex);
    cancelInternal(sp);
}

void
WorkQueue::moveFinished(const std::shared_ptr<SplitPoint>& sp, int moveNo, bool cancelRemaining) {
    std::lock_guard<std::mutex> L(mutex);
    sp->moveFinished(moveNo, cancelRemaining);
    maybeMoveToWaiting(sp);
    updateProbabilities(sp);
}

double
WorkQueue::getBestProbability(std::shared_ptr<SplitPoint>& bestSp) const {
    std::lock_guard<std::mutex> L(mutex);
    if (queue.empty())
        return 0.0;
    bestSp = *queue.begin();
    return bestSp->getPNextMoveUseful();
}

double
WorkQueue::getBestProbability() const {
    std::shared_ptr<SplitPoint> bestSp;
    return getBestProbability(bestSp);
}

void
WorkQueue::maybeMoveToWaiting(const std::shared_ptr<SplitPoint>& sp) {
    if (!sp->hasUnStartedMove()) {
        queue.erase(sp);
        if (sp->hasUnFinishedMove())
            waiting.insert(sp);
        else
            waiting.erase(sp);
    }
}

void
WorkQueue::insertInQueue(const std::shared_ptr<SplitPoint>& sp) {
    bool wasEmpty = queue.empty();
    queue.insert(sp);
    if (wasEmpty)
        cv.notify_all();
}

void
WorkQueue::updateProbabilities(const std::shared_ptr<SplitPoint>& sp) {
    std::vector<std::shared_ptr<SplitPoint>> tmpQueue, tmpWaiting;
    removeFromSet(sp, queue, tmpQueue);
    removeFromSet(sp, waiting, tmpWaiting);
    sp->computeProbabilities(fhInfo);
    for (const auto& s : tmpQueue)
        queue.insert(s);
    for (const auto& s : tmpWaiting)
        waiting.insert(s);
}

void
WorkQueue::removeFromSet(const std::shared_ptr<SplitPoint>& sp,
                         std::set<std::shared_ptr<SplitPoint>, SplitPointCompare>& spSet,
                        std::vector<std::shared_ptr<SplitPoint>>& spVec) {
    if (spSet.erase(sp) > 0)
        spVec.push_back(sp);
    const auto& children = sp->getChildren();
    for (const auto& wChild : children) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child)
            removeFromSet(child, spSet, spVec);
    }
}

void
WorkQueue::cancelInternal(const std::shared_ptr<SplitPoint>& sp) {
    sp->cancel();
    queue.erase(sp);
    waiting.erase(sp);

    for (const auto& wChild : sp->getChildren()) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child)
            cancelInternal(child);
    }
}


// ----------------------------------------------------------------------------

ParallelData::ParallelData(TranspositionTable& tt0)
    : wq(cv, fhInfo), tt(tt0) {
    totalHelperNodes = 0;
}

void
ParallelData::addRemoveWorkers(int numWorkers) {
    while (numWorkers < (int)threads.size()) {
        assert(!threads.back()->threadRunning());
        threads.pop_back();
    }
    for (int i = threads.size(); i < numWorkers; i++)
        threads.push_back(std::make_shared<WorkerThread>(i+1, *this, tt));
}

void
ParallelData::startAll() {
    totalHelperNodes = 0;
    for (auto& thread : threads)
        thread->start();
}

void
ParallelData::stopAll() {
    for (auto& thread : threads)
        thread->stop(false);
    for (auto& thread : threads)
        thread->stop(true);
}

int
ParallelData::numHelperThreads() const {
    return threads.size();
}

S64
ParallelData::getNumSearchedNodes() const {
    return totalHelperNodes;
}

void
ParallelData::addSearchedNodes(S64 nNodes) {
    totalHelperNodes += nNodes;
}

// ----------------------------------------------------------------------------

SplitPoint::SplitPoint(int threadNo0,
                       const std::shared_ptr<SplitPoint>& parentSp0, int parentMoveNo0,
                       const Position& pos0, const std::vector<U64>& posHashList0,
                       int posHashListSize0, const SearchTreeInfo& sti0,
                       const KillerTable& kt0, const History& ht0,
                       int alpha0, int beta0, int ply0)
    : pos(pos0), posHashList(posHashList0), posHashListSize(posHashListSize0),
      searchTreeInfo(sti0), kt(kt0), ht(ht0),
      alpha(alpha0), beta(beta0), ply(ply0),
      pSpUseful(0.0), pNextMoveUseful(0.0),
      threadNo(threadNo0), parent(parentSp0), parentMoveNo(parentMoveNo0),
      seqNo(0), currMoveNo(0), canceled(false) {
}

void
SplitPoint::addMove(int moveNo, const SplitPointMove& spMove) {
    assert(moveNo >= (int)spMoves.size());
    while ((int)spMoves.size() < moveNo)
        spMoves.push_back(SplitPointMove(Move(), 0, -1, -1, false));
    spMoves.push_back(spMove);
}

void
SplitPoint::setSeqNo() {
    seqNo = nextSeqNo++;
}

void
SplitPoint::computeProbabilities(const FailHighInfo& fhInfo) {
    if (parent) {
        double pMoveUseful =
            fhInfo.getMoveNeededProbability(parent->parentMoveNo,
                                            parent->currMoveNo,
                                            parentMoveNo, parent->isAllNode());
        pSpUseful = parent->pSpUseful * pMoveUseful;
    } else {
        pSpUseful = 1.0;
    }
    double pNextUseful =
        fhInfo.getMoveNeededProbability(parentMoveNo, currMoveNo, findNextMove(), isAllNode());
    pNextMoveUseful = pSpUseful * pNextUseful;

    bool deleted = false;
    for (const auto& wChild : children) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child)
            child->computeProbabilities(fhInfo);
        else
            deleted = true;
    }
    if (deleted)
        cleanUpChildren();
}

std::shared_ptr<SplitPoint>
SplitPoint::getParent() const {
    return parent;
}

const std::vector<std::weak_ptr<SplitPoint>>&
SplitPoint::getChildren() const {
    return children;
}

double
SplitPoint::getPMoveUseful(const FailHighInfo& fhInfo, int moveNo) const {
    return pSpUseful * fhInfo.getMoveNeededProbability(parentMoveNo,
                                                       currMoveNo,
                                                       moveNo, isAllNode());
}

void
SplitPoint::getPos(Position& pos, const Move& move) const {
    pos = this->pos;
    UndoInfo ui;
    pos.makeMove(move, ui);
}

void
SplitPoint::getPosHashList(const Position& pos, std::vector<U64>& posHashList,
                           int& posHashListSize) const {
    posHashList = this->posHashList;
    posHashListSize = this->posHashListSize;
    posHashList[posHashListSize++] = pos.zobristHash();
}

int
SplitPoint::getNextMove() {
    int m = findNextMove();
    assert(m >= 0);
    spMoves[m].setSearching(true);
    return m;
}

void
SplitPoint::returnMove(int moveNo) {
    assert((moveNo >= 0) && (moveNo < (int)spMoves.size()));
    SplitPointMove& spm = spMoves[moveNo];
    spm.setSearching(false);
}

void
SplitPoint::setOwnerCurrMove(int moveNo) {
    assert((moveNo >= 0) && (moveNo < (int)spMoves.size()));
    spMoves[moveNo].setCanceled(true);
    currMoveNo = moveNo;
}

void
SplitPoint::cancel() {
    canceled = true;
    for (SplitPointMove& spMove : spMoves)
        spMove.setCanceled(true);
}

void
SplitPoint::moveFinished(int moveNo, bool cancelRemaining) {
    assert((moveNo >= 0) && (moveNo < (int)spMoves.size()));
    spMoves[moveNo].setSearching(false);
    spMoves[moveNo].setCanceled(true);
    if (cancelRemaining)
        for (int i = moveNo+1; i < (int)spMoves.size(); i++)
            spMoves[i].setCanceled(true);
}

bool
SplitPoint::hasUnStartedMove() const {
    if (canceled)
        return false;
    for (int i = currMoveNo + 1; i < (int)spMoves.size(); i++)
        if (!spMoves[i].isCanceled() && !spMoves[i].isSearching())
            return true;
    return false;
}

bool
SplitPoint::hasUnFinishedMove() const {
    if (canceled)
        return false;
    for (int i = currMoveNo + 1; i < (int)spMoves.size(); i++)
        if (!spMoves[i].isCanceled())
            return true;
    return false;
}

void
SplitPoint::addChild(const std::weak_ptr<SplitPoint>& child) {
    children.push_back(child);
}

int
SplitPoint::findNextMove() const {
    for (int i = currMoveNo+1; i < (int)spMoves.size(); i++)
        if (!spMoves[i].isCanceled() && !spMoves[i].isSearching())
            return i;
    return -1;
}

void
SplitPoint::cleanUpChildren() {
    std::vector<std::weak_ptr<SplitPoint>> toKeep;
    for (const auto& wChild : children)
        if (wChild.lock())
            toKeep.push_back(wChild);
    children = toKeep;
}

bool
SplitPoint::isAncestorTo(const SplitPoint& sp) const {
    const SplitPoint* tmp = &sp;
    while (tmp) {
        if (tmp == this)
            return true;
        tmp = &*(tmp->parent);
    }
    return false;
}

bool
SplitPoint::isAllNode() const {
    int nFirst = 0;
    const SplitPoint* tmp = this;
    while (tmp) {
        if (tmp->parentMoveNo == 0)
            nFirst++;
        else
            break;
        tmp = &*(tmp->parent);
    }
    return (nFirst % 2) != 0;
}

void
SplitPoint::print(std::ostream& os, int level, const FailHighInfo& fhInfo) const {
    std::string pad(level*2, ' ');
    os << pad << "seq:" << seqNo << " pos:" << TextIO::toFEN(pos) << std::endl;
    os << pad << "parent:" << parentMoveNo << " hashListSize:" << posHashListSize <<
        " a:" << alpha << " b:" << beta << " ply:" << ply << " canceled:" << canceled << std::endl;
    os << pad << "p1:" << pSpUseful << " p2:" << pNextMoveUseful << " curr:" << currMoveNo << std::endl;
    os << pad << "moves:";
    for (int mi = 0; mi < (int)spMoves.size(); mi++) {
        const auto& spm = spMoves[mi];
        os << ' ' << TextIO::moveToUCIString(spm.getMove());
        if (spm.isCanceled())
            os << ",c";
        if (spm.isSearching())
            os << ",s";
        os << "," << fhInfo.getMoveNeededProbability(parentMoveNo, currMoveNo, mi, isAllNode());
    }
    os << std::endl;
    for (const auto& wChild : children) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child)
            child->print(os, level+1, fhInfo);
    }
}

// ----------------------------------------------------------------------------

SplitPointMove::SplitPointMove(const Move& move0, int lmr0, int depth0,
                               int captSquare0, bool inCheck0)
    : move(move0), lmr(lmr0), depth(depth0), captSquare(captSquare0),
      inCheck(inCheck0), canceled(false), searching(false) {
}

// ----------------------------------------------------------------------------

SplitPointHolder::SplitPointHolder(ParallelData& pd0,
                                   std::vector<std::shared_ptr<SplitPoint>>& spVec0)
    : pd(pd0), spVec(spVec0), state(State::EMPTY) {
}

SplitPointHolder::~SplitPointHolder() {
    if (state == State::QUEUED) {
//        pd.log([&](std::ostream& os){os << "cancel seqNo:" << sp->getSeqNo();});
        pd.wq.cancel(sp);
        assert(!spVec.empty());
        spVec.pop_back();
    }
}

void
SplitPointHolder::setSp(const std::shared_ptr<SplitPoint>& sp0) {
    assert(state == State::EMPTY);
    assert(sp0);
    sp = sp0;
    state = State::CREATED;
}

void
SplitPointHolder::addMove(int moveNo, const SplitPointMove& spMove) {
    assert(state == State::CREATED);
    sp->addMove(moveNo, spMove);
}

void
SplitPointHolder::addToQueue() {
    assert(state == State::CREATED);
    pd.wq.addWork(sp);
    spVec.push_back(sp);
//    pd.log([&](std::ostream& os){os << "add seqNo:" << sp->getSeqNo() << " ply:" << sp->getPly()
//                                    << " pNext:" << sp->getPNextMoveUseful()
//                                    << " pMove:" << sp->getParentMoveNo() << " vec:" << spVec.size();});
    state = State::QUEUED;
}

void
SplitPointHolder::setOwnerCurrMove(int moveNo) {
//    pd.log([&](std::ostream& os){os << "seqNo:" << sp->getSeqNo() << " currMove:" << moveNo;});
    pd.wq.setOwnerCurrMove(sp, moveNo);
}

bool
SplitPointHolder::isAllNode() const {
    return sp->isAllNode();
}

// ----------------------------------------------------------------------------

FailHighInfo::FailHighInfo()
    : totCount(0) {
    for (int i = 0; i < NUM_NODE_TYPES; i++) {
        for (int j = 0; j < NUM_STAT_MOVES; j++)
            failHiCount[i][j] = 0;
        failLoCount[i] = 0;
    }
}

double
FailHighInfo::getMoveNeededProbability(int parentMoveNo,
                                       int currMoveNo, int moveNo, bool allNode) const {
    std::lock_guard<std::mutex> L(mutex);
    const int pIdx = getNodeType(parentMoveNo, allNode);
    moveNo = std::min(moveNo, NUM_STAT_MOVES-1);
    if (moveNo < 0)
        return 0.0;

    int nNeeded = failLoCount[pIdx];
    for (int i = moveNo; i < NUM_STAT_MOVES; i++)
        nNeeded += failHiCount[pIdx][i];

    int nTotal = nNeeded;
    for (int i = currMoveNo; i < moveNo; i++)
        nTotal += failHiCount[pIdx][i];

    return (nTotal > 0) ? nNeeded / (double)nTotal : 0.5;
}

void
FailHighInfo::addData(int parentMoveNo, int nSearched, bool failHigh, bool allNode) {
    if (nSearched < 0)
        return;
    std::lock_guard<std::mutex> L(mutex);
    const int pIdx = getNodeType(parentMoveNo, allNode);
    if (failHigh) {
        nSearched = std::min(nSearched, NUM_STAT_MOVES-1);
        failHiCount[pIdx][nSearched]++;
    } else {
        failLoCount[pIdx]++;
    }
    totCount++;
    if (totCount >= 1000000)
        reScaleInternal(2);
}

void
FailHighInfo::reScale() {
    std::lock_guard<std::mutex> L(mutex);
    reScaleInternal(4);
}

void
FailHighInfo::reScaleInternal(int factor) {
    for (int i = 0; i < NUM_NODE_TYPES; i++) {
        for (int j = 0; j < NUM_STAT_MOVES; j++)
            failHiCount[i][j] /= factor;
        failLoCount[i] /= factor;
    }
    totCount /= factor;
}

void
FailHighInfo::print(std::ostream& os) const {
    std::lock_guard<std::mutex> L(mutex);
    for (int i = 0; i < NUM_NODE_TYPES; i++) {
        os << "fhInfo: " << i << ' ' << std::setw(6) << failLoCount[i];
        for (int j = 0; j < NUM_STAT_MOVES; j++)
            os << ' ' << std::setw(6) << failHiCount[i][j];
        os << std::endl;
    }
}
