/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2016  Peter Österlund, peterosterlund2@gmail.com

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
#include "searchTest.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "evaluate.hpp"
#include "textio.hpp"
#include "tbprobe.hpp"
#include "constants.hpp"
#include "posutil.hpp"

#include "syzygy/rtb-probe.hpp"

#include "gtest/gtest.h"

#define ASSERT_EQt(v1, v2) \
    do { \
        EXPECT_EQ(v1, v2); \
        if (!(v1 == v2)) \
            throw std::runtime_error("Aborting test"); \
    } while(false)


void
TBTest::initTB(const std::string& gtbPath, int cacheMB,
               const std::string& rtbPath) {
    TBProbe::initialize(gtbPath, cacheMB, rtbPath);
}

/** Copy selected TB files to a temporary directory. */
static void setupTBFiles(const std::vector<std::string>& tbFiles) {
#ifdef _WIN32
    throw std::runtime_error("Test not implemented for Windows");
#endif
    auto system = [](const std::string& cmd) {
        int ret = ::system(cmd.c_str());
        ASSERT_EQ(0, ret);
    };
    std::string tmpDir = "/tmp/tbtest";
    system("mkdir -p " + tmpDir);
    system("rm -f " + tmpDir + "/*");
    for (const std::string& file : tbFiles) {
        if (endsWith(file, ".gtb.cp4")) {
            system("ln -s /home/petero/chess/gtb/" + file + " " + tmpDir + "/" + file);
        } else if (endsWith(file, ".rtbw")) {
            system("ln -s /home/petero/chess/rtb/wdl/" + file + " " + tmpDir + "/" + file);
        } else if (endsWith(file, ".rtbz")) {
            system("ln -s /home/petero/chess/rtb/dtz/" + file + " " + tmpDir + "/" + file);
        } else {
            throw "Unsupported file type";
        }
    }
    TBTest::initTB("", 0, "");
    TBTest::initTB(tmpDir, gtbDefaultCacheMB, tmpDir);
}

/** Probe both DTM and WDL, check consistency and return DTM value. */
static int probeCompare(const Position& pos, int ply, int& score) {
    int dtm, wdl, wdl2, dtz;
    TranspositionTable::TTEntry ent;
    Position pos2(pos);
    int resDTM = TBProbe::gtbProbeDTM(pos2, ply, dtm);
    ASSERT_EQt(pos, pos2);
    int resWDL = TBProbe::gtbProbeWDL(pos2, ply, wdl);
    ASSERT_EQt(pos, pos2);
    int resWDL2 = TBProbe::rtbProbeWDL(pos2, ply, wdl2, ent);
    ASSERT_EQt(pos, pos2);
    int resDTZ = TBProbe::rtbProbeDTZ(pos2, ply, dtz, ent);
    ASSERT_EQt(pos, pos2);

    ASSERT_EQt(resDTM, resWDL);
    ASSERT_EQt(resWDL, resWDL2);
    ASSERT_EQt(resWDL2, resDTZ);

    if (!resDTM)
        return false;

    if (dtm > 0) {
        EXPECT_GT(wdl, 0);
        EXPECT_LE(wdl, dtm);
        EXPECT_GT(wdl2, 0);
        EXPECT_LE(wdl2, dtm);
        EXPECT_GT(dtz, 0);
        EXPECT_LE(dtz, dtm);
        EXPECT_GE(dtz, wdl2);
    } else if (dtm < 0) {
        EXPECT_LT(wdl, 0);
        EXPECT_GE(wdl, dtm);
        EXPECT_LT(wdl2, 0);
        EXPECT_GE(wdl2, dtm);
        EXPECT_LT(dtz, 0);
        EXPECT_GE(dtz, dtm);
        EXPECT_LE(dtz, wdl2);
    } else {
        ASSERT_EQt(0, wdl);
        ASSERT_EQt(0, wdl2);
        ASSERT_EQt(0, dtz);
    }

    score = dtm;
    return true;
}

/** Probe a position and its mirror positions and verify they have the same score. */
static int
probeDTM(const Position& pos, int ply, int& score) {
    std::string fen = TextIO::toFEN(pos);
    SCOPED_TRACE(fen);
    int ret = probeCompare(pos, ply, score);
    Position symPos = PosUtil::swapColors(pos);
    int score2;
    int ret2 = probeCompare(symPos, ply, score2);
    std::string fen2 = TextIO::toFEN(symPos);
    EXPECT_EQ(ret, ret2) << (fen + " == " + fen2);
    if (ret) {
        EXPECT_EQ(score, score2) << (fen + " == " + fen2);
    }

    if (pos.getCastleMask() == 0) {
        symPos = PosUtil::mirrorX(pos);
        fen2 = TextIO::toFEN(symPos);
        ret2 = probeCompare(symPos, ply, score2);
        EXPECT_EQ(ret, ret2) << (fen + " == " + fen2);
        if (ret) {
            EXPECT_EQ(score, score2) << (fen + " == " + fen2);
        }

        symPos = PosUtil::swapColors(PosUtil::mirrorX(pos));
        fen2 = TextIO::toFEN(symPos);
        ret2 = probeCompare(symPos, ply, score2);
        EXPECT_EQ(ret, ret2) << (fen + " == " + fen2);
        if (ret) {
            EXPECT_EQ(score, score2) << (fen + " == " + fen2);
        }
    }

    return ret;
}

TEST(TBTest, dtmTest) {
    TBTest::dtmTest();
}

void
TBTest::dtmTest() {
    const int mate0 = SearchConst::MATE0;
    int ply = 17;
    const int cacheMB = gtbDefaultCacheMB;

    Position pos = TextIO::readFEN("4k3/R7/4K3/8/8/8/8/8 w - - 0 1");
    int score;
    bool res = probeDTM(pos, ply, score);
    EXPECT_EQ(true, res);
    EXPECT_EQ(mate0 - ply - 2, score);

    initTB("/home/petero/chess/gtb/no_such_dir", cacheMB, "");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(false, res);
    initTB("/no/such/path;" + gtbDefaultPath + ";/test/;", cacheMB,
           "//dfasf/:" + rtbDefaultPath + ":a:b:");

    // Test castling
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(false, res);
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(true, res);
    EXPECT_EQ(mate0 - ply - 22, score);

    initTB("", cacheMB, "");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(false, res);
    initTB(gtbDefaultPath, cacheMB, rtbDefaultPath);

    // Test en passant
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - e3 0 1");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(true, res);
    EXPECT_EQ(0, score);
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - - 0 1");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(true, res);
    EXPECT_EQ(-(mate0 - ply - 48 - 1), score);

    // Test where en passant is only legal move
    pos = TextIO::readFEN("8/8/8/8/Pp6/1K6/3N4/k7 b - a3 0 1");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(true, res);
    EXPECT_EQ(-(mate0 - ply - 13), score);
    pos = TextIO::readFEN("k1K5/8/8/8/4pP2/4Q3/8/8 b - - 0 1");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(true, res);
    EXPECT_EQ(0, score);
    pos = TextIO::readFEN("k1K5/8/8/8/4pP2/4Q3/8/8 b - f3 0 1");
    res = probeDTM(pos, ply, score);
    EXPECT_EQ(true, res);
    EXPECT_EQ(-(mate0 - ply - 3), score);
}

TEST(TBTest, kpkTest) {
    TBTest::kpkTest();
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
                    pos.setPiece(Square(p), Piece::WPAWN);
                    pos.setPiece(Square(wk), Piece::WKING);
                    pos.setPiece(Square(bk), Piece::BKING);
                    pos.setWhiteMove(c == 0);
                    if (MoveGen::canTakeKing(pos))
                        continue;
                    int score;
                    int res = probeDTM(pos, ply, score);
                    ASSERT_EQ(true, res);
                    if (pos.isWhiteMove()) {
                        ASSERT_GE(score, 0);
                    } else {
                        ASSERT_LE(score, 0);
                    }
                    evaluate.connectPosition(pos);
                    int evalWhite = evaluate.evalPos();
                    if (!pos.isWhiteMove())
                        evalWhite = -evalWhite;
                    const int tempoBonusEG = 3;
                    if (score == 0) {
                        ASSERT_LE(abs(evalWhite), tempoBonusEG);
                    } else {
                        ASSERT_GT(evalWhite, tempoBonusEG);
                    }
                }
            }
        }
    }
}

TEST(TBTest, rtbTest) {
    TBTest::rtbTest();
}

void
TBTest::rtbTest() {
    int ply = 17;
    int wdl, dtm, dtz;
    TranspositionTable::TTEntry ent;

    Position pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NB3 w - - 0 1");
    bool resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_EQ(true, resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NB3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_EQ(true, resWDL);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3BB3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_EQ(true, resWDL);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NN3 b - - 0 1");
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_EQ(true, resWDL);
    EXPECT_EQ(0, wdl);
    EXPECT_EQ(0, ent.getEvalScore());

    initTB(gtbDefaultPath, 16, "");
    initTB(gtbDefaultPath, 16, "");
    initTB(gtbDefaultPath, 16, rtbDefaultPath);

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NN3 b - - 0 1");
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_EQ(true, resWDL);
    EXPECT_EQ(0, wdl);
    EXPECT_EQ(0, ent.getEvalScore());

    // Check that DTZ probes do not give too good (incorrect) bounds
    pos = TextIO::readFEN("8/8/8/8/7B/8/3k4/K2B4 w - - 0 1");
    bool resDTM = TBProbe::gtbProbeDTM(pos, ply, dtm);
    EXPECT_TRUE(resDTM);
    bool resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent, false);
    EXPECT_FALSE(resDTZ);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isWinScore(dtz)) << "dtz:" << dtz;
    EXPECT_LE(dtz, dtm);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    probeDTM(pos, ply, dtm);
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl));
    EXPECT_TRUE(SearchConst::isLoseScore(dtz));
    EXPECT_LE(dtz, wdl);

    // Tests where DTZ is close to 100
    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1"); // DTZ = 100
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl));
    EXPECT_TRUE(SearchConst::isLoseScore(dtz));
    EXPECT_LE(dtz, wdl);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 1 1"); // DTZ = 100
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl)); // WDL probes assume half-move clock is 0
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_EQ(0, dtz);
    EXPECT_EQ(-1, ent.getEvalScore());


    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 0 1"); // DTZ = 30
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isWinScore(dtz));
    EXPECT_GE(dtz, wdl);

    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 69 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isWinScore(dtz));

    // DTZ = 30, DTZ + hmc = 100. RTB does not know the answer
    // because the TB class has maxDTZ < 100
    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 70 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl)); // WDL probes assume hmc is 0
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(!resDTZ);

    // DTZ = 2, DTZ + hmc = 100. RTB does not know the answer
    // because the TB class has maxDTZ < 100
    pos = TextIO::readFEN("6kq/8/4N3/7Q/8/8/8/1K6 w - - 98 15");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl)); // WDL probes assume hmc is 0
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(!resDTZ);

    // DTZ + hmc > 100, draw
    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 71 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl)); // WDL probes assume hmc is 0
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_EQ(0, dtz);
    EXPECT_EQ(1, ent.getEvalScore());


    pos = TextIO::readFEN("8/1R6/4q3/6k1/8/8/6K1/1Q6 b - - 0 1"); // DTZ = 46
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl));
    EXPECT_TRUE(SearchConst::isLoseScore(dtz));
    EXPECT_LE(dtz, wdl);

    // DTZ + hmc = 100, but RTB still knows the answer because maxDTZ = 100
    pos = TextIO::readFEN("8/1R6/4q3/6k1/8/8/6K1/1Q6 b - - 54 1"); // DTZ = 46
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isLoseScore(dtz));

    // DTZ + hmc = 101, draw
    pos = TextIO::readFEN("8/1R6/4q3/6k1/8/8/6K1/1Q6 b - - 55 1"); // DTZ = 46
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isLoseScore(wdl));
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_EQ(0, dtz);
    EXPECT_EQ(-1, ent.getEvalScore());


    pos = TextIO::readFEN("1R5Q/8/6k1/8/8/8/8/K1q5 w - - 0 1"); // DTZ == 101
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_EQ(0, wdl);
    EXPECT_EQ(1000, ent.getEvalScore());
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_EQ(0, dtz);
    EXPECT_EQ(1000, ent.getEvalScore());

    pos = TextIO::readFEN("1R5Q/8/6k1/8/8/8/2q5/K7 b - - 0 1"); // DTZ == -102
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_EQ(0, wdl);
    EXPECT_EQ(-1000, ent.getEvalScore());
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_EQ(0, dtz);
    EXPECT_EQ(-1000, ent.getEvalScore());

    pos = TextIO::readFEN("8/8/8/pk1K4/8/3N1N2/8/8 w - - 0 1"); // DTZ == 22
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_TRUE(SearchConst::isWinScore(dtz));

    pos = TextIO::readFEN("8/8/8/pk1K4/8/3N1N2/8/8 w - - 85 1"); // DTZ == 22
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    EXPECT_TRUE(resWDL);
    EXPECT_TRUE(SearchConst::isWinScore(wdl)); // WDL probes ignore halfMoveClock
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    EXPECT_TRUE(resDTZ);
    EXPECT_EQ(0, dtz);
    EXPECT_EQ(7, ent.getEvalScore());

    pos = TextIO::readFEN("6k1/8/5Q2/6K1/6Pp/8/8/7Q b - g3 0 1");
    int success;
    dtz = Syzygy::probe_dtz(pos, &success, true);
    EXPECT_EQ(1, success);
    EXPECT_EQ(-2, dtz);

    pos = TextIO::readFEN("3K4/8/3k4/8/4p3/4B3/5P2/8 w - - 0 5");
    dtz = Syzygy::probe_dtz(pos, &success, true);
    EXPECT_EQ(1, success);
    EXPECT_EQ(15, dtz);
}

TEST(TBTest, tbTest) {
    TBTest::tbTest();
}

void
TBTest::tbTest() {
    int ply = 29;
    const int mate0 = SearchConst::MATE0;
    TranspositionTable& tt = SearchTest::tt;

    // DTM > 100 when ignoring 50-move rule, RTB probes must be used when available
    Position pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    TranspositionTable::TTEntry ent;
    bool res = TBProbe::tbProbe(pos, ply, -10, 10, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_LE, ent.getType());
    EXPECT_LT(ent.getScore(ply), 0);

    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_LE, ent.getType());
    EXPECT_LT(ent.getScore(ply), 0);

    initTB(gtbDefaultPath, gtbDefaultCacheMB, ""); // Disable syzygy tables
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_LE, ent.getType());
    EXPECT_EQ(0, ent.getScore(ply));
    EXPECT_EQ(-14, ent.getEvalScore());

    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);

    // Half-move clock small, DTM mate wins
    pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_EXACT, ent.getType());
    EXPECT_EQ(-(mate0 - ply - 23), ent.getScore(ply));
    res = TBProbe::tbProbe(pos, ply, -10, 10, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_LE, ent.getType());
    EXPECT_TRUE(SearchConst::isLoseScore(ent.getScore(ply)));

    // Half-move clock large, must follow DTZ path to win
    pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 90 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_LE, ent.getType());
    EXPECT_TRUE(SearchConst::isLoseScore(ent.getScore(ply)));
    EXPECT_GT(ent.getScore(ply), -(mate0 - ply - 23));
    res = TBProbe::tbProbe(pos, ply, -10, 10, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_LE, ent.getType());
    EXPECT_TRUE(SearchConst::isLoseScore(ent.getScore(ply)));

    // Mate in one, half-move clock small
    pos = TextIO::readFEN("8/8/4B3/8/kBK5/8/8/8 w - - 0 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_EXACT, ent.getType());
    EXPECT_EQ(mate0 - 2 - ply, ent.getScore(ply));

    // Mate in one, half-move clock large
    pos = TextIO::readFEN("8/8/4B3/8/kBK5/8/8/8 w - - 99 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_EXACT, ent.getType());
    EXPECT_EQ(mate0 - 2 - ply, ent.getScore(ply));
    // Same position, no GTB tables available
    initTB("/no/such/dir", gtbDefaultCacheMB, rtbDefaultPath);
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(!res || ent.getScore(ply) != 0);
    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);

    pos = TextIO::readFEN("8/8/3pk3/8/8/3NK3/3N4/8 w - - 70 1"); // DTZ = 38
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_EXACT, ent.getType());
    EXPECT_EQ(0, ent.getScore(ply));
    EXPECT_EQ(8, ent.getEvalScore());
    ent.clear();
    res = TBProbe::tbProbe(pos, ply, -15, 15, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_EXACT, ent.getType());
    EXPECT_EQ(0, ent.getScore(ply));
    EXPECT_EQ(8, ent.getEvalScore());

    pos = TextIO::readFEN("8/8/4k1N1/p7/8/8/3N2K1/8 w - - 0 1"); // DTZ = 116
    ent.clear();
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_EXACT, ent.getType());
    EXPECT_EQ(0, ent.getScore(ply));
    EXPECT_EQ(93, ent.getEvalScore());
    ent.clear();
    res = TBProbe::tbProbe(pos, ply, -15, 15, tt, ent);
    EXPECT_TRUE(res);
    EXPECT_EQ(TType::T_EXACT, ent.getType());
    EXPECT_EQ(0, ent.getScore(ply));
    EXPECT_EQ(1000, ent.getEvalScore());

    {
        pos = TextIO::readFEN("2R5/4k3/Q7/8/8/8/8/K7 w - - 98 1");
        std::shared_ptr<Search> sc = SearchTest::getSearch(pos);
        Move m = SearchTest::idSearch(*sc, 4, 0);
        EXPECT_EQ("a6e6", TextIO::moveToUCIString(m));
    }
    {
        pos = TextIO::readFEN("2R5/4k3/Q7/8/8/8/8/K7 w - - 97 1");
        std::shared_ptr<Search> sc = SearchTest::getSearch(pos);
        Move m = SearchTest::idSearch(*sc, 4, 1);
        EXPECT_EQ("c8c7", TextIO::moveToUCIString(m));
    }
}

TEST(TBTest, testTbSearch) {
    TBTest::testTbSearch();
}

void
TBTest::testTbSearch() {
    initTB("", gtbDefaultCacheMB, rtbDefaultPath); // Disable GTB TBs
    const int mate0 = SearchConst::MATE0;
    Position pos;

    {
        pos = TextIO::readFEN("8/8/8/8/7B/8/3k4/K2B4 w - - 0 1");
        std::shared_ptr<Search> sc = SearchTest::getSearch(pos);
        Move m = SearchTest::idSearch(*sc, 1, 0);
        const int mate19 = mate0 - (19 * 2 + 1);
        EXPECT_GE(m.score(), mate19);
    }
    {
        pos = TextIO::readFEN("8/8/8/8/7B/1B6/3k4/K7 b - - 1 1");
        std::shared_ptr<Search> sc = SearchTest::getSearch(pos);
        Move m = SearchTest::idSearch(*sc, 1, 0);
        const int mated18 = -(mate0 - (18 * 2 + 2));
        EXPECT_LE(m.score(), -600); // DTZ has info for wrong side, so not probed
        m = SearchTest::idSearch(*sc, 2, 0);
        EXPECT_LE(m.score(), mated18); // DTZ probed on next ply, where side is correct
    }
    {
        pos = TextIO::readFEN("8/8/8/8/7B/1B2Q3/3k4/K7 b - - 1 1");
        std::shared_ptr<Search> sc = SearchTest::getSearch(pos);
        Move m = SearchTest::idSearch(*sc, 1, 0);
        const int mated20 = -(mate0 - (20 * 2 + 2));
        EXPECT_EQ(m.score(), mated20); // DTZ has info for wrong side, so not probed, but WDL is probed
    }

    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);
}

static void getLegalMoves(Position& pos, MoveList& legalMoves) {
    legalMoves.clear();
    MoveGen::pseudoLegalMoves(pos, legalMoves);
    MoveGen::removeIllegal(pos, legalMoves);
}

static void compareMoves(const std::vector<std::string>& strMoves,
                         const std::vector<Move>& moves) {
    EXPECT_EQ(strMoves.size(), moves.size());
    for (const Move& m : moves)
        EXPECT_TRUE(contains(strMoves, TextIO::moveToUCIString(m)));
}

TEST(TBTest, testMissingTables) {
    TBTest::testMissingTables();
}

void
TBTest::testMissingTables() {
    TranspositionTable& tt = SearchTest::tt;
    for (int loop = 0; loop < 2; loop++) {
        bool gtb = loop == 1;
        // No progress move in TBs, must search all zeroing moves
        if (gtb)
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4"});
        else
            setupTBFiles(std::vector<std::string>{"KPvK.rtbw", "KPvK.rtbz"});
        Position pos = TextIO::readFEN("8/4P3/8/8/2k1K3/8/8/8 w - - 0 1");
        MoveList legalMoves;
        getLegalMoves(pos, legalMoves);
        std::vector<Move> movesToSearch;
        bool res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
        EXPECT_EQ(true, res);
        if (gtb)
            compareMoves(std::vector<std::string>{"e7e8q", "e7e8r", "e7e8b", "e7e8n"}, movesToSearch);
        {
            std::shared_ptr<Search> sc = SearchTest::getSearch(pos);
            Move m = SearchTest::idSearch(*sc, 4, 3);
            EXPECT_EQ("e7e8q", TextIO::moveToUCIString(m));
        }

        // Progress (queen promotion) in TB, no need to limit moves to search
        if (gtb)
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4", "kqk.gtb.cp4"});
        else
            setupTBFiles(std::vector<std::string>{"KPvK.rtbw", "KPvK.rtbz", "KQvK.rtbw", "KQvK.rtbz"});
        pos = TextIO::readFEN("8/4P3/8/8/2k1K3/8/8/8 w - - 0 1");
        getLegalMoves(pos, legalMoves);
        movesToSearch.clear();
        res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
        EXPECT_EQ(false, res);

        // No progress move in TBs, must search all unknown zeroing moves
        if (gtb)
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4", "krk.gtb.cp4"});
        else
            setupTBFiles(std::vector<std::string>{"KPvK.rtbw", "KPvK.rtbz", "KRvK.rtbw", "KRvK.rtbz"});
        pos = TextIO::readFEN("8/4P3/8/8/2k1K3/8/8/8 w - - 0 1");
        getLegalMoves(pos, legalMoves);
        movesToSearch.clear();
        res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
        if (gtb) {
            EXPECT_EQ(true, res);
            compareMoves(std::vector<std::string>{"e7e8q", "e7e8b", "e7e8n"}, movesToSearch);
        } else {
            EXPECT_EQ(false, res); // Rook promotion is an improvement when using only DTZ TBs
        }

        // Non-zeroing move makes progress, search all legal moves
        if (gtb) {
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4"});
            pos = TextIO::readFEN("8/4P3/8/8/1k2K3/8/8/8 w - - 0 1");
            getLegalMoves(pos, legalMoves);
            movesToSearch.clear();
            res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
            EXPECT_EQ(false, res);
        }
    }

    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);
}

TEST(TBTest, testMaxSubMate) {
    TBTest::testMaxSubMate();
}

void
TBTest::testMaxSubMate() {
    using MI = MatId;
    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);
    Position pos = TextIO::readFEN("3qk3/8/8/8/8/8/8/3QK3 w - - 0 1");
    int maxSub = TBProbe::getMaxSubMate(pos);
    EXPECT_EQ(TBProbe::getMaxDTZ(MI::WQ), maxSub);
}
