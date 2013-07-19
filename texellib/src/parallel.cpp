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
    thread = std::make_shared<std::thread>([this](){ mainLoop(); });
}

void
WorkerThread::stop(bool wait) {
    stopThread = true;
    if (wait) {
        pd.cv.notify_all();
        if (thread) {
            thread->join();
            thread.reset();
            stopThread = false;
        }
    }
}

class ThreadStopHandler : public Search::StopHandler {
public:
    ThreadStopHandler(const WorkerThread& wt, const WorkQueue& wq,
                      const SplitPoint& sp, const SplitPointMove& spm,
                      const FailHighInfo& fhInfo, int moveNo);

    bool shouldStop();

private:
    const WorkerThread& wt;
    const WorkQueue& wq;
    const SplitPoint& sp;
    const SplitPointMove& spMove;
    const FailHighInfo& fhInfo;
    const int moveNo;
    int counter;             // Counts number of calls to shouldStop
    int nextProbCheck;       // Next time test for SplitPoint switch should be performed
};

ThreadStopHandler::ThreadStopHandler(const WorkerThread& wt0, const WorkQueue& wq0,
                                     const SplitPoint& sp0, const SplitPointMove& spm0,
                                     const FailHighInfo& fhInfo0, int moveNo0)
    : wt(wt0), wq(wq0), sp(sp0), spMove(spm0), fhInfo(fhInfo0), moveNo(moveNo0),
      counter(0), nextProbCheck(1) {
}

bool
ThreadStopHandler::shouldStop() {
    if (wt.shouldStop() || spMove.isCanceled())
        return true;

    counter++;
    if (counter >= nextProbCheck) {
        nextProbCheck = counter + 1 + counter / 4;
        double myProb = sp.getPMoveUseful(fhInfo, moveNo);
        std::shared_ptr<SplitPoint> bestSp;
        double bestProb = wq.getBestProbability(bestSp);
        if ((bestProb > myProb + 0.01) && (bestProb >= (myProb + (1.0 - myProb) * 0.25)) &&
            !sp.isAncestorTo(*bestSp))
            return true;
    }

    return false;
}


void
WorkerThread::mainLoop() {
    if (!et)
        et = Evaluate::getEvalHashTables();
    if (!kt)
        kt = std::make_shared<KillerTable>();
    if (!ht)
        ht = std::make_shared<History>();

    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    Position pos;
    std::shared_ptr<SplitPoint> sp;
    while (!shouldStop()) {
        int moveNo = -1;
        std::shared_ptr<SplitPoint> newSp = pd.wq.getWork(moveNo);
        if (newSp) {
            const SplitPointMove& spMove = newSp->getSpMove(moveNo);
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
            Search sc(pos, posHashList, posHashListSize, st, pd);
            auto stopHandler(std::make_shared<ThreadStopHandler>(*this, pd.wq, *sp,
                                                                 spMove, pd.fhInfo, moveNo));
            sc.setStopHandler(stopHandler);
            int alpha = sp->getAlpha();
            int beta = sp->getBeta();
            int ply = sp->getPly();
            int depth = spMove.getDepth();
            int lmr = spMove.getLMR();
            int captSquare = spMove.getRecaptureSquare();
            bool inCheck = spMove.getInCheck();
            sc.setSearchTreeInfo(ply-1, sp->getSearchTreeInfo());
            try {
                int score = sc.negaScout(alpha, beta, ply,
                                         depth, captSquare, inCheck);
                if ((lmr > 0) && (score < beta))
                    score = sc.negaScout(alpha, beta, ply,
                                         depth + lmr, captSquare, inCheck);
                bool cancelRemaining = score < beta;
                pd.wq.moveFinished(sp, moveNo, cancelRemaining);
            } catch (const Search::StopSearch&) {
                if (!spMove.isCanceled() && !stopThread)
                    pd.wq.returnMove(sp, moveNo);
            }
        } else {
            sp.reset();
            pd.cv.wait_for(lock, std::chrono::microseconds(1000));
        }
    }
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
        if (parent->hasUnFinishedMove())
            parent->addChild(sp);
        else
            sp->cancel();
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
        return std::shared_ptr<SplitPoint>();
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

bool
ParallelData::isSMP() const {
    return !threads.empty();
}

// ----------------------------------------------------------------------------

SplitPoint::SplitPoint(const std::shared_ptr<SplitPoint>& parentSp0, int parentMoveNo0,
                       const Position& pos0, const std::vector<U64>& posHashList0,
                       int posHashListSize0, const SearchTreeInfo& sti0,
                       const KillerTable& kt0, const History& ht0,
                       int alpha0, int beta0, int ply0)
    : pos(pos0), posHashList(posHashList0), posHashListSize(posHashListSize0),
      searchTreeInfo(sti0), kt(kt0), ht(ht0),
      alpha(alpha0), beta(beta0), ply(ply0),
      pSpUseful(0.0), pNextMoveUseful(0.0),
      parent(parentSp0), parentMoveNo(parentMoveNo0),
      seqNo(0), currMoveNo(0) {
}

void
SplitPoint::addMove(const SplitPointMove& spMove) {
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
                                            parentMoveNo);
        pSpUseful = parent->pSpUseful * pMoveUseful;
    } else {
        pSpUseful = 1.0;
    }
    double pNextUseful =
        fhInfo.getMoveNeededProbability(parentMoveNo, currMoveNo, findNextMove());
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
                                                       moveNo);
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
    for (int i = currMoveNo + 1; i < (int)spMoves.size(); i++)
        if (!spMoves[i].isCanceled() && !spMoves[i].isSearching())
            return true;
    return false;
}

bool
SplitPoint::hasUnFinishedMove() const {
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

void
SplitPoint::print(std::ostream& os, int level, const FailHighInfo& fhInfo) const {
    std::string pad(level*2, ' ');
    os << pad << "seq:" << seqNo << " pos:" << TextIO::toFEN(pos) << std::endl;
    os << pad << "parent:" << parentMoveNo << " hashListSize:" << posHashListSize <<
        " a:" << alpha << " b:" << beta << " ply:" << ply << std::endl;
    os << pad << "p1:" << pSpUseful << " p2:" << pNextMoveUseful << " curr:" << currMoveNo << std::endl;
    os << pad << "moves:";
    for (int mi = 0; mi < (int)spMoves.size(); mi++) {
        const auto& spm = spMoves[mi];
        os << ' ' << TextIO::moveToUCIString(spm.getMove());
        if (spm.isCanceled())
            os << ",c";
        if (spm.isSearching())
            os << ",s";
        os << "," << fhInfo.getMoveNeededProbability(parentMoveNo, currMoveNo, mi);
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
SplitPointHolder::addMove(const SplitPointMove& spMove) {
    assert(state == State::CREATED);
    sp->addMove(spMove);
}

void
SplitPointHolder::addToQueue() {
    assert(state == State::CREATED);
    pd.wq.addWork(sp);
    spVec.push_back(sp);
    state = State::QUEUED;
}

// ----------------------------------------------------------------------------

FailHighInfo::FailHighInfo()
    : totCount(0) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < NUM_STAT_MOVES; j++)
            failHiCount[i][j] = 0;
        failLoCount[i] = 0;
    }
}

double
FailHighInfo::getMoveNeededProbability(int parentMoveNo,
                                       int currMoveNo, int moveNo) const {
    std::lock_guard<std::mutex> L(mutex);
    const int pIdx = parentMoveNo > 0 ? 1 : 0;
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
FailHighInfo::addData(int parentMoveNo, int nSearched, bool failHigh) {
    std::lock_guard<std::mutex> L(mutex);
    const int pIdx = parentMoveNo > 0 ? 1 : 0;
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
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < NUM_STAT_MOVES; j++)
            failHiCount[i][j] /= factor;
        failLoCount[i] /= factor;
    }
    totCount /= factor;
}
