/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * tbgenTest.cpp
 *
 *  Created on: Apr 4, 2015
 *      Author: petero
 */

#include "tbgenTest.hpp"
#include "tbgen.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "tbprobe.hpp"

#include "gtest/gtest.h"

TEST(TBGenTest, testPositionValue) {
    TBGenTest::testPositionValue();
}

void
TBGenTest::testPositionValue() {
    PositionValue pv;

    ASSERT_TRUE(pv.isUnInitialized());
    ASSERT_TRUE(!pv.isRemainingN());
    for (int n = 0; n < 50; n++)
        ASSERT_TRUE(!pv.isMatedInN(n));
    ASSERT_TRUE(!pv.isComputed());
    ASSERT_TRUE(!pv.isRemainingN());

    for (int n = 0; n < 50; n++) {
        pv.setMateInN(n);
        ASSERT_TRUE(!pv.isUnInitialized());
        for (int n2 = 0; n2 < 50; n2++)
            ASSERT_TRUE(!pv.isMatedInN(n2));
        ASSERT_TRUE(pv.isComputed());
        ASSERT_TRUE(!pv.isRemainingN());

        pv.setMatedInN(n);
        ASSERT_TRUE(!pv.isUnInitialized());
        for (int n2 = 0; n2 < 50; n2++)
            ASSERT_EQ(n==n2, pv.isMatedInN(n2));
        ASSERT_TRUE(pv.isComputed());
        ASSERT_TRUE(!pv.isRemainingN());

        pv.setRemaining(n);
        for (int n2 = 0; n2 < n; n2++) {
            ASSERT_TRUE(pv.isRemainingN());
            pv.decRemaining();
        }
        ASSERT_TRUE(!pv.isRemainingN());
    }

    pv.setDraw();
    ASSERT_TRUE(!pv.isUnInitialized());
    for (int n = 0; n < 50; n++)
        ASSERT_TRUE(!pv.isMatedInN(n));
    ASSERT_TRUE(pv.isComputed());
    ASSERT_TRUE(!pv.isRemainingN());

    pv.setInvalid();
    ASSERT_TRUE(!pv.isUnInitialized());
    for (int n = 0; n < 50; n++)
        ASSERT_TRUE(!pv.isMatedInN(n));
    ASSERT_TRUE(pv.isComputed());
    ASSERT_TRUE(!pv.isRemainingN());
}

static PieceCount
pieceCount(int nwq, int nwr, int nwb, int nwn,
           int nbq, int nbr, int nbb, int nbn) {
    PieceCount pc;
    pc.nwq = nwq;
    pc.nwr = nwr;
    pc.nwb = nwb;
    pc.nwn = nwn;
    pc.nbq = nbq;
    pc.nbr = nbr;
    pc.nbb = nbb;
    pc.nbn = nbn;
    return pc;
}

TEST(TBGenTest, testTBIndex) {
    TBGenTest::testTBIndex();
}

void
TBGenTest::testTBIndex() {
    TBIndex idx(1, 1, 17);
    ASSERT_EQ(17, idx.getIndex());

    idx.setIndex(0);
    ASSERT_EQ(0, idx.getIndex());
    ASSERT_TRUE(!idx.whiteMove());
    ASSERT_EQ(Square(0), idx.getSquare(0));
    ASSERT_EQ(Square(0), idx.getSquare(1));

    idx.swapSide();
    ASSERT_TRUE(idx.whiteMove());

    idx.setSquare(1, Square(17));

    ASSERT_TRUE(idx.whiteMove());
    ASSERT_EQ(Square(0), idx.getSquare(0));
    ASSERT_EQ(Square(17), idx.getSquare(1));

    std::vector<int> pieceTypes { Piece::WKING, Piece::BKING };
    idx.canonize(pieceTypes, false);
    ASSERT_TRUE(idx.whiteMove());
    ASSERT_EQ(Square(0), idx.getSquare(0));
    ASSERT_EQ(Square(10), idx.getSquare(1));
}

TEST(TBGenTest, testTBPosition) {
    TBGenTest::testTBPosition();
}

void
TBGenTest::testTBPosition() {
    TBPosition tbPos(pieceCount(0,0,0,0, 0,0,0,0));
    ASSERT_EQ(2*10*64, tbPos.nPositions());
    int nValid = 0;
    for (U32 idx = 0; idx < tbPos.nPositions(); idx++) {
        tbPos.setIndex(idx);
        if (!tbPos.indexValid())
            continue;
        if (!tbPos.canTakeKing())
            nValid++;
    }
    ASSERT_EQ(2*(1*33+3*(64-6)+3*(64-9)+3*(36-6)), nValid);
}

TEST(TBGenTest, testMoveGen) {
    TBGenTest::testMoveGen();
}

void
TBGenTest::testMoveGen() {
    {
        TBPosition tbPos(pieceCount(1,0,0,0, 0,0,0,0));
        ASSERT_EQ(2*10*64*64, tbPos.nPositions());
        Position pos;

        for (U32 idx = 0; idx < tbPos.nPositions(); idx++) {
            tbPos.setIndex(idx);
            if (!tbPos.indexValid())
                continue;
            tbPos.getPos(pos);
            ASSERT_EQ(1, BitBoard::bitCount(pos.pieceTypeBB(Piece::WKING)));
            ASSERT_EQ(1, BitBoard::bitCount(pos.pieceTypeBB(Piece::BKING)));
            ASSERT_TRUE(BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN)) <= 1);
            if (tbPos.canTakeKing()) {
                ASSERT_TRUE(MoveGen::canTakeKing(pos));
                continue;
            } else {
                ASSERT_TRUE(!MoveGen::canTakeKing(pos));
            }

            TbMoveList tbMoves;
            tbPos.getMoves(tbMoves);
            int nMoves = tbMoves.getSize();

            MoveList moves;
            MoveGen::pseudoLegalMoves(pos, moves);

            if (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(A1,B2,C3,D4,E5,F6,G7,H8))
                continue; // Too hard to compare because of symmetry handling

            if (moves.size != nMoves)
                std::cout << "idx:" << idx << " fen:" << TextIO::toFEN(pos) << std::endl;
            ASSERT_EQ(moves.size, nMoves);
        }
    }
    {
        TBPosition tbPos(pieceCount(1,0,0,0, 0,1,0,0));
        ASSERT_EQ(2*10*64*64*64, tbPos.nPositions());
        Position pos;

        for (U32 idx = 0; idx < tbPos.nPositions(); idx++) {
            tbPos.setIndex(idx);
            if (!tbPos.indexValid())
                continue;
            tbPos.getPos(pos);
            ASSERT_EQ(1, BitBoard::bitCount(pos.pieceTypeBB(Piece::WKING)));
            ASSERT_EQ(1, BitBoard::bitCount(pos.pieceTypeBB(Piece::BKING)));
            ASSERT_TRUE(BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN)) <= 1);
            if (tbPos.canTakeKing()) {
                ASSERT_TRUE(MoveGen::canTakeKing(pos));
                continue;
            } else {
                ASSERT_TRUE(!MoveGen::canTakeKing(pos));
            }

            TbMoveList tbMoves;
            tbPos.getMoves(tbMoves);
            int nMoves = tbMoves.getSize();

            MoveList moves;
            MoveGen::pseudoLegalMoves(pos, moves);

            if (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(A1,B2,C3,D4,E5,F6,G7,H8))
                continue; // Too hard to compare because of symmetry handling

            ASSERT_EQ(moves.size, nMoves);
        }
    }
}

void
TBGenTest::testGenerateInternal(const PieceCount& pc) {
#if 0
    VectorStorage vs;
    TBGenerator<VectorStorage> tbGen(vs, pc);
#else
    TranspositionTable tt(512*1024);
    TTStorage tts(tt);
    TBGenerator<TTStorage> tbGen(tts, pc);
#endif
    RelaxedShared<S64> maxTimeMillis(-1);
    tbGen.generate(maxTimeMillis, true);

    TBPosition tbPos(pc);
    ASSERT_EQ(2*10*64*64*64, tbPos.nPositions());
    Position pos;

    int minMateFail = 1000;
    int minMatedFail = 1000;
    for (U32 idx = 0; idx < tbPos.nPositions(); idx++) {
        tbPos.setIndex(idx);
        if (!tbPos.indexValid())
            continue;
        tbPos.getPos(pos);
        if (MoveGen::canTakeKing(pos)) {
            ASSERT_EQ((int)PositionValue::State::MATE_IN_0, tbGen.getValue(tbPos));
            continue;
        }

        int score;
        bool result = TBProbe::gtbProbeDTM(pos, 0, score);
        ASSERT_TRUE(result);

        int score2 = tbGen.getValue(tbPos);
        if (score == 0) {
            if (score2 != 0) {
                std::cout << "idx:" << idx << " s2:" << score2
                          << " fen:" << TextIO::toFEN(pos) << std::endl;
            }
            ASSERT_EQ(0, score2);
        } else if (score > 0) {
            int mateN = (SearchConst::MATE0 - score) / 2;
            if (mateN + (int)PositionValue::State::MATE_IN_0 != score2) {
                if (mateN < minMateFail) {
                    minMateFail = mateN;
                    std::cout << "idx:" << idx << " m:" << mateN << " s2:" << score2
                              << " fen:" << TextIO::toFEN(pos) << std::endl;
                }
            }
        } else {
            int matedN = (SearchConst::MATE0 + score - 1) / 2;
            if ((int)PositionValue::State::MATED_IN_0 - matedN != score2) {
                if (matedN < minMatedFail) {
                    minMatedFail = matedN;
                    std::cout << "idx:" << idx << " m:" << matedN << " s2:" << score2
                              << " fen:" << TextIO::toFEN(pos) << std::endl;
                }
            }
        }
    }
    ASSERT_EQ(1000, minMateFail);
    ASSERT_EQ(1000, minMatedFail);
}

TEST(TBGenTest, testGenerate) {
    TBGenTest::testGenerate();
}

void
TBGenTest::testGenerate() {
    bool checkAll = false;
    if (checkAll) {
        testGenerateInternal(pieceCount(2,0,0,0, 0,0,0,0));
        testGenerateInternal(pieceCount(1,1,0,0, 0,0,0,0));
        testGenerateInternal(pieceCount(1,0,1,0, 0,0,0,0));
        testGenerateInternal(pieceCount(1,0,0,1, 0,0,0,0));
        testGenerateInternal(pieceCount(1,0,0,0, 1,0,0,0));
        testGenerateInternal(pieceCount(1,0,0,0, 0,1,0,0));
        testGenerateInternal(pieceCount(1,0,0,0, 0,0,1,0));
        testGenerateInternal(pieceCount(1,0,0,0, 0,0,0,1));
        testGenerateInternal(pieceCount(0,2,0,0, 0,0,0,0));
        testGenerateInternal(pieceCount(0,1,1,0, 0,0,0,0));
        testGenerateInternal(pieceCount(0,1,0,1, 0,0,0,0));
        testGenerateInternal(pieceCount(0,1,0,0, 0,1,0,0));
        testGenerateInternal(pieceCount(0,1,0,0, 0,0,1,0));
        testGenerateInternal(pieceCount(0,1,0,0, 0,0,0,1));
        testGenerateInternal(pieceCount(0,0,2,0, 0,0,0,0));
        testGenerateInternal(pieceCount(0,0,1,1, 0,0,0,0));
        testGenerateInternal(pieceCount(0,0,1,0, 0,0,1,0));
        testGenerateInternal(pieceCount(0,0,1,0, 0,0,0,1));
        testGenerateInternal(pieceCount(0,0,0,2, 0,0,0,0));
        testGenerateInternal(pieceCount(0,0,0,1, 0,0,0,1));
    } else { // Quick test, just check KBNK
        testGenerateInternal(pieceCount(0,0,1,1, 0,0,0,0));
    }
}
