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

#include "cute.h"


/** Probe a position and its mirror positions and verify they have the same score. */
int
gtbProbe(const Position& pos, int ply, int& score) {
    std::string fen = TextIO::toFEN(pos);
    int ret = TBProbe::gtbProbe(pos, ply, score);
    Position symPos = swapColors(pos);
    int score2;
    int ret2 = TBProbe::gtbProbe(symPos, ply, score2);
    std::string fen2 = TextIO::toFEN(symPos);
    ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
    ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);

    if (pos.getCastleMask() == 0) {
        symPos = mirrorX(pos);
        fen2 = TextIO::toFEN(symPos);
        ret2 = TBProbe::gtbProbe(symPos, ply, score2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);

        symPos = swapColors(mirrorX(pos));
        fen2 = TextIO::toFEN(symPos);
        ret2 = TBProbe::gtbProbe(symPos, ply, score2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);
    }

    return ret;
}

void
TBTest::gtbTest() {
    const int mate0 = SearchConst::MATE0;
    int ply = 17;

    Position pos = TextIO::readFEN("4k3/R7/4K3/8/8/8/8/8 w - - 0 1");
    int score;
    bool res = gtbProbe(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 2, score);

    TBProbe::initialize("/home/petero/chess/gtb/no_such_dir");
    res = gtbProbe(pos, ply, score);
    ASSERT_EQUAL(false, res);
    TBProbe::initialize("/home/petero/chess/gtb");

    // Test castling
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
    res = gtbProbe(pos, ply, score);
    ASSERT_EQUAL(false, res);
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
    res = gtbProbe(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 22, score);

    TBProbe::initialize("");
    res = gtbProbe(pos, ply, score);
    ASSERT_EQUAL(false, res);
    TBProbe::initialize("/home/petero/chess/gtb");

    // Test en passant
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - e3 0 1");
    res = gtbProbe(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(0, score);
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - - 0 1");
    res = gtbProbe(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(-(mate0 - ply - 48 - 1), score);
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
                    int res = gtbProbe(pos, ply, score);
                    ASSERT_EQUAL(true, res);
                    if (pos.getWhiteMove()) {
                        ASSERT(score >= 0);
                    } else {
                        ASSERT(score <= 0);
                    }
                    int eval = evaluate.evalPos(pos);
                    if (!pos.getWhiteMove())
                        eval = -eval;
                    if (score == 0) {
                        ASSERT(eval == 0);
                    } else {
                        ASSERT(eval > 0);
                    }
                }
            }
        }
    }
}

cute::suite
TBTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(gtbTest));
    s.push_back(CUTE(kpkTest));
    return s;
}
