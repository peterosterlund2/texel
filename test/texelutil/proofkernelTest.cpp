/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * proofkernelTest.cpp
 *
 *  Created on: Oct 23, 2021
 *      Author: petero
 */

#include "proofkernelTest.hpp"
#include "proofkernel.hpp"
#include "position.hpp"
#include "textio.hpp"
#include "posutil.hpp"

#include "gtest/gtest.h"


TEST(ProofKernelTest, testPawnColumn) {
    ProofKernelTest::testPawnColumn();
}

void
ProofKernelTest::testPawnColumn() {
    using PawnColumn = ProofKernel::PawnColumn;
    auto WHITE = ProofKernel::WHITE;
    auto BLACK = ProofKernel::BLACK;

    PawnColumn col;
    ASSERT_EQ(0, col.nPawns());

    col.addPawn(0, WHITE);
    ASSERT_EQ(1, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));

    col.addPawn(1, BLACK);
    ASSERT_EQ(2, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(BLACK, col.getPawn(1));

    col.addPawn(0, BLACK);
    ASSERT_EQ(3, col.nPawns());
    ASSERT_EQ(BLACK, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));

    col.addPawn(1, WHITE);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(BLACK, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(WHITE, col.getPawn(2));
    ASSERT_EQ(BLACK, col.getPawn(3));

    col.removePawn(0);
    ASSERT_EQ(3, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));

    col.addPawn(3, WHITE);
    col.addPawn(4, BLACK);
    ASSERT_EQ(5, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(WHITE, col.getPawn(3));
    ASSERT_EQ(BLACK, col.getPawn(4));

    col.removePawn(3);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(BLACK, col.getPawn(3));

    col.setPawn(3, WHITE);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(WHITE, col.getPawn(3));

    for (int i = 0; i < 2; i++) {
        col.setPawn(1, BLACK);
        ASSERT_EQ(4, col.nPawns());
        ASSERT_EQ(WHITE, col.getPawn(0));
        ASSERT_EQ(BLACK, col.getPawn(1));
        ASSERT_EQ(BLACK, col.getPawn(2));
        ASSERT_EQ(WHITE, col.getPawn(3));
    }
}

TEST(ProofKernelTest, testPawnColPromotion) {
    ProofKernelTest::testPawnColPromotion();
}

void
ProofKernelTest::testPawnColPromotion() {
    using PawnColumn = ProofKernel::PawnColumn;
    auto WHITE = ProofKernel::WHITE;
    auto BLACK = ProofKernel::BLACK;

    auto setPawns = [](PawnColumn& col,
                       const std::vector<ProofKernel::PieceColor>& v) {
        while (col.nPawns() > 0)
            col.removePawn(0);
        for (auto c : v)
            col.addPawn(col.nPawns(), c);
    };

    PawnColumn col;
    setPawns(col, {WHITE, BLACK});
    ASSERT_EQ(0, col.nPromotions(WHITE));
    ASSERT_EQ(0, col.nPromotions(BLACK));

    setPawns(col, {BLACK, WHITE});
    ASSERT_EQ(1, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {WHITE});
    ASSERT_EQ(1, col.nPromotions(WHITE));
    ASSERT_EQ(0, col.nPromotions(BLACK));

    setPawns(col, {BLACK});
    ASSERT_EQ(0, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {BLACK, WHITE, WHITE});
    ASSERT_EQ(2, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {BLACK, WHITE, BLACK, WHITE, WHITE});
    ASSERT_EQ(2, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {WHITE, WHITE});
    ASSERT_EQ(2, col.nPromotions(WHITE));
    ASSERT_EQ(0, col.nPromotions(BLACK));

    setPawns(col, {BLACK, BLACK});
    ASSERT_EQ(0, col.nPromotions(WHITE));
    ASSERT_EQ(2, col.nPromotions(BLACK));
}

TEST(ProofKernelTest, testGoal) {
    ProofKernelTest::testGoal();
}

void
ProofKernelTest::testGoal() {
    auto test = [](const std::string& start, const std::string& goal, bool expected) {
        Position startPos = TextIO::readFEN(start);
        Position goalPos = TextIO::readFEN(goal);
        ASSERT_EQ(expected, ProofKernel(startPos, goalPos).isGoal()) << "start: " << start << "\ngoal: " << goal;

        startPos = PosUtil::swapColors(startPos);
        goalPos = PosUtil::swapColors(goalPos);
        ASSERT_EQ(expected, ProofKernel(startPos, goalPos).isGoal())
            << "start: " << TextIO::toFEN(startPos) << "\ngoal: " << TextIO::toFEN(goalPos);
    };

    test(TextIO::startPosFEN, TextIO::startPosFEN, true);
    test(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", true);

    test(TextIO::startPosFEN, "rnbqkbnr/1ppppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQk - 0 1", false);
    test("rnbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1ppppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQk - 0 1", true);
    test("rnbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/1p6/8/8/4P3/1PPP1PPP/RNBBKBNR w KQk - 0 1", true);
    test("rnbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1ppppppp/8/8/8/5P2/1PPPP1PP/RNBQKBBR w KQk - 0 1", false);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/1p6/8/8/2P2PP1/3PP2P/RBBQKBNB w Qk - 0 1", true);

    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/8/2PPPPPP/RNNQKNNR w KQk - 0 1", true);
    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/5P2/2PPP1PP/RBBQKBBR w KQk - 0 1", true);
    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/5P2/2PPP1PP/BNBQKBBR w Kk - 0 1", false);

    test("rnbqkbnr/2ppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RRBQKRQR w KQ - 0 1", true);
    test("rnbqkbnr/2ppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RQBQKQQR w KQ - 0 1", true);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RRBQKRQR w KQ - 0 1", false);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RQBQKQQR w KQk - 0 1", false);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1pppppp1/8/8/8/6P1/2PPPP2/RQBQKBQR w KQ - 0 1", true);

    // Blocked rook because of castling rights
    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/5P2/2PPP1PP/RBBQKBBR w KQkq - 0 1", false);
    test("rnbqkbnr/2ppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RQBQKQQR w KQk - 0 1", false);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1pppppp1/8/8/8/6P1/2PPPP2/RQBQKBQR w KQk - 0 1", false);

    // Blocked king because of castling rights
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQkq - 0 1", false);
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQk - 0 1", false);
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQq - 0 1", false);
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQ - 0 1", true);

    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQkq - 0 1", false);
    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQq - 0 1", false);
    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQk - 0 1", false);
    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQ - 0 1", true);

    // Blocked bishop
    test("rnbqkbnr/pp1ppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pp1ppppp/8/8/8/8/PPRPPPPP/RNBQKBNR w KQkq - 0 1", false);
    test("rnbqkbnr/p2ppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p2ppppp/8/8/8/8/P1RPPPPP/RNBQKBNR w KQkq - 0 1", true);

    // Enough number of pawns but in wrong files or wrong order within files
    test(TextIO::startPosFEN, "rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/RNBQKBNR w KQkq - 0 1", false);
    test(TextIO::startPosFEN, "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPPP/RNBQKBNR w KQkq - 0 1", false);
    test(TextIO::startPosFEN, "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPP1/RNBQKBNR w Qq - 0 1", false);
    test("rnbqkbnr/ppppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPP1/RNBQKBNR w Qq - 0 1", false);
    test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPP1/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPP1/RNBQKBNR w Qq - 0 1", false);
    test("rnbqkbnr/ppppp1pp/8/8/8/8/PPPPPPP1/RNBQKBNR w - - 0 1",
         "rnbqkbnr/p1ppp1p1/8/8/8/1P6/1PPPP1P1/RNBQKBNR w - - 0 1", false);

    // No unique match on b file
    test("r2qk2r/2pp1ppp/4p3/1P6/1p6/1P2P3/1pPP1PPP/R2QK2R w KQkq - 0 1",
         "r2q1k1r/2pp1ppp/4p3/1P6/1p6/4P3/2PP1PPP/R2Q1K1R w - - 0 1", false);

    // No unique match on b file, but same promotions for both matches
    test("r2qk2r/2pp1ppp/4p3/1p6/1P6/1p2P3/1PPP1PPP/R2QK2R w KQkq - 0 1",
         "r2q1k1r/2pp1ppp/4p3/1p6/1P6/4P3/2PP1PPP/R2Q1K1R w - - 0 1", true);
    test("r2qk2r/2pp1ppp/1p2p3/1P6/1p6/1P2P3/2PP1PPP/R2QK2R w KQkq - 0 1",
         "r2q1k1r/2pp1ppp/4p3/1p6/1P6/4P3/2PP1PPP/R2Q1K1R w - - 0 1", true);

    // Doubled passed pawn, but cannot promote both pawns because required in goal position
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPP1P/RNBQKBNR w KQkq - 0 1", false);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPP1P/R1BQKBNR w KQkq - 0 1", true);
    test("rnbqkbnr/p1pppp1p/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppp1p/8/8/8/1P6/1PPPPP1P/RNBQKBNR w KQkq - 0 1", true);

    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", true);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKB1R w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", false);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/2PPPPPP/RNBQKBNR w KQkq - 0 1", true);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKB1R w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/2PPPPPP/RNBQKBNR w KQkq - 0 1", false);

    // One example from the 100k file in the ChessPositionRanking project
    // Manually constructed proof kernel, one correct and one with wrong bishop promotion
    test("rnbqkbnr/1pp1ppp1/8/8/1p2p1pP/2P2P2/P2P4/RNBQKBNR w KQkq - 0 7",
         "2b1RBr1/1Bp2r2/nNrbbkr1/1K2qpNQ/1n2p3/2P1RP2/r2P4/4BQ1n w - - 0 1", true);
    test("rnbqkbnr/1pp1pp1p/8/8/1p2p1Pp/2P2P2/P2P4/RNBQKBNR w KQkq - 0 7",
         "2b1RBr1/1Bp2r2/nNrbbkr1/1K2qpNQ/1n2p3/2P1RP2/r2P4/4BQ1n w - - 0 1", false);

    // Bishop promotion not possible, bishop blocked after promotion
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnbqkbnr/1ppppppp/8/8/2B5/8/P1PPPPPP/RNBQKBNR w - - 0 1", false);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "Bnbqkbnr/1ppppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w - - 0 1", true);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "Bnbqkbnr/1ppppppp/8/3B4/8/8/2PPPPPP/RNBQKBNR w - - 0 1", false);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "Bnbqkbnr/2pppppp/1p6/3B4/8/8/2PPPPPP/RNBQKBNR w - - 0 1", true);

    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rn1qkbnr/pp1ppppp/8/8/8/5B2/P1PPPPPP/RNBQKBNR w - - 0 1", false);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/pp1ppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w - - 0 1", true);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/pp1ppppp/8/8/4B3/8/P2PPPPP/RNBQKBNR w - - 0 1", false);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/p2ppppp/1p6/8/4B3/8/P2PPPPP/RNBQKBNR w - - 0 1", true);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/pp2pppp/8/3p4/4B3/8/P2PPPPP/RNBQKBNR w - - 0 1", true);

    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnr/ppppppp1/8/8/5B2/8/PPPPPP1P/RNBQKBNR w - - 0 1", false);
    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnB/ppppppp1/8/8/8/8/PPPPPP1P/RNBQKBNR w - - 0 1", true);
    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnB/ppppppp1/8/8/1B6/8/PPPPPP2/RNBQKBNR w - - 0 1", false);
    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnB/pppppp2/6p1/8/1B6/8/PPPPPP2/RNBQKBNR w - - 0 1", true);

    test("r1bqkb1B/ppppppp1/8/8/8/7P/PPPPP2P/RNBQKBNR w - - 0 1",
         "r1bqkb1B/ppppppp1/8/8/3B4/8/PPPPP2P/RNBQKBNR w - - 0 1", false);
}
