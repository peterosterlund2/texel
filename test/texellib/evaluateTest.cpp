/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
#include "parameters.hpp"
#include "posutil.hpp"

#include "gtest/gtest.h"

/** Evaluation position and check position serialization. */
static int
evalPos(Evaluate& eval, const Position& pos, bool evalMirror, bool testMirror) {
    {
        Position pos1(pos);
        U64 h1 = pos1.historyHash();
        pos1.computeZobristHash();
        U64 h2 = pos1.historyHash();
        EXPECT_EQ(h1, h2);
    }

    Position pos2;
    Position::SerializeData data;
    pos.serialize(data);
    pos2.deSerialize(data);
    EXPECT_EQ(pos, pos2);
    EXPECT_EQ(pos.wMtrl(), pos2.wMtrl());
    EXPECT_EQ(pos.bMtrl(), pos2.bMtrl());
    EXPECT_EQ(pos.wMtrlPawns(), pos2.wMtrlPawns());
    EXPECT_EQ(pos.bMtrlPawns(), pos2.bMtrlPawns());

    eval.connectPosition(pos);
    int evalScore = eval.evalPos();

    if (evalMirror) {
        Position mir = PosUtil::mirrorX(pos);
        int mirrorEval = evalPos(eval, mir, false, false);
        if (testMirror) {
            EXPECT_LE(std::abs(evalScore - mirrorEval), 2);
        }
    }

    return evalScore;
}

/** Return static evaluation score for white, regardless of whose turn it is to move. */
int
evalWhite(const Position& pos, bool testMirror) {
    static auto et = Evaluate::getEvalHashTables();
    Evaluate eval(*et);
    return evalWhite(eval, pos, testMirror);
}

int
evalWhite(Evaluate& eval, const Position& pos, bool testMirror) {
    int ret = evalPos(eval, pos, true, testMirror);
    std::string fen = TextIO::toFEN(pos);
    Position symPos = PosUtil::swapColors(pos);
    std::string symFen = TextIO::toFEN(symPos);
    int symScore = evalPos(eval, symPos, true, testMirror);
    EXPECT_EQ(ret, symScore) << (fen + " == " + symFen);
    EXPECT_EQ(pos.materialId(), PositionTest::computeMaterialId(pos));
    EXPECT_EQ(symPos.materialId(), PositionTest::computeMaterialId(symPos));
    EXPECT_EQ(MatId::mirror(pos.materialId()), symPos.materialId());
    EXPECT_EQ(pos.materialId(), MatId::mirror(symPos.materialId()));
    if (!pos.isWhiteMove())
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
evalFEN(const std::string& fen, bool testMirror = false) {
    Position pos = TextIO::readFEN(fen);
    return evalWhite(pos, testMirror);
}

TEST(EvaluateTest, testEvalPos) {
    EvaluateTest::testEvalPos();
}

void
EvaluateTest::testEvalPos() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "e4"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "e5"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nf3"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nc6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Bb5"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nge7"), ui);
    EXPECT_LT(moveScore(pos, "Ke2"), 0);     // Losing right to castle is bad
    EXPECT_LT(moveScore(pos, "Kf1"), 0);
    EXPECT_LT(moveScore(pos, "Rg1"), 0);
    EXPECT_LT(moveScore(pos, "Rf1"), 0);

    pos = TextIO::readFEN("8/8/8/1r3k2/4pP2/4P3/8/4K2R w K - 0 1");
    EXPECT_EQ(true, pos.h1Castle());
    int cs1 = evalWhite(pos);
    pos.setCastleMask(pos.getCastleMask() & ~(1 << Position::H1_CASTLE));
    EXPECT_EQ(false, pos.h1Castle());
    int cs2 = evalWhite(pos);
    EXPECT_GE(cs2, cs1 - 7);    // No bonus for useless castle right

    pos = TextIO::readFEN("r3kb1r/p3pp1p/bpPq1np1/4N3/2pP4/2N1PQ2/P1PB1PPP/R3K2R b KQkq - 0 12");
    EXPECT_GT(moveScore(pos, "O-O-O"), 0);    // Black long castle is bad for black
    pos.makeMove(TextIO::stringToMove(pos, "O-O-O"), ui);
//    EXPECT_GT(moveScore(pos, "O-O"), 0);    // White short castle is good for white

    pos = TextIO::readFEN("r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");
//    EXPECT_GT(moveScore(pos, "O-O"), 0);    // Short castle is good for white

    pos = TextIO::readFEN("8/pp1bk3/8/8/8/8/PPPBK3/8 w - - 0 1");
    int sc1 = evalWhite(pos);
    pos.setPiece(Square(3, 1), Piece::EMPTY);
    pos.setPiece(Square(3, 2), Piece::WBISHOP);
    int sc2 = evalWhite(pos);
    EXPECT_GT(sc2, sc1);      // Easier to win if bishops on same color
}

static int material(const Position& pos) {
    return pos.wMtrl() - pos.bMtrl();
}

TEST(EvaluateTest, testMaterial) {
    EvaluateTest::testMaterial();
}

void
EvaluateTest::testMaterial() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    EXPECT_EQ(0, material(pos));

    const int pV = ::pV;
    const int qV = ::qV;
    EXPECT_NE(pV, 0);
    EXPECT_NE(qV, 0);
    EXPECT_GT(qV, pV);

    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "e4"), ui);
    EXPECT_EQ(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "d5"), ui);
    EXPECT_EQ(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "exd5"), ui);
    EXPECT_EQ(pV, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Qxd5"), ui);
    EXPECT_EQ(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Nc3"), ui);
    EXPECT_EQ(0, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Qxd2"), ui);
    EXPECT_EQ(-pV, material(pos));
    pos.makeMove(TextIO::stringToMove(pos, "Qxd2"), ui);
    EXPECT_EQ(-pV+qV, material(pos));

    int s1 = evalFEN("6k1/ppp2pp1/1nnnnn1p/8/8/7P/PPP2PP1/3QQ1K1 w - - 0 1");
    EXPECT_LT(s1, 0);
    int s2 = evalFEN("6k1/ppp2pp1/nnnnnnnp/8/8/7P/PPP2PP1/Q2QQ1K1 w - - 0 1");
    EXPECT_LT(s2, s1);
    int s3 = evalFEN("nnnnknnn/pppppppp/8/8/8/8/PPPPPPPP/Q2QK2Q w - - 0 1");
    EXPECT_LT(s3, 55);

    // Test symmetry of imbalances corrections
    evalFEN("3rr1k1/pppb1ppp/2n2n2/4p3/1bB1P3/2N1BN2/PPP1QPPP/6K1 w - - 0 1");
    evalFEN("3q1rk1/1p1bppbp/p2p1np1/8/1n1NP1PP/2Q1BP2/PPP1B3/1K1R3R w - - 0 1");
    evalFEN("r1bqkbnr/1pp2ppp/p1p5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("r1bqkbnr/1p3ppp/p7/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("r1bqkbnr/1pp2ppp/p1p5/4p3/4P3/5N2/P2P1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("r1bq4/pppp1kpp/2n2n2/2b1p3/4P3/8/PPPP1PPP/RNBQ1RK1 w - - 0 1");
}

TEST(EvaluateTest, testKingSafety) {
    EvaluateTest::testKingSafety();
}

void
EvaluateTest::testKingSafety() {
    // Test symmetry of king safety evaluation
    evalFEN("rnbq1r1k/pppp1ppp/4pn2/2b5/8/5NP1/PPPPPPBP/RNBQ1RK1 w - - 0 1");
    evalFEN("rn3r1k/pppq1ppp/3p1n2/2b1p3/8/5NPb/PPPPPPBP/RNBQ1RK1 w - - 0 1");
    evalFEN("rn3r1k/ppp2ppp/3p1n2/2b1p3/4P1q1/5bP1/PPPP1PNP/RNB1QRK1 w - - 0 1");
    evalFEN("rn3r1k/ppp1b1pp/3p1n2/2b1p3/4P1q1/5pP1/PPPP1P1P/RNB1QRKB w - - 0 1");
}

static const int tempoBonusEG = 3;

/** Return true if score is within tempoBonus of 0. */
static bool isDraw(int score) {
    if (abs(score) <= tempoBonusEG) {
        return true;
    } else {
        std::cout << "score:" << score << std::endl;
        return false;
    }
}

TEST(EvaluateTest, testEndGameEval) {
    EvaluateTest::testEndGameEval();
}

void
EvaluateTest::testEndGameEval() {
    Position pos;
    pos.setPiece(Square(4, 1), Piece::WKING);
    pos.setPiece(Square(4, 6), Piece::BKING);
    int score = evalWhite(pos, true);
    EXPECT_TRUE(isDraw(score));

    pos.setPiece(Square(3, 1), Piece::WBISHOP);
    score = evalWhite(pos, true);
    EXPECT_TRUE(isDraw(score)); // Insufficient material to mate

    pos.setPiece(Square(3, 1), Piece::WKNIGHT);
    score = evalWhite(pos, true);
    EXPECT_TRUE(isDraw(score)); // Insufficient material to mate

    pos.setPiece(Square(3, 1), Piece::WROOK);
    score = evalWhite(pos, true);
    const int rV = ::rV;
    EXPECT_GT(std::abs(score), rV + 90);   // Enough material to force mate

    pos.setPiece(Square(3, 6), Piece::BBISHOP);
    score = evalWhite(pos, true);
    const int bV = ::bV;
    EXPECT_GE(score, 0);
    EXPECT_LT(score, rV - bV);   // Insufficient excess material to mate

    pos.setPiece(Square(5, 6), Piece::BROOK);
    score = evalWhite(pos, true);
    EXPECT_LE(score, 0);
    EXPECT_LT(-score, bV);

    pos.setPiece(Square(2, 6), Piece::BBISHOP);
    score = evalWhite(pos, true);
    EXPECT_GT(-score, bV * 2);

    // KRPKB is win for white
    score = evalFEN("8/3bk3/8/8/8/3P4/3RK3/8 w - - 0 1", true);
    const int pV = ::pV;
    EXPECT_GT(score, rV + pV - bV - 100);

    // KNNK is a draw
    score = evalFEN("8/8/4k3/8/8/3NK3/3N4/8 w - - 0 1", true);
    EXPECT_TRUE(isDraw(score));

    const int nV = ::nV;
    score = evalFEN("8/8/8/4k3/N6N/P2K4/8/8 b - - 0 66", true);
    EXPECT_GT(score, nV * 2);

    pos = TextIO::readFEN("8/8/3k4/8/8/3NK3/2B5/8 b - - 0 1");
    score = evalWhite(pos, true);
    EXPECT_GT(score, 560);  // KBNK is won
    score = moveScore(pos, "Kc6");
    EXPECT_GT(score, 0);      // Black king going into wrong corner, good for white
    score = moveScore(pos, "Ke6");
    EXPECT_LT(score, tempoBonusEG*2); // Black king going away from wrong corner, good for black

    // KRN vs KR is generally drawn
    score = evalFEN("rk/p/8/8/8/8/NKR/8 w - - 0 1", true);
    EXPECT_LT(score, nV - 2 * pV);

    // KRKB, defending king should prefer corner that bishop cannot attack
    pos = TextIO::readFEN("6B1/8/8/8/8/2k5/4r3/2K5 w - - 0 93");
    score = evalWhite(pos, true);
    EXPECT_GE(score, -pV);
    score = moveScore(pos, "Kd1");
    EXPECT_LT(score, 0);
    score = moveScore(pos, "Kb1");
    EXPECT_GT(score + tempoBonusEG, 0);

    // Position with passed pawn and opposite side has a knight
    score = evalFEN("8/8/8/1P6/8/B7/1K5n/7k w - - 0 1");
    EXPECT_GT(score, pV);

    { // Test KRPKM
        int score1 = evalFEN("8/2b5/k7/P7/RK6/8/8/8 w - - 0 1", true);
        EXPECT_LT(score1, 223);
        int score2 = evalFEN("8/1b6/k7/P7/RK6/8/8/8 w - - 0 1", true);
        EXPECT_GT(score2, 300);
        int score3 = evalFEN("8/3b4/1k6/1P6/1RK5/8/8/8 w - - 0 1", true);
        EXPECT_GT(score3, 300);
        int score4 = evalFEN("8/3n4/1k6/1P6/1RK5/8/8/8 w - - 0 1", true);
        EXPECT_GT(score4, 400);
        int score5 = evalFEN("8/2n5/k7/P7/RK6/8/8/8 w - - 0 1", true);
        EXPECT_GT(score5, 332);
    }

    { // Test KQKRM+pawns
        int score = evalFEN("8/3pk3/2b1r3/4P3/3QK3/8/8/8 w - - 0 1");
        EXPECT_LT(score, pV / 2);
//        score = evalFEN("8/3pk3/2b2r2/5P2/3Q1K2/8/8/8 w - - 0 1");
//        EXPECT_GT(score, 89);
        score = evalFEN("8/3p1k2/2b2r2/8/5P2/3QK3/8/8 w - - 0 1");
        EXPECT_GT(score, 15);
        score = evalFEN("8/3p1k2/2b5/8/8/5r2/3QKP2/8 w - - 0 1");
        EXPECT_LT(score, pV / 2);
        score = evalFEN("8/4pk2/5b2/6p1/3r2Pp/8/2Q1K2P/8 w - - 0 1");
        EXPECT_LT(score, pV / 5);
        score = evalFEN("8/4pk2/5b2/6p1/3r3p/8/2Q1K1PP/8 w - - 0 1");
        EXPECT_GT(score, 4);
        score = evalFEN("8/4p1k1/4Pb2/5Pp1/3r2Pp/8/2Q1K2P/8 w - - 0 1");
        EXPECT_LT(score, pV);
        score = evalFEN("8/4p1k1/4Pb2/5Pp1/3r2Pp/3P4/2Q1K2P/8 w - - 0 1");
        EXPECT_LT(score, pV);
        score = evalFEN("7k/3p4/2p5/1r3b2/8/8/1P1Q1P2/4K3 w - - 0 1");
        EXPECT_LT(score, pV / 2);
        score = evalFEN("8/5p2/5Bp1/1k3qP1/3R4/4K3/8/8 w - - 0 1");
        EXPECT_GT(score, -pV / 2);

        score = evalFEN("7k/3p4/2p1n3/2P5/3r4/2QP1K2/8/8 w - - 0 1");
        EXPECT_LT(score, pV / 2);
        score = evalFEN("7k/3p4/2p1n3/2P5/3r4/2Q2K2/4P3/8 w - - 0 1");
        EXPECT_GT(score, pV);
        score = evalFEN("8/3p1k2/2p1n3/2P5/3rP3/2Q2K2/8/8 w - - 0 1");
        EXPECT_LT(score, pV / 2);
    }

    { // Test KQKNNNN
        int score = evalFEN("3nk3/3nnn2/8/8/3QK3/8/8/8 w - - 0 1");
        EXPECT_LT(score, -250);
        score = evalFEN("8/5K2/8/3nk3/3nnn2/8/1Q6/8 b - - 0 1");
        EXPECT_LE(score, -400);
    }
}

TEST(EvaluateTest, testEndGameCorrections) {
    EvaluateTest::testEndGameCorrections();
}

void
EvaluateTest::testEndGameCorrections() {
    // Four bishops on same color can not win
    int score = evalFEN("8/4k3/8/1B6/2B5/3B4/2K1B3/8 w - - 0 1");
    EXPECT_TRUE(isDraw(score));
    // Two bishops on same color can not win against knight
    score = evalFEN("8/3nk3/8/8/2B5/3B4/4K3/8 w - - 0 1");
    EXPECT_LE(score, 16);

    int kqk = evalFEN("8/4k3/8/8/8/3QK3/8/8 w - - 0 1");
    EXPECT_GT(kqk, 1275);

    int krk = evalFEN("8/4k3/8/8/8/3RK3/8/8 w - - 0 1");
    EXPECT_GT(krk, 930);
    int kqkn = evalFEN("8/3nk3/8/8/8/3QK3/8/8 w - - 0 1");
    EXPECT_GT(kqkn, 960);
    int kqkb = evalFEN("8/3bk3/8/8/8/3QK3/8/8 w - - 0 1");
    EXPECT_GT(kqkb, 960);

    EXPECT_GT(kqk, krk);
    EXPECT_GT(kqk, kqkn);
    EXPECT_GT(kqk, kqkb);

    int kbbk = evalFEN("8/4k3/8/8/8/2BBK3/8/8 w - - 0 1");
    EXPECT_GE(kbbk, 750);

//    EXPECT_GT(krk, kbbk);
//    EXPECT_GT(kqkn, kbbk);
//    EXPECT_GT(kqkb, kbbk);

    int kbnk = evalFEN("8/4k3/8/8/8/2BNK3/8/8 w - - 0 1");
    EXPECT_GT(kbnk, 475);
    EXPECT_LT(kbnk, 700);
    int kqkr = evalFEN("8/3rk3/8/8/8/3QK3/8/8 w - - 0 1");
    EXPECT_GT(kqkr, 475);
//    EXPECT_LT(kqkr, 900);

    EXPECT_GT(kbbk, kbnk);

    int kqkbn = evalFEN("8/2bnk3/8/8/8/3QK3/8/8 w - - 0 1");
    EXPECT_GE(kqkbn, 200);
    EXPECT_LE(kqkbn, 250);

    EXPECT_GT(kbnk, kqkbn);
    EXPECT_GT(kqkr, kqkbn);

    int kbbkn = evalFEN("8/3nk3/8/8/8/2BBK3/8/8 w - - 0 1");
    EXPECT_GT(kbbkn, 75);
    EXPECT_LT(kbbkn, 125);

    EXPECT_GT(kqkbn, kbbkn);

    int kqknn = evalFEN("8/2nnk3/8/8/8/3QK3/8/8 w - - 0 1");
    EXPECT_GT(kqknn, 25);
    EXPECT_LT(kqknn, 75);
    int kqkbb = evalFEN("8/2bbk3/8/8/8/3QK3/8/8 w - - 0 1");
    EXPECT_GT(kqkbb, 25);
    EXPECT_LT(kqkbb, 75);
    int kbbkb = evalFEN("8/3bk3/8/8/8/2BBK3/8/8 w - - 0 1");
    EXPECT_GE(kbbkb, 0);
    EXPECT_LT(kbbkb, 75);
    int kbnkb = evalFEN("8/3bk3/8/8/8/2NBK3/8/8 w - - 0 1");
    EXPECT_GE(kbnkb, 0);
    EXPECT_LT(kbnkb, 75);
    int kbnkn = evalFEN("8/3nk3/8/8/8/2NBK3/8/8 w - - 0 1");
    EXPECT_GE(kbnkn, 0);
    EXPECT_LT(kbnkn, 75);
    int knnkb = evalFEN("8/3bk3/8/8/8/2NNK3/8/8 w - - 0 1");
    EXPECT_GE(knnkb, 0);
    EXPECT_LT(knnkb, 50);
    int knnkn = evalFEN("8/3nk3/8/8/8/2NNK3/8/8 w - - 0 1");
    EXPECT_GE(knnkn, 0);
    EXPECT_LT(knnkn, 50);

    EXPECT_GT(kbbkn, kqknn);
    EXPECT_GT(kbbkn, kqkbb);
    EXPECT_GT(kbbkn, kbbkb);
    EXPECT_GT(kbbkn, kbnkb);
    EXPECT_GT(kbbkn, kbnkn);
    EXPECT_GT(kbbkn, knnkb);
    EXPECT_GT(kbbkn, knnkn);

    int krkb = evalFEN("8/3bk3/8/8/8/3RK3/8/8 w - - 0 1");
    EXPECT_GT(krkb, 0);
    EXPECT_LT(krkb, 50);
    int krkn = evalFEN("8/3nk3/8/8/8/3RK3/8/8 w - - 0 1", 1);
    EXPECT_GE(krkn, 0);
    EXPECT_LT(krkn, 50);

    // KRKBNN is a draw
    int kbnnkr = evalFEN("8/3rk3/8/8/8/3N4/2NBK3/8 w - - 0 1");
    EXPECT_GE(kbnnkr, -50);
    EXPECT_LT(kbnnkr, 50);

    score = evalFEN("4k3/8/4R1n1/4Pn2/8/8/P2K2b1/8 b - - 6 1", true);
    EXPECT_GE(score, -50);

    // KRKBBN is a win for the BBN side
    int kbbnkr = evalFEN("8/3rk3/8/8/8/3B4/2NBK3/8 w - - 0 1");
    EXPECT_GE(kbbnkr, 300);

    // KRBNKRB is a generally a win
    int krbnkrb = evalFEN("8/4k3/3br3/8/8/3RBN2/4K3/8 w - - 0 1");
    EXPECT_GT(krbnkrb, 71);
    EXPECT_LT(krbnkrb, 300);

    // KRRMKRR is generally a win, except that the 50 move rule
    // sometimes makes it a draw
    int krrnkrr = evalFEN("8/5r2/3r4/4k3/2R4R/4K3/4N3/8 w - -");
    EXPECT_GT(krrnkrr, 104);
    EXPECT_LT(krrnkrr, 370);
    int krrbkrr = evalFEN("8/5r2/3r4/4k3/2R4R/4K3/4B3/8 w - -");
    EXPECT_GT(krrbkrr, 199);
    EXPECT_LT(krrbkrr, 375);
}

TEST(EvaluateTest, testPassedPawns) {
    EvaluateTest::testPassedPawns();
}

void
EvaluateTest::testPassedPawns() {
    Position pos = TextIO::readFEN("8/8/8/P3k/8/8/p/K w");
    int score = evalWhite(pos);
    EXPECT_GE(score, 28); // Unstoppable passed pawn
    pos.setWhiteMove(false);
    score = evalWhite(pos);
    EXPECT_LE(score, 65); // Not unstoppable
    EXPECT_GT(evalFEN("8/8/P2k4/8/8/8/p7/K7 w - - 0 1"), 65); // Unstoppable passed pawn

    pos = TextIO::readFEN("4R3/8/8/p2K4/P7/4pk2/8/8 w - - 0 1");
    score = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("d5"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("d4"), Piece::WKING); // 4R3/8/8/p7/P2K4/4pk2/8/8 w - - 0 1
    int score2 = evalWhite(pos);
    EXPECT_GE(score2, score - 6); // King closer to passed pawn promotion square

    pos = TextIO::readFEN("4R3/8/8/3K4/8/4pk2/8/8 w - - 0 1");
    score = evalWhite(pos);
    pos.setPiece(TextIO::getSquare("d5"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("d4"), Piece::WKING);
    score2 = evalWhite(pos);
    EXPECT_GT(score2, score); // King closer to passed pawn promotion square

    // Connected passed pawn test. Disabled because it didn't help in tests
    //        pos = TextIO::readFEN("4k3/8/8/4P3/3P1K2/8/8/8 w - - 0 1");
    //        score = evalWhite(pos);
    //        pos.setPiece(TextIO::getSquare("d4"), Piece::EMPTY);
    //        pos.setPiece(TextIO::getSquare("d5"), Piece::WPAWN);
    //        score2 = evalWhite(pos);
    //        EXPECT_GT(score2, score); // Advancing passed pawn is good

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

TEST(EvaluateTest, testBishAndRookPawns) {
    EvaluateTest::testBishAndRookPawns();
}

void
EvaluateTest::testBishAndRookPawns() {
    const int bV = ::bV;
    const int winScore = bV;
    const int drawish = bV / 20;
    Position pos = TextIO::readFEN("k7/8/8/8/2B5/2K5/P7/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos, true), winScore);

    pos = TextIO::readFEN("k7/8/8/8/3B4/2K5/P7/8 w - - 0 1");
    EXPECT_LT(evalWhite(pos, true), drawish);

    pos = TextIO::readFEN("8/2k5/8/8/3B4/2K5/P7/8 w - - 0 1");
    EXPECT_LT(evalWhite(pos, true), 122);

    pos = TextIO::readFEN("8/2k5/8/8/3B4/2K4P/8/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos, true), 244);

    pos = TextIO::readFEN("8/2k5/8/8/4B3/2K4P/8/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos, true), 185);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K4P/8/8 w - - 0 1");
    EXPECT_LT(evalWhite(pos, true), drawish);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K4P/7P/8 w - - 0 1");
    EXPECT_LT(evalWhite(pos, true), drawish);

    pos = TextIO::readFEN("8/6k1/8/8/2B1B3/2K4P/7P/8 w - - 0 1");
    EXPECT_LT(evalWhite(pos, true), drawish);

    pos = TextIO::readFEN("8/6k1/8/2B5/4B3/2K4P/7P/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos, true), winScore);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K4P/P7/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos, true), winScore);

    pos = TextIO::readFEN("8/6k1/8/8/4B3/2K3PP/8/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos, true), winScore);
}

TEST(EvaluateTest, testBishAndPawnFortress) {
    EvaluateTest::testBishAndPawnFortress();
}

void
EvaluateTest::testBishAndPawnFortress() {
    EXPECT_TRUE(isDraw(evalFEN("1k5B/1p6/1P6/3K4/8/8/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("k6B/1p6/1P6/3K4/8/8/8/8 w - - 0 1", true)));
    EXPECT_GT(evalFEN("4k3/1p6/1P3B2/3K4/8/8/8/8 w - - 0 1", true), 0);

    EXPECT_TRUE(isDraw(evalFEN("2k4B/1pP5/1P6/3K4/8/8/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("7B/1pPk4/1P6/3K4/8/8/8/8 w - - 0 1", true)));
    EXPECT_GT(evalFEN("k6B/1pP5/1P6/3K4/8/8/8/8 w - - 0 1", true), 0);
    EXPECT_TRUE(isDraw(evalFEN("2k4B/1pP5/1P6/3K2B1/1P6/8/8/8 w - - 0 1", true)));
    EXPECT_GT(evalFEN("2k4B/1pP5/1P6/3K4/1P6/3B4/8/8 w - - 0 1", true), 0);

    EXPECT_GT(evalFEN("nk5B/1p6/1P6/1P6/1P6/1P3K2/1P6/8 w - - 0 1", true), 0);
    EXPECT_TRUE(isDraw(evalFEN("rk5B/1p6/1P5B/1P4B1/1P6/1P3K2/1P6/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("1k5B/1p6/1P6/1P6/1P6/1P3K2/1P6/7n w - - 0 1", true)));

    EXPECT_TRUE(isDraw(evalFEN("r1k4B/1pP5/1P6/3K4/1P6/8/3B4/8 w - - 0 1", true)));
    EXPECT_GT(evalFEN("n1k4B/1pP5/1P6/3K4/1P6/8/3B4/8 w - - 0 1", true), 0);

    EXPECT_TRUE(isDraw(evalFEN("2k5/1p6/1P6/4B1K1/8/8/8/8 b - - 0 1", true)));
    EXPECT_GT(evalFEN("2k5/Kp6/1P6/4B3/8/8/8/8 b - - 0 1", true), 0);
    EXPECT_TRUE(isDraw(evalFEN("k7/1pK5/1P6/8/3B4/8/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("3k4/1p6/1P6/5K2/3B4/8/8/8 w - - 0 1", true)));
    EXPECT_GT(evalFEN("1K1k4/1p6/1P6/8/3B4/8/8/8 w - - 0 1", true), 0);

    EXPECT_LT(evalFEN("8/8/6p1/2b5/2k2P1P/6p1/6P1/7K w - - 1 1", true), 0);
    EXPECT_LT(evalFEN("8/8/6p1/2b5/2k4P/6pP/6P1/7K w - - 1 1", true), 0);

    EXPECT_TRUE(isDraw(evalFEN("8/8/8/8/7p/4k1p1/5bP1/5K2 w - - 1 1", true)));
    EXPECT_LT(evalFEN("8/8/8/8/7p/4k1p1/5bP1/5K2 b - - 1 1", true), 0);
    EXPECT_GT(evalFEN("2k5/1pB5/1P3K2/P7/8/8/8/8 b - - 1 1", true), 0);
    EXPECT_GT(evalFEN("2k5/1p6/1P1BK3/P7/8/8/8/8 b - - 1 1", true), 0);
    EXPECT_TRUE(isDraw(evalFEN("2k1K3/1p6/1P6/P7/8/6B1/8/8 b - - 1 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("k1K3/1p6/1P6/P7/8/8/5B2/8 b - - 1 1", true)));
    EXPECT_GT(evalFEN("k3K3/1p6/1P6/P7/8/8/5B2/8 b - - 1 1", true), 0);
    EXPECT_TRUE(isDraw(evalFEN("k3K3/1p6/1P6/P7/8/8/7B/8 b - - 1 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("k7/1pK5/1P6/P7/8/8/7B/8 b - - 1 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("k7/1pK5/1P6/P7/8/4B3/8/8 b - - 1 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("k1K5/1p6/1P6/P7/8/4B3/8/8 b - - 1 1", true)));
    EXPECT_LT(evalFEN("8/8/8/2b5/4k2p/4P1p1/6P1/7K w - - 1 1", true), 0);
    EXPECT_TRUE(isDraw(evalFEN("8/4b3/4P3/8/7p/6p1/5kP1/7K w - - 1 2", true)));
    EXPECT_TRUE(isDraw(evalFEN("8/8/8/2b1k3/4P2p/6p1/6P1/7K w - - 1 1", true)));

//    EXPECT_TRUE(isDraw(evalFEN("8/8/8/8/6p1/6p1/4k1P1/6K1 b - - 0 10", true)));
    EXPECT_TRUE(isDraw(evalFEN("8/6p1/6p1/8/6p1/8/4k1P1/6K1 b - - 0 1", true)));
    EXPECT_LT(evalFEN("8/6p1/6p1/8/6p1/6P1/4k1K1/8 b - - 0 1", true), 0);

    EXPECT_TRUE(isDraw(evalFEN("7k/5K2/6P1/8/8/3B4/8/8 b - - 1 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("7k/1B3K2/6P1/8/8/3B4/8/8 b - - 1 1", true)));
    EXPECT_GT(evalFEN("7k/5K2/6P1/8/3B4/8/8/8 b - - 1 1", true), 500);
    EXPECT_GT(evalFEN("7k/5KP1/6P1/8/8/3B4/8/8 b - - 1 1", true), 700);
    EXPECT_GT(evalFEN("7k/5K2/6P1/8/8/3B4/8/8 w - - 1 1", true), 500);
    EXPECT_GT(evalFEN("8/5K1k/6P1/8/8/3B4/8/8 b - - 1 1", true), 485);
    EXPECT_GT(evalFEN("7k/5K2/8/6P1/2B5/8/8/8 b - - 1 1", true), 500);

    EXPECT_TRUE(isDraw(evalFEN("8/Bk6/1P6/2K5/8/8/8/8 b - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("k7/B7/1P6/8/8/5K2/8/8 b - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("k7/B7/1PK5/8/8/8/8/8 b - - 0 1", true)));
    EXPECT_GT(evalFEN("k7/B7/1PK5/8/8/8/8/8 w - - 0 1", true), 368);
    EXPECT_TRUE(isDraw(evalFEN("k7/B7/1P6/3K4/8/8/8/8 w - - 0 1", true)));

    EXPECT_TRUE(isDraw(evalFEN("6k1/6Pp/7P/8/3B4/3K4/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("6k1/6Pp/7P/8/3B4/3K4/8/8 b - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("6k1/6Pp/7P/8/3B4/3K3P/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("6k1/6Pp/7P/8/3B4/3K3P/8/8 b - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("8/5kPp/7P/7P/3B4/3K4/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("8/5kPp/7P/7P/3B4/3K4/8/8 b - - 0 1", true)));
    EXPECT_GT(evalFEN("6k1/6Pp/8/7P/3B4/3K4/8/8 w - - 0 1", true), 275);
    EXPECT_GT(evalFEN("6k1/6Pp/8/7P/3B4/3K4/8/8 b - - 0 1", true), 183);
    EXPECT_GT(evalFEN("8/5kPp/7P/7P/3B4/2BK4/8/8 w - - 0 1", true), 500);
    EXPECT_GT(evalFEN("8/5kPp/7P/8/3B4/3K2P1/8/8 w - - 0 1", true), 500);
    EXPECT_GT(evalFEN("8/5kPp/7P/8/3B4/3K4/1P6/8 w - - 0 1", true), 500);
    EXPECT_GT(evalFEN("8/5kPp/7P/8/8/3K4/2B5/8 w - - 0 1", true), 84);
//    EXPECT_GT(evalFEN("6k1/6Pp/8/8/8/3K4/3B4/8 w - - 0 1", true), 180);
    EXPECT_GT(evalFEN("6k1/6P1/7P/8/8/3K4/3B4/8 w - - 0 1", true), 500);
    EXPECT_TRUE(isDraw(evalFEN("6k1/7p/7P/8/8/3K4/3B4/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("8/5k1p/7P/8/8/3K4/3B4/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("7k/7p/7P/8/8/3K4/3B4/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("6k1/1p4Pp/7P/8/3B4/3K4/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("6k1/1p4Pp/7P/8/3B4/3K3P/8/8 w - - 0 1", true)));
    EXPECT_GT(evalFEN("6k1/6Pp/6pP/8/3B4/3K3P/8/8 w - - 0 1", true), 199);
    EXPECT_TRUE(isDraw(evalFEN("5k2/3p3p/5K1P/7P/3B3P/8/8/8 w - - 0 1", true)));
    EXPECT_TRUE(isDraw(evalFEN("6k1/6Pp/7P/8/3BK3/8/6pP/8 w - - 0 1", true)));
    EXPECT_GT(evalFEN("6k1/6Pp/7P/6p1/3BK1pP/8/8/8 w - - 0 1", true), 300);
    EXPECT_TRUE(isDraw(evalFEN("6k1/6Pp/7P/6pP/3BK1p1/8/8/8 w - - 0 1", true)));
}

TEST(EvaluateTest, testTrappedBishop) {
    EvaluateTest::testTrappedBishop();
}

void
EvaluateTest::testTrappedBishop() {
    Position pos = TextIO::readFEN("r2q1rk1/ppp2ppp/3p1n2/8/3P4/1P1Q1NP1/b1P2PBP/2KR3R w - - 0 1");
    EXPECT_GT(evalWhite(pos), -15); // Black has trapped bishop

    pos = TextIO::readFEN("r2q2k1/pp1b1p1p/2p2np1/3p4/3P4/1BNQ2P1/PPPB1P1b/2KR4 w - - 0 1");
    EXPECT_GT(evalWhite(pos), -73); // Black has trapped bishop
}

TEST(EvaluateTest, testKQKP) {
    EvaluateTest::testKQKP();
}

void
EvaluateTest::testKQKP() {
    const int pV = ::pV;
    const int qV = ::qV;
    const int winScore = 350;
    const int drawish = (pV + qV) / 20;

    // Pawn on a2
    Position pos = TextIO::readFEN("8/8/1K6/8/8/Q7/p7/1k6 w - - 0 1");
    EXPECT_LT(evalWhite(pos), drawish);
    pos = TextIO::readFEN("8/8/8/1K6/8/Q7/p7/1k6 w - - 0 1");
    EXPECT_GT(evalWhite(pos), winScore);
    pos = TextIO::readFEN("3Q4/8/8/8/K7/8/1kp5/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos), winScore);
    pos = TextIO::readFEN("8/8/8/8/8/1Q6/p3K3/k7 b - - 0 1");
    EXPECT_LT(evalWhite(pos), drawish);
    pos = TextIO::readFEN("3Q4/2K5/8/8/8/k7/p7/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos), winScore);

    // Pawn on c2
    pos = TextIO::readFEN("3Q4/8/8/8/3K4/8/1kp5/8 w - - 0 1");
    EXPECT_LT(evalWhite(pos), drawish);
    pos = TextIO::readFEN("3Q4/8/8/8/8/4K3/1kp5/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos), winScore);

    EXPECT_GT(evalFEN("8/8/8/4K3/8/8/2Q5/k7 w - - 0 1"), 0);
    EXPECT_TRUE(isDraw(evalFEN("8/8/8/4K3/8/8/2Q5/k7 b - - 0 1")));
}

TEST(EvaluateTest, testKQKRP) {
    EvaluateTest::testKQKRP();
}

void
EvaluateTest::testKQKRP() {
    EXPECT_LT(evalWhite(TextIO::readFEN("1k6/1p6/2r5/8/1K2Q3/8/8/8 w - - 0 1")), 50);
    EXPECT_GT(evalWhite(TextIO::readFEN("8/2k5/2p5/3r4/4Q3/2K5/8/8 w - - 0 1")), 141);
    EXPECT_LT(evalWhite(TextIO::readFEN("1k6/1p6/p1r5/8/1K6/4Q3/8/8 w - - 0 1")), 50);
    EXPECT_LT(evalWhite(TextIO::readFEN("1k6/1p6/1pr5/8/1K6/4Q3/8/8 w - - 0 1")), 50);
    EXPECT_LT(evalWhite(TextIO::readFEN("6k1/6p1/5rp1/8/6K1/3Q4/8/8 w - - 0 1")), 50);
    EXPECT_LT(evalWhite(TextIO::readFEN("8/8/8/3k4/8/3p2Q1/4r3/5K2 b - - 0 1")), 50);
    EXPECT_LT(evalWhite(TextIO::readFEN("8/8/8/8/2Q5/3pk3/4r3/5K2 w - - 0 1")), 50);
    EXPECT_GT(evalWhite(TextIO::readFEN("8/8/8/4Q3/8/3pk3/4r3/5K2 b - - 0 1")), 48);
    EXPECT_LT(evalWhite(TextIO::readFEN("8/8/8/2k5/8/2p2Q2/3r4/4K3 b - - 3 2")), 40);
    EXPECT_GT(evalWhite(TextIO::readFEN("1k6/8/1p6/2r5/3K4/8/4Q3/8 w - - 0 1")), 39);
    EXPECT_LT(evalWhite(TextIO::readFEN("1k6/8/1p6/2r5/3K4/8/5Q2/8 w - - 0 1")), 50);
    EXPECT_LT(evalWhite(TextIO::readFEN("8/8/8/5Q2/8/1kp5/3r4/4K3 w - - 0 1")), 15);
    EXPECT_GT(evalWhite(TextIO::readFEN("8/8/8/1Q6/8/1kp5/3r4/2K5 b - - 0 1")), 9);
    EXPECT_LT(evalWhite(TextIO::readFEN("8/8/8/8/Q7/2pk4/3r4/2K5 b - - 0 1")), 17);
    EXPECT_GT(evalWhite(TextIO::readFEN("8/8/8/3Q4/8/2pk4/3r4/2K5 b - - 0 1")), 25);
}

TEST(EvaluateTest, testKRKP) {
    EvaluateTest::testKRKP();
}

void
EvaluateTest::testKRKP() {
    const int pV = ::pV;
    const int rV = ::rV;
    const int winScore = 343;
    const int drawish = (pV + rV) / 20;
    Position pos = TextIO::readFEN("6R1/8/8/8/5K2/2kp4/8/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos), winScore);
    pos.setWhiteMove(!pos.isWhiteMove());
    EXPECT_LT(evalWhite(pos), drawish);
}

TEST(EvaluateTest, testKRPKR) {
    EvaluateTest::testKRPKR();
}

void
EvaluateTest::testKRPKR() {
    const int pV = ::pV;
    const int winScore = 2 * pV;
    const int drawish = pV * 2 / 3;
    Position pos = TextIO::readFEN("8/r7/4K1k1/4P3/8/5R2/8/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos), winScore);

    pos = TextIO::readFEN("4k3/7R/1r6/5K2/4P3/8/8/8 w - - 0 1");
    EXPECT_LT(evalWhite(pos), drawish);
}

TEST(EvaluateTest, testKPK) {
    EvaluateTest::testKPK();
}

void
EvaluateTest::testKPK() {
    const int pV = ::pV;
    const int rV = ::rV;
    const int winScore = rV - pV;
    const int drawish = (pV + rV) / 20;
    Position pos = TextIO::readFEN("8/8/8/3k4/8/8/3PK3/8 w - - 0 1");
    EXPECT_GT(evalWhite(pos), winScore);
    pos.setWhiteMove(!pos.isWhiteMove());
    EXPECT_LT(evalWhite(pos), drawish);
}

TEST(EvaluateTest, testKPKP) {
    EvaluateTest::testKPKP();
}

void
EvaluateTest::testKPKP() {
    EXPECT_TRUE(isDraw(evalFEN("1k6/1p6/1P6/3K4/8/8/8/8 w - - 0 1")));
    EXPECT_TRUE(isDraw(evalFEN("3k4/1p6/1P6/3K4/8/8/8/8 w - - 0 1")));
//    EXPECT_GT(evalFEN("2k5/Kp6/1P6/8/8/8/8/8 w - - 0 1"), 0);
}

TEST(EvaluateTest, testKBNK) {
    EvaluateTest::testKBNK();
}

void
EvaluateTest::testKBNK() {
    int s1 = evalWhite(TextIO::readFEN("B1N5/1K6/8/8/8/2k5/8/8 b - - 0 1"));
    EXPECT_GT(s1, 550);
    int s2 = evalWhite(TextIO::readFEN("1BN5/1K6/8/8/8/2k5/8/8 b - - 1 1"));
    EXPECT_GT(s2, s1);
    int s3 = evalWhite(TextIO::readFEN("B1N5/1K6/8/8/8/2k5/8/8 b - - 0 1"));
    EXPECT_LT(s3, s2);
    int s4 = evalWhite(TextIO::readFEN("B1N5/1K6/8/8/8/5k2/8/8 b - - 0 1"));
    EXPECT_GT(s4, s3);

    int s5 = evalWhite(TextIO::readFEN("B1N5/8/8/8/8/4K2k/8/8 b - - 0 1"));
    int s6 = evalWhite(TextIO::readFEN("B1N5/8/8/8/8/5K1k/8/8 b - - 0 1"));
    EXPECT_GT(s6, s5);
}

TEST(EvaluateTest, testKBPKB) {
    EvaluateTest::testKBPKB();
}

void
EvaluateTest::testKBPKB() {
    const int pV = ::pV;
    const int drawish = pV / 5;
    int score = evalWhite(TextIO::readFEN("8/3b4/3k4/8/3P4/3B4/3K4/8 w - - 0 1"));
    EXPECT_GE(score, 0);
    EXPECT_LT(score, drawish);

    score = evalWhite(TextIO::readFEN("8/1b1k4/8/3PK3/8/3B4/8/8 w - - 0 1"));
    EXPECT_GE(score, -6);
    EXPECT_LT(score, pV); // Close to known draw

    score = evalWhite(TextIO::readFEN("8/1b6/7k/8/P7/KB6/8/8 w - - 0 1"));
    EXPECT_GT(score, pV);

    score = evalWhite(TextIO::readFEN("8/4k3/P1K5/8/8/4b3/B7/8 w - - 0 1"));
    EXPECT_GE(score, 0);
    EXPECT_LT(score, drawish);

    score = evalWhite(TextIO::readFEN("1b6/4k3/P1K5/8/8/8/B7/8 w - - 0 1"));
    EXPECT_GT(score, pV / 3);

    score = evalWhite(TextIO::readFEN("1b6/4k3/2K5/P7/8/8/B7/8 w - - 0 1"));
    EXPECT_GE(score, 0);
    EXPECT_LT(score, drawish);

    score = evalWhite(TextIO::readFEN("8/1P3k2/8/8/K3b3/B7/8/8 w - - 0 1"));
    EXPECT_GE(score, pV / 3);
}

TEST(EvaluateTest, testKBPKN) {
    EvaluateTest::testKBPKN();
}

void
EvaluateTest::testKBPKN() {
    const int pV = ::pV;
    const int drawish = pV / 5;
    int score = evalWhite(TextIO::readFEN("8/3k4/8/3P3n/2KB4/8/8/8 w - - 0 1"));
    EXPECT_GT(score, pV);

    score = evalWhite(TextIO::readFEN("8/3k4/8/3P4/2KB3n/8/8/8 w - - 0 1"));
    EXPECT_GE(score, 0);
    EXPECT_LT(score, drawish);

    score = evalWhite(TextIO::readFEN("8/3k4/8/3P4/2KB2n1/8/8/8 w - - 0 1"));
    EXPECT_GE(score, 0);
    EXPECT_LT(score, drawish);

    score = evalWhite(TextIO::readFEN("2k5/8/8/3P4/2KB2n1/8/8/8 w - - 0 1"));
    EXPECT_GE(score, -15);
    EXPECT_LT(score, pV);

    score = evalWhite(TextIO::readFEN("2k5/8/8/3P3n/2KB4/8/8/8 w - - 0 1"));
    EXPECT_GT(score, pV);

    score = evalWhite(TextIO::readFEN("2k5/8/8/3P4/2KB3n/8/8/8 w - - 0 1"));
    EXPECT_GE(score, -15);
    EXPECT_LT(score, pV);
}

TEST(EvaluateTest, testKNPKB) {
    EvaluateTest::testKNPKB();
}

void
EvaluateTest::testKNPKB() {
    const int pV = ::pV;
    const int drawish = pV / 5;
    int score = evalWhite(TextIO::readFEN("8/8/3b4/3P4/3NK3/8/8/7k w - - 0 1"));
    EXPECT_GE(score, 0);
    EXPECT_LT(score, drawish);

    score = evalWhite(TextIO::readFEN("8/8/3P4/8/3NK3/b7/8/7k w - - 0 1"));
    EXPECT_GT(score, pV);

    score = evalWhite(TextIO::readFEN("8/8/8/3P4/4K3/4N3/b7/7k w - - 0 1"));
    EXPECT_LT(score, drawish);

    score = evalWhite(TextIO::readFEN("8/8/8/8/1K6/P3N3/b7/7k w - - 0 1"));
    EXPECT_GT(score, pV);

    score = evalWhite(TextIO::readFEN("8/3P4/4b3/4N3/3K1k2/8/8/8 b - - 0 1"));
    EXPECT_TRUE(isDraw(score));
    score = evalWhite(TextIO::readFEN("8/3P4/4b3/4N3/3K1k2/8/8/8 w - - 0 1"));
    EXPECT_GT(score, pV);

    score = evalWhite(TextIO::readFEN("8/3P4/4Nk2/8/3K4/7b/8/8 b - - 0 1"));
    EXPECT_GE(score, 95);

    score = evalWhite(TextIO::readFEN("8/3P4/3N4/8/3K2k1/7b/8/8 b - - 0 1"));
    EXPECT_GT(score, pV);
}

TEST(EvaluateTest, testKNPK) {
    EvaluateTest::testKNPK();
}

void
EvaluateTest::testKNPK() {
    const int pV = ::pV;
    const int nV = ::nV;
    int score = evalWhite(TextIO::readFEN("k7/P7/8/1N6/1K6/8/8/8 w - - 0 1"));
    EXPECT_TRUE(isDraw(score));
    score = evalWhite(TextIO::readFEN("8/Pk6/8/1N6/1K6/8/8/8 w - - 0 1"));
    EXPECT_TRUE(isDraw(score));

    score = evalWhite(TextIO::readFEN("k7/8/P7/1N6/1K6/8/8/8 w - - 0 1"));
    EXPECT_GT(score, nV);

    score = evalWhite(TextIO::readFEN("K7/P1k5/8/5N2/8/8/8/8 w - - 0 1"));
    EXPECT_GT(score, 300);
    score = evalWhite(TextIO::readFEN("K7/P1k5/8/5N2/8/8/8/8 b - - 0 1"));
    EXPECT_TRUE(isDraw(score));

    score = evalWhite(TextIO::readFEN("K7/P1k5/8/8/7N/8/8/8 b - - 0 1"));
    EXPECT_GT(score, (nV - pV) - 50);
    score = evalWhite(TextIO::readFEN("K7/P1k5/8/8/7N/8/8/8 w - - 0 1"));
    EXPECT_TRUE(isDraw(score));

    score = evalWhite(TextIO::readFEN("K7/P3k3/8/8/7N/8/8/8 w - - 0 1"));
    EXPECT_GT(score, pV + nV);
    score = evalWhite(TextIO::readFEN("K7/P3k3/8/8/7N/8/8/8 b - - 0 1"));
    EXPECT_GT(score, pV + nV);
}

TEST(EvaluateTest, testCantWin) {
    EvaluateTest::testCantWin();
}

void
EvaluateTest::testCantWin() {
    Position pos = TextIO::readFEN("8/8/8/3k4/3p4/3K4/4N3/8 w - - 0 1");
    int score1 = evalWhite(pos);
    EXPECT_LE(score1, tempoBonusEG);
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "Nxd4"), ui);
    int score2 = evalWhite(pos);
    EXPECT_LE(score2, 0);
    EXPECT_GE(score2, score1 - 2 * tempoBonusEG);
}

TEST(EvaluateTest, testKnightOutPost) {
    EvaluateTest::testKnightOutPost();
}

void
EvaluateTest::testKnightOutPost() {
    // Test knight fork bonus symmetry (currently no such term in the evaluation though)
    evalFEN("rnbqkb1r/ppp2Npp/3p4/8/2B1n3/8/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    evalFEN("rnbqkb1r/ppN3pp/3p4/8/2B1n3/8/PPPP1PPP/RNBQK2R b KQkq - 0 1");
}


DECLARE_PARAM(testUciPar1, 60, 10, 80, true);
DECLARE_PARAM(testUciPar2, 120, 100, 300, true);

int uciParVec[3];

DEFINE_PARAM(testUciPar1);
DEFINE_PARAM(testUciPar2);

TEST(EvaluateTest, testUciParam) {
    EvaluateTest::testUciParam();
}

void
EvaluateTest::testUciParam() {
    testUciPar1.registerParam("uciPar1", Parameters::instance());
    testUciPar2.registerParam("uciPar2", Parameters::instance());

    testUciPar2.addListener([](){ uciParVec[0] = uciParVec[2] = testUciPar2; });

    EXPECT_EQ(60, static_cast<int>(testUciPar1));
    EXPECT_EQ(120, static_cast<int>(testUciPar2));
    EXPECT_EQ(120, uciParVec[0]);
    EXPECT_EQ(0, uciParVec[1]);
    EXPECT_EQ(120, uciParVec[2]);

    Parameters::instance().set("uciPar1", "70");
    EXPECT_EQ(70, static_cast<int>(testUciPar1));
    EXPECT_EQ(120, static_cast<int>(testUciPar2));
    EXPECT_EQ(120, uciParVec[0]);
    EXPECT_EQ(0, uciParVec[1]);
    EXPECT_EQ(120, uciParVec[2]);

    Parameters::instance().set("uciPar2", "180");
    EXPECT_EQ(70, static_cast<int>(testUciPar1));
    EXPECT_EQ(180, static_cast<int>(testUciPar2));
    EXPECT_EQ(180, uciParVec[0]);
    EXPECT_EQ(0, uciParVec[1]);
    EXPECT_EQ(180, uciParVec[2]);

    // Test button parameters
    int cnt1 = 0;
    int id1 = UciParams::clearHash->addListener([&cnt1]() {
        cnt1++;
    }, false);
    EXPECT_EQ(0, cnt1);
    Parameters::instance().set("Clear Hash", "");
    EXPECT_EQ(1, cnt1);
    Parameters::instance().set("Clear hash", "");
    EXPECT_EQ(2, cnt1);
    Parameters::instance().set("clear hash", "");
    EXPECT_EQ(3, cnt1);

    int cnt2 = 0;
    std::shared_ptr<Parameters::ButtonParam> testButton2(std::make_shared<Parameters::ButtonParam>("testButton2"));
    Parameters::instance().addPar(testButton2);
    int id2 = testButton2->addListener([&cnt2]() {
        cnt2++;
    }, false);
    EXPECT_EQ(3, cnt1);
    EXPECT_EQ(0, cnt2);
    Parameters::instance().set("testButton2", "");
    EXPECT_EQ(3, cnt1);
    EXPECT_EQ(1, cnt2);
    Parameters::instance().set("Clear Hash", "");
    EXPECT_EQ(4, cnt1);
    EXPECT_EQ(1, cnt2);

    UciParams::clearHash->removeListener(id1);
    Parameters::instance().set("Clear Hash", "");
    EXPECT_EQ(4, cnt1);
    EXPECT_EQ(1, cnt2);
    Parameters::instance().set("testButton2", "");
    EXPECT_EQ(4, cnt1);
    EXPECT_EQ(2, cnt2);

    testButton2->removeListener(id2);
    Parameters::instance().set("Clear Hash", "");
    EXPECT_EQ(4, cnt1);
    EXPECT_EQ(2, cnt2);
    Parameters::instance().set("testButton2", "");
    EXPECT_EQ(4, cnt1);
    EXPECT_EQ(2, cnt2);
}

ParamTable<10> uciParTable { 0, 100, true,
    { 0,2,3,5,-7,7,5,3,0,-2 },
    { 0,1,2,3,-4,4,3,2,0,-1 }
};

TEST(EvaluateTest, testUciParamTable) {
    EvaluateTest::testUciParamTable();
}

void
EvaluateTest::testUciParamTable() {
    EXPECT_EQ(0, uciParTable[0]);
    EXPECT_EQ(2, uciParTable[1]);
    EXPECT_EQ(3, uciParTable[2]);

    uciParTable.registerParams("uciParTable", Parameters::instance());
    const int* table = uciParTable.getTable();

    Parameters::instance().set("uciParTable1", "11");
    {
        std::vector<int> expected { 0,11,3,5,-7,7,5,3,0,-11 };
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ(expected[i], uciParTable[i]);
            EXPECT_EQ(expected[i], table[i]);
        }
    }

    Parameters::instance().set("uciParTable2", "13");
    {
        std::vector<int> expected { 0,11,13,5,-7,7,5,13,0,-11 };
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ(expected[i], uciParTable[i]);
            EXPECT_EQ(expected[i], table[i]);
        }
    }

    Parameters::instance().set("uciParTable3", "17");
    {
        std::vector<int> expected { 0,11,13,17,-7,7,17,13,0,-11 };
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ(expected[i], uciParTable[i]);
            EXPECT_EQ(expected[i], table[i]);
        }
    }

    Parameters::instance().set("uciParTable4", "19");
    {
        std::vector<int> expected { 0,11,13,17,-19,19,17,13,0,-11 };
        for (int i = 0; i < 10; i++) {
            EXPECT_EQ(expected[i], uciParTable[i]);
            EXPECT_EQ(expected[i], table[i]);
        }
    }
}

TEST(EvaluateTest, testSwindleScore) {
    EvaluateTest::testSwindleScore();
}

void
EvaluateTest::testSwindleScore() {
    for (int e = 0; e < 3000; e++) {
        int s1 = Evaluate::swindleScore(e, 0);
        ASSERT_GE(s1, (e?1:0));
        ASSERT_LT(s1, 50);
        ASSERT_LE(s1, e);
        ASSERT_LE(s1, Evaluate::swindleScore(e+1, 0));
        int s2 = Evaluate::swindleScore(-e, 0);
        ASSERT_EQ(-s1, s2);
    }

    for (int e = 0; e < 1000; e += 10) {
        for (int d = 1; d < 35; d++) {
            int s0 = Evaluate::swindleScore(e, 0);
            int s1 = Evaluate::swindleScore(e, d);
            int s2 = Evaluate::swindleScore(e, d+1);
            ASSERT_LE(0, s0);
            ASSERT_LT(s0, s2);
            ASSERT_LT(s2, s1);
        }
        for (int d = 1; d < 35; d++) {
            int s0 = Evaluate::swindleScore(-e, 0);
            int s1 = Evaluate::swindleScore(-e, -d);
            int s2 = Evaluate::swindleScore(-e, -(d+1));
            ASSERT_GE(0, s0);
            ASSERT_GT(s0, s2);
            ASSERT_GT(s2, s1);
        }
    }

    int s0 = Evaluate::swindleScore(5000, 0);
    int s1 = Evaluate::swindleScore(3, 1000);
    EXPECT_GT(s1, s0);

    s0 = Evaluate::swindleScore(-5000, 0);
    s1 = Evaluate::swindleScore(-3, -1000);
    EXPECT_LT(s1, s0);
}
