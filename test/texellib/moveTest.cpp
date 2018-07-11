/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * moveTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "position.hpp"
#include "move.hpp"
#include "piece.hpp"

#include "gtest/gtest.h"


TEST(MoveTest, testMoveConstructor) {
    Square f = Square(4, 1);
    Square t = Square(4, 3);
    int p = Piece::WROOK;
    Move move(f, t, p);
    EXPECT_EQ(move.from(), f);
    EXPECT_EQ(move.to(), t);
    EXPECT_EQ(move.promoteTo(), p);
}

TEST(MoveTest, testEquals) {
    Move m1(Square(0, 6), Square(1, 7), Piece::WROOK);
    Move m2(Square(0, 6), Square(0, 7), Piece::WROOK);
    Move m3(Square(1, 6), Square(1, 7), Piece::WROOK);
    Move m4(Square(0, 6), Square(1, 7), Piece::WKNIGHT);
    Move m5(Square(0, 6), Square(1, 7), Piece::WROOK);
    EXPECT_FALSE(m1 == m2);
    EXPECT_FALSE(m1 == m3);
    EXPECT_FALSE(m1 == m4);
    EXPECT_EQ(m1, m5);
}
