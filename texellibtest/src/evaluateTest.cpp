/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
 * evaluateTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "evaluateTest.hpp"
#include "positionTest.hpp"
#include "evaluate.hpp"
#include "position.hpp"
#include "textio.hpp"

#include "cute.h"


Position
swapColors(const Position& pos) {
    Position sym;
    sym.setWhiteMove(!pos.getWhiteMove());
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int p = pos.getPiece(Position::getSquare(x, y));
            p = Piece::isWhite(p) ? Piece::makeBlack(p) : Piece::makeWhite(p);
            sym.setPiece(Position::getSquare(x, 7-y), p);
        }
    }

    int castleMask = 0;
    if (pos.a1Castle()) castleMask |= 1 << Position::A8_CASTLE;
    if (pos.h1Castle()) castleMask |= 1 << Position::H8_CASTLE;
    if (pos.a8Castle()) castleMask |= 1 << Position::A1_CASTLE;
    if (pos.h8Castle()) castleMask |= 1 << Position::H1_CASTLE;
    sym.setCastleMask(castleMask);

    if (pos.getEpSquare() >= 0) {
        int x = Position::getX(pos.getEpSquare());
        int y = Position::getY(pos.getEpSquare());
        sym.setEpSquare(Position::getSquare(x, 7-y));
    }

    sym.setHalfMoveClock(pos.getHalfMoveClock());
    sym.setFullMoveCounter(pos.getFullMoveCounter());

    return sym;
}

/** Return static evaluation score for white, regardless of whose turn it is to move. */
int
evalWhite(const Position& pos) {
    auto et = Evaluate::getEvalHashTables();
    Evaluate eval(*et);
    int ret = eval.evalPos(pos);
    Position symPos = swapColors(pos);
    int symScore = eval.evalPos(symPos);
    ASSERT_EQUAL(ret, symScore);
    ASSERT_EQUAL(pos.materialId(), PositionTest::computeMaterialId(pos));
    ASSERT_EQUAL(symPos.materialId(), PositionTest::computeMaterialId(symPos));
    if (!pos.getWhiteMove())
        ret = -ret;
    return ret;
}

/** Compute change in eval score for white after making "moveStr" in position "pos". */
static int
moveScore(const Position& pos, const std::string& moveStr) {
    int score1 = evalWhite(pos);
    Position tmpPos(pos);
    UndoInfo ui;
    tmpPos.makeMove(TextIO::stringToMove(tmpPos, moveStr), ui);
    int score2 = evalWhite(tmpPos);
//    printf("move:%s s1:%d s2:%d\n", moveStr, score1, score2);
    return score2 - score1;
}

/**
 * Test of evalPos method, of class Evaluate.
 */
static
void testEvalPos() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "e4"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "e5"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nf3"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nc6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Bb5"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nge7"), ui);
    ASSERT(moveScore(pos, "O-O") > 0);      // Castling is good
    ASSERT(moveScore(pos, "Ke2") < 0);      // Losing right to castle is bad
    ASSERT(moveScore(pos, "Kf1") < 0);
    ASSERT(moveScore(pos, "Rg1") < 0);
    ASSERT(moveScore(pos, "Rf1") < 0);

    pos = TextIO::readFEN("8/8/8/1r3k2/4pP2/4P3/8/4K2R w K - 0 1");
    ASSERT_EQUAL(true, pos.h1Castle());
    int cs1 = evalWhite(pos);
    pos.setCastleMask(pos.getCastleMask() & ~(1 << Position::H1_CASTLE));
    ASSERT_EQUAL(false, pos.h1Castle());
    int cs2 = evalWhite(pos);
    ASSERT(cs2 >= cs1);    // No bonus for useless castle right

    // Test rook open file bonus
    pos = TextIO::readFEN("r4rk1/1pp1qppp/3b1n2/4p3/2B1P1b1/1QN2N2/PP3PPP/R3R1K1 w - - 0 1");
    int ms1 = moveScore(pos, "Red1");
    int ms2 = moveScore(pos, "Rec1");
    int ms3 = moveScore(pos, "Rac1");
    int ms4 = moveScore(pos, "Rad1");
    ASSERT(ms1 > 0);        // Good to have rook on open file
    ASSERT(ms2 > 0);        // Good to have rook on half-open file
    ASSERT(ms1 > ms2);      // Open file better than half-open file
    ASSERT(ms3 > 0);
    ASSERT(ms4 > 0);
    ASSERT(ms4 > ms1);
    ASSERT(ms3 > ms2);

    pos = TextIO::readFEN("r3kb1r/p3pp1p/bpPq1np1/4N3/2pP4/2N1PQ2/P1PB1PPP/R3K2R b KQkq - 0 12");
    ASSERT(moveScore(pos, "O-O-O") > 0);    // Black long castle is bad for black
    pos.makeMove(TextIO::stringToMove(pos, "O-O-O"), ui);
    ASSERT(moveScore(pos, "O-O") > 0);      // White short castle is good for white

    pos = TextIO::readFEN("1nb1kbn1/pppp1ppp/8/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQ - 0 1");
    ASSERT(moveScore(pos, "O-O") > 0);      // Short castle is good for white

    pos = TextIO::readFEN("8/3k4/2p5/1pp5/1P1P4/3K4/8/8 w - - 0 1");
    int sc1 = moveScore(pos, "bxc5");
    int sc2 = moveScore(pos, "dxc5");
    ASSERT(sc1 < sc2);      // Don't give opponent a passed pawn.

    pos = TextIO::readFEN("8/pp1bk3/8/8/8/8/PPPBK3/8 w - - 0 1");
    sc1 = evalWhite(pos);
    pos.setPiece(Position::getSquare(3, 1), Piece::EMPTY);
    pos.setPiece(Position::getSquare(3, 0), Piece::WBISHOP);
    sc2 = evalWhite(pos);
    ASSERT(sc2 > sc1);      // Easier to win if bishops on same color

    // Test bishop mobility
    pos = TextIO::readFEN("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3");
    sc1 = moveScore(pos, "Bd3");
    sc2 = moveScore(pos, "Bc4");
    ASSERT(sc2 > sc1);
}

/**
 * Test of pieceSquareEval method, of class Evaluate.
 */
static void
testPieceSquareEval() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    int score = evalWhite(pos);
    ASSERT_EQUAL(0, score);    // Should be zero, by symmetry
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "e4"), ui);
    score = evalWhite(pos);
    ASSERT(score > 0);     // Centralizing a pawn is a good thing
    pos.makeMove(TextIO::stringToMove(pos, "e5"), ui);
    score = evalWhite(pos);
    ASSERT_EQUAL(0, score);    // Should be zero, by symmetry
    ASSERT(moveScore(pos, "Nf3") > 0);      // Developing knight is good
    pos.makeMove(TextIO::stringToMove(pos, "Nf3"), ui);
    ASSERT(moveScore(pos, "Nc6") < 0);      // Developing knight is good
    pos.makeMove(TextIO::stringToMove(pos, "Nc6"), ui);
    ASSERT(moveScore(pos, "Bb5") > 0);      // Developing bishop is good
    pos.makeMove(TextIO::stringToMove(pos, "Bb5"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nge7"), ui);
    ASSERT(moveScore(pos, "Qe2") > 0);      // Queen away from edge is good
    score = evalWhite(pos);
    pos.makeMove(TextIO::stringToMove(pos, "Bxc6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nxc6"), ui);
    int score2 = evalWhite(pos);
    ASSERT(score2 < score);                 // Bishop worth more than knight in this case

    pos = TextIO::readFEN("5k2/4nppp/p1n5/1pp1p3/4P3/2P1BN2/PP3PPP/3R2K1 w - - 0 1");
    ASSERT(moveScore(pos, "Rd7") > 0);      // Rook on 7:th rank is good
    ASSERT(moveScore(pos, "Rd8") <= 0);     // Rook on 8:th rank not particularly good
    pos.setPiece(TextIO::getSquare("a1"), Piece::WROOK);
    ASSERT(moveScore(pos, "Rac1") > 0);     // Rook on c-f files considered good

    pos = TextIO::readFEN("r4rk1/pppRRppp/1q4b1/n7/8/2N3B1/PPP1QPPP/6K1 w - - 0 1");
    score = evalWhite(pos);
    ASSERT(score > 100); // Two rooks on 7:th rank is very good
}

/**
 * Test of tradeBonus method, of class Evaluate.
 */
static void
testTradeBonus() {
    std::string fen = "8/5k2/6r1/2p1p3/3p4/2P2N2/3PPP2/4K1R1 w - - 0 1";
    Position pos = TextIO::readFEN(fen);
    int score1 = evalWhite(pos);
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "Rxg6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Kxg6"), ui);
    int score2 = evalWhite(pos);
    ASSERT(score2 > score1);    // White ahead, trading pieces is good

    pos = TextIO::readFEN(fen);
    pos.makeMove(TextIO::stringToMove(pos, "cxd4"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "cxd4"), ui);
    score2 = evalWhite(pos);
    ASSERT(score2 < score1);    // White ahead, trading pawns is bad

    pos = TextIO::readFEN("8/8/1b2b3/4kp2/5N2/4NKP1/6B1/8 w - - 0 62");
    score1 = evalWhite(pos);
    pos.makeMove(TextIO::stringToMove(pos, "Nxe6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Kxe6"), ui);
    score2 = evalWhite(pos);
    ASSERT(score2 > score1); // White ahead, trading pieces is good
}

static int material(const Position& pos) {
    return pos.wMtrl() - pos.bMtrl();
}

/**
 * Test of material method, of class Evaluate.
 */
static void
testMaterial() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    ASSERT_EQUAL(0, material(pos));

    const int pV = Evaluate::pV;
    const int qV = Evaluate::qV;
    ASSERT(pV != 0);
    ASSERT(qV != 0);
    ASSERT(qV > pV);

    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "e4"), ui);
    ASSERT_EQUAL(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "d5"), ui);
    ASSERT_EQUAL(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "exd5"), ui);
    ASSERT_EQUAL(pV, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Qxd5"), ui);
    ASSERT_EQUAL(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Nc3"), ui);
    ASSERT_EQUAL(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Qxd2"), ui);
    ASSERT_EQUAL(-pV, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Qxd2"), ui);
    ASSERT_EQUAL(-pV+qV, material(pos));
}

/**
 * Test of kingSafety method, of class Evaluate.
 */
static void
testKingSafety() {
    Position pos = TextIO::readFEN("r3kb1r/p1p1pppp/b2q1n2/4N3/3P4/2N1PQ2/P2B1PPP/R3R1K1 w kq - 0 1");
    int s1 = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("g7"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("b7"), Piece::BPAWN);
    int s2 = evalWhite(pos);
    ASSERT(s2 < s1);    // Half-open g-file is bad for white

    // Trapping rook with own king is bad
    pos = TextIO::readFEN("rnbqk1nr/pppp1ppp/8/8/1bBpP3/8/PPP2PPP/RNBQK1NR w KQkq - 2 4");
    s1 = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("e1"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("f1"), Piece::WKING);
    s2 = evalWhite(pos);
    ASSERT(s2 < s1);

    pos = TextIO::readFEN("rnbqk1nr/pppp1ppp/8/8/1bBpPB2/8/PPP1QPPP/RN1K2NR w kq - 0 1");
    s1 = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("d1"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("c1"), Piece::WKING);
    s2 = evalWhite(pos);
    ASSERT(s2 < s1);
}

/**
 * Test of endGameEval method, of class Evaluate.
 */
static void
testEndGameEval() {
    Position pos;
    pos.setPiece(Position::getSquare(4, 1), Piece::WKING);
    pos.setPiece(Position::getSquare(4, 6), Piece::BKING);
    int score = evalWhite(pos);
    ASSERT_EQUAL(0, score);

    pos.setPiece(Position::getSquare(3, 1), Piece::WBISHOP);
    score = evalWhite(pos);
    ASSERT(std::abs(score) < 50);   // Insufficient material to mate

    pos.setPiece(Position::getSquare(3, 1), Piece::WKNIGHT);
    score = evalWhite(pos);
    ASSERT(std::abs(score) < 50);   // Insufficient material to mate

    pos.setPiece(Position::getSquare(3, 1), Piece::WROOK);
    score = evalWhite(pos);
    const int rV = Evaluate::rV;
    ASSERT(std::abs(score) > rV + 100);   // Enough material to force mate

    pos.setPiece(Position::getSquare(3, 6), Piece::BBISHOP);
    score = evalWhite(pos);
    const int bV = Evaluate::bV;
    ASSERT(score >= 0);
    ASSERT(score < rV - bV);   // Insufficient excess material to mate

    pos.setPiece(Position::getSquare(5, 6), Piece::BROOK);
    score = evalWhite(pos);
    ASSERT(score <= 0);
    ASSERT(-score < bV);

    pos.setPiece(Position::getSquare(2, 6), Piece::BBISHOP);
    score = evalWhite(pos);
    ASSERT(-score > bV * 2 + 100);

    // KRPKN is win for white
    pos = TextIO::readFEN("8/3bk3/8/8/8/3P4/3RK3/8 w - - 0 1");
    score = evalWhite(pos);
    const int pV = Evaluate::pV;
    ASSERT(score > rV + pV - bV - 100);

    // KNNK is a draw
    pos = TextIO::readFEN("8/8/4k3/8/8/3NK3/3N4/8 w - - 0 1");
    score = evalWhite(pos);
    ASSERT(std::abs(score) < 50);

    const int nV = Evaluate::nV;
    pos = TextIO::readFEN("8/8/8/4k3/N6N/P2K4/8/8 b - - 0 66");
    score = evalWhite(pos);
    ASSERT(score > nV * 2);

    pos = TextIO::readFEN("8/8/3k4/8/8/3NK3/2B5/8 b - - 0 1");
    score = evalWhite(pos);
    ASSERT(score > bV + nV + 150);  // KBNK is won, should have a bonus
    score = moveScore(pos, "Kc6");
    ASSERT(score > 0);      // Black king going into wrong corner, good for white
    score = moveScore(pos, "Ke6");
    ASSERT(score < 0);      // Black king going away from wrong corner, good for black

    // KRN vs KR is generally drawn
    pos = TextIO::readFEN("rk/p/8/8/8/8/NKR/8 w - - 0 1");
    score = evalWhite(pos);
    ASSERT(score < nV - 2 * pV);

    // KRKB, defending king should prefer corner that bishop cannot attack
    pos = TextIO::readFEN("6B1/8/8/8/8/2k5/4r3/2K5 w - - 0 93");
    score = evalWhite(pos);
    ASSERT(score >= -pV);
    score = moveScore(pos, "Kd1");
    ASSERT(score < 0);
    score = moveScore(pos, "Kb1");
    ASSERT(score > 0);
}

/**
 * Passed pawn tests.
 */
static void
testPassedPawns() {
    Position pos = TextIO::readFEN("8/8/8/P3k/8/8/p/K w");
    int score = evalWhite(pos);
    ASSERT(score > 300); // Unstoppable passed pawn
    pos.setWhiteMove(false);
    score = evalWhite(pos);
    ASSERT(score <= 0); // Not unstoppable

    pos = TextIO::readFEN("4R3/8/8/p2K4/P7/4pk2/8/8 w - - 0 1");
    score = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("d5"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("d4"), Piece::WKING);
    int score2 = evalWhite(pos);
    ASSERT(score2 > score); // King closer to passed pawn promotion square

    pos = TextIO::readFEN("4R3/8/8/3K4/8/4pk2/8/8 w - - 0 1");
    score = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("d5"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("d4"), Piece::WKING);
    score2 = evalWhite(pos);
    ASSERT(score2 > score); // King closer to passed pawn promotion square

    // Connected passed pawn test. Disabled because it didn't help in tests
    //        pos = TextIO::readFEN("4k3/8/8/4P3/3P1K2/8/8/8 w - - 0 1");
    //        score = evalWhite(pos);
    //        pos.setPiece(TextIO::getSquare("d4"), Piece::EMPTY);
    //        pos.setPiece(TextIO::getSquare("d5"), Piece::WPAWN);
    //        score2 = evalWhite(pos);
    //        ASSERT(score2 > score); // Advancing passed pawn is good
}

/**
 * Test of endGameEval method, of class Evaluate.
 */
static void
testBishAndRookPawns() {
    const int pV = Evaluate::pV;
    const int bV = Evaluate::bV;
    const int winScore = pV + bV;
    const int drawish = (pV + bV) / 20;
    Position pos = TextIO::readFEN("k7/8/8/8/2B5/2K5/P7/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("k7/8/8/8/3B4/2K5/P7/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);

    pos = TextIO::readFEN("8/2k5/8/8/3B4/2K5/P7/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("8/2k5/8/8/3B4/2K4P/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("8/2k5/8/8/4B3/2K4P/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K4P/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K4P/7P/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);

    pos = TextIO::readFEN("8/6k1/8/8/2B1B3/2K4P/7P/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);

    pos = TextIO::readFEN("8/6k1/8/2B5/4B3/2K4P/7P/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K4P/P7/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K3PP/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
}

static void
testTrappedBishop() {
    Position pos = TextIO::readFEN("r2q1rk1/ppp2ppp/3p1n2/8/3P4/1P1Q1NP1/b1P2PBP/2KR3R w - - 0 1");
    ASSERT(evalWhite(pos) > 0); // Black has trapped bishop

    pos = TextIO::readFEN("r2q2k1/pp1b1p1p/2p2np1/3p4/3P4/1BNQ2P1/PPPB1P1b/2KR4 w - - 0 1");
    ASSERT(evalWhite(pos) > 0); // Black has trapped bishop
}

/**
 * Test of endGameEval method, of class Evaluate.
 */
static void
testKQKP() {
    const int pV = Evaluate::pV;
    const int qV = Evaluate::qV;
    const int winScore = qV - pV - 200;
    const int drawish = (pV + qV) / 20;

    // Pawn on a2
    Position pos = TextIO::readFEN("8/8/1K6/8/8/Q7/p7/1k6 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);
    pos = TextIO::readFEN("8/8/8/1K6/8/Q7/p7/1k6 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
    pos = TextIO::readFEN("3Q4/8/8/8/K7/8/1kp5/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
    pos = TextIO::readFEN("8/8/8/8/8/1Q6/p3K3/k7 b - - 0 1");
    ASSERT(evalWhite(pos) < drawish);

    // Pawn on c2
    pos = TextIO::readFEN("3Q4/8/8/8/3K4/8/1kp5/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);
    pos = TextIO::readFEN("3Q4/8/8/8/8/4K3/1kp5/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
}

static void
testKRKP() {
    const int pV = Evaluate::pV;
    const int rV = Evaluate::rV;
    const int winScore = rV - pV;
    const int drawish = (pV + rV) / 20;
    Position pos = TextIO::readFEN("6R1/8/8/8/5K2/2kp4/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
    pos.setWhiteMove(!pos.getWhiteMove());
    ASSERT(evalWhite(pos) < drawish);
}

static void
testKRPKR() {
    const int pV = Evaluate::pV;
    const int winScore = 2 * pV;
    const int drawish = pV * 2 / 3;
    Position pos = TextIO::readFEN("8/r7/4K1k1/4P3/8/5R2/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("4k3/7R/1r6/5K2/4P3/8/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);
}

static void
testKPK() {
    const int pV = Evaluate::pV;
    const int rV = Evaluate::rV;
    const int winScore = rV - pV;
    const int drawish = (pV + rV) / 20;
    Position pos = TextIO::readFEN("8/8/8/3k4/8/8/3PK3/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
    pos.setWhiteMove(!pos.getWhiteMove());
    ASSERT(evalWhite(pos) < drawish);
}

static void
testKBNK() {
    int s1 = evalWhite(TextIO::readFEN("B1N5/1K6/8/8/8/2k5/8/8 b - - 0 1"));
    const int nV = Evaluate::nV;
    const int bV = Evaluate::bV;
    ASSERT(s1 > nV + bV);
    int s2 = evalWhite(TextIO::readFEN("1BN5/1K6/8/8/8/2k5/8/8 b - - 1 1"));
    ASSERT(s2 > s1);
    int s3 = evalWhite(TextIO::readFEN("B1N5/1K6/8/8/8/2k5/8/8 b - - 0 1"));
    ASSERT(s3 < s2);
    int s4 = evalWhite(TextIO::readFEN("B1N5/1K6/8/8/8/5k2/8/8 b - - 0 1"));
    ASSERT(s4 > s3);

    int s5 = evalWhite(TextIO::readFEN("B1N5/8/8/8/8/4K2k/8/8 b - - 0 1"));
    int s6 = evalWhite(TextIO::readFEN("B1N5/8/8/8/8/5K1k/8/8 b - - 0 1"));
    ASSERT(s6 > s5);
}

static void
testCantWin() {
    Position pos = TextIO::readFEN("8/8/8/3k4/3p4/3K4/4N3/8 w - - 0 1");
    int score1 = evalWhite(pos);
    ASSERT(score1 < 0);
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "Nxd4"), ui);
    int score2 = evalWhite(pos);
    ASSERT(score2 <= 0);
    ASSERT(score2 > score1);
}

static void
testPawnRace() {
    const int pV = Evaluate::pV;
    const int winScore = 400;
    const int drawish = 100;
    Position pos = TextIO::readFEN("8/8/K7/1P3p2/8/6k1/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
    pos = TextIO::readFEN("8/8/K7/1P3p2/8/6k1/8/8 b - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("8/8/K7/1P3p2/6k1/8/8/8 b - - 0 1");
    ASSERT(std::abs(evalWhite(pos)) < drawish);
    pos = TextIO::readFEN("8/8/K7/1P6/5pk1/8/8/8 b - - 0 1");
    ASSERT(evalWhite(pos) < -winScore);
    pos = TextIO::readFEN("8/K7/8/1P6/5pk1/8/8/8 b - - 0 1");
    ASSERT(std::abs(evalWhite(pos)) < drawish);
    pos = TextIO::readFEN("8/K7/8/8/1PP2p1k/8/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish + pV);
    ASSERT(evalWhite(pos) > 0);
    pos = TextIO::readFEN("8/K7/8/8/1PP2p1k/8/8/8 b - - 0 1");
    ASSERT(evalWhite(pos) < -winScore + pV);
}

static void
testKnightOutPost() {
    Position pos = TextIO::readFEN("rnrq2nk/ppp1p1pp/8/4Np2/3P4/8/P3P3/R1RQ2NK w KQkq - 0 1");
    int s1 = evalWhite(pos);
    pos = TextIO::readFEN("rnrq2nk/ppp1p1pp/8/3PNp2/8/8/P3P3/R1RQ2NK w KQkq - 0 1");
    int s2 = evalWhite(pos);
    ASSERT(s2 < s1);
}

cute::suite
EvaluateTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testEvalPos));
    s.push_back(CUTE(testPieceSquareEval));
    s.push_back(CUTE(testTradeBonus));
    s.push_back(CUTE(testMaterial));
    s.push_back(CUTE(testKingSafety));
    s.push_back(CUTE(testEndGameEval));
    s.push_back(CUTE(testPassedPawns));
    s.push_back(CUTE(testBishAndRookPawns));
    s.push_back(CUTE(testTrappedBishop));
    s.push_back(CUTE(testKQKP));
    s.push_back(CUTE(testKRKP));
    s.push_back(CUTE(testKRPKR));
    s.push_back(CUTE(testKBNK));
    s.push_back(CUTE(testKPK));
    s.push_back(CUTE(testCantWin));
    s.push_back(CUTE(testPawnRace));
    s.push_back(CUTE(testKnightOutPost));
    return s;
}
