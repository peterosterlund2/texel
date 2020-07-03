/*
    Texel - A UCI chess engine.
    Copyright (C) 2012,2015  Peter Österlund, peterosterlund2@gmail.com

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
 * historyTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "history.hpp"
#include "textio.hpp"

#include "gtest/gtest.h"


TEST(HistoryTest, testGetHistScore) {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    History hs;
    Move m1 = TextIO::stringToMove(pos, "e4");
    Move m2 = TextIO::stringToMove(pos, "d4");
    ASSERT_EQ(0, hs.getHistScore(pos, m1));

    hs.addSuccess(pos, m1, 1);
    ASSERT_EQ(1 * 49 / 1, hs.getHistScore(pos, m1));
    ASSERT_EQ(0, hs.getHistScore(pos, m2));

    hs.addSuccess(pos, m1, 1);
    ASSERT_EQ(1 * 49 / 1, hs.getHistScore(pos, m1));
    ASSERT_EQ(0, hs.getHistScore(pos, m2));

    hs.addFail(pos, m1, 1);
    ASSERT_EQ((2 * 49 +3/2) / 3, hs.getHistScore(pos, m1));
    ASSERT_EQ(0, hs.getHistScore(pos, m2));

    hs.addFail(pos, m1, 1);
    ASSERT_EQ(2 * 49 / 4, hs.getHistScore(pos, m1));
    ASSERT_EQ(0, hs.getHistScore(pos, m2));

    hs.addSuccess(pos, m2, 1);
    ASSERT_EQ(2 * 49 / 4, hs.getHistScore(pos, m1));
    ASSERT_EQ(1 * 49 / 1, hs.getHistScore(pos, m2));
}
