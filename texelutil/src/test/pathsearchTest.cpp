/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * pathsearchTest.cpp
 *
 *  Created on: Aug 22, 2015
 *      Author: petero
 */

#include "pathsearchTest.hpp"
#include "../pathsearch.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include <climits>

#include "cute.h"



void
PathSearchTest::checkBlockedConsistency(PathSearch& ps, Position& pos) {
    U64 blocked;
    if (!ps.computeBlocked(pos, blocked))
        return;
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    UndoInfo ui;
    for (int i = 0; i < moves.size; i++) {
        if ((1ULL << moves[i].from()) & blocked) {
            pos.makeMove(moves[i], ui);
            int hs = ps.distLowerBound(pos);
            ASSERT_EQUAL(INT_MAX, hs);
            pos.unMakeMove(moves[i], ui);
        }
    }
}

int
PathSearchTest::hScore(PathSearch& ps, const std::string& fen) {
    {
        Position pos0 = TextIO::readFEN(TextIO::startPosFEN);
        checkBlockedConsistency(ps, pos0);
    }
    Position pos = TextIO::readFEN(fen);
    checkBlockedConsistency(ps, pos);
    int hScore = ps.distLowerBound(pos);
    ASSERT(hScore >= 0);
    return hScore;
}

void
PathSearchTest::testMaterial() {
    {
        PathSearch ps(TextIO::startPosFEN);
        ASSERT_EQUAL(0, hScore(ps, TextIO::startPosFEN));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1"));
        ASSERT(hScore(ps, "r1bqkbnr/pppppppp/n7/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1") <= 1);
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w Kkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RN1QKBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pp1ppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rn1qkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1"));
    }
    {
        std::string goal("1nbqkbnr/1ppppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQk - 0 1");
        PathSearch ps(goal);
        ASSERT_EQUAL(0, hScore(ps, goal));
        ASSERT(hScore(ps, "1nbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQk - 0 1") <= 40);
    }
    {
        PathSearch ps("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPP1/RNNQKBNR w KQkq - 0 1");
        ASSERT_EQUAL(INT_MAX, hScore(ps, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPP1/RNBQKBNR w KQkq - 0 1"));
    }
    {
        PathSearch ps("1nbqkbnr/p1pppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQk - 0 1");
        ASSERT(hScore(ps, TextIO::startPosFEN) <= 20);
    }
    {
        PathSearch ps("rnbqkbnr/pppppppp/8/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        ASSERT(hScore(ps, TextIO::startPosFEN) <= 4);
    }
    {
        PathSearch ps("rnbqk1nr/b1pp1ppp/1p6/4p3/8/5N2/PPPPPPPP/R1BQKB1R w KQkq - 0 1");
        ASSERT(hScore(ps, TextIO::startPosFEN) <= 8);
    }
    {
        PathSearch ps("rnbqkbnr/ppp2ppp/8/8/8/8/PPP2PPP/RNBQKBNR w KQkq - 0 1");
        ASSERT(hScore(ps, TextIO::startPosFEN) >= 4);
        ASSERT(hScore(ps, TextIO::startPosFEN) <= 10);
    }
}

void
PathSearchTest::testNeighbors() {
    // Pawns
    ASSERT_EQUAL(BitBoard::sqMask(B2,C2,A3,B3,C3,D3),
                 PathSearch::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              0));
    ASSERT_EQUAL(BitBoard::sqMask(A2,B2,C2,D2),
                 PathSearch::computeNeighbors(Piece::BPAWN,
                                              BitBoard::sqMask(B1,C1),
                                              0));
    ASSERT_EQUAL(BitBoard::sqMask(A3,C3,D3,C2),
                 PathSearch::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              BitBoard::sqMask(B3)));
    ASSERT_EQUAL(BitBoard::sqMask(A3,B3,C3,D3,C2),
                 PathSearch::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              BitBoard::sqMask(B2)));
    ASSERT_EQUAL(BitBoard::sqMask(A3,D3),
                 PathSearch::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              BitBoard::sqMask(B3,C3)));
    ASSERT_EQUAL(0,
                 PathSearch::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B1),
                                              0));
    ASSERT_EQUAL(0,
                 PathSearch::computeNeighbors(Piece::BPAWN,
                                              BitBoard::sqMask(A8),
                                              0));
    ASSERT_EQUAL(BitBoard::sqMask(A2,B2),
                 PathSearch::computeNeighbors(Piece::BPAWN,
                                              BitBoard::sqMask(A1),
                                              0));
    // Kings
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WKING : Piece::BKING;
        ASSERT_EQUAL(BitBoard::sqMask(B2),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(A1),
                                                  BitBoard::sqMask(B1,A2)));
        ASSERT_EQUAL(BitBoard::sqMask(B1,A2,B2),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(A1),
                                                  0));
    }

    // Knights
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WKNIGHT : Piece::BKNIGHT;
        ASSERT_EQUAL(BitBoard::sqMask(D1,D3,A4),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(B2),
                                                  BitBoard::sqMask(C1,C2,C4)));
        ASSERT_EQUAL(BitBoard::sqMask(D1,D3,C4),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(B2),
                                                  BitBoard::sqMask(C1,C2,A4)));
        ASSERT_EQUAL(BitBoard::sqMask(B1,D1,F1,B3,D3,F3,A4,C4,E4),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(B2,D2),
                                                  0));
    }

    // Bishops
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WBISHOP : Piece::BBISHOP;
        ASSERT_EQUAL(BitBoard::sqMask(A2,B2,C2,C3,D3,D4,E5,F6,G7,H8),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(A1,B1),
                                                  BitBoard::sqMask(E4)));
    }

    // Rooks
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WROOK : Piece::BROOK;
        ASSERT_EQUAL(BitBoard::sqMask(A1,B1,D1,E1,F1,G1,H1,
                                      A2,B2,C2,E2,F2,G2,H2,
                                      A3,B3,C3,D3,F3,G3,H3,
                                      C4,D4,C5,D5,C6,D6,C7,D7,C8,D8),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(C1,D2,E3),
                                                  BitBoard::sqMask(E4)));
    }

    // Queens
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WQUEEN : Piece::BQUEEN;
        ASSERT_EQUAL(BitBoard::sqMask(A1,B1,C1,D1,E1,F1,H1,
                                      A2,B2,C2,A3,A4,B4,B5,B6,B7,B8),
                     PathSearch::computeNeighbors(p,
                                                  BitBoard::sqMask(G1,B3),
                                                  BitBoard::sqMask(F2,G2,H2,C3,C4)));
    }
}

void
PathSearchTest::testShortestPath() {
    PathSearch ps(TextIO::startPosFEN);
    std::shared_ptr<PathSearch::ShortestPathData> spd;
    spd = ps.shortestPaths(Piece::WKING,
                           TextIO::getSquare("h8"),
                           BitBoard::sqMask(G2,G3,G4,G5,G6,G7,F7,E7,D7,C7,B7));
    ASSERT_EQUAL(~BitBoard::sqMask(G2,G3,G4,G5,G6,G7,F7,E7,D7,C7,B7), spd->fromSquares);
    ASSERT_EQUAL(0, spd->pathLen[H8]);
    ASSERT_EQUAL(13, spd->pathLen[A1]);
    ASSERT_EQUAL(12, spd->pathLen[F6]);

    spd = ps.shortestPaths(Piece::BKNIGHT, TextIO::getSquare("a1"), 0);
    ASSERT_EQUAL(~0ULL, spd->fromSquares);
    ASSERT_EQUAL(0, spd->pathLen[A1]);
    ASSERT_EQUAL(6, spd->pathLen[H8]);
    ASSERT_EQUAL(5, spd->pathLen[A8]);
    ASSERT_EQUAL(4, spd->pathLen[B2]);
    ASSERT_EQUAL(4, spd->pathLen[C3]);

    spd = ps.shortestPaths(Piece::WROOK, TextIO::getSquare("a1"), 0);
    ASSERT_EQUAL(~0ULL, spd->fromSquares);
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int d = ((x != 0) ? 1 : 0) + ((y != 0) ? 1 : 0);
            int sq = Position::getSquare(x, y);
            ASSERT_EQUAL(d, (int)(spd->pathLen[sq]));
        }
    }

    spd = ps.shortestPaths(Piece::WPAWN, TextIO::getSquare("d8"),
                           BitBoard::sqMask(D3,E2,F1));
    int expected[64] = {
        -1,-1,-1, 0,-1,-1,-1,-1,
        -1,-1, 1, 1, 1,-1,-1,-1,
        -1, 2, 2, 2, 2, 2,-1,-1,
         3, 3, 3, 3, 3, 3, 3,-1,
         4, 4, 4, 4, 4, 4, 4, 4,
         5, 5, 5,-1, 5, 5, 5, 5,
         5, 5, 5, 6,-1, 5, 5, 5,
         6, 6, 6, 6, 6,-1, 6, 6,
    };
    for (int i = 0; i < 64; i++) {
        ASSERT_EQUAL(expected[Position::mirrorY(i)], (int)spd->pathLen[i]);
        ASSERT_EQUAL(expected[Position::mirrorY(i)] >= 0, (spd->fromSquares & (1ULL << i)) != 0);
    }
}

void
PathSearchTest::testValidPieceCount() {
    auto isValid = [](const std::string fen) {
        Position pos = TextIO::readFEN(fen);
        bool ok = false;
        try {
            PathSearch::validatePieceCounts(pos);
            ok = true;
        } catch (const ChessParseError& e) {
        }
        return ok;
    };
    ASSERT(isValid(TextIO::startPosFEN));
    ASSERT(!isValid("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNNQKBNR w KQkq - 0 1"));
    ASSERT(isValid("rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQkq - 0 1"));
    ASSERT(!isValid("rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNNQKQNR w KQkq - 0 1"));
    ASSERT(isValid("rnbqkbnr/pppppppp/8/8/8/8/2PPPPPP/RNNQKQNR w KQkq - 0 1"));
    ASSERT(!isValid("rnbqkrnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    ASSERT(isValid("rnbqkrnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    ASSERT(!isValid("rnbqkrqr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    ASSERT(isValid("rnbqkrqr/p1pp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
}

void
PathSearchTest::testPawnReachable() {
    {
        PathSearch ps("rnbqkbnr/pppppppp/8/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        ASSERT_EQUAL(BitBoard::sqMask(A1), PathSearch::bPawnReachable[A1]);
        ASSERT_EQUAL(BitBoard::sqMask(A2,A1,B1), PathSearch::bPawnReachable[A2]);
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(   B2,C2,D2,E2,F2,G2,H2,
                                      A7,B7,C7,D7,E7,F7,G7,H7,
                                         A1,E1,H1,A8,E8,H8),
                     blocked);
    }
    {
        PathSearch ps("4k3/1p6/2P5/3P4/B3P3/3P1P2/2P3P1/4K3 w - - 0 1");
        Position pos = TextIO::readFEN("4k3/1p6/2P5/3P4/4P1B1/3P4/2P2PP1/4K3 w - - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(C2,G2,D3,E4,D5,C6,B7), blocked);
        ASSERT_EQUAL(INT_MAX, hScore(ps, TextIO::toFEN(pos)));
    }
    {
        PathSearch ps("rnbqkbnr/pppppppp/8/8/5P2/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
        ASSERT_EQUAL(INT_MAX, hScore(ps, TextIO::startPosFEN));
        ASSERT_EQUAL(2, hScore(ps, "r1bqkbnr/pppppppp/n7/8/8/5P2/PPPP1PPP/RNBQKBNR w KQkq - 0 1"));
        ASSERT(hScore(ps, "r1bqkbnr/pppppppp/n7/8/8/5P2/PPPP1PPP/RNBQKBNR b KQkq - 0 1") <= 9);
        ASSERT(hScore(ps, "r1bqkbnr/pppppppp/n7/8/8/5P2/PPPP1PPP/RNBQKBNR b KQkq - 0 1") >= 3);
    }
    {
        PathSearch ps("r1bqkbnr/pppppppp/8/8/8/5P2/PPPP1PPP/RNBQKBNR w KQkq - 3 6");
        ASSERT(hScore(ps, TextIO::startPosFEN) <= 10);
    }
    {
        PathSearch ps("2b1kqr1/p2p3p/3p4/p2PpP2/PpP2p2/6P1/8/RRB1KQ1N w - - 0 1");
        ASSERT(hScore(ps, TextIO::startPosFEN) <= 62);
    }
    {
        PathSearch ps("r2qk2r/1pp3p1/1p4p1/8/8/8/PPP3PP/RNBQKBNR w KQkq - 0 1");
        ASSERT_EQUAL(INT_MAX, hScore(ps, "r2qk2r/ppp3pp/8/8/8/8/PPPPPPPP/R2QKBNR w KQkq - 0 1"));
    }
    {
        PathSearch ps("8/rnbqkbnr/pppppppp/8/8/PPPPPPPP/RNBQKBNR/8 w - - 0 1");
        Position pos = TextIO::readFEN("rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR w KQkq - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(D3), blocked);
        ASSERT_EQUAL(44, hScore(ps, TextIO::toFEN(pos)));
    }
}

void
PathSearchTest::testCastling() {
    {
        PathSearch ps("rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w KQkq - 0 1");
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(E2,E7,A1,E1,H1,A8,E8,H8), blocked);
    }
    {
        PathSearch ps("rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w K - 0 1");
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(E2,E7,E1,H1), blocked);
    }
    {
        PathSearch ps("rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w Q - 0 1");
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(E2,E7,E1,A1), blocked);
    }
    {
        PathSearch ps("rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w k - 0 1");
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(E2,E7,E8,H8), blocked);
    }
    {
        PathSearch ps("rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w q - 0 1");
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT(res);
        ASSERT_EQUAL(BitBoard::sqMask(E2,E7,E8,A8), blocked);
    }
}

void
PathSearchTest::testReachable() {
    { // Queen is trapped, can not reach d3
        PathSearch ps("rnbqkbnr/pppppppp/8/8/8/2Q5/1PPPPPPP/1NB1KBNR w Kkq - 0 1");
        ASSERT_EQUAL(INT_MAX, hScore(ps, TextIO::startPosFEN));
    }
    { // Queen is trapped, can not reach d3
        PathSearch ps("rnbqkbnr/pppppppp/8/8/8/2Q5/1PPPPPP1/1NB1KBN1 w kq - 0 1");
        ASSERT_EQUAL(INT_MAX, hScore(ps, TextIO::startPosFEN));
    }
    { // Unreachable, 2 promotions required, only 1 available
        PathSearch ps("B3k2B/1pppppp1/8/8/8/8/PPPP1PPP/RN1QK1NR w KQ - 0 1");
        ASSERT_EQUAL(INT_MAX, hScore(ps, TextIO::startPosFEN));
    }
    { // Unreachable, only 1 pawn promotion available, but need to promote to
      // both knight (to satisfy piece counts) and bishop (existing bishops can
      // not reach target square).
        PathSearch ps("Nn1qk2B/1pppppp1/8/8/8/8/PPPP1PPP/RN1QK1NR w KQ - 0 1");
        ASSERT_EQUAL(INT_MAX, hScore(ps, TextIO::startPosFEN));
    }
}

void
PathSearchTest::testRemainingMoves() {
    {
        PathSearch ps("rnbqkbnr/pppppppp/8/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        ASSERT_EQUAL(4, hScore(ps, TextIO::startPosFEN));
    }
    {
        PathSearch ps("rnbqk1nr/b1pp1ppp/1p6/4p3/8/5N2/PPPPPPPP/R1BQKB1R w KQkq - 0 1");
        ASSERT_EQUAL(8, hScore(ps, TextIO::startPosFEN));
    }
    { // Reachable, 2 promotions required and available, 6 captured required and available
        PathSearch ps("B3k2B/1pppppp1/8/8/8/8/PPP2PPP/RN1QK1NR w KQ - 0 1");
        ASSERT(hScore(ps, TextIO::startPosFEN) >= 20);
        ASSERT(hScore(ps, TextIO::startPosFEN) <= 76);
    }
}


cute::suite
PathSearchTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testMaterial));
    s.push_back(CUTE(testNeighbors));
    s.push_back(CUTE(testShortestPath));
    s.push_back(CUTE(testValidPieceCount));
    s.push_back(CUTE(testPawnReachable));
    s.push_back(CUTE(testCastling));
    s.push_back(CUTE(testReachable));
    s.push_back(CUTE(testRemainingMoves));

    // FIXME!! Add test that tries to reach starting position with castling rights removed. (16 plies)

    return s;
}
