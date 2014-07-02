/*
    Texel - A UCI chess engine.
    Copyright (C) 2014  Peter Österlund, peterosterlund2@gmail.com

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
 * tbTest.cpp
 *
 *  Created on: Jun 2, 2014
 *      Author: petero
 */

#include "tbTest.hpp"
#include "evaluateTest.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "evaluate.hpp"
#include "textio.hpp"
#include "tbprobe.hpp"
#include "constants.hpp"

#include "syzygy/rtb-probe.hpp"

#include "cute.h"

/** Probe both DTM and WDL, check consistency and return DTM value. */
static int probeCompare(const Position& pos, int ply, int& score) {
    int dtm, wdl, wdl2;
    Position pos2(pos);
    int resDTM = TBProbe::gtbProbeDTM(pos2, ply, dtm);
    ASSERT(pos.equals(pos2));
    int resWDL = TBProbe::gtbProbeWDL(pos2, ply, wdl);
    ASSERT(pos.equals(pos2));
    int resWDL2 = TBProbe::rtbProbeWDL(pos2, ply, wdl2);
    ASSERT(pos.equals(pos2));

    ASSERT_EQUAL(resDTM, resWDL);
    ASSERT_EQUAL(resWDL, resWDL2);
    if (!resDTM)
        return false;

    if (dtm > 0) {
        ASSERT(wdl > 0);
        ASSERT(wdl <= dtm);
        ASSERT(wdl2 > 0);
        ASSERT(wdl2 <= dtm);
    } else if (dtm < 0) {
        ASSERT(wdl < 0);
        ASSERT(wdl >= dtm);
        ASSERT(wdl2 < 0);
        ASSERT(wdl2 >= dtm);
    } else {
        ASSERT_EQUAL(0, wdl);
        ASSERT_EQUAL(0, wdl2);
    }

    score = dtm;
    return true;
}

/** Probe a position and its mirror positions and verify they have the same score. */
static int
probeDTM(const Position& pos, int ply, int& score) {
    std::string fen = TextIO::toFEN(pos);
    int ret = probeCompare(pos, ply, score);
    Position symPos = swapColors(pos);
    int score2;
    int ret2 = probeCompare(symPos, ply, score2);
    std::string fen2 = TextIO::toFEN(symPos);
    ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
    if (ret)
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);

    if (pos.getCastleMask() == 0) {
        symPos = mirrorX(pos);
        fen2 = TextIO::toFEN(symPos);
        ret2 = probeCompare(symPos, ply, score2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);

        symPos = swapColors(mirrorX(pos));
        fen2 = TextIO::toFEN(symPos);
        ret2 = probeCompare(symPos, ply, score2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);
    }

    return ret;
}

void
TBTest::dtmTest() {
    const int mate0 = SearchConst::MATE0;
    int ply = 17;
    const int cacheMB = 16;

    Position pos = TextIO::readFEN("4k3/R7/4K3/8/8/8/8/8 w - - 0 1");
    int score;
    bool res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 2, score);

    TBProbe::initialize("/home/petero/chess/gtb/no_such_dir", cacheMB, "");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    TBProbe::initialize(gtbDefaultPath, cacheMB, rtbDefaultPath);

    // Test castling
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 22, score);

    TBProbe::initialize("", cacheMB, "");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    TBProbe::initialize(gtbDefaultPath, cacheMB, rtbDefaultPath);

    // Test en passant
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - e3 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(0, score);
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(-(mate0 - ply - 48 - 1), score);

    // Test where en passant is only legal move
    pos = TextIO::readFEN("8/8/8/8/Pp6/1K6/3N4/k7 b - a3 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(-(mate0 - ply - 13), score);
    pos = TextIO::readFEN("k1K5/8/8/8/4pP2/4Q3/8/8 b - - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(0, score);
    pos = TextIO::readFEN("k1K5/8/8/8/4pP2/4Q3/8/8 b - f3 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(-(mate0 - ply - 3), score);
}

void
TBTest::kpkTest() {
    auto et = Evaluate::getEvalHashTables();
    Evaluate evaluate(*et);
    const int ply = 1;
    for (int p = A2; p <= H7; p++) {
        for (int wk = 0; wk < 64; wk++) {
            if (wk == p)
                continue;
            for (int bk = 0; bk < 64; bk++) {
                if (bk == wk || bk == p)
                    continue;
                for (int c = 0; c < 2; c++) {
                    Position pos;
                    pos.setPiece(p, Piece::WPAWN);
                    pos.setPiece(wk, Piece::WKING);
                    pos.setPiece(bk, Piece::BKING);
                    pos.setWhiteMove(c == 0);
                    if (MoveGen::canTakeKing(pos))
                        continue;
                    int score;
                    int res = probeDTM(pos, ply, score);
                    ASSERT_EQUAL(true, res);
                    if (pos.getWhiteMove()) {
                        ASSERT(score >= 0);
                    } else {
                        ASSERT(score <= 0);
                    }
                    int evalWhite = evaluate.evalPos(pos);
                    if (!pos.getWhiteMove())
                        evalWhite = -evalWhite;
                    if (score == 0) {
                        ASSERT(evalWhite == 0);
                    } else {
                        ASSERT(evalWhite > 0);
                    }
                }
            }
        }
    }
}

void
TBTest::rtbTest() {
    int ply = 17;
    int wdl;

    Position pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NB3 w - - 0 1");
    bool resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT_EQUAL(true, resWDL);
    ASSERT(SearchConst::isWinScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NB3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT_EQUAL(true, resWDL);
    ASSERT(SearchConst::isLoseScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3BB3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT_EQUAL(true, resWDL);
    ASSERT(SearchConst::isLoseScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NN3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT_EQUAL(true, resWDL);
    ASSERT_EQUAL(0, wdl);

    TBProbe::initialize(gtbDefaultPath, 16, "");
    TBProbe::initialize(gtbDefaultPath, 16, "");
    TBProbe::initialize(gtbDefaultPath, 16, rtbDefaultPath);

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NN3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT_EQUAL(true, resWDL);
    ASSERT_EQUAL(0, wdl);
}

cute::suite
TBTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(dtmTest));
    s.push_back(CUTE(kpkTest));
    s.push_back(CUTE(rtbTest));
    return s;
}
