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
 * parallelTest.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: petero
 */

#define _GLIBCXX_USE_NANOSLEEP

#include "parallelTest.hpp"
#include "parallel.hpp"
#include "position.hpp"
#include "textio.hpp"
#include "searchUtil.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <chrono>

#include "cute.h"


void
ParallelTest::testFailHighInfo() {
    const double eps = 1e-8;
    FailHighInfo fhi;

    // Probability 0.5 when no data available
    for (int i = 0; i < 20; i++) {
        for (int j = 0; i < 20; i++) {
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(0, i, j), eps);
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(1, i, j), eps);
        }
    }

    for (int i = 0; i < 15; i++)
        fhi.addData(0, i, true);
    for (int curr = 0; curr < 15; curr++) {
        for (int m = curr; m < 15; m++) {
            double e = (15 - m) / (double)(15 - curr);
            ASSERT_EQUAL_DELTA(e, fhi.getMoveNeededProbability(0, curr, m), eps);
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(1, curr, m), eps);
        }
    }
    for (int i = 0; i < 15; i++)
        fhi.addData(0, i, false);
    for (int curr = 0; curr < 15; curr++) {
        for (int m = curr; m < 15; m++) {
            double e = (15 - m + 15) / (double)(15 - curr + 15);
            ASSERT_EQUAL_DELTA(e, fhi.getMoveNeededProbability(0, curr, m), eps);
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(1, curr, m), eps);
        }
    }
}

void
ParallelTest::testWorkQueue() {
    const double eps = 1e-8;
    TranspositionTable tt(10);
    ParallelData pd(tt);
    WorkQueue& wq = pd.wq;
    FailHighInfo& fhi = pd.fhInfo;
    int moveNo = -1;
    std::shared_ptr<SplitPoint> sp = wq.getWork(moveNo);
    ASSERT(!sp);
    double prob = wq.getBestProbability();
    ASSERT_EQUAL_DELTA(0.0, prob, eps);
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true);
                if (m > 0)
                    fhi.addData(m, i, false);
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    auto sp1 = std::make_shared<SplitPoint>(nullRoot, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, -10, 10, 1);
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, 4, -1, false));
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("g1f3"), 0, 4, -1, false));
    ASSERT_EQUAL(-10, sp1->getAlpha());
    ASSERT_EQUAL(10, sp1->getBeta());
    ASSERT_EQUAL(1, sp1->getPly());
    ASSERT_EQUAL(0, sp1->getChildren().size());
    ASSERT_EQUAL(3, sp1->spMoves.size());
    ASSERT(pos.equals(sp1->pos));

    wq.addWork(sp1);
    ASSERT_EQUAL(1, sp1->findNextMove());
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    ASSERT_EQUAL_DELTA(511 / 1023.0, wq.getBestProbability(), eps);

    sp = wq.getWork(moveNo);
    ASSERT_EQUAL(1, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(2, sp1->findNextMove());
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    ASSERT_EQUAL_DELTA(255 / 1023.0, wq.getBestProbability(), eps);

    wq.setOwnerCurrMove(sp1, 1);
    ASSERT_EQUAL(2, sp1->findNextMove());
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    ASSERT_EQUAL_DELTA(255 / 511.0, wq.getBestProbability(), eps);

    sp = wq.getWork(moveNo);
    ASSERT_EQUAL(2, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(-1, sp1->findNextMove());
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(1, wq.waiting.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);

    wq.returnMove(sp, 2);
    ASSERT_EQUAL(2, sp1->findNextMove());
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    ASSERT_EQUAL_DELTA(255 / 511.0, wq.getBestProbability(), eps);

    sp = wq.getWork(moveNo);
    ASSERT_EQUAL(2, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(-1, sp1->findNextMove());
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(1, wq.waiting.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);

    wq.moveFinished(sp1, 2, false);
    ASSERT_EQUAL(-1, sp1->findNextMove());
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);

    // Split point contains no moves, should not be added to queue/waiting
    sp.reset();
    sp1 = std::make_shared<SplitPoint>(nullRoot, 0,
                                       pos, posHashList, posHashListSize,
                                       sti, kt, ht, -10, 10, 1);
    wq.addWork(sp1);
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());

    // Split point contains only one move, should not be added to queue/waiting
    sp1 = std::make_shared<SplitPoint>(nullRoot, 0,
                                       pos, posHashList, posHashListSize,
                                       sti, kt, ht, -10, 10, 1);
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("f2f4"), 0, 4, -1, false));
    wq.addWork(sp1);
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());


    // Test return non-last currently searched move
    sp1 = std::make_shared<SplitPoint>(nullRoot, 1,
                                       pos, posHashList, posHashListSize,
                                       sti, kt, ht, -10, 10, 1);
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("a2a4"), 0, 4, -1, false));
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("b2b4"), 0, 4, -1, false));
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, 4, -1, false));
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    wq.addWork(sp1);
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    sp = wq.getWork(moveNo);
    sp = wq.getWork(moveNo);
    ASSERT_EQUAL(2, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(3, sp1->findNextMove());
    ASSERT_EQUAL_DELTA((127 + 1023) / (1023.0*2), wq.getBestProbability(), eps);
    wq.returnMove(sp, 1);
    ASSERT_EQUAL(1, sp1->findNextMove());
    ASSERT_EQUAL_DELTA((511 + 1023) / (1023.0*2), wq.getBestProbability(), eps);
    sp = wq.getWork(moveNo);
    ASSERT_EQUAL(1, moveNo);
    ASSERT_EQUAL(3, sp1->findNextMove());
    ASSERT_EQUAL_DELTA((127 + 1023) / (1023.0*2), wq.getBestProbability(), eps);
    wq.moveFinished(sp1, 2, true);
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(1, wq.waiting.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);
    wq.moveFinished(sp1, 1, true);
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);
}

void
ParallelTest::testWorkQueueParentChild() {
    const double eps = 1e-8;
    TranspositionTable tt(10);
    ParallelData pd(tt);
    WorkQueue& wq = pd.wq;
    FailHighInfo& fhi = pd.fhInfo;

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true);
                if (m == 0)
                    fhi.addData(m, i, false);
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    auto sp1 = std::make_shared<SplitPoint>(nullRoot, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, -10, 10, 1);
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, 4, -1, false));
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, 4, -1, false));
    sp1->addMove(SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    wq.addWork(sp1);
    ASSERT(sp1->isAncestorTo(*sp1));

    pos.makeMove(TextIO::uciStringToMove("e2e4"), ui);
    posHashList[posHashListSize++] = pos.zobristHash();
    auto sp2 = std::make_shared<SplitPoint>(sp1, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, -10, 10, 1);
    sp2->addMove(SplitPointMove(TextIO::uciStringToMove("e7e5"), 0, 4, -1, false));
    sp2->addMove(SplitPointMove(TextIO::uciStringToMove("c7c5"), 0, 4, -1, false));
    wq.addWork(sp2);
    ASSERT( sp1->isAncestorTo(*sp2));
    ASSERT(!sp2->isAncestorTo(*sp1));

    pos.makeMove(TextIO::uciStringToMove("e7e5"), ui);
    posHashList[posHashListSize++] = pos.zobristHash();
    auto sp3 = std::make_shared<SplitPoint>(sp2, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, -10, 10, 1);
    sp3->addMove(SplitPointMove(TextIO::uciStringToMove("g1f3"), 0, 4, -1, false));
    sp3->addMove(SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    sp3->addMove(SplitPointMove(TextIO::uciStringToMove("c2c3"), 0, 4, -1, false));
    wq.addWork(sp3);
    ASSERT( sp1->isAncestorTo(*sp3));
    ASSERT( sp2->isAncestorTo(*sp3));
    ASSERT(!sp3->isAncestorTo(*sp1));
    ASSERT(!sp3->isAncestorTo(*sp2));

    pos = TextIO::readFEN(TextIO::startPosFEN);
    posHashListSize = 1;
    pos.makeMove(TextIO::uciStringToMove("d2d4"), ui);
    posHashList[posHashListSize++] = pos.zobristHash();
    auto sp4 = std::make_shared<SplitPoint>(sp1, 2,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, -10, 10, 1);
    sp4->addMove(SplitPointMove(TextIO::uciStringToMove("d7d5"), 0, 4, -1, false));
    sp4->addMove(SplitPointMove(TextIO::uciStringToMove("g8f6"), 0, 4, -1, false));
    wq.addWork(sp4);
    ASSERT( sp1->isAncestorTo(*sp4));
    ASSERT(!sp2->isAncestorTo(*sp4));
    ASSERT(!sp3->isAncestorTo(*sp4));
    ASSERT(!sp4->isAncestorTo(*sp1));
    ASSERT(!sp4->isAncestorTo(*sp2));
    ASSERT(!sp4->isAncestorTo(*sp3));

    ASSERT_EQUAL(4, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
    ASSERT_EQUAL(2, sp1->getChildren().size());
    ASSERT_EQUAL(1, sp2->getChildren().size());
    ASSERT_EQUAL(0, sp3->getChildren().size());
    ASSERT_EQUAL(0, sp4->getChildren().size());
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp2->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp2->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp3->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp3->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2)*511/1023, sp4->getPNextMoveUseful(), eps);

    std::vector<std::shared_ptr<SplitPoint>> q(wq.queue.begin(), wq.queue.end());
    ASSERT_EQUAL(sp1, q[0]);
    ASSERT_EQUAL(sp2, q[1]);
    ASSERT_EQUAL(sp3, q[2]);
    ASSERT_EQUAL(sp4, q[3]);

    wq.setOwnerCurrMove(sp3, 1);
    ASSERT_EQUAL(4, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp2->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp2->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp3->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0), sp3->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2)*511/1023, sp4->getPNextMoveUseful(), eps);

    std::vector<std::shared_ptr<SplitPoint>> q2(wq.queue.begin(), wq.queue.end());
    ASSERT_EQUAL(sp3, q2[0]);
    ASSERT_EQUAL(sp1, q2[1]);
    ASSERT_EQUAL(sp2, q2[2]);
    ASSERT_EQUAL(sp4, q2[3]);

    wq.cancel(sp2);
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL(2, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());

    std::vector<std::shared_ptr<SplitPoint>> q3(wq.queue.begin(), wq.queue.end());
    ASSERT_EQUAL(sp1, q3[0]);
    ASSERT_EQUAL(sp4, q3[1]);
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2)*511/1023, sp4->getPNextMoveUseful(), eps);

    wq.setOwnerCurrMove(sp1, 1);
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL(2, wq.queue.size());
    ASSERT_EQUAL(0, wq.waiting.size());

    std::vector<std::shared_ptr<SplitPoint>> q4(wq.queue.begin(), wq.queue.end());
    ASSERT_EQUAL(sp1, q4[0]);
    ASSERT_EQUAL(sp4, q4[1]);
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0)*511/1023, sp4->getPNextMoveUseful(), eps);
}

void
ParallelTest::testSplitPointHolder() {
    TranspositionTable tt(10);
    ParallelData pd(tt);
    WorkQueue& wq = pd.wq;
    FailHighInfo& fhi = pd.fhInfo;
    std::vector<std::shared_ptr<SplitPoint>> spVec;

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true);
                if (m == 0)
                    fhi.addData(m, i, false);
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    {
        SplitPointHolder sph(wq, spVec);
        ASSERT_EQUAL(0, wq.queue.size());
        ASSERT_EQUAL(0, spVec.size());
        sph.setSp(std::make_shared<SplitPoint>(nullRoot, 0,
                                               pos, posHashList, posHashListSize,
                                               sti, kt, ht, -10, 10, 1));
        ASSERT_EQUAL(0, wq.queue.size());
        ASSERT_EQUAL(0, spVec.size());
        sph.addMove(SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, 4, -1, false));
        sph.addMove(SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, 4, -1, false));
        sph.addMove(SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
        ASSERT_EQUAL(0, wq.queue.size());
        ASSERT_EQUAL(0, spVec.size());
        sph.addToQueue();
        ASSERT_EQUAL(1, wq.queue.size());
        ASSERT_EQUAL(1, spVec.size());
        {
            SplitPointHolder sph2(wq, spVec);
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
        }
        ASSERT_EQUAL(1, wq.queue.size());
        ASSERT_EQUAL(1, spVec.size());
        {
            SplitPointHolder sph2(wq, spVec);
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
            sph2.setSp(std::make_shared<SplitPoint>(spVec.back(), 0,
                                                    pos, posHashList, posHashListSize,
                                                    sti, kt, ht, -10, 10, 1));
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
            sph2.addMove(SplitPointMove(TextIO::uciStringToMove("g8f6"), 0, 4, -1, false));
            sph2.addMove(SplitPointMove(TextIO::uciStringToMove("c7c6"), 0, 4, -1, false));
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
            sph2.addToQueue();
            ASSERT_EQUAL(2, wq.queue.size());
            ASSERT_EQUAL(2, spVec.size());
        }
        ASSERT_EQUAL(1, wq.queue.size());
        ASSERT_EQUAL(1, spVec.size());
    }
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(0, spVec.size());
}

static void
probeTT(Position& pos, const Move& m, TranspositionTable& tt, TranspositionTable::TTEntry& ent) {
    UndoInfo ui;
    pos.makeMove(m, ui);
    ent.clear();
    tt.probe(pos.historyHash(), ent);
    pos.unMakeMove(m, ui);
}

void
ParallelTest::testWorkerThread() {
    TranspositionTable tt(16);
    ParallelData pd(tt);
    WorkQueue& wq = pd.wq;
    FailHighInfo& fhi = pd.fhInfo;
    std::vector<std::shared_ptr<SplitPoint>> spVec;

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true);
                if (m == 0)
                    fhi.addData(m, i, false);
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    pd.addRemoveWorkers(3);

    {
        SplitPointHolder sph(wq, spVec);
        auto sp = std::make_shared<SplitPoint>(nullRoot, 0,
                                               pos, posHashList, posHashListSize,
                                               sti, kt, ht, -10, 10, 1);
        sph.setSp(sp);
        const int plyScale = SearchConst::plyScale;
        int depth = 10 * plyScale;
        sph.addMove(SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, depth, -1, false));
        sph.addMove(SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, depth, -1, false));
        sph.addMove(SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, depth, -1, false));
        sph.addToQueue();
        pd.startAll();
        while (sp->hasUnFinishedMove()) {
            std::cout << "waiting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        pd.stopAll();

        TranspositionTable::TTEntry ent;
        probeTT(pos, TextIO::uciStringToMove("e2e4"), tt, ent);
        ASSERT(ent.getType() == TType::T_EMPTY); // Only searched by "master" thread, which does not exist in this test

        probeTT(pos, TextIO::uciStringToMove("c2c4"), tt, ent);
        ASSERT(ent.getType() != TType::T_EMPTY);
        ASSERT_EQUAL(10 * plyScale, ent.getDepth());
        probeTT(pos, TextIO::uciStringToMove("d2d4"), tt, ent);
        ASSERT(ent.getType() != TType::T_EMPTY);
        ASSERT_EQUAL(10 * plyScale, ent.getDepth());
    }
}

cute::suite
ParallelTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testFailHighInfo));
    s.push_back(CUTE(testWorkQueue));
    s.push_back(CUTE(testWorkQueueParentChild));
    s.push_back(CUTE(testSplitPointHolder));
    s.push_back(CUTE(testWorkerThread));
    return s;
}
