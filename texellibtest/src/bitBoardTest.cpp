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
 * bitBoardTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "bitBoardTest.hpp"
#include "bitBoard.hpp"
#include "textio.hpp"

#include <algorithm>

#include "cute.h"


/** Test of kingAttacks, of class BitBoard. */
static void
testKingAttacks() {
    ASSERT_EQUAL(5, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("g1")]));
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("h1")]));
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("a1")]));
    ASSERT_EQUAL(5, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("a2")]));
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("h8")]));
    ASSERT_EQUAL(5, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("a6")]));
    ASSERT_EQUAL(8, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("b2")]));
}

/** Test of knightAttacks, of class BitBoard. */
static void
testKnightAttacks() {
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("g1")]));
    ASSERT_EQUAL(2, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("a1")]));
    ASSERT_EQUAL(2, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("h1")]));
    ASSERT_EQUAL(4, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("h6")]));
    ASSERT_EQUAL(4, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("b7")]));
    ASSERT_EQUAL(8, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("c6")]));
    ASSERT_EQUAL((1ULL<<TextIO::getSquare("e2")) |
                 (1ULL<<TextIO::getSquare("f3")) |
                 (1ULL<<TextIO::getSquare("h3")),
                 BitBoard::knightAttacks[TextIO::getSquare("g1")]);
}

/** Test of squaresBetween[][], of class BitBoard. */
static void
testSquaresBetween() {
    // Tests that the set of nonzero elements is correct
    for (int sq1 = 0; sq1 < 64; sq1++) {
        for (int sq2 = 0; sq2 < 64; sq2++) {
            int d = BitBoard::getDirection(sq1, sq2);
            if (d == 0) {
                ASSERT_EQUAL(0, BitBoard::squaresBetween[sq1][sq2]);
            } else {
                int dx = Position::getX(sq1) - Position::getX(sq2);
                int dy = Position::getY(sq1) - Position::getY(sq2);
                if (std::abs(dx * dy) == 2) { // Knight direction
                    ASSERT_EQUAL(0, BitBoard::squaresBetween[sq1][sq2]);
                } else {
                    if ((std::abs(dx) > 1) || (std::abs(dy) > 1)) {
                        ASSERT(BitBoard::squaresBetween[sq1][sq2] != 0);
                    } else {
                        ASSERT_EQUAL(0, BitBoard::squaresBetween[sq1][sq2]);
                    }
                }
            }
        }
    }

    ASSERT_EQUAL(0x0040201008040200ULL, BitBoard::squaresBetween[0][63]);
    ASSERT_EQUAL(0x000000001C000000ULL, BitBoard::squaresBetween[TextIO::getSquare("b4")][TextIO::getSquare("f4")]);
}

/**
 * If there is a piece type that can move from "from" to "to", return the
 * corresponding direction, 8*dy+dx.
 */
static int computeDirection(int from, int to) {
    int dx = Position::getX(to) - Position::getX(from);
    int dy = Position::getY(to) - Position::getY(from);
    if (dx == 0) {                   // Vertical rook direction
        if (dy == 0) return 0;
        return (dy > 0) ? 8 : -8;
    }
    if (dy == 0)                    // Horizontal rook direction
        return (dx > 0) ? 1 : -1;
    if (std::abs(dx) == std::abs(dy)) // Bishop direction
        return ((dy > 0) ? 8 : -8) + (dx > 0 ? 1 : -1);
    if (std::abs(dx * dy) == 2)     // Knight direction
        return dy * 8 + dx;
    return 0;
}

static void
testGetDirection() {
    for (int from = 0; from < 64; from++)
        for (int to = 0; to < 64; to++)
            ASSERT_EQUAL(computeDirection(from, to), BitBoard::getDirection(from, to));
}

static int computeDistance(int from, int to) {
    int dx = Position::getX(to) - Position::getX(from);
    int dy = Position::getY(to) - Position::getY(from);
    return std::max(std::abs(dx), std::abs(dy));
}

static void
testGetDistance() {
    for (int from = 0; from < 64; from++)
        for (int to = 0; to < 64; to++)
            ASSERT_EQUAL(computeDistance(from, to), BitBoard::getDistance(from, to));
}

static void
testTrailingZeros() {
    for (int i = 0; i < 64; i++) {
        U64 mask = 1ULL << i;
        ASSERT_EQUAL(i, BitBoard::numberOfTrailingZeros(mask));
    }
}


cute::suite
BitBoardTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testKingAttacks));
    s.push_back(CUTE(testKnightAttacks));
    s.push_back(CUTE(testSquaresBetween));
    s.push_back(CUTE(testGetDirection));
    s.push_back(CUTE(testGetDistance));
    s.push_back(CUTE(testTrailingZeros));
    return s;
}
