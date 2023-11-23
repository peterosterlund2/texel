/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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

#include "bitBoard.hpp"
#include "textio.hpp"
#include "random.hpp"

#include <algorithm>

#include "gtest/gtest.h"

TEST(BitBoardTest, testKingAttacks) {
    EXPECT_EQ(5, BitBoard::bitCount(BitBoard::kingAttacks(TextIO::getSquare("g1"))));
    EXPECT_EQ(3, BitBoard::bitCount(BitBoard::kingAttacks(TextIO::getSquare("h1"))));
    EXPECT_EQ(3, BitBoard::bitCount(BitBoard::kingAttacks(TextIO::getSquare("a1"))));
    EXPECT_EQ(5, BitBoard::bitCount(BitBoard::kingAttacks(TextIO::getSquare("a2"))));
    EXPECT_EQ(3, BitBoard::bitCount(BitBoard::kingAttacks(TextIO::getSquare("h8"))));
    EXPECT_EQ(5, BitBoard::bitCount(BitBoard::kingAttacks(TextIO::getSquare("a6"))));
    EXPECT_EQ(8, BitBoard::bitCount(BitBoard::kingAttacks(TextIO::getSquare("b2"))));
}

TEST(BitBoardTest, testKnightAttacks) {
    EXPECT_EQ(3, BitBoard::bitCount(BitBoard::knightAttacks(TextIO::getSquare("g1"))));
    EXPECT_EQ(2, BitBoard::bitCount(BitBoard::knightAttacks(TextIO::getSquare("a1"))));
    EXPECT_EQ(2, BitBoard::bitCount(BitBoard::knightAttacks(TextIO::getSquare("h1"))));
    EXPECT_EQ(4, BitBoard::bitCount(BitBoard::knightAttacks(TextIO::getSquare("h6"))));
    EXPECT_EQ(4, BitBoard::bitCount(BitBoard::knightAttacks(TextIO::getSquare("b7"))));
    EXPECT_EQ(8, BitBoard::bitCount(BitBoard::knightAttacks(TextIO::getSquare("c6"))));
    EXPECT_EQ((1ULL<<TextIO::getSquare("e2")) |
              (1ULL<<TextIO::getSquare("f3")) |
              (1ULL<<TextIO::getSquare("h3")),
              BitBoard::knightAttacks(TextIO::getSquare("g1")));
}

TEST(BitBoardTest, testPawnAttacks) {
    for (Square sq : AllSquares()) {
        int x = sq.getX();
        int y = sq.getY();
        U64 atk = BitBoard::wPawnAttacksMask(1ULL << sq);
        U64 expected = 0;
        if (y < 7) {
            if (x > 0)
                expected |= 1ULL << (sq + 7);
            if (x < 7)
                expected |= 1ULL << (sq + 9);
        }
        EXPECT_EQ(expected, atk);

        atk = BitBoard::bPawnAttacksMask(1ULL << sq);
        expected = 0;
        if (y > 0) {
            if (x > 0)
                expected |= 1ULL << (sq - 9);
            if (x < 7)
                expected |= 1ULL << (sq - 7);
        }
        EXPECT_EQ(expected, atk);
    }

    EXPECT_EQ(BitBoard::sqMask(A5,B5,C5),
              BitBoard::wPawnAttacksMask(BitBoard::sqMask(A4,B4)));
    EXPECT_EQ(BitBoard::sqMask(A6,C6,E6),
              BitBoard::wPawnAttacksMask(BitBoard::sqMask(B5,D5)));

    EXPECT_EQ(BitBoard::sqMask(B1,G1),
              BitBoard::bPawnAttacksMask(BitBoard::sqMask(A2,H2)));
    EXPECT_EQ(BitBoard::sqMask(F3,H3,F2,H2),
              BitBoard::bPawnAttacksMask(BitBoard::sqMask(G4,G3)));
}

TEST(BitBoardTest, testSquaresBetween) {
    // Tests that the set of nonzero elements is correct
    for (Square sq1 : AllSquares()) {
        for (Square sq2 : AllSquares()) {
            int d = BitBoard::getDirection(sq1, sq2);
            if (d == 0) {
                ASSERT_EQ(0, BitBoard::squaresBetween(sq1, sq2));
            } else {
                int dx = sq1.getX() - sq2.getX();
                int dy = sq1.getY() - sq2.getY();
                if (std::abs(dx * dy) == 2) { // Knight direction
                    ASSERT_EQ(0, BitBoard::squaresBetween(sq1, sq2));
                } else {
                    if ((std::abs(dx) > 1) || (std::abs(dy) > 1)) {
                        ASSERT_NE(BitBoard::squaresBetween(sq1, sq2), 0);
                    } else {
                        ASSERT_EQ(0, BitBoard::squaresBetween(sq1, sq2));
                    }
                }
            }
        }
    }

    ASSERT_EQ(0x0040201008040200ULL, BitBoard::squaresBetween(Square(0), Square(63)));
    ASSERT_EQ(0x000000001C000000ULL, BitBoard::squaresBetween(TextIO::getSquare("b4"),
                                                              TextIO::getSquare("f4")));
}

/**
 * If there is a piece type that can move from "from" to "to", return the
 * corresponding direction, 8*dy+dx.
 */
static int computeDirection(Square from, Square to) {
    int dx = to.getX() - from.getX();
    int dy = to.getY() - from.getY();
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

TEST(BitBoardTest, testGetDirection) {
    for (Square from : AllSquares())
        for (Square to : AllSquares())
            ASSERT_EQ(computeDirection(from, to),
                      BitBoard::getDirection(from, to));
}

static int computeDistance(Square from, Square to, bool taxi) {
    int dx = to.getX() - from.getX();
    int dy = to.getY() - from.getY();
    if (taxi)
        return std::abs(dx) + std::abs(dy);
    else
        return std::max(std::abs(dx), std::abs(dy));
}

TEST(BitBoardTest, testGetDistance) {
    for (Square from : AllSquares()) {
        for (Square to : AllSquares()) {
            ASSERT_EQ(computeDistance(from, to, false), BitBoard::getKingDistance(from, to));
            ASSERT_EQ(computeDistance(from, to, true ), BitBoard::getTaxiDistance(from, to));
        }
    }
}

TEST(BitBoardTest, testTrailingZeros) {
    for (Square i : AllSquares()) {
        U64 mask = 1ULL << i;
        ASSERT_EQ(i, BitBoard::firstSquare(mask));
        U64 mask2 = mask;
        ASSERT_EQ(i, BitBoard::extractSquare(mask2));
        ASSERT_EQ(0, mask2);
    }
    U64 mask = 0xffffffffffffffffULL;
    int cnt = 0;
    while (mask) {
        ASSERT_EQ(cnt, BitBoard::extractSquare(mask).asInt());
        cnt++;
    }
    ASSERT_EQ(64, cnt);
}

TEST(BitBoardTest, testLastSquare) {
    for (int i = 0; i < 64; i++) {
        U64 mask = 1ULL << i;
        ASSERT_EQ(i, BitBoard::lastSquare(mask).asInt());
        if (i > 0) {
            ASSERT_EQ(i - 1, BitBoard::lastSquare(mask - 1).asInt());
            if (i < 63) {
                ASSERT_EQ(i, BitBoard::lastSquare(mask + 1).asInt());
            }
        }
    }
    for (int i = 0; i < 1000; i++) {
        U64 m = hashU64(i+1);
        ASSERT_NE(0, m);
        Square expected;
        for (Square b : AllSquares())
            if (m & (1ULL << b))
                expected = b;
        ASSERT_EQ(expected, BitBoard::lastSquare(m));
    }
}


static U64 mirrorXSlow(U64 mask) {
    U64 ret = 0;
    while (mask != 0) {
        Square sq = BitBoard::extractSquare(mask);
        int x = sq.getX();
        int y = sq.getY();
        Square sq2(7-x, y);
        ret |= (1ULL << sq2);
    }
    return ret;
}

static U64 mirrorYSlow(U64 mask) {
    U64 ret = 0;
    while (mask != 0) {
        Square sq = BitBoard::extractSquare(mask);
        int x = sq.getX();
        int y = sq.getY();
        Square sq2(x, 7-y);
        ret |= (1ULL << sq2);
    }
    return ret;
}

TEST(BitBoardTest, testMaskAndMirror) {
    EXPECT_EQ(BitBoard::sqMask(A1,H1,A8,H8), BitBoard::maskCorners);
    EXPECT_EQ(BitBoard::sqMask(A1,B1,C1,D1,E1,F1,G1,H1), BitBoard::maskRow1);
    EXPECT_EQ(BitBoard::sqMask(A2,B2,C2,D2,E2,F2,G2,H2), BitBoard::maskRow2);
    EXPECT_EQ(BitBoard::sqMask(A3,B3,C3,D3,E3,F3,G3,H3), BitBoard::maskRow3);
    EXPECT_EQ(BitBoard::sqMask(A4,B4,C4,D4,E4,F4,G4,H4), BitBoard::maskRow4);
    EXPECT_EQ(BitBoard::sqMask(A5,B5,C5,D5,E5,F5,G5,H5), BitBoard::maskRow5);
    EXPECT_EQ(BitBoard::sqMask(A6,B6,C6,D6,E6,F6,G6,H6), BitBoard::maskRow6);
    EXPECT_EQ(BitBoard::sqMask(A7,B7,C7,D7,E7,F7,G7,H7), BitBoard::maskRow7);
    EXPECT_EQ(BitBoard::sqMask(A8,B8,C8,D8,E8,F8,G8,H8), BitBoard::maskRow8);

    EXPECT_EQ(BitBoard::sqMask(A1,B1,C1,D1,E1,F1,G1,H1,
                                  A8,B8,C8,D8,E8,F8,G8,H8), BitBoard::maskRow1Row8);
    EXPECT_EQ(BitBoard::mirrorX(BitBoard::maskRow1Row8), BitBoard::maskRow1Row8);
    EXPECT_EQ(BitBoard::mirrorY(BitBoard::maskRow1Row8), BitBoard::maskRow1Row8);

    EXPECT_EQ(BitBoard::mirrorX(BitBoard::maskDarkSq), BitBoard::maskLightSq);
    EXPECT_EQ(BitBoard::mirrorY(BitBoard::maskDarkSq), BitBoard::maskLightSq);
    EXPECT_EQ(BitBoard::mirrorX(BitBoard::maskLightSq), BitBoard::maskDarkSq);
    EXPECT_EQ(BitBoard::mirrorY(BitBoard::maskLightSq), BitBoard::maskDarkSq);

    EXPECT_EQ(BitBoard::sqMask(A1,B1,C1), 7);
    EXPECT_EQ(BitBoard::sqMask(B1,C1,D1,F1,G1), 0x6E);
    EXPECT_EQ(BitBoard::sqMask(F1,G1), 0x60L);
    EXPECT_EQ(BitBoard::sqMask(B1,C1,D1), 0xEL);
    EXPECT_EQ(BitBoard::sqMask(G1,H1), 0xC0L);
    EXPECT_EQ(BitBoard::sqMask(B1,C1), 0x6L);
    EXPECT_EQ(BitBoard::sqMask(A1,B1), 0x3L);
    EXPECT_EQ(BitBoard::sqMask(F8,G8), 0x6000000000000000L);
    EXPECT_EQ(BitBoard::sqMask(G8,H8), 0xC000000000000000L);
    EXPECT_EQ(BitBoard::sqMask(B8,C8), 0x600000000000000L);
    EXPECT_EQ(BitBoard::sqMask(A8,B8), 0x300000000000000L);

    EXPECT_EQ(BitBoard::sqMask(C2,B3,F2,G3,B6,C7,G6,F7), 0x24420000422400ULL);
    EXPECT_EQ(BitBoard::sqMask(A8,B8,A7,B7), 0x0303000000000000ULL);

    EXPECT_EQ(BitBoard::sqMask(G8,H8,G7,H7), 0xC0C0000000000000ULL);
    EXPECT_EQ(BitBoard::sqMask(A1,B1,A2,B2), 0x0303ULL);
    EXPECT_EQ(BitBoard::sqMask(G1,H1,G2,H2), 0xC0C0ULL);
    EXPECT_EQ(BitBoard::sqMask(A8,B8,A7), 0x0301000000000000ULL);
    EXPECT_EQ(BitBoard::sqMask(G8,H8,H7), 0xC080000000000000ULL);
    EXPECT_EQ(BitBoard::sqMask(A1,B1,A2), 0x0103ULL);
    EXPECT_EQ(BitBoard::sqMask(G1,H1,H2), 0x80C0ULL);
    EXPECT_EQ(BitBoard::sqMask(A8,B8,C8,D8,D7), 0x0F08000000000000ULL);
    EXPECT_EQ(BitBoard::sqMask(A8,B8,A7), 0x0301000000000000ULL);
    EXPECT_EQ(BitBoard::sqMask(E8,F8,G8,H8,E7), 0xF010000000000000ULL);
    EXPECT_EQ(BitBoard::sqMask(G8,H8,H7), 0xC080000000000000ULL);
    EXPECT_EQ(BitBoard::sqMask(A1,B1,C1,D1,D2), 0x080FULL);
    EXPECT_EQ(BitBoard::sqMask(A1,B1,A2), 0x0103ULL);
    EXPECT_EQ(BitBoard::sqMask(E1,F1,G1,H1,E2), 0x10F0ULL);
    EXPECT_EQ(BitBoard::sqMask(G1,H1,H2), 0x80C0ULL);

    for (Square sq : AllSquares()) {
        U64 m = 1ULL << sq;
        switch (sq.getX()) {
        case 0:
            ASSERT_NE((m & BitBoard::maskFileA), 0);
            ASSERT_EQ((m & BitBoard::maskFileB), 0);
            ASSERT_EQ((m & BitBoard::maskFileC), 0);
            ASSERT_EQ((m & BitBoard::maskFileD), 0);
            ASSERT_EQ((m & BitBoard::maskFileE), 0);
            ASSERT_EQ((m & BitBoard::maskFileF), 0);
            ASSERT_EQ((m & BitBoard::maskFileG), 0);
            ASSERT_EQ((m & BitBoard::maskFileH), 0);
            ASSERT_NE((m & BitBoard::maskAToDFiles), 0);
            ASSERT_EQ((m & BitBoard::maskEToHFiles), 0);
            break;
        case 1:
            ASSERT_EQ((m & BitBoard::maskFileA), 0);
            ASSERT_NE((m & BitBoard::maskFileB), 0);
            ASSERT_EQ((m & BitBoard::maskFileC), 0);
            ASSERT_EQ((m & BitBoard::maskFileD), 0);
            ASSERT_EQ((m & BitBoard::maskFileE), 0);
            ASSERT_EQ((m & BitBoard::maskFileF), 0);
            ASSERT_EQ((m & BitBoard::maskFileG), 0);
            ASSERT_EQ((m & BitBoard::maskFileH), 0);
            ASSERT_NE((m & BitBoard::maskAToDFiles), 0);
            ASSERT_EQ((m & BitBoard::maskEToHFiles), 0);
            break;
        case 2:
            ASSERT_EQ((m & BitBoard::maskFileA), 0);
            ASSERT_EQ((m & BitBoard::maskFileB), 0);
            ASSERT_NE((m & BitBoard::maskFileC), 0);
            ASSERT_EQ((m & BitBoard::maskFileD), 0);
            ASSERT_EQ((m & BitBoard::maskFileE), 0);
            ASSERT_EQ((m & BitBoard::maskFileF), 0);
            ASSERT_EQ((m & BitBoard::maskFileG), 0);
            ASSERT_EQ((m & BitBoard::maskFileH), 0);
            ASSERT_NE((m & BitBoard::maskAToDFiles), 0);
            ASSERT_EQ((m & BitBoard::maskEToHFiles), 0);
            break;
        case 3:
            ASSERT_EQ((m & BitBoard::maskFileA), 0);
            ASSERT_EQ((m & BitBoard::maskFileB), 0);
            ASSERT_EQ((m & BitBoard::maskFileC), 0);
            ASSERT_NE((m & BitBoard::maskFileD), 0);
            ASSERT_EQ((m & BitBoard::maskFileE), 0);
            ASSERT_EQ((m & BitBoard::maskFileF), 0);
            ASSERT_EQ((m & BitBoard::maskFileG), 0);
            ASSERT_EQ((m & BitBoard::maskFileH), 0);
            ASSERT_NE((m & BitBoard::maskAToDFiles), 0);
            ASSERT_EQ((m & BitBoard::maskEToHFiles), 0);
            break;
        case 4:
            ASSERT_EQ((m & BitBoard::maskFileA), 0);
            ASSERT_EQ((m & BitBoard::maskFileB), 0);
            ASSERT_EQ((m & BitBoard::maskFileC), 0);
            ASSERT_EQ((m & BitBoard::maskFileD), 0);
            ASSERT_NE((m & BitBoard::maskFileE), 0);
            ASSERT_EQ((m & BitBoard::maskFileF), 0);
            ASSERT_EQ((m & BitBoard::maskFileG), 0);
            ASSERT_EQ((m & BitBoard::maskFileH), 0);
            ASSERT_EQ((m & BitBoard::maskAToDFiles), 0);
            ASSERT_NE((m & BitBoard::maskEToHFiles), 0);
            break;
        case 5:
            ASSERT_EQ((m & BitBoard::maskFileA), 0);
            ASSERT_EQ((m & BitBoard::maskFileB), 0);
            ASSERT_EQ((m & BitBoard::maskFileC), 0);
            ASSERT_EQ((m & BitBoard::maskFileD), 0);
            ASSERT_EQ((m & BitBoard::maskFileE), 0);
            ASSERT_NE((m & BitBoard::maskFileF), 0);
            ASSERT_EQ((m & BitBoard::maskFileG), 0);
            ASSERT_EQ((m & BitBoard::maskFileH), 0);
            ASSERT_EQ((m & BitBoard::maskAToDFiles), 0);
            ASSERT_NE((m & BitBoard::maskEToHFiles), 0);
            break;
        case 6:
            ASSERT_EQ((m & BitBoard::maskFileA), 0);
            ASSERT_EQ((m & BitBoard::maskFileB), 0);
            ASSERT_EQ((m & BitBoard::maskFileC), 0);
            ASSERT_EQ((m & BitBoard::maskFileD), 0);
            ASSERT_EQ((m & BitBoard::maskFileE), 0);
            ASSERT_EQ((m & BitBoard::maskFileF), 0);
            ASSERT_NE((m & BitBoard::maskFileG), 0);
            ASSERT_EQ((m & BitBoard::maskFileH), 0);
            ASSERT_EQ((m & BitBoard::maskAToDFiles), 0);
            ASSERT_NE((m & BitBoard::maskEToHFiles), 0);
            break;
        case 7:
            ASSERT_EQ((m & BitBoard::maskFileA), 0);
            ASSERT_EQ((m & BitBoard::maskFileB), 0);
            ASSERT_EQ((m & BitBoard::maskFileC), 0);
            ASSERT_EQ((m & BitBoard::maskFileD), 0);
            ASSERT_EQ((m & BitBoard::maskFileE), 0);
            ASSERT_EQ((m & BitBoard::maskFileF), 0);
            ASSERT_EQ((m & BitBoard::maskFileG), 0);
            ASSERT_NE((m & BitBoard::maskFileH), 0);
            ASSERT_EQ((m & BitBoard::maskAToDFiles), 0);
            ASSERT_NE((m & BitBoard::maskEToHFiles), 0);
            break;
        }
    }

    for (Square sq : AllSquares()) {
        U64 m = 1ULL << sq;
        ASSERT_EQ(mirrorXSlow(m), BitBoard::mirrorX(m));
        ASSERT_EQ(mirrorYSlow(m), BitBoard::mirrorY(m));
        m = ~m;
        ASSERT_EQ(mirrorXSlow(m), BitBoard::mirrorX(m));
        ASSERT_EQ(mirrorYSlow(m), BitBoard::mirrorY(m));
    }
}

TEST(BitBoardTest, testSliders) {
    Position pos = TextIO::readFEN("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1");
    ASSERT_EQ(BitBoard::sqMask(B1,C1,D1,E1,A2,A3,A4,A5,A6,A7,A8),
              BitBoard::rookAttacks(A1, pos.occupiedBB()));
}
