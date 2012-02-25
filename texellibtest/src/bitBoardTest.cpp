/*
 * bitBoardTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "bitBoardTest.hpp"

#include "cute.h"

#include "bitBoard.hpp"


/** Test of kingAttacks, of class BitBoard. */
static void
testKingAttacks() {
#if 0
    ASSERT_EQUAL(5, Long.bitCount(BitBoard.kingAttacks[TextIO::getSquare("g1")]));
    ASSERT_EQUAL(3, Long.bitCount(BitBoard.kingAttacks[TextIO::getSquare("h1")]));
    ASSERT_EQUAL(3, Long.bitCount(BitBoard.kingAttacks[TextIO::getSquare("a1")]));
    ASSERT_EQUAL(5, Long.bitCount(BitBoard.kingAttacks[TextIO::getSquare("a2")]));
    ASSERT_EQUAL(3, Long.bitCount(BitBoard.kingAttacks[TextIO::getSquare("h8")]));
    ASSERT_EQUAL(5, Long.bitCount(BitBoard.kingAttacks[TextIO::getSquare("a6")]));
    ASSERT_EQUAL(8, Long.bitCount(BitBoard.kingAttacks[TextIO::getSquare("b2")]));
#endif
}

/** Test of knightAttacks, of class BitBoard. */
static void
testKnightAttacks() {
#if 0
    ASSERT_EQUAL(3, Long.bitCount(BitBoard.knightAttacks[TextIO::getSquare("g1")]));
    ASSERT_EQUAL(2, Long.bitCount(BitBoard.knightAttacks[TextIO::getSquare("a1")]));
    ASSERT_EQUAL(2, Long.bitCount(BitBoard.knightAttacks[TextIO::getSquare("h1")]));
    ASSERT_EQUAL(4, Long.bitCount(BitBoard.knightAttacks[TextIO::getSquare("h6")]));
    ASSERT_EQUAL(4, Long.bitCount(BitBoard.knightAttacks[TextIO::getSquare("b7")]));
    ASSERT_EQUAL(8, Long.bitCount(BitBoard.knightAttacks[TextIO::getSquare("c6")]));
    ASSERT_EQUAL((1ULL<<TextIO::getSquare("e2")) |
		 (1ULL<<TextIO::getSquare("f3")) |
		 (1ULL<<TextIO::getSquare("h3")),
		 BitBoard.knightAttacks[TextIO::getSquare("g1")]);
#endif
}

/** Test of squaresBetween[][], of class BitBoard. */
static void
testSquaresBetween() {
#if 0
    // Tests that the set of nonzero elements is correct
    for (int sq1 = 0; sq1 < 64; sq1++) {
	for (int sq2 = 0; sq2 < 64; sq2++) {
	    int d = BitBoard.getDirection(sq1, sq2);
	    if (d == 0) {
		ASSERT_EQUAL(0, BitBoard.squaresBetween[sq1][sq2]);
	    } else {
		int dx = Position::getX(sq1) - Position::getX(sq2);
		int dy = Position::getY(sq1) - Position::getY(sq2);
		if (Math.abs(dx * dy) == 2) { // Knight direction
		    ASSERT_EQUAL(0, BitBoard.squaresBetween[sq1][sq2]);
		} else {
		    if ((Math.abs(dx) > 1) || (Math.abs(dy) > 1)) {
			ASSERT(BitBoard.squaresBetween[sq1][sq2] != 0);
		    } else {
			ASSERT_EQUAL(0, BitBoard.squaresBetween[sq1][sq2]);
		    }
		}
	    }
	}
    }

    ASSERT_EQUAL(0x0040201008040200L, BitBoard.squaresBetween[0][63]);
    ASSERT_EQUAL(0x000000001C000000L, BitBoard.squaresBetween[TextIO::getSquare("b4")][TextIO::getSquare("f4")]);
#endif
}

/**
 * If there is a piece type that can move from "from" to "to", return the
 * corresponding direction, 8*dy+dx.
 */
static int computeDirection(int from, int to) {
#if 0
    int dx = Position::getX(to) - Position::getX(from);
    int dy = Position::getY(to) - Position::getY(from);
    if (dx == 0) {                   // Vertical rook direction
	if (dy == 0) return 0;
	return (dy > 0) ? 8 : -8;
    }
    if (dy == 0)                    // Horizontal rook direction
	return (dx > 0) ? 1 : -1;
    if (Math.abs(dx) == Math.abs(dy)) // Bishop direction
	return ((dy > 0) ? 8 : -8) + (dx > 0 ? 1 : -1);
    if (Math.abs(dx * dy) == 2)     // Knight direction
	return dy * 8 + dx;
    return 0;
#endif
}

static void
testGetDirection() {
#if 0
    for (int from = 0; from < 64; from++) {
	for (int to = 0; to < 64; to++) {
	    ASSERT_EQUAL(computeDirection(from, to), BitBoard.getDirection(from, to));
	}
    }
#endif
}

static void
testTrailingZeros() {
#if 0
    for (int i = 0; i < 64; i++) {
	U64 mask = 1ULL << i;
	ASSERT_EQUAL(i, BitBoard.numberOfTrailingZeros(mask));
    }
#endif
}


cute::suite
BitBoardTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testKingAttacks));
    s.push_back(CUTE(testKnightAttacks));
    s.push_back(CUTE(testSquaresBetween));
    s.push_back(CUTE(testGetDirection));
    s.push_back(CUTE(testTrailingZeros));
    return s;
}
