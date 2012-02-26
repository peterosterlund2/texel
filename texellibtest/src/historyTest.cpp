/*
 * historyTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "historyTest.hpp"
#include "history.hpp"
#include "textio.hpp"

#include "cute.h"


/**
 * Test of getHistScore method, of class History.
 */
static void
testGetHistScore() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    History hs;
    Move m1 = TextIO::stringToMove(pos, "e4");
    Move m2 = TextIO::stringToMove(pos, "d4");
    ASSERT_EQUAL(0, hs.getHistScore(pos, m1));

    hs.addSuccess(pos, m1, 1);
    ASSERT_EQUAL(1 * 49 / 1, hs.getHistScore(pos, m1));
    ASSERT_EQUAL(0, hs.getHistScore(pos, m2));

    hs.addSuccess(pos, m1, 1);
    ASSERT_EQUAL(1 * 49 / 1, hs.getHistScore(pos, m1));
    ASSERT_EQUAL(0, hs.getHistScore(pos, m2));

    hs.addFail(pos, m1, 1);
    ASSERT_EQUAL(2 * 49 / 3, hs.getHistScore(pos, m1));
    ASSERT_EQUAL(0, hs.getHistScore(pos, m2));

    hs.addFail(pos, m1, 1);
    ASSERT_EQUAL(2 * 49 / 4, hs.getHistScore(pos, m1));
    ASSERT_EQUAL(0, hs.getHistScore(pos, m2));

    hs.addSuccess(pos, m2, 1);
    ASSERT_EQUAL(2 * 49 / 4, hs.getHistScore(pos, m1));
    ASSERT_EQUAL(1 * 49 / 1, hs.getHistScore(pos, m2));
}

cute::suite
HistoryTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testGetHistScore));
    return s;
}
