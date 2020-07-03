/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * bookTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "book.hpp"
#include "textio.hpp"
#include "moveGen.hpp"

#include "gtest/gtest.h"

/** Check that move is a legal move in position pos. */
static void
checkValid(Position& pos, const Move& move) {
    ASSERT_FALSE(move.isEmpty());
    MoveList moveList;
    MoveGen::pseudoLegalMoves(pos, moveList);
    MoveGen::removeIllegal(pos, moveList);
    bool contains = false;
    for (int mi = 0; mi < moveList.size; mi++)
        if (moveList[mi] == move) {
            contains = true;
            break;
        }
    ASSERT_TRUE(contains) << "Illegal move: " << TextIO::moveToUCIString(move) << '\n'
                          << TextIO::asciiBoard(pos)
                          << "fen:" << TextIO::toFEN(pos);
}

TEST(BookTest, testGetBookMove) {
    Position pos(TextIO::readFEN(TextIO::startPosFEN));
    Book book(true);
    Move move;
    book.getBookMove(pos, move);
    checkValid(pos, move);
}

TEST(BookTest, testGetAllBookMoves) {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    Book book(true);
    std::string moveListString = book.getAllBookMoves(pos);
    std::vector<std::string> strMoves;
    splitString(moveListString, strMoves);
    EXPECT_GT(strMoves.size(), 1);
    for (size_t i = 0; i < strMoves.size(); i++) {
        std::string strMove = strMoves[i];
        size_t idx = strMove.find_first_of('(');
        ASSERT_NE(idx, strMove.npos);
        ASSERT_GT(idx, 0);
        strMove = strMove.substr(0, idx);
        Move m = TextIO::stringToMove(pos, strMove);
        checkValid(pos, m);
    }
}
