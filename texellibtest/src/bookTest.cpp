/*
 * bookTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "bookTest.hpp"

#include "cute.h"

#if 0
    /**
     * Test of getBookMove method, of class Book.
     */
    public void testGetBookMove() {
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        Book book = new Book(true);
        Move move = book.getBookMove(pos);
        checkValid(pos, move);
    }

    /**
     * Test of getAllBookMoves method, of class Book.
     */
    public void testGetAllBookMoves() {
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        Book book = new Book(true);
        std::string moveListString = book.getAllBookMoves(pos);
        String[] strMoves = moveListString.split("\\([0-9]*\\) ");
        ASSERT(strMoves.length > 1);
        for (String strMove : strMoves) {
            Move m = TextIO::stringToMove(pos, strMove);
            checkValid(pos, m);
        }
    }

    /** Check that move is a legal move in position pos. */
    private void checkValid(const Position& pos, const Move& move) {
        ASSERT(move != null);
        MoveGen::MoveList moveList;
	MoveGen::pseudoLegalMoves(pos, moveList);
        MoveGen::removeIllegal(pos, moveList);
        bool contains = false;
        for (int mi = 0; mi < moveList.size; mi++)
            if (moveList.m[mi].equals(move)) {
                contains = true;
                break;
            }
        ASSERT(contains);
    }
#endif

cute::suite
BookTest::getSuite() const {
    cute::suite s;
#if 0
    s.push_back(CUTE(testGetBookMove));
    s.push_back(CUTE(testGetAllBookMoves));
#endif
    return s;
}
