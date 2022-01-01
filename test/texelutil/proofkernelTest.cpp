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
#include "proofgame.hpp"
#include "extproofkernel.hpp"
#include "position.hpp"
#include "textio.hpp"
#include "posutil.hpp"
#include "util/util.hpp"
#include "stloutput.hpp"

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
    ASSERT_EQ(1, col.nPawns(WHITE));
    ASSERT_EQ(0, col.nPawns(BLACK));
    ASSERT_EQ(WHITE, col.getPawn(0));

    col.addPawn(1, BLACK);
    ASSERT_EQ(2, col.nPawns());
    ASSERT_EQ(1, col.nPawns(WHITE));
    ASSERT_EQ(1, col.nPawns(BLACK));
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(BLACK, col.getPawn(1));

    col.addPawn(0, BLACK);
    ASSERT_EQ(3, col.nPawns());
    ASSERT_EQ(1, col.nPawns(WHITE));
    ASSERT_EQ(2, col.nPawns(BLACK));
    ASSERT_EQ(BLACK, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));

    col.addPawn(1, WHITE);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(2, col.nPawns(WHITE));
    ASSERT_EQ(2, col.nPawns(BLACK));
    ASSERT_EQ(BLACK, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(WHITE, col.getPawn(2));
    ASSERT_EQ(BLACK, col.getPawn(3));

    col.removePawn(0);
    ASSERT_EQ(3, col.nPawns());
    ASSERT_EQ(2, col.nPawns(WHITE));
    ASSERT_EQ(1, col.nPawns(BLACK));
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));

    col.addPawn(3, WHITE);
    col.addPawn(4, BLACK);
    ASSERT_EQ(5, col.nPawns());
    ASSERT_EQ(3, col.nPawns(WHITE));
    ASSERT_EQ(2, col.nPawns(BLACK));
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(WHITE, col.getPawn(3));
    ASSERT_EQ(BLACK, col.getPawn(4));

    col.removePawn(3);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(2, col.nPawns(WHITE));
    ASSERT_EQ(2, col.nPawns(BLACK));
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(BLACK, col.getPawn(3));

    col.setPawn(3, WHITE);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(3, col.nPawns(WHITE));
    ASSERT_EQ(1, col.nPawns(BLACK));
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(WHITE, col.getPawn(3));

    for (int i = 0; i < 2; i++) {
        col.setPawn(1, BLACK);
        ASSERT_EQ(4, col.nPawns());
        ASSERT_EQ(2, col.nPawns(WHITE));
        ASSERT_EQ(2, col.nPawns(BLACK));
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

static U64 computeBlocked(const Position& startPos, const Position& goalPos) {
    U64 blocked;
    ProofGame pg(TextIO::toFEN(startPos), TextIO::toFEN(goalPos), {}, std::cerr);
    if (!pg.computeBlocked(startPos, blocked))
        blocked = 0xffffffffffffffffULL; // If goal not reachable, consider all pieces blocked
    return blocked;
}

void
ProofKernelTest::testGoal() {
    auto test = [](const std::string& start, const std::string& goal, bool expected,
                   int minMoves) {
        Position startPos = TextIO::readFEN(start);
        Position goalPos = TextIO::readFEN(goal);
        ProofKernel pk1(startPos, goalPos, computeBlocked(startPos, goalPos));
        ASSERT_EQ(expected, pk1.isGoal()) << "start: " << start << "\ngoal: " << goal;
        ASSERT_EQ(minMoves, pk1.minMovesToGoal()) << "start: " << start << "\ngoal: " << goal;

        startPos = PosUtil::swapColors(startPos);
        goalPos = PosUtil::swapColors(goalPos);
        ProofKernel pk2(startPos, goalPos, computeBlocked(startPos, goalPos));
        ASSERT_EQ(expected, pk2.isGoal())
            << "start: " << TextIO::toFEN(startPos) << "\ngoal: " << TextIO::toFEN(goalPos);
        ASSERT_EQ(minMoves, pk2.minMovesToGoal()) << "start: " << start << "\ngoal: " << goal;
    };

    const std::string startFEN = TextIO::startPosFEN;
    test(startFEN, startFEN, true, 0);
    test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w Kkq - 0 1",
         "rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", false, 1);
    test(startFEN, "rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", true, 1);

    test(startFEN, "rnbqkbnr/1ppppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQk - 0 1", false, 1);
    test("rnbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1ppppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQk - 0 1", true, 0);
    test("rnbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/1p6/8/8/4P3/1PPP1PPP/RNBBKBNR w KQk - 0 1", true, 0);
    test("rnbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1ppppppp/8/8/8/5P2/1PPPP1PP/RNBQKBBR w KQk - 0 1", false, 0);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/1p6/8/8/2P2PP1/3PP2P/RBBQKBNB w Qk - 0 1", true, 0);

    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/8/2PPPPPP/RNNQKNNR w KQk - 0 1", true, 0);
    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/5P2/2PPP1PP/RBBQKBBR w KQk - 0 1", true, 0);
    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/5P2/2PPP1PP/BNBQKBBR w Kk - 0 1", false, 0);

    test("rnbqkbnr/2ppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RRBQKRQR w KQ - 0 1", true, 0);
    test("rnbqkbnr/2ppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RQBQKQQR w KQ - 0 1", true, 0);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RRBQKRQR w KQ - 0 1", false, 1);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RQBQKQQR w KQk - 0 1", false, 2);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1pppppp1/8/8/8/6P1/2PPPP2/RQBQKBQR w KQ - 0 1", true, 1);

    // Blocked rook because of castling rights
    test("rnbqkbnr/2pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2pppppp/8/8/8/5P2/2PPP1PP/RBBQKBBR w KQkq - 0 1", false, 1);
    test("rnbqkbnr/2ppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/2ppppp1/8/8/8/6P1/2PPPP2/RQBQKQQR w KQk - 0 1", false, 1);
    test("rnbqkbnr/1pppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/1pppppp1/8/8/8/6P1/2PPPP2/RQBQKBQR w KQk - 0 1", false, 2);

    // Blocked king because of castling rights
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQkq - 0 1", false, 1);
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQk - 0 1", false, 1);
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQq - 0 1", false, 1);
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNNQKBNR w KQ - 0 1", true, 0);

    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQkq - 0 1", false, 1);
    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQq - 0 1", false, 1);
    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQk - 0 1", false, 1);
    test("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKNNR w KQ - 0 1", true, 0);

    // Blocked bishop
    test("rnbqkbnr/pp1ppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pp1ppppp/8/8/8/8/PPRPPPPP/RNBQKBNR w KQkq - 0 1", false, 1);
    test("rnbqkbnr/p2ppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p2ppppp/8/8/8/8/P1RPPPPP/RNBQKBNR w KQkq - 0 1", true, 0);

    // Enough number of pawns but in wrong files or wrong order within files
    test(startFEN, "rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/RNBQKBNR w KQkq - 0 1", false, 1);
    test(startFEN, "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPPP/RNBQKBNR w KQkq - 0 1", false, 2);
    test(startFEN, "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPP1/RNBQKBNR w Qq - 0 1", false, 2);
    test("rnbqkbnr/ppppppp1/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPP1/RNBQKBNR w Qq - 0 1", false, 1);
    test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPP1/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1ppppp1/8/8/8/1P6/1PPPPPP1/RNBQKBNR w Qq - 0 1", false, 1);
    test("rnbqkbnr/ppppp1pp/8/8/8/8/PPPPPPP1/RNBQKBNR w - - 0 1",
         "rnbqkbnr/p1ppp1p1/8/8/8/1P6/1PPPP1P1/RNBQKBNR w - - 0 1", false, 2);

    // No unique match on b file
    test("r2qk2r/2pp1ppp/4p3/1P6/1p6/1P2P3/1pPP1PPP/R2QK2R w KQkq - 0 1",
         "r2q1k1r/2pp1ppp/4p3/1P6/1p6/4P3/2PP1PPP/R2Q1K1R w - - 0 1", false, 1);

    // No unique match on b file, but same promotions for both matches
    test("r2qk2r/2pp1ppp/4p3/1p6/1P6/1p2P3/1PPP1PPP/R2QK2R w KQkq - 0 1",
         "r2q1k1r/2pp1ppp/4p3/1p6/1P6/4P3/2PP1PPP/R2Q1K1R w - - 0 1", true, 1);
    test("r2qk2r/2pp1ppp/1p2p3/1P6/1p6/1P2P3/2PP1PPP/R2QK2R w KQkq - 0 1",
         "r2q1k1r/2pp1ppp/4p3/1p6/1P6/4P3/2PP1PPP/R2Q1K1R w - - 0 1", true, 1);

    // Doubled passed pawn, but cannot promote both pawns because required in goal position
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPP1P/RNBQKBNR w KQkq - 0 1", false, 1);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPP1P/R1BQKBNR w KQkq - 0 1", true, 1);
    test("rnbqkbnr/p1pppp1p/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppp1p/8/8/8/1P6/1PPPPP1P/RNBQKBNR w KQkq - 0 1", true, 0);

    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", true, 0);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKB1R w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", false, 0);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/2PPPPPP/RNBQKBNR w KQkq - 0 1", true, 0);
    test("rnbqkbnr/p1pppppp/8/8/8/1P6/1PPPPPPP/R1BQKB1R w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/1P6/2PPPPPP/RNBQKBNR w KQkq - 0 1", false, 0);

    // One example from the 100k file in the ChessPositionRanking project
    // Manually constructed proof kernel, one correct and one with wrong bishop promotion
    test("rnbqkbnr/1pp1ppp1/8/8/1p2p1pP/2P2P2/P2P4/RNBQKBNR w KQkq - 0 7",
         "2b1RBr1/1Bp2r2/nNrbbkr1/1K2qpNQ/1n2p3/2P1RP2/r2P4/4BQ1n w - - 0 1", true, 0);
    test("rnbqkbnr/1pp1pp1p/8/8/1p2p1Pp/2P2P2/P2P4/RNBQKBNR w KQkq - 0 7",
         "2b1RBr1/1Bp2r2/nNrbbkr1/1K2qpNQ/1n2p3/2P1RP2/r2P4/4BQ1n w - - 0 1", false, 0);

    // Bishop promotion not possible, bishop blocked after promotion
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnbqkbnr/1ppppppp/8/8/2B5/8/P1PPPPPP/RNBQKBNR w - - 0 1", false, 0);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "Bnbqkbnr/1ppppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w - - 0 1", true, 0);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "Bnbqkbnr/1ppppppp/8/3B4/8/8/2PPPPPP/RNBQKBNR w - - 0 1", false, 0);
    test("rnbqkbnr/1ppppppp/8/8/8/P7/P1PPPPPP/RNBQKBNR w - - 0 1",
         "Bnbqkbnr/2pppppp/1p6/3B4/8/8/2PPPPPP/RNBQKBNR w - - 0 1", true, 0);

    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rn1qkbnr/pp1ppppp/8/8/8/5B2/P1PPPPPP/RNBQKBNR w - - 0 1", false, 0);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/pp1ppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w - - 0 1", true, 0);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/pp1ppppp/8/8/4B3/8/P2PPPPP/RNBQKBNR w - - 0 1", false, 0);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/p2ppppp/1p6/8/4B3/8/P2PPPPP/RNBQKBNR w - - 0 1", true, 0);
    test("rnbqkbnr/pp1ppppp/8/8/8/2P5/P1PPPPPP/RNBQKBNR w - - 0 1",
         "rnBqkbnr/pp2pppp/8/3p4/4B3/8/P2PPPPP/RNBQKBNR w - - 0 1", true, 0);

    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnr/ppppppp1/8/8/5B2/8/PPPPPP1P/RNBQKBNR w - - 0 1", false, 0);
    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnB/ppppppp1/8/8/8/8/PPPPPP1P/RNBQKBNR w - - 0 1", true, 0);
    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnB/ppppppp1/8/8/1B6/8/PPPPPP2/RNBQKBNR w - - 0 1", false, 0);
    test("rnbqkbnr/ppppppp1/8/8/8/7P/PPPPPP1P/RNBQKBNR w - - 0 1",
         "rnbqkbnB/pppppp2/6p1/8/1B6/8/PPPPPP2/RNBQKBNR w - - 0 1", true, 0);

    test("r1bqkb1B/ppppppp1/8/8/8/7P/PPPPP2P/RNBQKBNR w - - 0 1",
         "r1bqkb1B/ppppppp1/8/8/3B4/8/PPPPP2P/RNBQKBNR w - - 0 1", false, 0);

    // Bishop promotion possible, but cannot reach goal square after promotion
    test("r1bqkbn1/p1pppp2/6p1/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1",
         "rBbqkbn1/p1pppp2/6p1/8/8/8/PPPPPPP1/RNBQKBNR w KQ - 0 1", false, 0);

    // Endgame positions
    test(startFEN, "8/8/4kr2/8/8/4PP2/2RRKR2/8 w - - 0 1", false, 4);
    test("rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "8/8/4kr2/8/8/4PP2/2RRKR2/8 w - - 0 1", true, 4);

    // Some examples from the 100k file in the ChessPositionRanking project
    test(startFEN, "1b6/rbk2nBn/BqB1K3/1r4RB/3RQN1N/P2p4/1RQ4b/r1q2nnB b - - 0 1", false, 4);
    test(startFEN, "kN6/Pp5r/1q1B2Rr/nn1p2R1/1q1pPn1B/1b1NN3/2P3PK/BQb1r3 w - - 0 1", false, 4);
    test(startFEN, "qB1K1B2/Q1pq1k2/1bNR2p1/1bPP2Q1/1nbN4/b4rp1/1Pn1R2Q/3RR3 w - - 0 1", false, 4);
    test(startFEN, "2N3NQ/4N2N/2R1Kb1P/3pp2q/nb1NP3/b1r1Rp1p/1k1Pn1Nr/3B3R b - - 1 2", false, 4);
    test(startFEN, "rn5N/1Bp1B1p1/rr3Qnk/N1KP4/1p2P2q/Qb2nN2/r1PB2P1/1b1R3R w - - 0 1", false, 4);
    test(startFEN, "r1q2QR1/6N1/N2bp1P1/k2p2pK/1n2R1B1/Nb1nqp2/3PB3/1nBR3n w - - 0 1", false, 4);
    test(startFEN, "3K4/3p2RB/Q1pP1Nbp/4pR2/1PQ1Pr1k/b1bnN3/Q1Pr3N/b1n2B1q b - - 0 3", false, 3);
}

TEST(ProofKernelTest, testGoalPossible) {
    ProofKernelTest::testGoalPossible();
}

void
ProofKernelTest::testGoalPossible() {
    auto test = [](const std::string& start, const std::string& goal, bool expected,
                   int minMoves) {
        Position startPos = TextIO::readFEN(start);
        Position goalPos = TextIO::readFEN(goal);
        ProofKernel pk1(startPos, goalPos, computeBlocked(startPos, goalPos));
        ASSERT_EQ(expected, pk1.goalPossible()) << "start: " << start << "\ngoal: " << goal;
        ASSERT_EQ(minMoves, pk1.minMovesToGoal()) << "start: " << start << "\ngoal: " << goal;

        startPos = PosUtil::swapColors(startPos);
        goalPos = PosUtil::swapColors(goalPos);
        ProofKernel pk2(startPos, goalPos, computeBlocked(startPos, goalPos));
        ASSERT_EQ(expected, pk2.goalPossible())
            << "start: " << TextIO::toFEN(startPos) << "\ngoal: " << TextIO::toFEN(goalPos);
        ASSERT_EQ(minMoves, pk2.minMovesToGoal()) << "start: " << start << "\ngoal: " << goal;
    };

    const std::string startFEN = TextIO::startPosFEN;
    test(startFEN, startFEN, true, 0);
    test(startFEN, "2K1Nbk1/p3N1Nr/2PR2p1/2R2R1P/P3rb1B/1brQ2n1/p1q3P1/Nq2nB2 b - - 0 1", false, 4);
    test(startFEN, "2Qr2Bq/1n1K1N1P/1qpnN1kp/5N1q/3B2p1/3b1BP1/R2bb1B1/3rrn2 w - - 0 1 ", true, 4);
    test(startFEN, "rnbqkbnr/n2pp2n/6pp/4p3/2P4P/6P1/N2P1P1N/RNBQKBNR w KQkq - 0 1", false, 3);
}

TEST(ProofKernelTest, testMoveToString) {
    ProofKernelTest::testMoveToString();
}

void
ProofKernelTest::testMoveToString() {
    using PkMove = ProofKernel::PkMove;
    using PieceColor = ProofKernel::PieceColor;
    using PieceType = ProofKernel::PieceType;

    auto test = [](const std::string& strMove, const PkMove& move) {
        ASSERT_EQ(strMove, toString(move)) << "move: " << strMove;
        PkMove move2 = strToPkMove(strMove);
        std::string str2 = toString(move2);
        ASSERT_EQ(strMove, str2);
    };

    test("wPa0xPb1", PkMove::pawnXPawn(PieceColor::WHITE, 0, 0, 1, 1));
    test("bPc3xPb2", PkMove::pawnXPawn(PieceColor::BLACK, 2, 3, 1, 2));

    test("bPf1xQe0", PkMove::pawnXPiece(PieceColor::BLACK, 5, 1, 4, 0, PieceType::QUEEN));
    test("wPh0xRg2", PkMove::pawnXPiece(PieceColor::WHITE, 7, 0, 6, 2, PieceType::ROOK));
    test("wPc1xLBd1", PkMove::pawnXPiece(PieceColor::WHITE, 2, 1, 3, 1, PieceType::LIGHT_BISHOP));
    test("wPc1xDBd1", PkMove::pawnXPiece(PieceColor::WHITE, 2, 1, 3, 1, PieceType::DARK_BISHOP));
    test("wPc1xNd1", PkMove::pawnXPiece(PieceColor::WHITE, 2, 1, 3, 1, PieceType::KNIGHT));

    test("wPg0xDBfQ", PkMove::pawnXPieceProm(PieceColor::WHITE, 6, 0, 5,
                                             PieceType::DARK_BISHOP, PieceType::QUEEN));

    test("bPa1xhb1", PkMove::pawnXPromPawn(PieceColor::BLACK, 0, 1, 1, 1, 7));
    test("wPb0xga2", PkMove::pawnXPromPawn(PieceColor::WHITE, 1, 0, 0, 2, 6));

    test("wPb0xgaLB", PkMove::pawnXPromPawnProm(PieceColor::WHITE, 1, 0, 0, 6,
                                                PieceType::LIGHT_BISHOP));
    test("wPa0xgbDB", PkMove::pawnXPromPawnProm(PieceColor::WHITE, 0, 0, 1, 6,
                                                PieceType::DARK_BISHOP));
    test("wPb0xgaR", PkMove::pawnXPromPawnProm(PieceColor::WHITE, 1, 0, 0, 6,
                                                PieceType::ROOK));
    test("wPb0xgaN", PkMove::pawnXPromPawnProm(PieceColor::WHITE, 1, 0, 0, 6,
                                                PieceType::KNIGHT));

    test("bxPc1", PkMove::pieceXPawn(PieceColor::BLACK, 2, 1));
    test("wxPf2", PkMove::pieceXPawn(PieceColor::WHITE, 5, 2));

    test("wxN", PkMove::pieceXPiece(PieceColor::WHITE, PieceType::KNIGHT));
    test("bxR", PkMove::pieceXPiece(PieceColor::BLACK, PieceType::ROOK));
}

void
ProofKernelTest::testMove(const Position& pos, const ProofKernel& pk,
                          const ProofKernel::PkMove& m) {
    using PieceColor = ProofKernel::PieceColor;
    using PieceType = ProofKernel::PieceType;
    using PawnColumn = ProofKernel::PawnColumn;
    using SquareColor = ProofKernel::SquareColor;

    std::string fen = TextIO::toFEN(pos);
    PieceType taken;
    if (m.otherPromotionFile != -1) {
        ASSERT_TRUE(m.otherPromotionFile >= 0);
        ASSERT_TRUE(m.otherPromotionFile < 8);
        const PawnColumn& col = pk.columns[m.otherPromotionFile];
        ASSERT_TRUE(col.nPawns() > 0) << "fen: " << fen << " move: " << toString(m);
        if (m.color == PieceColor::WHITE) {
            ASSERT_EQ(PieceColor::BLACK, col.getPawn(0));
        } else {
            ASSERT_EQ(PieceColor::WHITE, col.getPawn(col.nPawns() - 1));
        }
        taken = PieceType::PAWN;
    } else {
        taken = m.takenPiece;
    }
    if (m.fromFile != -1) {
        ASSERT_TRUE(m.fromFile >= 0);
        ASSERT_TRUE(m.fromFile < 8);
        const PawnColumn& col = pk.columns[m.fromFile];
        ASSERT_TRUE(m.fromIdx >= 0);
        ASSERT_TRUE(m.fromIdx < col.nPawns());
    }
    PieceColor oc = m.color == PieceColor::WHITE ? PieceColor::BLACK : PieceColor::WHITE;
    ASSERT_TRUE(pk.pieceCnt[oc][taken] > 0) << "fen: " << fen << " move: " << toString(m);
    if (m.toFile != -1) {
        ASSERT_TRUE(m.toFile >= 0);
        ASSERT_TRUE(m.toFile < 8);
        const PawnColumn& col = pk.columns[m.toFile];
        if (m.promotedPiece == PieceType::EMPTY) {
            ASSERT_TRUE(m.toIdx >= 0);
            if (m.takenPiece == PieceType::PAWN) {
                ASSERT_TRUE(m.toIdx < col.nPawns());
            } else {
                ASSERT_TRUE(m.toIdx <= col.nPawns());
            }
        } else {
            if (m.color == PieceColor::WHITE) {
                int d = (m.otherPromotionFile == m.fromFile) ? 2 : 1;
                ASSERT_EQ(pk.columns[m.fromFile].nPawns()-d, m.fromIdx);
            } else {
                ASSERT_EQ(0, m.fromIdx);
            }
            ASSERT_TRUE(m.promotedPiece >= PieceType::QUEEN);
            ASSERT_TRUE(m.promotedPiece <= PieceType::KNIGHT);
            if (m.promotedPiece == PieceType::DARK_BISHOP) {
                ASSERT_EQ(SquareColor::DARK, col.promotionSquareType(m.color));
            } else if (m.promotedPiece == PieceType::LIGHT_BISHOP) {
                ASSERT_EQ(SquareColor::LIGHT, col.promotionSquareType(m.color));
            }
            ASSERT_TRUE(pk.pieceCnt[m.color][PieceType::PAWN] > 0);
        }
    }
}

TEST(ProofKernelTest, testMoveGen) {
    ProofKernelTest::testMoveGen();
}

void
ProofKernelTest::testMoveGen() {
    using PkMove = ProofKernel::PkMove;

    auto test = [](const std::string& start, const std::string& goal,
                   std::vector<std::string> expected, bool onlyPieceXPiece = false) {
        Position startPos = TextIO::readFEN(start);
        Position goalPos = TextIO::readFEN(goal);
        ProofKernel pk(startPos, goalPos, computeBlocked(startPos, goalPos));
        pk.onlyPieceXPiece = onlyPieceXPiece;
        std::vector<PkMove> moves;
        pk.genMoves(moves);
        std::vector<std::string> strMoves;
        for (const PkMove& m : moves) {
            testMove(startPos, pk, m);
            strMoves.push_back(toString(m));
        }
        std::sort(strMoves.begin(), strMoves.end());
        std::sort(expected.begin(), expected.end());
        ASSERT_EQ(expected, strMoves)
            << "start: " << TextIO::toFEN(startPos) << "\ngoal: " << TextIO::toFEN(goalPos);
    };

    test("1n2k3/4p3/8/8/8/8/4P3/4K3 w - - 0 1", "3qk3/8/8/8/8/8/8/3RK3 w - - 0 1",
         {"wPe0xNd0", "wPe0xNf0", "wxPe1", "bxPe0", "wxN"});
    test("1n2k3/4p3/8/8/8/8/4P3/4K1B1 w - - 0 1", "3qk3/8/8/8/8/8/8/3RK3 w - - 0 1",
         {"wPe0xNd0", "wPe0xNf0", "wxPe1", "bxPe0", "bPe1xDBd0", "bPe1xDBf0", "wxN", "bxDB"});

    // XXX No need to generate promotion moves when there is a move that creates
    // a passed pawn on the same file, since promotion of a passed pawn can be
    // handled in the next ply.
    test("4k3/4p3/8/8/8/8/2P5/4K3 w - - 0 1", "3qk3/8/8/8/8/8/8/3RK3 w - - 0 1",
         { "wPc0xeb0", "wPc0xebQ", "wPc0xebR", "wPc0xebDB", "wPc0xebN",
           "wPc0xed0", "wPc0xedQ", "wPc0xedR", "wPc0xedDB", "wPc0xedN",
           "bPe0xcd0", "bPe0xcdQ", "bPe0xcdR", "bPe0xcdLB", "bPe0xcdN",
           "bPe0xcf0", "bPe0xcfQ", "bPe0xcfR", "bPe0xcfLB", "bPe0xcfN",
           "bxPc0", "wxPe0"
         });
    test("4k3/4p3/8/8/8/8/3P4/4K3 w - - 0 1", "3qk3/8/8/8/8/8/8/3RK3 w - - 0 1",
         { "wPd0xPe0", "bPe0xPd0",
           "wPd0xec0", "wPd0xecQ", "wPd0xecR", "wPd0xecLB", "wPd0xecN",
           "wPd0xee0", "wPd0xeeQ", "wPd0xeeR", "wPd0xeeLB", "wPd0xeeN",
           "bPe0xdd0", "bPe0xddQ", "bPe0xddR", "bPe0xddLB", "bPe0xddN",
           "bPe0xdf0", "bPe0xdfQ", "bPe0xdfR", "bPe0xdfLB", "bPe0xdfN",
           "bxPd0", "wxPe0"
         });
    test("4k3/p6p/8/8/8/8/P7/4K3 w - - 0 1", "4k3/n7/8/8/1P6/8/8/4K3 w - - 0 1",
         {"wPa0xhb0", "wxPa1", "wxPh0", "bxPa0"});
    test("4k3/p6p/8/8/8/8/7P/4K3 w - - 0 1", "4k3/7n/8/6P1/8/8/8/4K3 w - - 0 1",
         {"wPh0xag0", "wxPa0", "wxPh1", "bxPh0"});

    test("4k3/4pp2/4p3/8/8/4P3/4PP2/4K3 w - - 0 1", "1n2k1n1/8/8/8/8/8/8/1N2K1N1 w - - 0 1",
         { "wPe0xPf1", "wPe1xPf1", "bPe2xPf0", "bPe3xPf0",
           "wPf0xPe2", "wPf0xPe3", "bPf1xPe0", "bPf1xPe1",
           "bxPe0", "bxPe1", "wxPe2", "wxPe3", "bxPf0", "wxPf1"
         });

    test("4k3/pp6/8/8/8/8/PP6/4K3 w - - 0 1", "1n2k3/8/8/8/8/8/8/1NN1K3 w - - 0 1",
         { "wPa0xPb1", "wPb0xPa1", "bPa1xPb0", "bPb1xPa0", "wxPa1", "wxPb1", "bxPa0", "bxPb0" });
    test("4k3/6pp/8/8/8/8/6PP/4K3 w - - 0 1", "1n2k3/8/8/8/8/8/8/1NN1K3 w - - 0 1",
         { "wPg0xPh1", "wPh0xPg1", "bPg1xPh0", "bPh1xPg0", "wxPg1", "wxPh1", "bxPg0", "bxPh0" });

    test("4k3/2p5/1p6/1p6/2P5/2P5/1PP5/4K3 w - - 0 1", "1n2k3/8/8/8/8/8/8/1NN1K3 w - - 0 1",
         { "wPb0xPc3", "wPc0xPb1", "wPc0xPb2", "wPc1xPb1", "wPc1xPb2", "wPc2xPb1", "wPc2xPb2",
           "bPc3xPb0", "bPb1xPc0", "bPb2xPc0", "bPb1xPc1", "bPb2xPc1", "bPb1xPc2", "bPb2xPc2",
           "wxPb1", "wxPb2", "wxPc3", "bxPb0", "bxPc0", "bxPc1", "bxPc2"
         });

    test("1n2k3/p6p/8/8/8/8/P6P/1N2K3 w - - 0 1", "4k3/8/p7/6p1/1P6/7P/8/4K3 w - - 0 1",
         {"wPa0xNb0", "wPh0xNg0", "wxPa1", "wxPh1", "bPa1xNb0", "bPh1xNg0", "bxPa0", "bxPh0", "wxN", "bxN"});
    test("1n2k3/p6p/8/8/8/8/P6P/1N2K3 w - - 0 1", "4k3/p7/8/6p1/1P6/8/7P/4K3 w - - 0 1",
         {"wPa0xNb0", "wxPh1", "bPh1xNg0", "bxPa0", "wxN", "bxN"});

    test("1nbqkr2/8/8/8/8/8/P7/4K3 w - - 0 1", "4k3/8/8/8/1P6/8/8/4K3 w - - 0 1",
         {"wPa0xNb0",  "wPa0xNbN",  "wPa0xNbDB",  "wPa0xNbR",  "wPa0xNbQ",
          "wPa0xLBb0",
          "wPa0xRb0",  "wPa0xRbN",  "wPa0xRbDB",  "wPa0xRbR",  "wPa0xRbQ",
          "wPa0xQb0",  "wPa0xQbN",  "wPa0xQbDB",  "wPa0xQbR",  "wPa0xQbQ",
          "bxPa0", "wxN", "wxLB", "wxQ", "wxR"
         });
    test("4k3/8/3b4/8/8/8/1P6/4K3 w - - 0 1", "B3k3/8/8/8/8/8/8/4K3 w - - 0 1",
         {"wPb0xDBa0", "wPb0xDBc0", "wxDB", "bxPb0"});

    test("4k3/p1p5/8/P7/p7/p7/P7/4K3 w - - 0 1", "4k3/8/pP6/8/p7/p7/P7/4K3 w - - 0 1",
         {"wPa3xcb0", "wxPa1", "wxPa2", "wxPa4", "wxPc0", "bxPa3"});
    test("4k3/p1p5/8/P7/p7/p7/P7/4K3 w - - 0 1", "4k3/p7/1P6/8/p7/p7/P7/4K3 w - - 0 1",
         {"wPa3xcb0", "wxPa1", "wxPa2", "wxPc0", "bxPa3"});

    test("4k3/1p6/8/8/1P6/1p6/8/4K3 w - - 0 1", "4k3/8/1p6/2P5/8/8/8/4K3 w - - 0 1",
         {"wPb0xba0", "wPb0xbc0", "wxPb0", "wxPb2", "bxPb1"});
    test("4k3/1p6/8/8/1P6/1p6/8/4K3 w - - 0 1", "4k3/1p6/8/2P5/8/8/8/4K3 w - - 0 1",
         {"wPb0xba0", "wPb0xbc0", "wxPb0", "bxPb1"});
    test("4k3/8/8/8/1P6/1p6/8/4K3 w - - 0 1", "4k3/8/8/2P5/8/8/8/4K3 w - - 0 1",
         {"wPb0xba0", "wPb0xbaN", "wPb0xbaLB", "wPb0xbaR", "wPb0xbaQ",
          "wPb0xbc0", "wPb0xbcN", "wPb0xbcLB", "wPb0xbcR", "wPb0xbcQ",
          "bPb0xba0", "bPb0xbaN", "bPb0xbaDB", "bPb0xbaR", "bPb0xbaQ",
          "bPb0xbc0", "bPb0xbcN", "bPb0xbcDB", "bPb0xbcR", "bPb0xbcQ",
          "wxPb0", "bxPb1"
         });

    test("4k3/8/8/1P6/1p6/1p6/8/4K3 w - - 0 1", "4k3/8/2P5/8/8/8/8/4K3 w - - 0 1",
         {"wPb1xba0", "wPb1xbaN", "wPb1xbaLB", "wPb1xbaR", "wPb1xbaQ",
          "wPb1xbc0", "wPb1xbcN", "wPb1xbcLB", "wPb1xbcR", "wPb1xbcQ",
          "bPb0xba0", "bPb0xbaN", "bPb0xbaDB", "bPb0xbaR", "bPb0xbaQ",
          "bPb0xbc0", "bPb0xbcN", "bPb0xbcDB", "bPb0xbcR", "bPb0xbcQ",
          "bPb1xba0", "bPb1xbc0", "wxPb0", "wxPb1", "bxPb2"
         });

    test("r3k1r1/7p/8/1P6/8/8/8/4K3 w q - 0 1", "r3k3/8/8/1N5p/8/8/8/4K3 w q - 0 1",
         {"wPb0xRa0",
          "wPb0xRc0", "wPb0xRcN", "wPb0xRcLB", "wPb0xRcR", "wPb0xRcQ",
          "bPh0xbg0", "bPh0xbgN", "bPh0xbgDB", "bPh0xbgR", "bPh0xbgQ", 
          "wxPh0", "bxPb0", "wxR"
         });
    test("1r2k2r/p7/8/6P1/8/8/8/4K3 w k - 0 1", "1r2k2r/8/8/6N1/8/8/8/4K3 w k - 0 1",
         {"wPg0xRh0", "wPg0xah0",
          "wPg0xRf0", "wPg0xRfN", "wPg0xRfDB", "wPg0xRfR", "wPg0xRfQ",
          "wPg0xaf0", "wPg0xafN", "wPg0xafDB", "wPg0xafR", "wPg0xafQ",
          "bPa0xgb0", "bPa0xgbN", "bPa0xgbLB", "bPa0xgbR", "bPa0xgbQ",
          "wxPa0", "bxPg0", "wxR"
         });

    test("r3k1r1/7p/8/8/4P3/8/8/4K3 w q - 0 1", "r3k3/8/8/1N5p/8/8/8/4K3 w q - 0 1",
         {"wPe0xRd0", "wPe0xRdN", "wPe0xRdDB",
          "wPe0xRf0", "wPe0xRfN", "wPe0xRfDB",
          "wxPh0", "bxPe0", "wxR"
         });
    test("4k3/8/8/4p3/8/8/7P/R3K1R1 w Q - 0 1", "4k3/8/8/4n3/7P/8/8/R3K1R1 w Q - 0 1",
         {"bPe0xRd0", "bPe0xRdN", "bPe0xRdLB",
          "bPe0xRf0", "bPe0xRfN", "bPe0xRfLB",
          "bxPh0", "wxPe0", "bxR"
         });

    test("r3k1r1/7p/8/8/3P4/8/8/4K3 w q - 0 1", "r3k1r1/8/8/8/8/2N5/8/4K3 w q - 0 1",
         {"wPd0xRc0", "wPd0xRe0", "wPd0xhc0", "wPd0xhe0", "wxPh0", "bxPd0", "wxR"});
    test("4k3/8/8/3p4/8/8/7P/R3K1R1 w Q - 0 1", "4k3/8/8/4n3/8/8/8/R3K1R1 w Q - 0 1",
         {"bPd0xRc0", "bPd0xRe0", "bPd0xhc0", "bPd0xhe0", "wxPd0", "bxPh0", "bxR"});

    // Blocked pawns
    test("4k3/1p6/8/8/8/8/PPP5/4K3 w - - 0 1", "4k3/1n6/8/8/8/8/PP6/4K3 w - - 0 1",
         {"wPc0xPb1", "wxPb1",
          "bPb1xPc0", "bPb1xca1", "bPb1xcc0", "bxPc0"
         });
    test("4k3/ppp5/8/8/8/8/1P6/4K3 w - - 0 1", "4k3/pp6/8/8/8/8/1N6/4K3 w - - 0 1",
         {"wPb0xPc0", "wPb0xca0", "wPb0xcc0", "wxPc0",
          "bPc0xPb0", "bxPb0"
         });
    test("1n2k3/ppp5/8/8/8/8/1P6/4K3 w - - 0 1", "4k3/pp6/8/8/8/8/1N6/4K3 w - - 0 1",
         {"wPb0xPc0", "wPb0xca0", "wPb0xcc0", "wxPc0",
          "wPb0xNa0", "wPb0xNc0", "wPb0xNc1",
          "bPc0xPb0", "bxPb0", "wxN"
         });
    test("4k3/1p6/8/8/2P5/8/PPP5/4K3 w - - 0 1", "4k3/1n6/8/8/8/8/PPP5/4K3 w - - 0 1",
         {"wPc1xPb1", "wxPb1",
          "bPb1xPc1", "bPb1xca1", "bPb1xcc1", "bxPc1"
         });
    test("4k3/ppp5/8/2p5/8/8/1P6/4K3 w - - 0 1", "4k3/ppp5/8/8/8/8/1N6/4K3 w - - 0 1",
         {"wPb0xPc0", "wPb0xca0", "wPb0xcc0", "wxPc0",
          "bPc0xPb0", "bxPb0"
         });

    test("1n2k3/4p3/8/8/8/8/4P3/4K3 w - - 0 1", "3qk3/8/8/8/8/8/8/3RK3 w - - 0 1",
         {"wxN"}, true);
}

TEST(ProofKernelTest, testMakeMove) {
    ProofKernelTest::testMakeMove();
}

void
ProofKernelTest::testMakeMove() {
    auto WHITE = ProofKernel::WHITE;
    auto BLACK = ProofKernel::BLACK;
    using PieceType = ProofKernel::PieceType;
    using PkMove = ProofKernel::PkMove;
    using PkUndoInfo = ProofKernel::PkUndoInfo;

    auto test = [](const std::string& start, const std::string& goal,
                   const PkMove& move) {
        Position startPos = TextIO::readFEN(start);
        Position goalPos = TextIO::readFEN(goal);

        ProofKernel pk0(startPos, goalPos, computeBlocked(startPos, goalPos));
        ProofKernel pk (startPos, goalPos, computeBlocked(startPos, goalPos));
        ProofKernel pkG(goalPos , goalPos, computeBlocked(goalPos, goalPos));

        ASSERT_EQ(pk0, pk);
        ASSERT_NE(pkG, pk);
        testMove(startPos, pk, move);
        ASSERT_TRUE(pk.goalPossible()) << "start: " << start << " goal: " << goal << " move: " << toString(move);

        PkUndoInfo ui;
        pk.makeMove(move, ui);

        ASSERT_TRUE(pk.isGoal()) << "start: " << start << " goal: " << goal << " move: " << toString(move);
        ASSERT_TRUE(pk.goalPossible());
        ASSERT_NE(pk0, pk);
        ASSERT_EQ(pkG, pk) << "start: " << start << " goal: " << goal << " move: " << toString(move);

        pk.unMakeMove(move, ui);
        ASSERT_EQ(pk0, pk) << "start: " << start << " goal: " << goal << " move: " << toString(move);
        ASSERT_NE(pkG, pk);
    };

    test("4k3/4p3/4p3/8/8/5P2/5P2/4K3 w - - 0 1", "4k3/4p3/8/4P3/8/8/5P2/4K3 w - - 0 1",
         PkMove::pawnXPawn(WHITE, 5, 1, 4, 0));
    test("4k3/4p3/4p3/8/8/5P2/5P2/4K3 w - - 0 1", "4k3/4p3/8/4P3/8/8/5P2/4K3 w - - 0 1",
         PkMove::pawnXPawn(WHITE, 5, 0, 4, 0));
    test("4k3/4p3/4p3/8/8/5P2/5P2/4K3 w - - 0 1", "4k3/4P3/8/4p3/8/8/5P2/4K3 w - - 0 1",
         PkMove::pawnXPawn(WHITE, 5, 1, 4, 1));
    test("4k3/4p3/4p3/8/8/5P2/5P2/4K3 w - - 0 1", "4k3/4P3/8/4p3/8/8/5P2/4K3 w - - 0 1",
         PkMove::pawnXPawn(WHITE, 5, 0, 4, 1));
    test("1n2k3/4p3/6p1/5P2/5P2/6p1/8/1N2K3 w - - 0 1", "1n2k3/4p3/6P1/6p1/5P2/8/8/1N2K3 w - - 0 1",
         PkMove::pawnXPawn(WHITE, 5, 1, 6, 1));
    test("1n2k3/4p3/6p1/5PP1/5P2/6p1/8/1N2K3 w - - 0 1", "1n2k3/4p3/6P1/6P1/5P2/6p1/8/1N2K3 w - - 0 1",
         PkMove::pawnXPawn(WHITE, 5, 1, 6, 2));

    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4k3/4p3/4p3/8/4P3/8/5P2/1N2K3 w - - 0 1",
         PkMove::pawnXPiece(WHITE, 5, 1, 4, 0, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4k3/4p3/4p3/8/4P3/8/5P2/1N2K3 w - - 0 1",
         PkMove::pawnXPiece(WHITE, 5, 0, 4, 0, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4k3/4p3/8/8/4P3/4p3/5P2/1N2K3 w - - 0 1",
         PkMove::pawnXPiece(WHITE, 5, 1, 4, 1, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4k3/4p3/8/8/4P3/4p3/5P2/1N2K3 w - - 0 1",
         PkMove::pawnXPiece(WHITE, 5, 0, 4, 1, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4k3/8/8/4P3/4p3/4p3/5P2/1N2K3 w - - 0 1",
         PkMove::pawnXPiece(WHITE, 5, 0, 4, 2, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4k3/8/8/4P3/4p3/4p3/5P2/1N2K3 w - - 0 1",
         PkMove::pawnXPiece(WHITE, 5, 1, 4, 2, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n2k3/4p3/8/5p2/8/5P2/5P2/4K3 w - - 0 1",
         PkMove::pawnXPiece(BLACK, 4, 0, 5, 2, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n2k3/4p3/8/5p2/8/5P2/5P2/4K3 w - - 0 1",
         PkMove::pawnXPiece(BLACK, 4, 1, 5, 2, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n2k3/4p3/5P2/5p2/8/8/5P2/4K3 w - - 0 1",
         PkMove::pawnXPiece(BLACK, 4, 0, 5, 1, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n2k3/4p3/5P2/5p2/8/8/5P2/4K3 w - - 0 1",
         PkMove::pawnXPiece(BLACK, 4, 1, 5, 1, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n2k3/4p3/5P2/8/5P2/5p2/8/4K3 w - - 0 1",
         PkMove::pawnXPiece(BLACK, 4, 0, 5, 0, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n2k3/4p3/5P2/8/5P2/5p2/8/4K3 w - - 0 1",
         PkMove::pawnXPiece(BLACK, 4, 1, 5, 0, PieceType::KNIGHT));

    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4R3/1k2p3/4p3/8/8/8/1K3P2/1N6 w - - 0 1",
         PkMove::pawnXPieceProm(WHITE, 5, 1, 4, PieceType::KNIGHT, PieceType::ROOK));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n6/1k2p3/8/8/8/5P2/1K3P2/5b2 w - - 0 1",
         PkMove::pawnXPieceProm(BLACK, 4, 0, 5, PieceType::KNIGHT, PieceType::LIGHT_BISHOP));

    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n6/1k2p3/6P1/8/8/5P2/8/1N2K3 w - - 0 1",
         PkMove::pawnXPromPawn(WHITE, 5, 0, 6, 0, 4));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n6/1k2p3/6P1/8/8/5P2/8/1N2K3 w - - 0 1",
         PkMove::pawnXPromPawn(WHITE, 5, 1, 6, 0, 4));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n6/1k2p3/8/8/3p4/K4P2/8/1N6 w - - 0 1",
         PkMove::pawnXPromPawn(BLACK, 4, 0, 3, 0, 5));
    test("1n2k3/4p3/8/5P2/5P2/5p2/8/1N2K3 w - - 0 1", "1n2k3/4p3/6P1/8/5P2/8/8/1N2K3 w - - 0 1",
         PkMove::pawnXPromPawn(WHITE, 5, 1, 6, 0, 5));
    test("1n2k3/4p3/6p1/5P2/5P2/6p1/8/1N2K3 w - - 0 1", "1n2k3/4p3/6P1/6p1/5P2/8/8/1N2K3 w - - 0 1",
         PkMove::pawnXPromPawn(WHITE, 5, 1, 6, 1, 6));
    test("1n2k3/4p3/6p1/5PP1/5P2/6p1/8/1N2K3 w - - 0 1", "1n2k3/4p1P1/6p1/6P1/5P2/8/8/1N2K3 w - - 0 1",
         PkMove::pawnXPromPawn(WHITE, 5, 1, 6, 2, 6));
    test("1n2k3/4p3/8/5P2/5p2/5p2/8/1N2K3 w - - 0 1", "1n2k3/4p3/8/8/5p2/8/6p1/1N2K3 w - - 0 1",
         PkMove::pawnXPromPawn(BLACK, 5, 0, 6, 0, 5));

    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n4Q1/1k2p3/8/8/8/5P2/8/1N2K3 w - - 0 1",
         PkMove::pawnXPromPawnProm(WHITE, 5, 1, 6, 4, PieceType::QUEEN));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n6/1k2p3/8/8/8/K4P2/8/1N1n4 w - - 0 1",
         PkMove::pawnXPromPawnProm(BLACK, 4, 0, 3, 5, PieceType::KNIGHT));

    test("4k3/4p3/8/8/8/8/2P5/4K3 w - - 0 1", "4k3/4p3/8/8/8/8/8/4K3 w - - 0 1",
         PkMove::pieceXPawn(BLACK, 2, 0));
    test("4k3/4p3/8/8/8/8/2P5/4K3 w - - 0 1", "4k3/8/8/8/8/8/2P5/4K3 w - - 0 1",
         PkMove::pieceXPawn(WHITE, 4, 0));

    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "1n2k3/4p3/4p3/8/8/5P2/5P2/4K3 w - - 0 1",
         PkMove::pieceXPiece(BLACK, PieceType::KNIGHT));
    test("1n2k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1", "4k3/4p3/4p3/8/8/5P2/5P2/1N2K3 w - - 0 1",
         PkMove::pieceXPiece(WHITE, PieceType::KNIGHT));
}

TEST(ProofKernelTest, testSearch) {
    ProofKernelTest::testSearch();
}

void
ProofKernelTest::testSearch() {
    using PkMove = ProofKernel::PkMove;
    using ExtPkMove = ProofKernel::ExtPkMove;

    auto test = [](const std::string& start, const std::string& goal,
                   bool expectedSolution, const std::string& expectedPath) {
        Position startPos = TextIO::readFEN(start);
        Position goalPos = TextIO::readFEN(goal);

        ProofKernel pk(startPos, goalPos, computeBlocked(startPos, goalPos));

        std::vector<PkMove> moves;
        std::vector<ExtPkMove> extMoves;
        bool found = pk.findProofKernel(moves, extMoves) == ProofKernel::EXT_PROOF_KERNEL;
        ASSERT_EQ(expectedSolution, found) << "start: " << start << " goal: " << goal;

        std::string path;
        for (const PkMove& m : moves) {
            if (!path.empty())
                path += ' ';
            path += toString(m);
        }
        if (found)
            std::cout << "moves: " << path << std::endl;
        if (expectedPath != "*") {
            ASSERT_EQ(expectedPath, path) << "start: " << start << " goal: " << goal;
        } else {
            int nCapt = (BitBoard::bitCount(startPos.occupiedBB()) -
                         BitBoard::bitCount(goalPos.occupiedBB()));
            ASSERT_EQ(nCapt, (int)moves.size());
        }

        for (const ExtPkMove& m : extMoves) {
            ASSERT_EQ(toString(m), toString(strToExtPkMove(toString(m))));
        }

        if (expectedPath.empty() || expectedPath == "*") {
            startPos = PosUtil::swapColors(startPos);
            goalPos = PosUtil::swapColors(goalPos);
            ProofKernel pk2(startPos, goalPos, computeBlocked(startPos, goalPos));
            found = pk2.findProofKernel(moves, extMoves) == ProofKernel::EXT_PROOF_KERNEL;
            ASSERT_EQ(expectedSolution, found) << "start: " << start << " goal: " << goal;
            for (const ExtPkMove& m : extMoves) {
                ASSERT_EQ(toString(m), toString(strToExtPkMove(toString(m))));
            }
        }
    };

    test("4k3/pp6/8/8/8/8/PP6/4K3 w - - 0 1", "1b2k3/8/8/8/8/8/8/BN2K3 w - - 0 1",
         true, "wPa0xPb1");
    test("4k3/pp6/8/8/8/8/PP6/4K3 w - - 0 1", "1b2k3/8/8/8/8/8/8/NB2K3 w - - 0 1",
         false, "");
    test("4k3/pp6/8/8/8/8/PP6/4K3 w - - 0 1", "b3k3/8/8/8/8/8/8/NB2K3 w - - 0 1",
         true, "wPb0xPa1");

    test("2b1k3/1p1p4/8/8/3P4/8/8/4K3 w - - 0 1", "4k3/1p1p4/8/8/3N4/8/8/4K3 w - - 0 1",
         false, "wxLB");

    const std::string startFEN = TextIO::startPosFEN;
    test(startFEN, "3rRQ2/1P2q1q1/NK2brn1/1q3b2/B3k3/1BB4N/RbrrP1Pq/n2br3 w - - 0 1",
         false, "bxLB");
    test(startFEN, "b3b3/1qrN1n2/1Pkppp2/4P2K/1Bp5/bR1bN2Q/1P1PNr2/NBn1B2R w - - 0 1",
         false, "bxDB");

    test(startFEN, "1r1n4/1N1qb2N/kr2r1r1/1RpN1K2/1R2B1Q1/bP3Q1B/B1n2Q2/1r1Bb1b1 w - - 0 1",
         false, "");
    test(startFEN, "1Q1nkbr1/4b1Qq/2pn2N1/r1P2R1b/r2NqP2/Q2R4/r1n5/1RK1qb2 w - - 0 1",
         true, "*");
    test(startFEN, "2r3n1/RpQp2n1/1p6/1Q2Kp1p/Nn1N1NRR/Br1R2b1/b5Rp/4k3 w - - 0 1",
         true, "*");

    test(startFEN, "3rRQ2/1P2q1q1/NK2brn1/1q3b2/B3k3/1BB4N/RbrrP1Pq/n2br3 w - - 0 1",
         false, "bxLB");
    test(startFEN, "3q1r1R/4nr2/NNbp1n1R/4kNB1/p2n3N/4nQ2/NPb1RP2/b1KQ4 b - - 0 1",
         true, "*");

    test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq - 0 1",
         "B1Q5/1p1bR3/1RPQqb1b/3n4/1RK1N1Nn/b4pQ1/2Qr3B/1r2qbBk w - - 0 1",
         false, "");
    test(startFEN, "B1Q5/1p1bR3/1RPQqb1b/3n4/1RK1N1Nn/b4pQ1/2Qr3B/1r2qbBk w - - 0 1",
         false, "");
    test(startFEN, "BnbqkbBr/1ppppp1p/8/8/3P4/8/1PP1PP1P/RNBRK1NR w KQk - 0 1",
         false, "");

    // Rook/queen promotion not allowed next to uncastled king
    test(startFEN, "r1bRkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
         true, "*");
    test(startFEN, "rnbqkR1r/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
         true, "*");
    test(startFEN, "r1bQkbnr/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
         true, "*");
    test(startFEN, "rnbqkQ1r/pppp1ppp/8/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
         true, "*");

    test(startFEN, "rnbqkb1r/ppp2ppp/8/8/3R4/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1",
         false, "");
    test(startFEN, "rnbqkb1r/ppp2ppp/8/8/3R4/8/PPPP1PPP/RNBQKBNR w KQ - 0 1",
         true, "*");

    // Blocked pawns
    test("4k3/ppp5/8/2p5/8/8/1P6/4K3 w - - 0 1", "4k3/ppp5/8/8/8/8/1N6/4K3 w - - 0 1",
         false, "");

    test(startFEN, "1N2Q1n1/r6B/Q4B1b/KP1qPN1b/1RN4R/B5nn/1q2P1Pr/1q5k w - - 0 1",
         true, "*");

    // "Piece takes piece" move required
    test(startFEN, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq - 0 1",
         true, "bxN");

    test("rn2kb2/8/8/6pP/6P1/8/8/4K3 w q - 0 1", "r3kb2/8/8/6p1/3B1B2/8/8/4K3 w q - 0 1",
         true, "wPg0xNh0");

    // No extended proof kernel because bishop on wrong color
    test(startFEN, "q2R3N/k2nK2p/bPR2Q1q/2r4N/rr2B2P/3BbN1p/r3q1q1/2Q2n1b b - - 0 1",
         false, "");
    test(startFEN, "1rkQ1r2/b5p1/1nnRrRN1/pRNN4/B4Nb1/1bn1PNP1/3K4/1qn2r2 b - - 0 1",
         false, "");
    test(startFEN, "Rrq3K1/1p3bPQ/P7/B4RNb/1R1n4/kP2nbq1/2PnbpR1/3N1q2 w - - 0 1",
         false, "");
    test("3bk3/1P6/8/8/8/8/8/4K3 w - - 0 1", "B3k3/8/8/8/8/8/8/4K3 w - - 0 1",
         false, "");
}

TEST(ProofKernelTest, testExtMoveToString) {
    ProofKernelTest::testExtMoveToString();
}

void
ProofKernelTest::testExtMoveToString() {
    using ExtPkMove = ProofKernel::ExtPkMove;
    using PieceColor = ProofKernel::PieceColor;
    using PieceType = ProofKernel::PieceType;

    auto test = [](const std::string& str, const ExtPkMove& m) {
        ASSERT_EQ(str, toString(m));
        ASSERT_EQ(strToExtPkMove(str), m);
        ASSERT_EQ(str, toString(strToExtPkMove(str)));
    };

    test("wPg5-g8Q", ExtPkMove(PieceColor::WHITE, PieceType::PAWN, G5, false, G8, PieceType::QUEEN));
    test("bRh8-f6", ExtPkMove(PieceColor::BLACK, PieceType::ROOK, H8, false, F6, PieceType::EMPTY));
    test("wPa3-a6", ExtPkMove(PieceColor::WHITE, PieceType::PAWN, A3, false, A6, PieceType::EMPTY));
    test("wPa6xb7", ExtPkMove(PieceColor::WHITE, PieceType::PAWN, A6, true, B7, PieceType::EMPTY));
    test("wxh8", ExtPkMove(PieceColor::WHITE, PieceType::EMPTY, -1, true, H8, PieceType::EMPTY));
    test("bxc1", ExtPkMove(PieceColor::BLACK, PieceType::EMPTY, -1, true, C1, PieceType::EMPTY));
}

static void
strToPath(const std::string& str, std::vector<ProofKernel::PkMove>& path) {
    path.clear();
    std::vector<std::string> strMoves;
    splitString(str, strMoves);
    for (const std::string& s : strMoves)
        path.push_back(strToPkMove(s));
}

TEST(ProofKernelTest, testExtKernel) {
    ProofKernelTest::testExtKernel();
}

void
ProofKernelTest::testExtKernel() {
    using PkMove = ProofKernel::PkMove;
    using ExtPkMove = ProofKernel::ExtPkMove;

    auto test = [](const std::string& start, const std::string& goal,
                   const std::string& proofKernel,
                   bool expectedSolution, const std::string& expectedPath) {
        Position startPos = TextIO::readFEN(start);
        Position goalPos = TextIO::readFEN(goal);
        ExtProofKernel epk(startPos, goalPos);

        std::vector<PkMove> moves;
        strToPath(proofKernel, moves);
        std::vector<ExtPkMove> extMoves;
        bool found = epk.findExtKernel(moves, extMoves);
        ASSERT_EQ(expectedSolution, found) << "start: " << start << " goal: " << goal << " pk: " << proofKernel;

        std::string extPath;
        for (const ExtPkMove& m : extMoves) {
            if (!extPath.empty())
                extPath += ' ';
            extPath += toString(m);
            ASSERT_EQ(toString(m), toString(strToExtPkMove(toString(m))));
        }
        if (found)
            std::cout << "extMoves: " << extPath << std::endl;
        if (expectedPath != "*") {
            ASSERT_EQ(expectedPath, extPath) << "start: " << start << " goal: " << goal << " pk: " << proofKernel;
        }
    };

    // Prefer capture on rank 4/5
    test(TextIO::startPosFEN, "rnbqkbnr/Pppppppp/8/8/8/8/P1PPPPPP/RNBQKBNR b KQkq - 0 1",
         "wPb0xPa1", true, "wPb2-b4 bPa7-a5 wPb4xa5");
    test(TextIO::startPosFEN, "rnbqkbnr/1ppppppp/8/8/8/8/PpPPPPPP/RNBQKBNR w KQkq - 0 1",
         "bPa1xPb0", true, "bPa7-a5 wPb2-b4 bPa5xb4");

    // Pawn needs to move to leave room for knight to be captured by other pawn
    test("rn2kb2/8/8/6pP/6P1/8/8/4K3 w q - 0 1", "r3kb2/8/8/6p1/3B1B2/8/8/4K3 w q - 0 1",
         "wPg0xNh0", true, "wPh5-h6 bNb8-h5 wPg4xh5");

    // Misc
    test(TextIO::startPosFEN, "rnbqkbnr/ppp1pppp/8/3P4/3p4/8/PP2PPPP/RNB1KBNR w KQkq - 0 1",
         "bPd1xPc0 bPc0xQd0", true, "*");
    test(TextIO::startPosFEN, "3kB3/4pnb1/qpp2bR1/3Nr2r/NB2K1P1/nb1p1P2/nn3Br1/5B2 w - - 0 1",
         "bPa1xPb0 bPb0xPa0 bPf1xPe0 bPg1xPh0 bPc1xQd0 bPd2xRc0", true, "*");
    test(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1",
         "bxPe0", true, "bxe2");
    test(TextIO::startPosFEN, "rnbqkbnr/ppppp1pp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "wxPf1", true, "wxf7");
    test("rnbqkbnr/pppppppp/8/8/3P4/8/PPPBPPPP/RN1QKBNR w KQkq - 0 1",
         "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RN1QKBNR w KQkq - 0 1",
         "bxDB", true, "bxd2");
    test("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQK1NR w KQkq - 0 1",
         "bxLB", true, "bxf1");
    test(TextIO::startPosFEN, "r1bqkb1r/pppppp2/8/7p/6pP/6P1/PPPPP1P1/RNBQKBNR w KQkq - 0 1",
         "wPh0xDBg1 wPf0xNg1 wPg1xNh0", false, "");
    test("r1bqk2r/pppppp2/8/7p/6pP/6P1/PPPPP1P1/RNBQKBNR w KQkq - 0 1",
         "r1bqk2r/pppppp2/8/7P/6pP/8/PPPPP1P1/RNBQKBNR w KQkq - 0 1",
         "wPg1xPh1", false, "");
    test(TextIO::startPosFEN, "rnbqkbnr/1ppppppp/1p6/8/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "bPa1xPb0", true, "wPb2-b6 bPa7xb6");
    test(TextIO::startPosFEN, "rnbqk1nr/pppppp1p/6p1/8/8/7P/PPPPPP1P/RNBQKBNR w KQkq - 0 1",
         "wPg0xDBh1", false, "");
    test(TextIO::startPosFEN, "rnbqkbnr/2pppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 0 1",
         "wPa0xPb1 wPb1xPa0", false, "");

    // Capture promoted piece
    test("rnbqkbnr/1ppppppp/8/p7/1P6/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnb1kbnr/1ppppppp/8/8/8/8/P1PPPPPP/RN1QKBNR w KQkq - 0 1",
         "bPa1xPb0 bPb0xDBcQ wxQ wxQ", true, "bPa5xb4 bPb4-b2 bPb2xc1Q wxc1 wxd8");
    test("rnbqkbnr/1ppppppp/8/p7/1P6/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "r1bqkbnr/1ppppppp/8/8/8/8/P1PPPPPP/RNB1KBNR w KQkq - 0 1",
         "wPb0xPa1 wPa1xNbQ bxQ bxQ", true, "wPb4xa5 wPa5-a7 wPa7xb8Q bxb8 bxd1");

    test(TextIO::startPosFEN, "r2qkbnr/p1pppppp/1p6/8/8/8/1PPPPPPP/RNB1KBNR w KQkq - 0 1",
         "wPa0xNb2 wPb2xLBcQ bxQ bxQ", true, "wPa2-a6 bPb7-b6 bNb8-b7 wPa6xb7 wPb7xc8Q bxc8 bxd1");
    test(TextIO::startPosFEN, "1n1qkbnr/p1pppppp/8/8/8/8/2PPPPPP/R1B1KB1R w KQk - 0 1",
         "wPa0xPb1 wPb1xRaQ wPb0xLBcN bxQ bxQ bxN bxN bxN", true, "*");

    // Invalid proof kernel for legal positions
    test(TextIO::startPosFEN, "5rk1/1ppb3p/p1pb4/6q1/3P1p1r/2P1R2P/PP1BQ1P1/5RKN w - - 0 1",
         "wPc0xPd1 wPd0xPe1 wPd0xNc0 wPe0xNd0 bPf1xLBe0 bPe0xNd0 bPd0xNc1 bPg1xef0",
         false, "");
    test(TextIO::startPosFEN, "5rk1/1ppb3p/p1pb4/6q1/3P1p1r/2P1R2P/PP1BQ1P1/5RKN w - - 0 1",
         "wPc0xPd1 bPe1xPd0 bPd0xQc0 wPd0xNc0 wPe0xNd0 bPg1xLBf0 bPf0xNeN wPf0xNe0", false, "");

    // Capture promotion
    test("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPB/RNBQK1NR w KQkq - 0 1",
         "rnbqkbn1/p1ppppp1/p7/8/8/8/PPP1PPP1/RNBQK1NR w KQq - 0 1",
         "bPb1xQa1 bPh0xDBgN wPd0xNc1 wPc1xRb1",
         true, "wQd1-a6 bPb7xa6 bPh7-h2 wDBc1-g1 bPh2xg1N bNg1-c5 wPd4xc5 bRa8-b6 wPc5xb6");
    test("r1bqkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "R1bqkbnr/p1pppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w KQk - 0 1",
         "wPb0xRaR", true, "wPb2-b7 wPb7xa8R");
    test("r1bqkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "R1bqkbnr/p1pppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w KQk - 0 1",
         "wPb0xLBaR", true, "wPb2-b7 bLBc8-a8 wPb7xa8R");

    // Capture just promoted pawn
    test(TextIO::startPosFEN, "rnbqkbnr/ppp1pp1p/8/8/3p4/3P3N/PPPP1P1P/RNBQKB1R w KQkq - 0 1",
         "bxPg0 wPe0xgd1", true, "bxg2 bPg7-g1N bNg1-d3 wPe2xd3");
    test(TextIO::startPosFEN, "r1bqkb2/pppppp1p/8/8/6P1/2P3P1/PPP1PP2/R1BQKB1R w KQq - 0 1",
         "wPg0xNf1 wPf1xNg1 wPd0xgc1 wPh0xRg0 bxN bxN", true,
         "bNb8-f3 wPg2xf3 bPg7-g3 bNg8-g4 wPf3xg4 bPg3-g1N bNg1-c3 wPd2xc3 bRa8-g3 wPh2xg3 bxb1 bxg1");
    test(TextIO::startPosFEN, "rnbqkbnr/ppp1pp2/2p3p1/6p1/8/8/PPPPPP1P/R1BQKB2 w Qkq - 0 1",
         "bPg1xNf1 bPf1xNg0 bPd1xgc1 bPh1xRg1", true,
         "wNb1-f6 bPg7xf6 wPg2-g6 wNg1-g5 bPf6xg5 wPg6-g8N wNg8-c6 bPd7xc6 wRa1-g6 bPh7xg6");
    test(TextIO::startPosFEN, "r1bqkb1r/pppppp1p/8/8/8/2P2P2/PPP1PP1P/RNBQKBNR w KQkq - 0 1",
         "wPg0xNf1 wPd0xgc1 wxN", true,
         "bNb8-f3 wPg2xf3 bPg7-g1N bNg1-c3 wPd2xc3 wxg8");
    test("r1bqkb1r/p1pppppp/8/8/8/8/PPPPPP1P/RNBQKB1R w KQkq - 0 1",
         "R1bqkb1r/p1pppp1p/8/8/8/8/P1PPPP1P/RNBQKB1R w KQk - 0 1",
         "wPb0xgaR", true, "bPg7-g1N wPb2-b7 bNg1-a8 wPb7xa8R");

    // Non-capture promotions needed
    test(TextIO::startPosFEN, "kNN2b1Q/1R1n2P1/bB2p3/3rQB2/1q3Q2/2ppQ1r1/P1PQq3/B1K1nr2 w - - 0 1",
         "wPb0xPa1 wPd0xPe1 wPh0xPg1 bPf1xRe0", true, "*");
    test(TextIO::startPosFEN, "5b2/5B2/pPbB3b/P1nb1p2/q4NQR/1qP2rp1/1kr2N2/R2NKRrr b - - 0 1",
         "bPb1xPc0 bPe1xPf0 bPh1xPg0 wPd0xNc2", true, "*");
    test(TextIO::startPosFEN, "1RRqk1nr/pPpp1ppp/1p2p3/8/8/8/3PPPPP/RNBQKBNR w KQ - 0 1", //
         "wPc0xLBb2 wPa0xNb2 wPb0xDBc0 wPc0xRb1", true,
         "wPc2-c6 bPb7-b6 bLBc8-b7 wPc6xb7 wPa2-a6 wPb7-b8 bNb8-b7 wPa6xb7 wPb2-b4 bDBf8-c5 wPb4xc5 "
         "wPc5-c6 wPb7-b8 bRa8-b7 wPc6xb7");
    test(TextIO::startPosFEN, "1RRqk1nr/pPpp1ppp/1p2p3/8/8/8/3PPPPP/RNBQKBNR w KQ - 0 1",
         "wPc0xLBb2 wPa0xNb3", false, "");
    test(TextIO::startPosFEN, "1RRqk1nr/pPpp1ppp/4p3/1p6/8/8/3PPPPP/RNBQKBNR w KQ - 0 1",
         "wPc0xLBb2 wPa0xNb3", false, "");
    test(TextIO::startPosFEN, "1RRqk1nr/pPpp1ppp/4p3/1p6/8/8/3PPPPP/RNBQKBNR w KQ - 0 1",
         "wPa0xNb2 wPc0xLBb3", true, "wPa2-a5 bPb7-b5 bNb8-b6 wPa5xb6 wPc2-c6 bLBc8-b7 wPc6xb7");
    test(TextIO::startPosFEN, "1RRqk1nr/pPpp1ppp/4p3/8/1p6/8/3PPPPP/RNBQKBNR w KQ - 0 1",
         "wPc0xLBb2 wPa0xNb3", true, "wPc2-c4 bPb7-b4 bLBc8-b5 wPc4xb5 wPa2-a5 bNb8-b6 wPa5xb6");
    test(TextIO::startPosFEN, "rnbqkbnr/3ppppp/8/8/8/1P2P3/PpPP1PPP/1rrQK1NR w kq - 0 1",
         "bPc1xDBb0 bPa1xNb1 bPb3xLBc1 bPc1xRb2", true,
         "bPc7-c3 wPb2-b3 wDBc1-b2 bPc3xb2 bPa7-a3 bPb2-b1 wNb1-b2 bPa3xb2 bPb7-b5 wLBf1-c4 bPb5xc4 "
         "bPc4-c3 bPb2-b1 wRa1-b2 bPc3xb2");
    test(TextIO::startPosFEN, "r1bqkb1r/pPpppppp/1p6/8/8/8/1P1PPPPP/RNBQKBNR w KQkq - 0 1",
         "wPa0xNb2 wPc0xNb2 bxPb3", false, "");
    test(TextIO::startPosFEN, "rnbqkbnr/1p1ppppp/8/8/8/1P6/PpPPPPPP/R1BQKB1R w KQkq - 0 1",
         "bPa1xNb0 bPc1xNb1 wxPb0", false, "");
    test("rnbqkbnr/ppppp1p1/8/8/8/1P3pPp/P1PPPP1P/RNBQKBNR w kq - 0 1",
         "rnbqkbnr/ppppp1p1/8/8/5P2/1P4P1/P1P1PP1P/R1BQKB1R w kq - 0 1",
         "bPf1xNg0 BPh1xNg1 wPd0xge1 wPe1xgf1", true,
         "wNb1-g2 bPf3xg2 bPg2-g1 wNg1-g2 bPh3xg2 bNg1-e3 wPd2xe3 bPg2-g1N bNg1-f4 wPe3xf4");

    // More than one pawn needs to move away to leave room for a capture
    test("r1bqkbnr/pppppppp/8/8/1P6/1P6/P2PPPPP/RNBQKBNR w KQkq - 0 1",
         "r1bqkb1r/pppppp1p/6p1/1P6/1P6/1P6/3PPPPP/RNBQKBNR w KQkq - 0 1",
         "wPa0xNb0", true, "wPb4-b5 wPb3-b4 bNg8-b3 wPa2xb3");
    test("rnbqkbnr/pppp2pp/4p3/4p3/8/8/PPPPPPPP/R1BQKBNR w KQkq - 0 1",
         "rnbqkbnr/ppp3pp/4p3/4p3/4p3/1P6/P1PPPPPP/R1BQKB1R w KQkq - 0 1",
         "bPd1xNe3", true, "bPe5-e4 bPe6-e5 wNg1-e6 bPd7xe6");
}
