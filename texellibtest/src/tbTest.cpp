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

static void initTB(const std::string& gtbPath, int cacheMB,
                   const std::string& rtbPath) {
    TBProbe::initializeGTB(gtbPath, cacheMB);
    TBProbe::initializeRTB(rtbPath);
}

/** Probe both DTM and WDL, check consistency and return DTM value. */
static int probeCompare(const Position& pos, int ply, int& score) {
    int dtm, wdl, wdl2, dtz;
    Position pos2(pos);
    int resDTM = TBProbe::gtbProbeDTM(pos2, ply, dtm);
    ASSERT(pos.equals(pos2));
    int resWDL = TBProbe::gtbProbeWDL(pos2, ply, wdl);
    ASSERT(pos.equals(pos2));
    int resWDL2 = TBProbe::rtbProbeWDL(pos2, ply, wdl2);
    ASSERT(pos.equals(pos2));
    int resDTZ = TBProbe::rtbProbeDTZ(pos2, ply, dtz);
    ASSERT(pos.equals(pos2));

    ASSERT_EQUAL(resDTM, resWDL);
    ASSERT_EQUAL(resWDL, resWDL2);
    ASSERT_EQUAL(resWDL2, resDTZ);

    if (!resDTM)
        return false;

    if (dtm > 0) {
        ASSERT(wdl > 0);
        ASSERT(wdl <= dtm);
        ASSERT(wdl2 > 0);
        ASSERT(wdl2 <= dtm);
        ASSERT(dtz > 0);
        ASSERT(dtz <= dtm);
        ASSERT(dtz >= wdl2);
    } else if (dtm < 0) {
        ASSERT(wdl < 0);
        ASSERT(wdl >= dtm);
        ASSERT(wdl2 < 0);
        ASSERT(wdl2 >= dtm);
        ASSERT(dtz < 0);
        ASSERT(dtz >= dtm);
        ASSERT(dtz <= wdl2);
    } else {
        ASSERT_EQUAL(0, wdl);
        ASSERT_EQUAL(0, wdl2);
        ASSERT_EQUAL(0, dtz);
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
        if (ret)
            ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);

        symPos = swapColors(mirrorX(pos));
        fen2 = TextIO::toFEN(symPos);
        ret2 = probeCompare(symPos, ply, score2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
        if (ret)
            ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);
    }

    return ret;
}

void
TBTest::dtmTest() {
    const int mate0 = SearchConst::MATE0;
    int ply = 17;
    const int cacheMB = gtbDefaultCacheMB;

    Position pos = TextIO::readFEN("4k3/R7/4K3/8/8/8/8/8 w - - 0 1");
    int score;
    bool res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 2, score);

    initTB("/home/petero/chess/gtb/no_such_dir", cacheMB, "");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    initTB("/no/such/path;" + gtbDefaultPath + ";/test/;", cacheMB,
           "//dfasf/:" + rtbDefaultPath + ":a:b:");

    // Test castling
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 22, score);

    initTB("", cacheMB, "");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    initTB(gtbDefaultPath, cacheMB, rtbDefaultPath);

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
    int wdl, dtm, dtz;

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

    initTB(gtbDefaultPath, 16, "");
    initTB(gtbDefaultPath, 16, "");
    initTB(gtbDefaultPath, 16, rtbDefaultPath);

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NN3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT_EQUAL(true, resWDL);
    ASSERT_EQUAL(0, wdl);

    // Check that DTZ probes do not give too good (incorrect) bounds
    pos = TextIO::readFEN("8/8/8/8/7B/8/3k4/K2B4 w - - 0 1");
    bool resDTM = TBProbe::gtbProbeDTM(pos, ply, dtm);
    ASSERT(resDTM);
    bool resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isWinScore(dtz));
    ASSERT(dtz <= dtm);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    probeDTM(pos, ply, dtm);
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isLoseScore(wdl));
    ASSERT(SearchConst::isLoseScore(dtz));
    ASSERT(dtz <= wdl);

    // Tests where DTZ is close to 100
    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isLoseScore(wdl));
    ASSERT(SearchConst::isLoseScore(dtz));
    ASSERT(dtz <= wdl);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 1 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isLoseScore(wdl)); // WDL probes assume half-move clock is 0
    ASSERT_EQUAL(0, dtz);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/8/8/8/K1q5 w - - 0 1"); // DTZ == 101
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, wdl);
    ASSERT_EQUAL(0, dtz);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/8/8/2q5/K7 b - - 0 1"); // DTZ == -102
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, wdl);
    ASSERT_EQUAL(0, dtz);
}

/** Test TBProbe::tbProbe() function. */
void
TBTest::tbTest() {
    int ply = 29;
    const int mate0 = SearchConst::MATE0;

    // DTM > 100 when ignoring 50-move rule, RTB probes must be used when available
    Position pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    TranspositionTable::TTEntry ent;
    bool res = TBProbe::tbProbe(pos, ply, -10, 10, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(ent.getScore(ply) < 0);

    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(ent.getScore(ply) < 0);

    initTB(gtbDefaultPath, gtbDefaultCacheMB, ""); // Disable syzygy tables
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT(ent.getScore(ply) < 0);
    ASSERT(ent.getScore(ply) >= -(mate0 - ply - 100));

    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);

    // Half-move clock small, DTM mate wins
    pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(-(mate0 - ply - 23), ent.getScore(ply));
    res = TBProbe::tbProbe(pos, ply, -10, 10, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(SearchConst::isLoseScore(ent.getScore(ply)));

    // Half-move clock large, must follow DTZ path to win
    pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 90 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(SearchConst::isLoseScore(ent.getScore(ply)));
    ASSERT(ent.getScore(ply) > -(mate0 - ply - 23));
    res = TBProbe::tbProbe(pos, ply, -10, 10, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(SearchConst::isLoseScore(ent.getScore(ply)));

    // Mate in one, half-move clock small
    pos = TextIO::readFEN("8/8/4B3/8/kBK5/8/8/8 w - - 0 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(mate0 - 2 - ply, ent.getScore(ply));

    // Mate in one, half-move clock large
    pos = TextIO::readFEN("8/8/4B3/8/kBK5/8/8/8 w - - 99 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(mate0 - 2 - ply, ent.getScore(ply));
    // Same position, no GTB tables available
    initTB("/no/such/dir", gtbDefaultCacheMB, rtbDefaultPath);
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, ent);
    ASSERT(!res || ent.getScore(ply) != 0);
    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);
}

cute::suite
TBTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(dtmTest));
    s.push_back(CUTE(kpkTest));
    s.push_back(CUTE(rtbTest));
    s.push_back(CUTE(tbTest));
    return s;
}
