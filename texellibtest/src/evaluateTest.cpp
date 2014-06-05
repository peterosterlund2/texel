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


int swapSquareX(int square) {
    int x = Position::getX(square);
    int y = Position::getY(square);
    return Position::getSquare(7-x, y);
}

int swapSquareY(int square) {
    int x = Position::getX(square);
    int y = Position::getY(square);
    return Position::getSquare(x, 7-y);
}

Position
swapColors(const Position& pos) {
    Position sym;
    sym.setWhiteMove(!pos.getWhiteMove());
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int sq = Position::getSquare(x, y);
            int p = pos.getPiece(sq);
            p = Piece::isWhite(p) ? Piece::makeBlack(p) : Piece::makeWhite(p);
            sym.setPiece(swapSquareY(sq), p);
        }
    }

    int castleMask = 0;
    if (pos.a1Castle()) castleMask |= 1 << Position::A8_CASTLE;
    if (pos.h1Castle()) castleMask |= 1 << Position::H8_CASTLE;
    if (pos.a8Castle()) castleMask |= 1 << Position::A1_CASTLE;
    if (pos.h8Castle()) castleMask |= 1 << Position::H1_CASTLE;
    sym.setCastleMask(castleMask);

    if (pos.getEpSquare() >= 0)
        sym.setEpSquare(swapSquareY(pos.getEpSquare()));

    sym.setHalfMoveClock(pos.getHalfMoveClock());
    sym.setFullMoveCounter(pos.getFullMoveCounter());

    return sym;
}

/** Mirror position in X direction, remove castling rights. */
Position mirrorX(const Position& pos) {
    Position mir;
    mir.setWhiteMove(pos.getWhiteMove());
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int sq = Position::getSquare(x, y);
            int p = pos.getPiece(sq);
            mir.setPiece(swapSquareX(sq), p);
        }
    }

    if (pos.getEpSquare() >= 0)
        mir.setEpSquare(swapSquareX(pos.getEpSquare()));

    mir.setHalfMoveClock(pos.getHalfMoveClock());
    mir.setFullMoveCounter(pos.getFullMoveCounter());

    return mir;
}

/** Evaluation position and check position serialization. */
int evalPos(Evaluate& eval, const Position& pos, bool testMirror) {
    {
        Position pos1(pos);
        U64 h1 = pos1.historyHash();
        pos1.computeZobristHash();
        U64 h2 = pos1.historyHash();
        ASSERT_EQUAL(h1, h2);
    }

    Position pos2;
    Position::SerializeData data;
    pos.serialize(data);
    pos2.deSerialize(data);
    ASSERT(pos.equals(pos2));
    ASSERT_EQUAL(pos.wMtrl(), pos2.wMtrl());
    ASSERT_EQUAL(pos.bMtrl(), pos2.bMtrl());
    ASSERT_EQUAL(pos.wMtrlPawns(), pos2.wMtrlPawns());
    ASSERT_EQUAL(pos.bMtrlPawns(), pos2.bMtrlPawns());

    if (testMirror) {
        Position mir = mirrorX(pos);
        evalPos(eval, mir, false);
    }

    return eval.evalPos(pos);
}

int evalPos(Evaluate& eval, const Position& pos) {
    return evalPos(eval, pos, true);
}

/** Return static evaluation score for white, regardless of whose turn it is to move. */
int
evalWhite(const Position& pos) {
    static std::shared_ptr<Evaluate::EvalHashTables> et;
    if (!et)
        et = Evaluate::getEvalHashTables();
    Evaluate eval(*et);
    return evalWhite(eval, pos);
}

int
evalWhite(Evaluate& eval, const Position& pos) {
    int ret = evalPos(eval, pos);
    std::string fen = TextIO::toFEN(pos);
    Position symPos = swapColors(pos);
    std::string symFen = TextIO::toFEN(symPos);
    int symScore = evalPos(eval, symPos);
    ASSERT_EQUALM((fen + " == " + symFen).c_str(), ret, symScore);
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

static int
evalFEN(const std::string& fen) {
    Position pos = TextIO::readFEN(fen);
    return evalWhite(pos);
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
    ASSERT(cs2 >= cs1 - 7);    // No bonus for useless castle right

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
//    ASSERT(ms4 > ms1);
//    ASSERT(ms3 > ms2);

    pos = TextIO::readFEN("r3kb1r/p3pp1p/bpPq1np1/4N3/2pP4/2N1PQ2/P1PB1PPP/R3K2R b KQkq - 0 12");
    ASSERT(moveScore(pos, "O-O-O") > 0);    // Black long castle is bad for black
    pos.makeMove(TextIO::stringToMove(pos, "O-O-O"), ui);
    ASSERT(moveScore(pos, "O-O") > 0);      // White short castle is good for white

    pos = TextIO::readFEN("rnbqkbnr/pppp1ppp/8/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");
    ASSERT(moveScore(pos, "O-O") > 0);      // Short castle is good for white

    pos = TextIO::readFEN("8/3k4/2p5/1pp5/1P1P4/3K4/8/8 w - - 0 1");
    int sc1 = moveScore(pos, "bxc5");
    int sc2 = moveScore(pos, "dxc5");
    ASSERT(sc1 < sc2);      // Don't give opponent a passed pawn.

    pos = TextIO::readFEN("8/pp1bk3/8/8/8/8/PPPBK3/8 w - - 0 1");
    sc1 = evalWhite(pos);
    pos.setPiece(Position::getSquare(3, 1), Piece::EMPTY);
    pos.setPiece(Position::getSquare(3, 2), Piece::WBISHOP);
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
    score = evalWhite(pos);
    pos.makeMove(TextIO::stringToMove(pos, "Bxc6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nxc6"), ui);
    int score2 = evalWhite(pos);
    ASSERT(score2 < score);                 // Bishop worth more than knight in this case
    ASSERT(moveScore(pos, "Qe2") >= -3);    // Queen away from edge is good

    pos = TextIO::readFEN("5k2/4nppp/p1n5/1pp1p3/4P3/2P1BN2/PP3PPP/3R2K1 w - - 0 1");
    ASSERT(moveScore(pos, "Rd7") > 0);      // Rook on 7:th rank is good
//    ASSERT(moveScore(pos, "Rd8") >= 0);      // Rook on 8:th rank also good
    pos.setPiece(TextIO::getSquare("a1"), Piece::WROOK);
    pos.setPiece(TextIO::getSquare("d1"), Piece::EMPTY);
    ASSERT(moveScore(pos, "Rac1") >= 0);     // Rook on c-f files considered good

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

    const int pV = ::pV;
    const int qV = ::qV;
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

    int s1 = evalFEN("6k1/ppp2pp1/1nnnnn1p/8/8/7P/PPP2PP1/3QQ1K1 w - - 0 1");
    ASSERT(s1 < 0);
    int s2 = evalFEN("6k1/ppp2pp1/nnnnnnnp/8/8/7P/PPP2PP1/Q2QQ1K1 w - - 0 1");
    ASSERT(s2 < s1);
    int s3 = evalFEN("nnnnknnn/pppppppp/8/8/8/8/PPPPPPPP/Q2QK2Q w - - 0 1");
    ASSERT(s3 < 55);

    // Test symmetry of imbalances corrections
    evalFEN("3rr1k1/pppb1ppp/2n2n2/4p3/1bB1P3/2N1BN2/PPP1QPPP/6K1 w - - 0 1");
    evalFEN("3q1rk1/1p1bppbp/p2p1np1/8/1n1NP1PP/2Q1BP2/PPP1B3/1K1R3R w - - 0 1");
    evalFEN("r1bqkbnr/1pp2ppp/p1p5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("r1bqkbnr/1p3ppp/p7/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("r1bqkbnr/1pp2ppp/p1p5/4p3/4P3/5N2/P2P1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("r1bq4/pppp1kpp/2n2n2/2b1p3/4P3/8/PPPP1PPP/RNBQ1RK1 w - - 0 1");
}

static void movePiece(Position& pos, const std::string& from, const std::string& to) {
    int f = TextIO::getSquare(from);
    int t = TextIO::getSquare(to);
    int p = pos.getPiece(f);
    pos.setPiece(f, Piece::EMPTY);
    pos.setPiece(t, p);
}

/**
 * Test of kingSafety method, of class Evaluate.
 */
static void
testKingSafety() {
    Position pos = TextIO::readFEN("r3kb1r/p1p1pppp/b2q1n2/4N3/3P4/2N1PQ2/P2B1PPP/R3R1K1 w kq - 0 1");
    int s1 = evalWhite(pos);
    movePiece(pos, "g7", "b7");
    int s2 = evalWhite(pos);
    ASSERT(s2 < s1);    // Half-open g-file is bad for white

    // Trapping rook with own king is bad
    pos = TextIO::readFEN("rnbqk1nr/pppp1ppp/8/8/1bBpP3/8/PPP2PPP/RNBQK1NR w KQkq - 2 4");
    s1 = evalWhite(pos);
    pos = TextIO::readFEN("rnbqk1nr/pppp1ppp/8/8/1bBpP3/8/PPP2PPP/RNBQ1KNR w kq - 2 4");
    s2 = evalWhite(pos);
    ASSERT(s2 < s1);

    pos = TextIO::readFEN("rnbqk1nr/pppp1ppp/8/8/1bBpPB2/8/PPP1QPPP/RN1K2NR w kq - 0 1");
    s1 = evalWhite(pos);
    movePiece(pos, "d1", "c1"); // rnbqk1nr/pppp1ppp/8/8/1bBpPB2/8/PPP1QPPP/RNK3NR w kq - 0 1
    s2 = evalWhite(pos);
    ASSERT(s2 <= s1);

    // Opposite castling
    pos = TextIO::readFEN("rnbq1rk1/1p2ppbp/p2p1np1/8/3NP3/2N1BP2/PPPQ2PP/2KR1B1R w - - 0 1");
    int sKc1Ph2 = evalWhite(pos);
    movePiece(pos, "c1", "b1");
    int sKb1Ph2 = evalWhite(pos);
    movePiece(pos, "h2", "h3");
    int sKb1Ph3 = evalWhite(pos);
    movePiece(pos, "b1", "c1");
    int sKc1Ph3 = evalWhite(pos);
    ASSERT_EQUAL(sKb1Ph3 - sKb1Ph2, sKc1Ph3 - sKc1Ph2); // Pawn storm bonus same if own king moves within its flank

    int sKg8Ph3 = evalWhite(pos);
    movePiece(pos, "h3", "h2");
    int sKg8Ph2 = evalWhite(pos);
    movePiece(pos, "g8", "h8");
    int sKh8Ph2 = evalWhite(pos);
    movePiece(pos, "h2", "h3");
    int sKh8Ph3 = evalWhite(pos);
    ASSERT_EQUAL(sKg8Ph3 - sKg8Ph2, sKh8Ph3 - sKh8Ph2); // Pawn storm bonus same if other king moves within its flank

    // Test symmetry of king safety evaluation
    evalFEN("rnbq1r1k/pppp1ppp/4pn2/2b5/8/5NP1/PPPPPPBP/RNBQ1RK1 w - - 0 1");
    evalFEN("rn3r1k/pppq1ppp/3p1n2/2b1p3/8/5NPb/PPPPPPBP/RNBQ1RK1 w - - 0 1");
    evalFEN("rn3r1k/ppp2ppp/3p1n2/2b1p3/4P1q1/5bP1/PPPP1PNP/RNB1QRK1 w - - 0 1");
    evalFEN("rn3r1k/ppp1b1pp/3p1n2/2b1p3/4P1q1/5pP1/PPPP1P1P/RNB1QRKB w - - 0 1");
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
    ASSERT_EQUAL(0, score);   // Insufficient material to mate

    pos.setPiece(Position::getSquare(3, 1), Piece::WKNIGHT);
    score = evalWhite(pos);
    ASSERT_EQUAL(0, score);   // Insufficient material to mate

    pos.setPiece(Position::getSquare(3, 1), Piece::WROOK);
    score = evalWhite(pos);
    const int rV = ::rV;
    ASSERT(std::abs(score) > rV + 90);   // Enough material to force mate

    pos.setPiece(Position::getSquare(3, 6), Piece::BBISHOP);
    score = evalWhite(pos);
    const int bV = ::bV;
    ASSERT(score >= 0);
    ASSERT(score < rV - bV);   // Insufficient excess material to mate

    pos.setPiece(Position::getSquare(5, 6), Piece::BROOK);
    score = evalWhite(pos);
    ASSERT(score <= 0);
    ASSERT(-score < bV);

    pos.setPiece(Position::getSquare(2, 6), Piece::BBISHOP);
    score = evalWhite(pos);
    ASSERT(-score > bV * 2);

    // KRPKB is win for white
    score = evalFEN("8/3bk3/8/8/8/3P4/3RK3/8 w - - 0 1");
    const int pV = ::pV;
    ASSERT(score > rV + pV - bV - 100);

    // KNNK is a draw
    score = evalFEN("8/8/4k3/8/8/3NK3/3N4/8 w - - 0 1");
    ASSERT_EQUAL(0, score);

    const int nV = ::nV;
    score = evalFEN("8/8/8/4k3/N6N/P2K4/8/8 b - - 0 66");
    ASSERT(score > nV * 2);

    pos = TextIO::readFEN("8/8/3k4/8/8/3NK3/2B5/8 b - - 0 1");
    score = evalWhite(pos);
    ASSERT(score > 575);  // KBNK is won
    score = moveScore(pos, "Kc6");
    ASSERT(score > 0);      // Black king going into wrong corner, good for white
    score = moveScore(pos, "Ke6");
    ASSERT(score < 0);      // Black king going away from wrong corner, good for black

    // KRN vs KR is generally drawn
    score = evalFEN("rk/p/8/8/8/8/NKR/8 w - - 0 1");
    ASSERT(score < nV - 2 * pV);

    // KRKB, defending king should prefer corner that bishop cannot attack
    pos = TextIO::readFEN("6B1/8/8/8/8/2k5/4r3/2K5 w - - 0 93");
    score = evalWhite(pos);
    ASSERT(score >= -pV);
    score = moveScore(pos, "Kd1");
    ASSERT(score < 0);
    score = moveScore(pos, "Kb1");
    ASSERT(score > 0);

    // Position with passed pawn and opposite side has a knight
    score = evalFEN("8/8/8/1P6/8/B7/1K5n/7k w - - 0 1");
    ASSERT(score > pV);

    { // Test KRPKM
        int score1 = evalFEN("8/2b5/k7/P7/RK6/8/8/8 w - - 0 1");
        ASSERT(score1 < 150);
        int score2 = evalFEN("8/1b6/k7/P7/RK6/8/8/8 w - - 0 1");
        ASSERT(score2 > 300);
        int score3 = evalFEN("8/3b4/1k6/1P6/1RK5/8/8/8 w - - 0 1");
        ASSERT(score3 > 300);
        int score4 = evalFEN("8/3n4/1k6/1P6/1RK5/8/8/8 w - - 0 1");
        ASSERT(score4 > 400);
        int score5 = evalFEN("8/2n5/k7/P7/RK6/8/8/8 w - - 0 1");
        ASSERT(score5 > 400);
    }
}

static void
testEndGameSymmetry() {
    // Test symmetry for pawnless endings
    {
        int score1 = evalFEN("8/8/3rk3/8/8/8/8/3QK3 w - - 0 1");
        int score2 = evalFEN("8/8/8/Q4r2/K4k2/8/8/8 w - - 0 1");
        ASSERT_EQUAL(score2, score1);
        int score3 = evalFEN("3KQ3/8/8/8/8/3kr3/8/8 w - - 0 1");
        ASSERT_EQUAL(score3, score1);
        int score4 = evalFEN("8/8/8/2k4K/2r4Q/8/8/8 w - - 0 1");
        ASSERT_EQUAL(score4, score1);
    }
    {
        int score1 = evalFEN("8/8/3rk3/8/8/8/8/3RK3 w - - 0 1");
        int score2 = evalFEN("8/8/8/R4r2/K4k2/8/8/8 w - - 0 1");
        ASSERT_EQUAL(score2, score1);
        int score3 = evalFEN("3KR3/8/8/8/8/3kr3/8/8 w - - 0 1");
        ASSERT_EQUAL(score3, score1);
        int score4 = evalFEN("8/8/8/2k4K/2r4R/8/8/8 w - - 0 1");
        ASSERT_EQUAL(score4, score1);
    }
}

static void
evalEGConsistency(const std::string& fen, const std::string& wSq, int wPiece,
                  const std::string& bSq, int bPiece, int fuzz) {
    Position pos = TextIO::readFEN(fen);
    int s00 = evalWhite(pos);
    std::string f00 = TextIO::toFEN(pos);
    pos.setPiece(TextIO::getSquare(wSq), wPiece);
    int s10 = evalWhite(pos);
    std::string f10 = TextIO::toFEN(pos);
    pos.setPiece(TextIO::getSquare(bSq), bPiece);
    int s11 = evalWhite(pos);
    std::string f11 = TextIO::toFEN(pos);
    pos.setPiece(TextIO::getSquare(wSq), Piece::EMPTY);
    int s01 = evalWhite(pos);
    std::string f01 = TextIO::toFEN(pos);
    ASSERTM(f10 + " >= " + f00, s10 >= s00 - fuzz);
    ASSERTM(f01 + " <= " + f00, s01 <= s00 + fuzz);
    ASSERTM(f10 + " >= " + f11, s10 >= s11 - fuzz);
    ASSERTM(f01 + " <= " + f11, s01 <= s11 + fuzz);
}

static int
evalEgFen(const std::string& fen, int fuzz = 0) {
    for (int wp = Piece::WQUEEN; wp <= Piece::WPAWN; wp++) {
        for (int bp = Piece::BQUEEN; bp <= Piece::BPAWN; bp++) {
            evalEGConsistency(fen, "a2", wp, "a7", bp, fuzz);
            for (int wp2 = Piece::WQUEEN; wp2 <= Piece::WPAWN; wp2++) {
                for (int bp2 = Piece::BQUEEN; bp2 <= Piece::BPAWN; bp2++) {
                    Position pos = TextIO::readFEN(fen);
                    pos.setPiece(TextIO::getSquare("a2"), wp);
                    evalEGConsistency(TextIO::toFEN(pos), "b2", wp2, "b7", bp2, fuzz);
                    pos.setPiece(TextIO::getSquare("a7"), bp);
                    evalEGConsistency(TextIO::toFEN(pos), "b2", wp2, "b7", bp2, fuzz);
                    pos.setPiece(TextIO::getSquare("a2"), Piece::EMPTY);
                    evalEGConsistency(TextIO::toFEN(pos), "b2", wp2, "b7", bp2, fuzz);
                }
            }
        }
    }
    return evalFEN(fen);
}

static void
testEndGameCorrections() {
    // Four bishops on same color can not win
    int score = evalEgFen("8/4k3/8/1B6/2B5/3B4/2K1B3/8 w - - 0 1");
    ASSERT_EQUAL(0, score);
    // Two bishops on same color can not win against knight
    score = evalEgFen("8/3nk3/8/8/2B5/3B4/4K3/8 w - - 0 1");
    ASSERT(score <= 0);

    int kqk = evalEgFen("8/4k3/8/8/8/3QK3/8/8 w - - 0 1");
    ASSERT(kqk > 1275);

    int krk = evalEgFen("8/4k3/8/8/8/3RK3/8/8 w - - 0 1");
    ASSERT(krk > 975);
    int kqkn = evalEgFen("8/3nk3/8/8/8/3QK3/8/8 w - - 0 1", 2);
    ASSERT(kqkn > 975);
    int kqkb = evalEgFen("8/3bk3/8/8/8/3QK3/8/8 w - - 0 1", 3);
    ASSERT(kqkb > 975);

    ASSERT(kqk > krk);
    ASSERT(kqk > kqkn);
    ASSERT(kqk > kqkb);

    int kbbk = evalEgFen("8/4k3/8/8/8/2BBK3/8/8 w - - 0 1", 6);
    ASSERT(kbbk >= 775);

    ASSERT(krk > kbbk);
    ASSERT(kqkn > kbbk);
    ASSERT(kqkb > kbbk);

    int kbnk = evalEgFen("8/4k3/8/8/8/2BNK3/8/8 w - - 0 1");
    ASSERT(kbnk > 475);
    ASSERT(kbnk < 625);
    int kqkr = evalEgFen("8/3rk3/8/8/8/3QK3/8/8 w - - 0 1");
    ASSERT(kqkr > 475);
    ASSERT(kqkr < 625);

    ASSERT(kbbk > kbnk);
    ASSERT(kbbk > kqkr);

    int kqkbn = evalEgFen("8/2bnk3/8/8/8/3QK3/8/8 w - - 0 1");
    ASSERT(kqkbn >= 200);
    ASSERT(kqkbn <= 250);

    ASSERT(kbnk > kqkbn);
    ASSERT(kqkr > kqkbn);

    int kbbkn = evalEgFen("8/3nk3/8/8/8/2BBK3/8/8 w - - 0 1");
    ASSERT(kbbkn > 75);
    ASSERT(kbbkn < 125);

    ASSERT(kqkbn > kbbkn);

    int kqknn = evalEgFen("8/2nnk3/8/8/8/3QK3/8/8 w - - 0 1");
    ASSERT(kqknn > 25);
    ASSERT(kqknn < 75);
    int kqkbb = evalEgFen("8/2bbk3/8/8/8/3QK3/8/8 w - - 0 1");
    ASSERT(kqkbb > 25);
    ASSERT(kqkbb < 75);
    int kbbkb = evalEgFen("8/3bk3/8/8/8/2BBK3/8/8 w - - 0 1", 1);
    ASSERT(kbbkb > 25);
    ASSERT(kbbkb < 75);
    int kbnkb = evalEgFen("8/3bk3/8/8/8/2NBK3/8/8 w - - 0 1");
    ASSERT(kbnkb > 25);
    ASSERT(kbnkb < 75);
    int kbnkn = evalEgFen("8/3nk3/8/8/8/2NBK3/8/8 w - - 0 1");
    ASSERT(kbnkn > 25);
    ASSERT(kbnkn < 75);
    int knnkb = evalEgFen("8/3bk3/8/8/8/2NNK3/8/8 w - - 0 1");
    ASSERT(knnkb > 0);
    ASSERT(knnkb < 50);
    int knnkn = evalEgFen("8/3nk3/8/8/8/2NNK3/8/8 w - - 0 1");
    ASSERT(knnkn > 0);
    ASSERT(knnkn < 50);

    ASSERT(kbbkn > kqknn);
    ASSERT(kbbkn > kqkbb);
    ASSERT(kbbkn > kbbkb);
    ASSERT(kbbkn > kbnkb);
    ASSERT(kbbkn > kbnkn);
    ASSERT(kbbkn > knnkb);
    ASSERT(kbbkn > knnkn);

    int krkb = evalEgFen("8/3bk3/8/8/8/3RK3/8/8 w - - 0 1", 1);
    ASSERT(krkb > 0);
    ASSERT(krkb < 50);
    int krkn = evalEgFen("8/3nk3/8/8/8/3RK3/8/8 w - - 0 1", 1);
    ASSERT(krkn > 0);
    ASSERT(krkn < 50);
}

/**
 * Passed pawn tests.
 */
static void
testPassedPawns() {
    Position pos = TextIO::readFEN("8/8/8/P3k/8/8/p/K w");
    int score = evalWhite(pos);
    ASSERT(score >= 29); // Unstoppable passed pawn
    pos.setWhiteMove(false);
    score = evalWhite(pos);
    ASSERT(score <= 0); // Not unstoppable
    ASSERT(evalFEN("8/8/P2k4/8/8/8/p7/K7 w - - 0 1") > 88); // Unstoppable passed pawn

    pos = TextIO::readFEN("4R3/8/8/p2K4/P7/4pk2/8/8 w - - 0 1");
    score = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("d5"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("d4"), Piece::WKING); // 4R3/8/8/p7/P2K4/4pk2/8/8 w - - 0 1
    int score2 = evalWhite(pos);
    ASSERT(score2 >= score - 5); // King closer to passed pawn promotion square

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

    // Test symmetry of candidate passed pawn evaluation
    evalFEN("rnbqkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evalFEN("rnbqkbnr/p2ppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    evalFEN("rnbqkbnr/p2ppppp/8/P7/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1");
    evalFEN("rnbqkbnr/p2ppppp/8/P2P4/8/2P5/1P2PPPP/RNBQKBNR w KQkq - 0 1");
    evalFEN("rnbqkbnr/pp1ppppp/8/P2P4/8/2P5/1P2PPPP/RNBQKBNR w KQkq - 0 1");
    evalFEN("rnbqkbnr/pp1ppppp/8/PP1P4/8/2P5/4PPPP/RNBQKBNR w KQkq - 0 1");
    evalFEN("rnbqkbnr/p2ppppp/8/PP6/8/2P5/4PPPP/RNBQKBNR w KQkq - 0 1");
    evalFEN("rnbqkbnr/p2ppppp/8/P2P4/8/2P5/4PPPP/RNBQKBNR w KQkq - 0 1");

    // Test symmetry of "king supporting passed pawn" evaluation
    evalFEN("8/6K1/4R3/7p/2q5/5p1Q/5k2/8 w - - 2 89");
}

/**
 * Test of endGameEval method, of class Evaluate.
 */
static void
testBishAndRookPawns() {
    const int bV = ::bV;
    const int winScore = bV;
    const int drawish = bV / 20;
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
testBishAndPawnFortress() {
    ASSERT_EQUAL(0, evalFEN("1k5B/1p6/1P6/3K4/8/8/8/8 w - - 0 1"));
    ASSERT_EQUAL(0, evalFEN("k6B/1p6/1P6/3K4/8/8/8/8 w - - 0 1"));
    ASSERT(evalFEN("4k3/1p6/1P3B2/3K4/8/8/8/8 w - - 0 1") > 0);

    ASSERT_EQUAL(0, evalFEN("2k4B/1pP5/1P6/3K4/8/8/8/8 w - - 0 1"));
    ASSERT_EQUAL(0, evalFEN("7B/1pPk4/1P6/3K4/8/8/8/8 w - - 0 1"));
    ASSERT(evalFEN("k6B/1pP5/1P6/3K4/8/8/8/8 w - - 0 1") > 0);
    ASSERT_EQUAL(0, evalFEN("2k4B/1pP5/1P6/3K2B1/1P6/8/8/8 w - - 0 1"));
    ASSERT(evalFEN("2k4B/1pP5/1P6/3K4/1P6/3B4/8/8 w - - 0 1") > 0);

    ASSERT(evalFEN("nk5B/1p6/1P6/1P6/1P6/1P3K2/1P6/8 w - - 0 1") > 0);
    ASSERT_EQUAL(0, evalFEN("rk5B/1p6/1P5B/1P4B1/1P6/1P3K2/1P6/8 w - - 0 1"));
    ASSERT_EQUAL(0, evalFEN("1k5B/1p6/1P6/1P6/1P6/1P3K2/1P6/7n w - - 0 1"));

    ASSERT_EQUAL(0, evalFEN("r1k4B/1pP5/1P6/3K4/1P6/8/3B4/8 w - - 0 1"));
    ASSERT(evalFEN("n1k4B/1pP5/1P6/3K4/1P6/8/3B4/8 w - - 0 1") > 0);

    ASSERT_EQUAL(0, evalFEN("2k5/1p6/1P6/4B1K1/8/8/8/8 b - - 0 1"));
    ASSERT(evalFEN("2k5/Kp6/1P6/4B3/8/8/8/8 b - - 0 1") > 0);
    ASSERT_EQUAL(0, evalFEN("k7/1pK5/1P6/8/3B4/8/8/8 w - - 0 1"));
    ASSERT_EQUAL(0, evalFEN("3k4/1p6/1P6/5K2/3B4/8/8/8 w - - 0 1"));
    ASSERT(evalFEN("1K1k4/1p6/1P6/8/3B4/8/8/8 w - - 0 1") > 0);
}

static void
testTrappedBishop() {
    Position pos = TextIO::readFEN("r2q1rk1/ppp2ppp/3p1n2/8/3P4/1P1Q1NP1/b1P2PBP/2KR3R w - - 0 1");
    ASSERT(evalWhite(pos) > -15); // Black has trapped bishop

    pos = TextIO::readFEN("r2q2k1/pp1b1p1p/2p2np1/3p4/3P4/1BNQ2P1/PPPB1P1b/2KR4 w - - 0 1");
    ASSERT(evalWhite(pos) > -pV/2); // Black has trapped bishop
}

/**
 * Test of endGameEval method, of class Evaluate.
 */
static void
testKQKP() {
    const int pV = ::pV;
    const int qV = ::qV;
    const int winScore = 350;
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

    ASSERT(evalFEN("8/8/8/4K3/8/8/2Q5/k7 w - - 0 1") > 0);
    ASSERT_EQUAL(0, evalFEN("8/8/8/4K3/8/8/2Q5/k7 b - - 0 1"));
}

static void
testKRKP() {
    const int pV = ::pV;
    const int rV = ::rV;
    const int winScore = 418;
    const int drawish = (pV + rV) / 20;
    Position pos = TextIO::readFEN("6R1/8/8/8/5K2/2kp4/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
    pos.setWhiteMove(!pos.getWhiteMove());
    ASSERT(evalWhite(pos) < drawish);
}

static void
testKRPKR() {
    const int pV = ::pV;
    const int winScore = 2 * pV;
    const int drawish = pV * 2 / 3;
    Position pos = TextIO::readFEN("8/r7/4K1k1/4P3/8/5R2/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);

    pos = TextIO::readFEN("4k3/7R/1r6/5K2/4P3/8/8/8 w - - 0 1");
    ASSERT(evalWhite(pos) < drawish);
}

static void
testKPK() {
    const int pV = ::pV;
    const int rV = ::rV;
    const int winScore = rV - pV;
    const int drawish = (pV + rV) / 20;
    Position pos = TextIO::readFEN("8/8/8/3k4/8/8/3PK3/8 w - - 0 1");
    ASSERT(evalWhite(pos) > winScore);
    pos.setWhiteMove(!pos.getWhiteMove());
    ASSERT(evalWhite(pos) < drawish);
}

static void
testKPKP() {
    ASSERT_EQUAL(0, evalFEN("1k6/1p6/1P6/3K4/8/8/8/8 w - - 0 1"));
    ASSERT_EQUAL(0, evalFEN("3k4/1p6/1P6/3K4/8/8/8/8 w - - 0 1"));
    ASSERT(evalFEN("2k5/Kp6/1P6/8/8/8/8/8 w - - 0 1") > 0);
}

static void
testKBNK() {
    int s1 = evalWhite(TextIO::readFEN("B1N5/1K6/8/8/8/2k5/8/8 b - - 0 1"));
    ASSERT(s1 > 570);
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
testKBPKB() {
    const int pV = ::pV;
    const int drawish = pV / 5;
    int score = evalWhite(TextIO::readFEN("8/3b4/3k4/8/3P4/3B4/3K4/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < drawish);

    score = evalWhite(TextIO::readFEN("8/1b1k4/8/3PK3/8/3B4/8/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < pV); // Close to known draw

    score = evalWhite(TextIO::readFEN("8/1b6/7k/8/P7/KB6/8/8 w - - 0 1"));
    ASSERT(score > pV);

    score = evalWhite(TextIO::readFEN("8/4k3/P1K5/8/8/4b3/B7/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < drawish);

    score = evalWhite(TextIO::readFEN("1b6/4k3/P1K5/8/8/8/B7/8 w - - 0 1"));
    ASSERT(score > pV / 3);

    score = evalWhite(TextIO::readFEN("1b6/4k3/2K5/P7/8/8/B7/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < drawish);

    score = evalWhite(TextIO::readFEN("8/1P3k2/8/8/K3b3/B7/8/8 w - - 0 1"));
    ASSERT(score >= pV / 3);
}

static void
testKBPKN() {
    const int pV = ::pV;
    const int drawish = pV / 5;
    int score = evalWhite(TextIO::readFEN("8/3k4/8/3P3n/2KB4/8/8/8 w - - 0 1"));
    ASSERT(score > pV);

    score = evalWhite(TextIO::readFEN("8/3k4/8/3P4/2KB3n/8/8/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < drawish);

    score = evalWhite(TextIO::readFEN("8/3k4/8/3P4/2KB2n1/8/8/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < drawish);

    score = evalWhite(TextIO::readFEN("2k5/8/8/3P4/2KB2n1/8/8/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < pV);

    score = evalWhite(TextIO::readFEN("2k5/8/8/3P3n/2KB4/8/8/8 w - - 0 1"));
    ASSERT(score > pV);

    score = evalWhite(TextIO::readFEN("2k5/8/8/3P4/2KB3n/8/8/8 w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < pV);
}

static void
testKNPKB() {
    const int pV = ::pV;
    const int drawish = pV / 5;
    int score = evalWhite(TextIO::readFEN("8/8/3b4/3P4/3NK3/8/8/7k w - - 0 1"));
    ASSERT(score >= 0);
    ASSERT(score < drawish);

    score = evalWhite(TextIO::readFEN("8/8/3P4/8/3NK3/b7/8/7k w - - 0 1"));
    ASSERT(score > pV);

    score = evalWhite(TextIO::readFEN("8/8/8/3P4/4K3/4N3/b7/7k w - - 0 1"));
    ASSERT(score < drawish);

    score = evalWhite(TextIO::readFEN("8/8/8/8/1K6/P3N3/b7/7k w - - 0 1"));
    ASSERT(score > pV);

    score = evalWhite(TextIO::readFEN("8/3P4/4b3/4N3/3K1k2/8/8/8 b - - 0 1"));
    ASSERT(score == 0);
    score = evalWhite(TextIO::readFEN("8/3P4/4b3/4N3/3K1k2/8/8/8 w - - 0 1"));
    ASSERT(score > pV);

    score = evalWhite(TextIO::readFEN("8/3P4/4Nk2/8/3K4/7b/8/8 b - - 0 1"));
    ASSERT(score > pV);

    score = evalWhite(TextIO::readFEN("8/3P4/3N4/8/3K2k1/7b/8/8 b - - 0 1"));
    ASSERT(score > pV);
}

static void
testKNPK() {
    const int pV = ::pV;
    const int nV = ::nV;
    int score = evalWhite(TextIO::readFEN("k7/P7/8/1N6/1K6/8/8/8 w - - 0 1"));
    ASSERT(score == 0);
    score = evalWhite(TextIO::readFEN("8/Pk6/8/1N6/1K6/8/8/8 w - - 0 1"));
    ASSERT(score == 0);

    score = evalWhite(TextIO::readFEN("k7/8/P7/1N6/1K6/8/8/8 w - - 0 1"));
    ASSERT(score > nV);

    score = evalWhite(TextIO::readFEN("K7/P1k5/8/5N2/8/8/8/8 w - - 0 1"));
    ASSERT(score > pV + nV);
    score = evalWhite(TextIO::readFEN("K7/P1k5/8/5N2/8/8/8/8 b - - 0 1"));
    ASSERT(score == 0);

    score = evalWhite(TextIO::readFEN("K7/P1k5/8/8/7N/8/8/8 b - - 0 1"));
    ASSERT(score > nV - pV);
    score = evalWhite(TextIO::readFEN("K7/P1k5/8/8/7N/8/8/8 w - - 0 1"));
    ASSERT(score == 0);

    score = evalWhite(TextIO::readFEN("K7/P3k3/8/8/7N/8/8/8 w - - 0 1"));
    ASSERT(score > pV + nV);
    score = evalWhite(TextIO::readFEN("K7/P3k3/8/8/7N/8/8/8 b - - 0 1"));
    ASSERT(score > pV + nV);
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
    const int pV = ::pV;
    const int winScore = 162;
    const int drawish = 78;
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
    ASSERT(evalWhite(pos) < -winScore + pV*3/2);
}

static void
testKnightOutPost() {
    Position pos = TextIO::readFEN("rnrq2nk/ppp1p1pp/8/4Np2/3P4/8/P3P3/R1RQ2NK w KQkq - 0 1");
    int s1 = evalWhite(pos);
    pos = TextIO::readFEN("rnrq2nk/ppp1p1pp/8/3PNp2/8/8/P3P3/R1RQ2NK w KQkq - 0 1");
    int s2 = evalWhite(pos);
    ASSERT(s2 < s1);

    // Test knight fork bonus symmetry (currently no such term in the evaluation though)
    evalFEN("rnbqkb1r/ppp2Npp/3p4/8/2B1n3/8/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("rnbqkb1r/ppN3pp/3p4/8/2B1n3/8/PPPP1PPP/RNBQK2R b KQkq - 0 1");
}


DECLARE_PARAM(testUciPar1, 60, 10, 80, true);
DECLARE_PARAM(testUciPar2, 120, 100, 300, true);

int uciParVec[3];

DEFINE_PARAM(testUciPar1);
DEFINE_PARAM(testUciPar2);

static void
testUciParam() {
    testUciPar1.registerParam("uciPar1", Parameters::instance());
    testUciPar2.registerParam("uciPar2", Parameters::instance());

    testUciPar2.addListener([](){ uciParVec[0] = uciParVec[2] = testUciPar2; });

    ASSERT_EQUAL(60, static_cast<int>(testUciPar1));
    ASSERT_EQUAL(120, static_cast<int>(testUciPar2));
    ASSERT_EQUAL(120, uciParVec[0]);
    ASSERT_EQUAL(0, uciParVec[1]);
    ASSERT_EQUAL(120, uciParVec[2]);

    Parameters::instance().set("uciPar1", "70");
    ASSERT_EQUAL(70, static_cast<int>(testUciPar1));
    ASSERT_EQUAL(120, static_cast<int>(testUciPar2));
    ASSERT_EQUAL(120, uciParVec[0]);
    ASSERT_EQUAL(0, uciParVec[1]);
    ASSERT_EQUAL(120, uciParVec[2]);

    Parameters::instance().set("uciPar2", "180");
    ASSERT_EQUAL(70, static_cast<int>(testUciPar1));
    ASSERT_EQUAL(180, static_cast<int>(testUciPar2));
    ASSERT_EQUAL(180, uciParVec[0]);
    ASSERT_EQUAL(0, uciParVec[1]);
    ASSERT_EQUAL(180, uciParVec[2]);
}

ParamTable<10> uciParTable { 0, 100, true,
    { 0,2,3,5,-7,7,5,3,0,-2 },
    { 0,1,2,3,-4,4,3,2,0,-1 }
};

ParamTableMirrored<10> uciParTableM(uciParTable);

static void
testUciParamTable() {
    ASSERT_EQUAL(0, uciParTable[0]);
    ASSERT_EQUAL(2, uciParTable[1]);
    ASSERT_EQUAL(3, uciParTable[2]);

    ASSERT_EQUAL(-2, uciParTableM[0]);
    ASSERT_EQUAL(0, uciParTableM[1]);
    ASSERT_EQUAL(3, uciParTableM[2]);
    ASSERT_EQUAL(0, uciParTableM[9]);
    ASSERT_EQUAL(2, uciParTableM[8]);
    ASSERT_EQUAL(3, uciParTableM[7]);

    uciParTable.registerParams("uciParTable", Parameters::instance());
    const int* table = uciParTable.getTable();
    const int* tableM = uciParTableM.getTable();

    Parameters::instance().set("uciParTable1", "11");
    {
        std::vector<int> expected { 0,11,3,5,-7,7,5,3,0,-11 };
        for (int i = 0; i < 10; i++) {
            ASSERT_EQUAL(expected[i], uciParTable[i]);
            ASSERT_EQUAL(expected[i], table[i]);
            ASSERT_EQUAL(expected[10-1-i], uciParTableM[i]);
            ASSERT_EQUAL(expected[10-1-i], tableM[i]);
        }
    }

    Parameters::instance().set("uciParTable2", "13");
    {
        std::vector<int> expected { 0,11,13,5,-7,7,5,13,0,-11 };
        for (int i = 0; i < 10; i++) {
            ASSERT_EQUAL(expected[i], uciParTable[i]);
            ASSERT_EQUAL(expected[i], table[i]);
            ASSERT_EQUAL(expected[10-1-i], uciParTableM[i]);
            ASSERT_EQUAL(expected[10-1-i], tableM[i]);
        }
    }

    Parameters::instance().set("uciParTable3", "17");
    {
        std::vector<int> expected { 0,11,13,17,-7,7,17,13,0,-11 };
        for (int i = 0; i < 10; i++) {
            ASSERT_EQUAL(expected[i], uciParTable[i]);
            ASSERT_EQUAL(expected[i], table[i]);
            ASSERT_EQUAL(expected[10-1-i], uciParTableM[i]);
            ASSERT_EQUAL(expected[10-1-i], tableM[i]);
        }
    }

    Parameters::instance().set("uciParTable4", "19");
    {
        std::vector<int> expected { 0,11,13,17,-19,19,17,13,0,-11 };
        for (int i = 0; i < 10; i++) {
            ASSERT_EQUAL(expected[i], uciParTable[i]);
            ASSERT_EQUAL(expected[i], table[i]);
            ASSERT_EQUAL(expected[10-1-i], uciParTableM[i]);
            ASSERT_EQUAL(expected[10-1-i], tableM[i]);
        }
    }
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
    s.push_back(CUTE(testEndGameSymmetry));
    s.push_back(CUTE(testEndGameCorrections));
    s.push_back(CUTE(testPassedPawns));
    s.push_back(CUTE(testBishAndRookPawns));
    s.push_back(CUTE(testBishAndPawnFortress));
    s.push_back(CUTE(testTrappedBishop));
    s.push_back(CUTE(testKQKP));
    s.push_back(CUTE(testKRKP));
    s.push_back(CUTE(testKRPKR));
    s.push_back(CUTE(testKBNK));
    s.push_back(CUTE(testKPK));
    s.push_back(CUTE(testKPKP));
    s.push_back(CUTE(testCantWin));
    s.push_back(CUTE(testPawnRace));
    s.push_back(CUTE(testKnightOutPost));
    s.push_back(CUTE(testKBPKB));
    s.push_back(CUTE(testKBPKN));
    s.push_back(CUTE(testKNPKB));
    s.push_back(CUTE(testKNPK));
    s.push_back(CUTE(testUciParam));
    s.push_back(CUTE(testUciParamTable));
    return s;
}
