/*
 * pieceTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "pieceTest.hpp"

#include "cute.h"

#include "move.hpp"
#include "piece.hpp"

/**
 * Test of isWhite method, of class Piece.
 */
static void
testIsWhite() {
    ASSERT_EQUAL(false, Piece::isWhite(Piece::BBISHOP));
    ASSERT_EQUAL(true , Piece::isWhite(Piece::WBISHOP));
    ASSERT_EQUAL(true , Piece::isWhite(Piece::WKING));
    ASSERT_EQUAL(false, Piece::isWhite(Piece::BKING));
}

cute::suite
PieceTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testIsWhite));
    return s;
}
