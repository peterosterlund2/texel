/*
 * killerTableTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "killerTableTest.hpp"
#include "killerTable.hpp"
#include "textio.hpp"
#include "piece.hpp"

#include "cute.h"

/**
 * Test of addKiller method, of class KillerTable.
 */
static void testAddKiller() {
    KillerTable kt;
    Move m(TextIO::getSquare("b1"), TextIO::getSquare("b5"), Piece::EMPTY);
    kt.addKiller(3, m);
    kt.addKiller(7, m);
    kt.addKiller(3, m);
    kt.addKiller(3, m);
}

/**
 * Test of getKillerScore method, of class KillerTable.
 */
static void testGetKillerScore() {
    KillerTable kt;
    Move m1(TextIO::getSquare("b1"), TextIO::getSquare("b5"), Piece::EMPTY);
    Move m2(TextIO::getSquare("c1"), TextIO::getSquare("d2"), Piece::EMPTY);
    Move m3(TextIO::getSquare("e1"), TextIO::getSquare("g1"), Piece::EMPTY);
    kt.addKiller(0, m1);
    ASSERT_EQUAL(4, kt.getKillerScore(0, m1));
    ASSERT_EQUAL(0, kt.getKillerScore(0, m2));
    ASSERT_EQUAL(0, kt.getKillerScore(0, Move(m2)));
    kt.addKiller(0, m1);
    ASSERT_EQUAL(4, kt.getKillerScore(0, m1));
    kt.addKiller(0, m2);
    ASSERT_EQUAL(4, kt.getKillerScore(0, m2));
    ASSERT_EQUAL(4, kt.getKillerScore(0, Move(m2)));    // Must compare by value
    ASSERT_EQUAL(3, kt.getKillerScore(0, m1));
    kt.addKiller(0, Move(m2));
    ASSERT_EQUAL(4, kt.getKillerScore(0, m2));
    ASSERT_EQUAL(3, kt.getKillerScore(0, m1));
    ASSERT_EQUAL(0, kt.getKillerScore(0, m3));
    kt.addKiller(0, m3);
    ASSERT_EQUAL(0, kt.getKillerScore(0, m1));
    ASSERT_EQUAL(3, kt.getKillerScore(0, m2));
    ASSERT_EQUAL(4, kt.getKillerScore(0, m3));

    ASSERT_EQUAL(0, kt.getKillerScore(1, m3));
    ASSERT_EQUAL(2, kt.getKillerScore(2, m3));
    ASSERT_EQUAL(0, kt.getKillerScore(3, m3));
    ASSERT_EQUAL(0, kt.getKillerScore(4, m3));

    kt.addKiller(2, m2);
    ASSERT_EQUAL(4, kt.getKillerScore(2, m2));
    ASSERT_EQUAL(3, kt.getKillerScore(0, m2));
}


cute::suite
KillerTableTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testAddKiller));
    s.push_back(CUTE(testGetKillerScore));
    return s;
}
