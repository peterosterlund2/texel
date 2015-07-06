/*
 * polyglotTest.cpp
 *
 *  Created on: Jul 5, 2015
 *      Author: petero
 */

#include "polyglotTest.hpp"

#include "../polyglot.hpp"
#include "textio.hpp"

#include "cute.h"

void
PolyglotTest::testHashKey() {
    Position pos = TextIO::readFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    ASSERT_EQUAL(0x463b96181691fc9cULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    ASSERT_EQUAL(0x823c9b50fd114196ULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    ASSERT_EQUAL(0x0756b94461c50fb0ULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    ASSERT_EQUAL(0x662fafb965db29d4ULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    ASSERT_EQUAL(0x22a48b5a8e47ff78ULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3");
    ASSERT_EQUAL(0x652a607ca3f242c1ULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4");
    ASSERT_EQUAL(0x00fdd303c946bdd9ULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3");
    ASSERT_EQUAL(0x3c8123ea7b067637ULL, PolyglotBook::getHashKey(pos));

    pos = TextIO::readFEN("rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4");
    ASSERT_EQUAL(0x5c3f9b829b279560ULL, PolyglotBook::getHashKey(pos));
}

void
PolyglotTest::testMove() {
    Position pos = TextIO::readFEN("r3k2r/ppp1qppp/2npbn2/2b1p3/2B1P3/2NP1N2/PPPBQPPP/R3K2R w KQkq - 0 1");
    ASSERT_EQUAL((4 << 0) | (2 << 3) | (3 << 6) | (1 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("d2e3")));

    // Castling
    ASSERT_EQUAL((7 << 0) | (0 << 3) | (4 << 6) | (0 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e1g1")));
    ASSERT_EQUAL((0 << 0) | (0 << 3) | (4 << 6) | (0 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e1c1")));
    ASSERT_EQUAL((7 << 0) | (7 << 3) | (4 << 6) | (7 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e8g8")));
    ASSERT_EQUAL((0 << 0) | (7 << 3) | (4 << 6) | (7 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e8c8")));

    // Not castling, even though from/to squares match castling moves
    pos = TextIO::readFEN("r3q2r/ppp1kppp/2npbn2/2b1p3/2B1P3/2NP1N2/PPPBKPPP/R3Q2R w - - 0 1");
    ASSERT_EQUAL((6 << 0) | (0 << 3) | (4 << 6) | (0 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e1g1")));
    ASSERT_EQUAL((2 << 0) | (0 << 3) | (4 << 6) | (0 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e1c1")));
    ASSERT_EQUAL((6 << 0) | (7 << 3) | (4 << 6) | (7 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e8g8")));
    ASSERT_EQUAL((2 << 0) | (7 << 3) | (4 << 6) | (7 << 9) | (0 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("e8c8")));

    // Promotion
    pos = TextIO::readFEN("r3q2r/pPp1kppp/2npbn2/2b1p3/2B1P3/2NP1N2/PpPBKPPP/R3Q2R w - - 0 1");
    ASSERT_EQUAL((0 << 0) | (7 << 3) | (1 << 6) | (6 << 9) | (1 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b7a8n")));
    ASSERT_EQUAL((0 << 0) | (7 << 3) | (1 << 6) | (6 << 9) | (2 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b7a8b")));
    ASSERT_EQUAL((0 << 0) | (7 << 3) | (1 << 6) | (6 << 9) | (3 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b7a8r")));
    ASSERT_EQUAL((0 << 0) | (7 << 3) | (1 << 6) | (6 << 9) | (4 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b7a8q")));

    ASSERT_EQUAL((0 << 0) | (0 << 3) | (1 << 6) | (1 << 9) | (1 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b2a1n")));
    ASSERT_EQUAL((0 << 0) | (0 << 3) | (1 << 6) | (1 << 9) | (2 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b2a1b")));
    ASSERT_EQUAL((0 << 0) | (0 << 3) | (1 << 6) | (1 << 9) | (3 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b2a1r")));
    ASSERT_EQUAL((0 << 0) | (0 << 3) | (1 << 6) | (1 << 9) | (4 << 12),
                 PolyglotBook::getMove(pos, TextIO::uciStringToMove("b2a1q")));
}

cute::suite
PolyglotTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testHashKey));
    s.push_back(CUTE(testMove));
    return s;
}
