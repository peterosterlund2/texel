/*
    Texel - A UCI chess engine.
    Copyright (C) 2022  Peter Österlund, peterosterlund2@gmail.com

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
 * nnutilTest.hpp
 *
 *  Created on: Jul 17, 2022
 *      Author: petero
 */

#include "nnutilTest.hpp"
#include "textio.hpp"
#include "posutil.hpp"
#include "nnutil.hpp"

#include "gtest/gtest.h"

TEST(NNUtilTest, testRecord) {
    NNUtilTest::testRecord();
}

void
NNUtilTest::testRecord() {
    auto test = [](const std::string& fen, int score = 1234) {
        NNUtil::Record r;
        Position pos = TextIO::readFEN(fen);
        bool wtm = pos.isWhiteMove();
        NNUtil::posToRecord(pos, score, r);
        Position pos2;
        int score2;
        NNUtil::recordToPos(r, pos2, score2);
        if (!wtm) {
            score2 = -score2;
            pos2 = PosUtil::swapColors(pos2);
        }
        std::string fen2 = TextIO::toFEN(pos2);

        ASSERT_EQ(fen, fen2);
        ASSERT_EQ(score2, score);
    };

    test("r1bq1rk1/pp1ppp1p/6p1/3P4/1pP1P1n1/5N2/P4PPP/R1BQKB1R w - - 0 1", 525);
    test("r1b1kb1r/ppp2ppp/2n1pq2/3p4/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq - 18 1", -12345);
    test("r1b1kb1r/ppp2ppp/2n1pq2/3p4/3P4/5N2/PPP2PPP/RNBQKB1R w K - 19 1", -12345);
    test("r1b1kb1r/ppp2ppp/2n1pq2/3p4/3P4/5N2/PPP2PPP/RNBQKB1R w Q - 20 1", -12346);
    test("r1b1kb1r/ppp2ppp/2n1pq2/3p4/3P4/5N2/PPP2PPP/RNBQKB1R w k - 21 1", 12347);
    test("r1b1kb1r/ppp2ppp/2n1pq2/3p4/3P4/5N2/PPP2PPP/RNBQKB1R w q - 22 1", 12348);
    test("4k3/2q1nnn1/8/8/8/8/1QQQ4/4K3 w - - 0 1", 0);

    test("r1b1kb1r/ppp2ppp/2n1pq2/3p4/3P4/5N2/PPP2PPP/RNBQKB1R b KQk - 18 1", 321);
}
