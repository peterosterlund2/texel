/*
 * historyTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "historyTest.hpp"

#include "cute.h"

#if 0
    /**
     * Test of getHistScore method, of class History.
     */
    @Test
    public void testGetHistScore() throws ChessParseError {
        System.out.println("getHistScore");
        Position pos = TextIO.readFEN(TextIO.startPosFEN);
        History hs = new History();
        Move m1 = TextIO.stringToMove(pos, "e4");
        Move m2 = TextIO.stringToMove(pos, "d4");
        assertEquals(0, hs.getHistScore(pos, m1));

        hs.addSuccess(pos, m1, 1);
        assertEquals(1 * 49 / 1, hs.getHistScore(pos, m1));
        assertEquals(0, hs.getHistScore(pos, m2));

        hs.addSuccess(pos, m1, 1);
        assertEquals(1 * 49 / 1, hs.getHistScore(pos, m1));
        assertEquals(0, hs.getHistScore(pos, m2));

        hs.addFail(pos, m1, 1);
        assertEquals(2 * 49 / 3, hs.getHistScore(pos, m1));
        assertEquals(0, hs.getHistScore(pos, m2));

        hs.addFail(pos, m1, 1);
        assertEquals(2 * 49 / 4, hs.getHistScore(pos, m1));
        assertEquals(0, hs.getHistScore(pos, m2));

        hs.addSuccess(pos, m2, 1);
        assertEquals(2 * 49 / 4, hs.getHistScore(pos, m1));
        assertEquals(1 * 49 / 1, hs.getHistScore(pos, m2));
    }
#endif


cute::suite
HistoryTest::getSuite() const {
    cute::suite s;
//    s.push_back(CUTE());
    return s;
}
