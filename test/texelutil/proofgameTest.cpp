/*
    Texel - A UCI chess engine.
    Copyright (C) 2015-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * proofgameTest.cpp
 *
 *  Created on: Aug 22, 2015
 *      Author: petero
 */

#include "proofgameTest.hpp"
#include "proofgame.hpp"
#include "proofkernel.hpp"
#include "proofgamefilter.hpp"
#include "pkseq.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "posutil.hpp"
#include <climits>

#include "gtest/gtest.h"

// Enable to also run slow tests
//#define RUN_SLOW_TESTS

void
ProofGameTest::checkBlockedConsistency(ProofGame& ps, Position& pos) {
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
            ASSERT_EQ(INT_MAX, hs);
            pos.unMakeMove(moves[i], ui);
        }
    }
}

static std::string
mirrorFenY(const std::string& fen) {
    Position pos = TextIO::readFEN(fen);
    pos = PosUtil::swapColors(pos);
    return TextIO::toFEN(pos);
}

int
ProofGameTest::hScore(const std::string& initFen, const std::string& goalFen,
                      bool useNonAdmissible, bool testMirrorY) {
    Position initPos = TextIO::readFEN(initFen);
    ProofGame ps(initFen, goalFen, true, {});
    ps.useNonAdmissible = useNonAdmissible;
    {
        Position pos0 = TextIO::readFEN(TextIO::startPosFEN);
        checkBlockedConsistency(ps, pos0);
    }
    checkBlockedConsistency(ps, initPos);
    const int score = ps.distLowerBound(initPos);
    EXPECT_GE(score, 0);

    if (testMirrorY) {
        int score2 = hScore(mirrorFenY(initFen), mirrorFenY(goalFen), useNonAdmissible, false);
        EXPECT_EQ(score, score2);
    }

    return score;
}

TEST(ProofGameTest, testMaterial) {
    ProofGameTest::testMaterial();
}

void
ProofGameTest::testMaterial() {
    {
        std::string goalFen = TextIO::startPosFEN;
        ASSERT_EQ(0, hScore(goalFen, TextIO::startPosFEN));
        ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", goalFen));
        ASSERT_LE(hScore("r1bqkbnr/pppppppp/n7/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1", goalFen), 1);
        ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w Kkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RN1QKBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pp1ppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rn1qkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", goalFen));
        ASSERT_EQ(INT_MAX, hScore("rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1", goalFen));
    }
    {
        std::string goal("1nbqkbnr/1ppppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQk - 0 1");
        ASSERT_EQ(0, hScore(goal, goal));
        ASSERT_LE(hScore("1nbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQk - 0 1", goal), 40);
    }
    ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPP1/RNBQKBNR w KQkq - 0 1",
                              "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPP1/RNNQKBNR w KQkq - 0 1"));
    ASSERT_LE(hScore(TextIO::startPosFEN, "1nbqkbnr/p1pppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQk - 0 1"), 20);
    ASSERT_LE(hScore(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1"), 4);
    ASSERT_LE(hScore(TextIO::startPosFEN, "rnbqk1nr/b1pp1ppp/1p6/4p3/8/5N2/PPPPPPPP/R1BQKB1R w KQkq - 0 1"), 8);
    {
        std::string goal("rnbqkbnr/ppp2ppp/8/8/8/8/PPP2PPP/RNBQKBNR w KQkq - 0 1");
        ASSERT_GE(hScore(TextIO::startPosFEN, goal), 4);
        ASSERT_LE(hScore(TextIO::startPosFEN, goal), 10);
    }
}

TEST(ProofGameTest, testNeighbors) {
    ProofGameTest::testNeighbors();
}

void
ProofGameTest::testNeighbors() {
    // Pawns
    ASSERT_EQ(BitBoard::sqMask(B2,C2,A3,B3,C3,D3),
                 ProofGame::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              0));
    ASSERT_EQ(BitBoard::sqMask(A2,B2,C2,D2),
                 ProofGame::computeNeighbors(Piece::BPAWN,
                                              BitBoard::sqMask(B1,C1),
                                              0));
    ASSERT_EQ(BitBoard::sqMask(A3,C3,D3,C2),
                 ProofGame::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              BitBoard::sqMask(B3)));
    ASSERT_EQ(BitBoard::sqMask(A3,B3,C3,D3,C2),
                 ProofGame::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              BitBoard::sqMask(B2)));
    ASSERT_EQ(BitBoard::sqMask(A3,D3),
                 ProofGame::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B4,C4),
                                              BitBoard::sqMask(B3,C3)));
    ASSERT_EQ(0,
                 ProofGame::computeNeighbors(Piece::WPAWN,
                                              BitBoard::sqMask(B1),
                                              0));
    ASSERT_EQ(0,
                 ProofGame::computeNeighbors(Piece::BPAWN,
                                              BitBoard::sqMask(A8),
                                              0));
    ASSERT_EQ(BitBoard::sqMask(A2,B2),
                 ProofGame::computeNeighbors(Piece::BPAWN,
                                              BitBoard::sqMask(A1),
                                              0));
    // Kings
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WKING : Piece::BKING;
        ASSERT_EQ(BitBoard::sqMask(B2),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(A1),
                                                  BitBoard::sqMask(B1,A2)));
        ASSERT_EQ(BitBoard::sqMask(B1,A2,B2),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(A1),
                                                  0));
    }

    // Knights
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WKNIGHT : Piece::BKNIGHT;
        ASSERT_EQ(BitBoard::sqMask(D1,D3,A4),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(B2),
                                                  BitBoard::sqMask(C1,C2,C4)));
        ASSERT_EQ(BitBoard::sqMask(D1,D3,C4),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(B2),
                                                  BitBoard::sqMask(C1,C2,A4)));
        ASSERT_EQ(BitBoard::sqMask(B1,D1,F1,B3,D3,F3,A4,C4,E4),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(B2,D2),
                                                  0));
    }

    // Bishops
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WBISHOP : Piece::BBISHOP;
        ASSERT_EQ(BitBoard::sqMask(A2,B2,C2,C3,D3,D4,E5,F6,G7,H8),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(A1,B1),
                                                  BitBoard::sqMask(E4)));
    }

    // Rooks
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WROOK : Piece::BROOK;
        ASSERT_EQ(BitBoard::sqMask(A1,B1,D1,E1,F1,G1,H1,
                                      A2,B2,C2,E2,F2,G2,H2,
                                      A3,B3,C3,D3,F3,G3,H3,
                                      C4,D4,C5,D5,C6,D6,C7,D7,C8,D8),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(C1,D2,E3),
                                                  BitBoard::sqMask(E4)));
    }

    // Queens
    for (int i = 0; i < 2; i++) {
        Piece::Type p = (i == 0) ? Piece::WQUEEN : Piece::BQUEEN;
        ASSERT_EQ(BitBoard::sqMask(A1,B1,C1,D1,E1,F1,H1,
                                      A2,B2,C2,A3,A4,B4,B5,B6,B7,B8),
                     ProofGame::computeNeighbors(p,
                                                  BitBoard::sqMask(G1,B3),
                                                  BitBoard::sqMask(F2,G2,H2,C3,C4)));
    }
}

void
ProofGameTest::comparePaths(Piece::Type p, int sq, U64 blocked, int maxMoves,
                             const std::vector<int>& expected, bool testColorReversed) {
    ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN, true, {});
    auto spd = ps.shortestPaths(p, sq, blocked, maxMoves);
    for (int i = 0; i < 64; i++) {
        ASSERT_EQ(expected[Square::mirrorY(i)], (int)spd->pathLen[i]);
        ASSERT_EQ(expected[Square::mirrorY(i)] >= 0, (spd->fromSquares & (1ULL << i)) != 0);
    }

    if (testColorReversed) {
        Piece::Type oP = (Piece::Type)(Piece::isWhite(p) ? Piece::makeBlack(p) : Piece::makeWhite(p));
        int oSq = Square::getSquare(Square::getX(sq), 7 - Square::getY(sq));
        U64 oBlocked = 0;
        std::vector<int> oExpected(64);
        for (int s = 0; s < 64; s++) {
            int oS = Square::getSquare(Square::getX(s), 7 - Square::getY(s));
            if ((1ULL << s) & blocked)
                oBlocked |= (1ULL << oS);
            oExpected[oS] = expected[s];
        }
        comparePaths(oP, oSq, oBlocked, maxMoves, oExpected, false);
    }
}

TEST(ProofGameTest, testShortestPath) {
    ProofGameTest::testShortestPath();
}

void
ProofGameTest::testShortestPath() {
    ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN, true, {});
    std::shared_ptr<ProofGame::ShortestPathData> spd;
    spd = ps.shortestPaths(Piece::WKING, H8,
                           BitBoard::sqMask(G2,G3,G4,G5,G6,G7,F7,E7,D7,C7,B7), 8);
    ASSERT_EQ(~BitBoard::sqMask(G2,G3,G4,G5,G6,G7,F7,E7,D7,C7,B7), spd->fromSquares);
    ASSERT_EQ(0, spd->pathLen[H8]);
    ASSERT_EQ(13, spd->pathLen[A1]);
    ASSERT_EQ(12, spd->pathLen[F6]);

    spd = ps.shortestPaths(Piece::BKNIGHT, A1, 0, 8);
    ASSERT_EQ(~0ULL, spd->fromSquares);
    ASSERT_EQ(0, spd->pathLen[A1]);
    ASSERT_EQ(6, spd->pathLen[H8]);
    ASSERT_EQ(5, spd->pathLen[A8]);
    ASSERT_EQ(4, spd->pathLen[B2]);
    ASSERT_EQ(4, spd->pathLen[C3]);

    spd = ps.shortestPaths(Piece::WROOK, A1, 0, 8);
    ASSERT_EQ(~0ULL, spd->fromSquares);
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int d = ((x != 0) ? 1 : 0) + ((y != 0) ? 1 : 0);
            int sq = Square::getSquare(x, y);
            ASSERT_EQ(d, (int)(spd->pathLen[sq]));
        }
    }

    spd = ps.shortestPaths(Piece::WPAWN, D8, BitBoard::sqMask(D3,E2,F1), 8);
    auto spd5 = ps.shortestPaths(Piece::WPAWN, D8, BitBoard::sqMask(D3,E2,F1), 5);
    ProofGame::ShortestPathData spd2;
    ProofGame::shortestPaths(Piece::WPAWN, D8, BitBoard::sqMask(D3,E2,F1), spd5.get(), spd2);
    ASSERT_EQ(spd->fromSquares, spd2.fromSquares);
    for (int i = 0; i < 64; i++) {
        ASSERT_EQ(spd->pathLen[i], spd2.pathLen[i]);
    }

    std::vector<int> expected[7] = {
        {   -1,-1,-1, 0,-1,-1,-1,-1,
            -1,-1,-1, 1,-1,-1,-1,-1,
            -1,-1,-1, 2,-1,-1,-1,-1,
            -1,-1,-1, 3,-1,-1,-1,-1,
            -1,-1,-1, 4,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,
        },
        {   -1,-1,-1, 0,-1,-1,-1,-1,
            -1,-1, 1, 1, 1,-1,-1,-1,
            -1,-1, 2, 2, 2,-1,-1,-1,
            -1,-1, 3, 3, 3,-1,-1,-1,
            -1,-1, 4, 4, 4,-1,-1,-1,
            -1,-1, 5,-1, 5,-1,-1,-1,
            -1,-1, 5,-1,-1,-1,-1,-1,
            -1,-1, 6,-1,-1,-1,-1,-1,
        },
        {   -1,-1,-1, 0,-1,-1,-1,-1,
            -1,-1, 1, 1, 1,-1,-1,-1,
            -1, 2, 2, 2, 2, 2,-1,-1,
            -1, 3, 3, 3, 3, 3,-1,-1,
            -1, 4, 4, 4, 4, 4,-1,-1,
            -1, 5, 5,-1, 5, 5,-1,-1,
            -1, 5, 5, 6,-1, 5,-1,-1,
            -1, 6, 6, 6,-1,-1,-1,-1,
        },
        {   -1,-1,-1, 0,-1,-1,-1,-1,
            -1,-1, 1, 1, 1,-1,-1,-1,
            -1, 2, 2, 2, 2, 2,-1,-1,
             3, 3, 3, 3, 3, 3, 3,-1,
             4, 4, 4, 4, 4, 4, 4,-1,
             5, 5, 5,-1, 5, 5, 5,-1,
             5, 5, 5, 6,-1, 5, 5,-1,
             6, 6, 6, 6, 6,-1, 6,-1,
        },
        {   -1,-1,-1, 0,-1,-1,-1,-1,
            -1,-1, 1, 1, 1,-1,-1,-1,
            -1, 2, 2, 2, 2, 2,-1,-1,
             3, 3, 3, 3, 3, 3, 3,-1,
             4, 4, 4, 4, 4, 4, 4, 4,
             5, 5, 5,-1, 5, 5, 5, 5,
             5, 5, 5, 6,-1, 5, 5, 5,
             6, 6, 6, 6, 6,-1, 6, 6,
        },
        {   -1,-1,-1, 0,-1,-1,-1,-1,
            -1,-1, 1, 1, 1,-1,-1,-1,
            -1, 2, 2, 2, 2, 2,-1,-1,
             3, 3, 3, 3, 3, 3, 3,-1,
             4, 4, 4, 4, 4, 4, 4, 4,
             5, 5, 5,-1, 5, 5, 5, 5,
             5, 5, 5, 6,-1, 5, 5, 5,
             6, 6, 6, 6, 6,-1, 6, 6,
        },
        {   -1,-1,-1, 0,-1,-1,-1,-1,
            -1,-1, 1, 1, 1,-1,-1,-1,
            -1, 2, 2, 2, 2, 2,-1,-1,
             3, 3, 3, 3, 3, 3, 3,-1,
             4, 4, 4, 4, 4, 4, 4, 4,
             5, 5, 5,-1, 5, 5, 5, 5,
             5, 5, 5, 6,-1, 5, 5, 5,
             6, 6, 6, 6, 6,-1, 6, 6, }
    };
    for (int maxCapt = 0; maxCapt < 16; maxCapt++) {
        int tIdx = std::min(maxCapt, 6);
        comparePaths(Piece::WPAWN, D8, BitBoard::sqMask(D3,E2,F1),
                     maxCapt, expected[tIdx]);
    }

    {
        std::vector<int> expected = {
            -1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1, 0,
            -1,-1,-1,-1,-1,-1, 1, 1,
            -1,-1,-1,-1,-1, 2,-1,-1,
            -1,-1,-1,-1, 3, 3, 3,-1,
            -1,-1,-1,-1, 4, 4, 4,-1,
        };
        comparePaths(Piece::WPAWN, H5, BitBoard::sqMask(G3,H3), 3, expected);
    }
    {
        Piece::Type p = Piece::WQUEEN;
        U64 blocked = BitBoard::sqMask(B8);
        spd = ps.shortestPaths(p, H8, blocked, 8);
        ASSERT_EQ(BitBoard::sqMask(A1,H1), spd->getNextSquares(p, A8, blocked));
        ASSERT_EQ(BitBoard::sqMask(C8,D8,G7,H7,E5,H2,C3), spd->getNextSquares(p, C7, blocked));
    }
    {
        Piece::Type p = Piece::WROOK;
        U64 blocked = BitBoard::sqMask(B8);
        spd = ps.shortestPaths(p, H8, blocked, 8);
        ASSERT_EQ(BitBoard::sqMask(A1,A2,A3,A4,A5,A6,A7), spd->getNextSquares(p, A8, blocked));
        ASSERT_EQ(BitBoard::sqMask(C8,H2), spd->getNextSquares(p, C2, blocked));
    }
    {
        Piece::Type p = Piece::WBISHOP;
        U64 blocked = BitBoard::sqMask(C3);
        spd = ps.shortestPaths(p, H8, blocked, 8);
        ASSERT_EQ(BitBoard::sqMask(H8), spd->getNextSquares(p, D4, blocked));
        ASSERT_EQ(BitBoard::sqMask(B2), spd->getNextSquares(p, A1, blocked));
        ASSERT_EQ(BitBoard::sqMask(A3,C1), spd->getNextSquares(p, B2, blocked));
        ASSERT_EQ(BitBoard::sqMask(C5,D6,E7,F8), spd->getNextSquares(p, A3, blocked));
        ASSERT_EQ(BitBoard::sqMask(F6), spd->getNextSquares(p, E7, blocked));
        ASSERT_EQ(BitBoard::sqMask(H8), spd->getNextSquares(p, F6, blocked));
    }
    {
        Piece::Type p = Piece::WKNIGHT;
        U64 blocked = BitBoard::sqMask(E3);
        spd = ps.shortestPaths(p, D6, blocked, 8);
        ASSERT_EQ(BitBoard::sqMask(B2,C3,F2), spd->getNextSquares(p, D1, blocked));
        ASSERT_EQ(BitBoard::sqMask(B5,E4), spd->getNextSquares(p, C3, blocked));
    }
    {
        Piece::Type p = Piece::WPAWN;
        U64 blocked = BitBoard::sqMask(D3);
        spd = ps.shortestPaths(p, D8, blocked, 2);
        ASSERT_EQ(BitBoard::sqMask(C3,E3), spd->getNextSquares(p, D2, blocked));
        ASSERT_EQ(BitBoard::sqMask(D4,E4,F4), spd->getNextSquares(p, E3, blocked));
        ASSERT_EQ(BitBoard::sqMask(F5,E5), spd->getNextSquares(p, F4, blocked));
        ASSERT_EQ(BitBoard::sqMask(E7), spd->getNextSquares(p, F6, blocked));
    }
    {
        Piece::Type p = Piece::BPAWN;
        U64 blocked = 0;
        spd = ps.shortestPaths(p, A1, blocked, 6);
        ASSERT_EQ(BitBoard::sqMask(A5), spd->getNextSquares(p, A7, blocked));
        ASSERT_EQ(BitBoard::sqMask(A5,B5), spd->getNextSquares(p, A6, blocked));
        ASSERT_EQ(BitBoard::sqMask(A1), spd->getNextSquares(p, B2, blocked));
        ASSERT_EQ(BitBoard::sqMask(A2,B2), spd->getNextSquares(p, B3, blocked));
    }
}

TEST(ProofGameTest, testValidPieceCount) {
    ProofGameTest::testValidPieceCount();
}

void
ProofGameTest::testValidPieceCount() {
    auto isValid = [](const std::string fen) {
        Position pos = TextIO::readFEN(fen);
        bool ok = false;
        try {
            ProofGame::validatePieceCounts(pos);
            ok = true;
        } catch (const ChessError&) {
        }
        return ok;
    };
    ASSERT_TRUE( isValid(TextIO::startPosFEN));
    ASSERT_TRUE(!isValid("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNNQKBNR w KQkq - 0 1"));
    ASSERT_TRUE( isValid("rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNNQKBNR w KQkq - 0 1"));
    ASSERT_TRUE(!isValid("rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/RNNQKQNR w KQkq - 0 1"));
    ASSERT_TRUE( isValid("rnbqkbnr/pppppppp/8/8/8/8/2PPPPPP/RNNQKQNR w KQkq - 0 1"));
    ASSERT_TRUE(!isValid("rnbqkrnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    ASSERT_TRUE( isValid("rnbqkrnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    ASSERT_TRUE(!isValid("rnbqkrqr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    ASSERT_TRUE( isValid("rnbqkrqr/p1pp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
}

TEST(ProofGameTest, testPawnReachable) {
    ProofGameTest::testPawnReachable();
}

void
ProofGameTest::testPawnReachable() {
    {
        const int maxCapt = 5; // Max number of captures relevant for pawn movements
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y <= 7; y += 7) {
                ASSERT_EQ(0, ProofGame::wPawnReachable[Square::getSquare(x, y)][maxCapt]);
                ASSERT_EQ(0, ProofGame::wPawnReachable[Square::getSquare(x, y)][maxCapt]);
            }
        }
        ASSERT_EQ(BitBoard::sqMask(A2), ProofGame::bPawnReachable[A2][maxCapt]);
        ASSERT_EQ(BitBoard::sqMask(A3,A2,B2), ProofGame::bPawnReachable[A3][maxCapt]);
        ASSERT_EQ(BitBoard::sqMask(C3, C2), ProofGame::bPawnReachable[C3][0]);
        ASSERT_EQ(BitBoard::sqMask(B4, B5, B6, B7), ProofGame::wPawnReachable[B4][0]);
        ASSERT_EQ(BitBoard::sqMask(B4, A5, B5, C5, A6, B6, C6, A7, B7, C7),
                  ProofGame::wPawnReachable[B4][1]);
        ASSERT_EQ(BitBoard::sqMask(B4, A5, B5, C5, A6, B6, C6, D6, A7, B7, C7, D7),
                  ProofGame::wPawnReachable[B4][2]);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/pppppppp/8/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1", true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(   B2,C2,D2,E2,F2,G2,H2,
                                      A7,B7,C7,D7,E7,F7,G7,H7,
                                      A1,C1,D1,E1,F1,H1,
                                      A8,C8,D8,E8,F8,H8),
                     blocked);
    }
    {
        std::string start("4k3/1p6/2P5/3P4/4P1B1/3P4/2P2PP1/4K3 w - - 0 1");
        std::string goal("4k3/1p6/2P5/3P4/B3P3/3P1P2/2P3P1/4K3 w - - 0 1");
        Position pos = TextIO::readFEN(start);
        ProofGame ps(start, goal, true, {});
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(C2,G2,D3,E4,D5,C6,B7), blocked);
        ASSERT_EQ(INT_MAX, hScore(start, goal));
    }
    {
        std::string goal("rnbqkbnr/pppppppp/8/8/5P2/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
        ASSERT_EQ(INT_MAX, hScore(TextIO::startPosFEN, goal));
        ASSERT_EQ(2, hScore("r1bqkbnr/pppppppp/n7/8/8/5P2/PPPP1PPP/RNBQKBNR w KQkq - 0 1", goal));
        ASSERT_LE(hScore("r1bqkbnr/pppppppp/n7/8/8/5P2/PPPP1PPP/RNBQKBNR b KQkq - 0 1", goal), 9);
        ASSERT_GE(hScore("r1bqkbnr/pppppppp/n7/8/8/5P2/PPPP1PPP/RNBQKBNR b KQkq - 0 1", goal), 3);
    }
    ASSERT_LE(hScore(TextIO::startPosFEN, "r1bqkbnr/pppppppp/8/8/8/5P2/PPPP1PPP/RNBQKBNR w KQkq - 3 6"), 10);
    ASSERT_LE(hScore(TextIO::startPosFEN, "2b1kqr1/p2p3p/3p4/p2PpP2/PpP2p2/6P1/8/RRB1KQ1N w - - 0 1"), 62);
    ASSERT_EQ(INT_MAX, hScore("r2qk2r/ppp3pp/8/8/8/8/PPPPPPPP/R2QKBNR w KQkq - 0 1",
                              "r2qk2r/1pp3p1/1p4p1/8/8/8/PPP3PP/RNBQKBNR w KQkq - 0 1"));
    {
        std::string start("rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR w KQkq - 0 1");
        std::string goal("8/rnbqkbnr/pppppppp/8/8/PPPPPPPP/RNBQKBNR/8 w - - 0 1");
        Position pos = TextIO::readFEN(start);
        ProofGame ps(start, goal, true, {});
        U64 blocked;
        bool res = ps.computeBlocked(pos, blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(D3), blocked);
        ASSERT_EQ(44, hScore(start, goal));
    }

    // Reachable, pawn can not reach goal square, but can be promoted to piece that can
    ASSERT_LE(hScore(TextIO::startPosFEN, "rnbqkbnr/ppp1ppp1/2p5/8/8/8/PPPPPPP1/RNBQKBNR w KQq - 0 1"), 24);

    // Not reachable, white pawn can not reach square where it needs to be captured
    ASSERT_EQ(INT_MAX, hScore(TextIO::startPosFEN, "rnbqkbnr/ppp1pppp/2p5/8/8/8/PPPPPPP1/RNBQKBNR w KQkq - 0 1"));

    // Not reachable, white c1 bishop can not reach required capture square a6.
    ASSERT_EQ(INT_MAX, hScore(TextIO::startPosFEN, "rnbqkbnr/p1pppppp/p7/8/8/3P4/PPP1PPPP/RN1QKBNR w KQkq - 0 1"));

    { // Pg7 cannot move so bishop cannot reach h8
        ASSERT_EQ(INT_MAX, hScore("8/3k2P1/8/4P3/7B/8/4K3/8 w - - 0 1",
                                  "7B/2k3P1/8/4N3/8/8/4K3/8 w - - 0 1"));
        ASSERT_EQ(INT_MAX, hScore("8/2k3P1/4P3/8/7B/8/4K3/8 w - - 0 1",
                                  "7B/2k3P1/8/4N3/8/8/4K3/8 w - - 0 1"));
        ASSERT_GE(hScore("1nn5/2k3P1/8/4P3/7B/8/4K3/8 w - - 0 1",
                         "7B/2k3P1/8/4N3/8/8/4K3/8 w - - 0 1"), 14);
        ASSERT_LE(hScore("1nn5/2k3P1/8/4P3/7B/8/4K3/8 w - - 0 1",
                         "7B/2k3P1/8/4N3/8/8/4K3/8 w - - 0 1"), 16);
        ASSERT_EQ(INT_MAX, hScore("8/3k2P1/8/8/6PB/8/4K3/8 w - - 0 1",
                                  "7B/3k2P1/8/8/6P1/8/4K3/8 w - - 0 1"));
    }

    // Pd3,f3,g2 blocked, so Bf1 cannot get to g6
    ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/1P1P1P2/P1P1P1PP/RNBQKBNR w KQkq - 0 1",
                              "rnbqkbnr/ppppp1pp/6p1/8/2P1P3/1P1P1P2/P5PP/RNBQK1NR w KQkq - 0 1"));

    ASSERT_EQ(6, hScore(TextIO::startPosFEN, "rnbqkbnr/1p1ppppp/1p6/p7/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1"));
    ASSERT_EQ(INT_MAX, hScore("4b3/4bRPN/1P2pQB1/1r1qRnK1/1BqPr1b1/1q1r1QB1/N1b3N1/1k5n w - - 30 83",
                              "b3b2B/4bRPN/1P2pQB1/1r1qRnK1/bB2r3/3r1QB1/N1q1q1N1/1k5n b - - 0 1"));

    {
        std::string init = "rnb1kbnr/p1pp1p1p/8/1P6/3P1p1p/8/1P1PP1P1/RNBQKBNR w KQkq - 0 8";
        std::string goal = "1r2R2b/2RbR1nn/nN1p4/1b1n1qb1/N1BP4/1N3NBr/3P2Q1/1k3KBq b - - 0 1";
        ProofGame ps(init, goal, true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(init), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(D2,D4), blocked);
        ASSERT_GE(hScore(init, goal), 107);
    }

#if 0
    // Reachable by en passant capture
    ASSERT_LE(hScore("rnbqkbnr/p1pppppp/8/8/1pP5/8/PP1PPPPP/RNBQKBNR b KQkq c3 0 1",
                     "rnbqkbnr/p1pppppp/8/8/8/2p5/PP1PPPPP/RNBQKBNR w KQkq - 0 1"), 1);
#endif

    ASSERT_GE(hScore("1nn1k3/8/4p3/8/8/4P3/8/4K3 w - - 0 1",
                     "4k3/8/8/4P3/4p3/8/8/4K3 w - - 0 1"), 4);
    ASSERT_LE(hScore("1nn1k3/8/4p3/8/8/4P3/8/4K3 w - - 0 1",
                     "4k3/8/8/4P3/4p3/8/8/4K3 w - - 0 1"), 18);
    ASSERT_LE(hScore("4k3/7p/4p3/8/7P/4P3/7P/4K3 w - - 0 1",
                     "4k3/7p/4p2P/8/7P/4P3/8/4K3 w - - 0 1"), INT_MAX);
    ASSERT_EQ(INT_MAX, hScore("4k3/7p/4p3/8/P6P/p3P3/P6P/4K3 w - - 0 1",
                              "4k3/7p/4p3/P7/P6P/p3P3/7P/4K3 w - - 0 1"));
    ASSERT_EQ(8, hScore("3k4/8/5P2/5p2/8/3K1P2/8/8 w - - 0 1",
                        "3k4/8/5N2/5p2/8/3K1P2/8/8 w - - 0 1"));
}

TEST(ProofGameTest, testBlocked) {
    ProofGameTest::testBlocked();
}

void
ProofGameTest::testBlocked() {
    {
        std::string start("5Nkr/1bpnbpp1/2P1pq1p/p7/1p2PBP1/P2P1P2/1PQ1B2P/RN1K3R b - - 0 20");
        std::string goal("2r2rk1/1bPn1pp1/4pq1p/p7/1p2PBPb/P4P2/1PQNB2P/R2K3R w - - 1 21");
        ProofGame ps(start, goal, true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(B2,H2,A3,F3,E4,G4,E6,H6,F7,G7), blocked);
        int s = hScore(start, goal);
        ASSERT_LE(s, 35);
        ASSERT_GE(s, 15);
    }
}

TEST(ProofGameTest, testCastling) {
    ProofGameTest::testCastling();
}

void
ProofGameTest::testCastling() {
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w KQkq - 0 1", true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,A1,E1,H1,A8,E8,H8), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w K - 0 1", true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E1,H1), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w Q - 0 1", true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E1,A1), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w k - 0 1", true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E8,H8), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w q - 0 1", true, {});
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E8,A8), blocked);
    }
    EXPECT_THROW({ // Invalid castling rights
        ProofGame ps(TextIO::startPosFEN,
                     "1nbq1Rnr/1p1pk2p/5p2/3p4/1b3p1p/p7/2P1PK2/RNBQ1BNR w q - 0 1", false, {});
    }, ChessError);
}

TEST(ProofGameTest, testReachable) {
    ProofGameTest::testReachable();
}

void
ProofGameTest::testReachable() {
    // Queen is trapped, can not reach d3
    ASSERT_EQ(INT_MAX, hScore(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/2Q5/1PPPPPPP/1NB1KBNR w Kkq - 0 1"));

    // Queen is trapped, can not reach d3
    ASSERT_EQ(INT_MAX, hScore(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/2Q5/1PPPPPP1/1NB1KBN1 w kq - 0 1"));

    // Unreachable, 2 promotions required, only 1 available
    ASSERT_EQ(INT_MAX, hScore(TextIO::startPosFEN, "B3k2B/1pppppp1/8/8/8/8/PPPP1PPP/RN1QK1NR w KQ - 0 1"));

    // Unreachable, only 1 pawn promotion available, but need to promote to
    // both knight (to satisfy piece counts) and bishop (existing bishops can
    // not reach target square).
    ASSERT_EQ(INT_MAX, hScore(TextIO::startPosFEN, "Nn1qk2B/1pppppp1/8/8/8/8/PPPP1PPP/RN1QK1NR w KQ - 0 1"));

    // Unreachable, too many captures needed to be able to promote pawn to knight.
    ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq - 0 1",
                              "rnbqk1nr/pppp1ppp/8/2b5/8/8/PPPPPPP1/RNBQKBNR w KQkq - 0 1"));

    { // Reachable, use promotion to get bishop through blocking boundary
        std::string goal("r1bqkbnr/B1pppppp/1p6/8/8/1P6/2PPPPPP/RN1QKBNR w KQkq - 0 1");
        ASSERT_GE(hScore("rnbqkbnr/2pppppp/1p6/8/8/1P6/P1PPPPPP/RNBQKBNR w KQkq - 0 1", goal), 12);
        ASSERT_LE(hScore("rnbqkbnr/2pppppp/1p6/8/8/1P6/P1PPPPPP/RNBQKBNR w KQkq - 0 1", goal), 16);
    }
    // Reachable, capture blocked bishop and promote a new bishop on the same square
    ASSERT_LE(hScore("B2rk3/1ppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1",
                     "B3k3/1ppppppp/3r4/8/8/8/1PPPPPPP/4K3 w - - 0 1"), 12);

    // Not reachable, bishop can not reach goal square, no promotion possible
    ASSERT_EQ(INT_MAX, hScore("3rk3/1ppppppp/B7/8/8/8/1PPPPPPP/4K3 w - - 0 1",
                              "B3k3/1ppppppp/3r4/8/8/8/1PPPPPPP/4K3 w - - 0 1"));

    // Reachable, one promotion needed and available
    ASSERT_LE(hScore(TextIO::startPosFEN, "rnbqkbnB/pp1pppp1/1p6/8/8/1P6/P1PPPPP1/RN1QKBNR w KQq - 0 1"), 20);

    // Unreachable, no promotion possible, can not reach capture square
    ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPB/RNBQK1NR w KQkq - 0 1",
                              "rnbqkbn1/p1ppppp1/p4r2/8/8/8/PPPP1PP1/RNBQK1NR w KQq - 0 1"));

    // Unreachable, too many captures needed to be able to promote pawn to knight.
    ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq - 0 1",
                              "rnbq2nr/pppkb1pp/3pp3/8/8/8/PPPPPPP1/RNBQKBNR w KQ - 0 1"));

    ASSERT_EQ(INT_MAX, hScore("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPB/RNBQK1NR w KQkq - 0 1",
                              "rnbqkbn1/p1ppppp1/p7/8/8/8/PPP1PPP1/RNBQK1NR w KQq - 0 1"));

    { // Trivially reachable, but position only has one reverse move.
        std::string start = "4k3/1p6/8/2P5/B7/1P6/8/4K3 b - - 0 3";
        std::string goal  = "4k3/8/8/1pP5/B7/1P6/8/4K3 w - b6 0 2";
        ASSERT_EQ(0, hScore(goal, goal));
        ASSERT_EQ(0, hScore(start, goal));

        ProofGame ps(start, goal, true, {});
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(1, best);
        ASSERT_EQ(1, result.proofGame.size());
    }

    {
        std::string start = "rnbqkbnr/1p1pppp1/8/3p3p/1p2PP2/8/P1PP2PP/RNBQK1NR w KQkq - 0 6";
        std::string goal  = "r1bqkbnr/1p1pp1p1/8/5P2/1p1ppPp1/8/P1P4P/RNBQK1NR w KQkq - 0 1";
        ASSERT_GE(hScore(start, goal), 14);
    }

    ASSERT_LE(hScore("1nbqkbnr/4p3/p5P1/1P4p1/4p2P/1P1P3p/2qpQP2/RN2K1NR w - - 0 1",
                     "k1q2Nr1/1bn2Q2/1bqNRbN1/5RK1/7P/2NPp2p/1q1pn2N/3R1q2 w - - 0 1"), 108);
}

TEST(ProofGameTest, testRemainingMoves) {
    ProofGameTest::testRemainingMoves();
}

void
ProofGameTest::testRemainingMoves() {
    ASSERT_EQ(4, hScore(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1"));
    ASSERT_EQ(8, hScore(TextIO::startPosFEN, "rnbqk1nr/b1pp1ppp/1p6/4p3/8/5N2/PPPPPPPP/R1BQKB1R w KQkq - 0 1"));

    { // Reachable, 2 promotions required and available, 6 captured required and available
        std::string goal("B3k2B/1pppppp1/8/8/8/8/PPP2PPP/RN1QK1NR w KQ - 0 1");
        ASSERT_GE(hScore(TextIO::startPosFEN, goal), 20);
        ASSERT_LE(hScore(TextIO::startPosFEN, goal), 76);
    }

    { // King reachability limited by opponent blocked pawn attacks
        ASSERT_EQ(24, hScore("2k5/8/8/1p2p3/1P2P3/K7/8/8 w - - 0 1",
                             "8/8/K7/1p2p3/1P2P3/8/8/4k3 w - - 0 1"));
        ASSERT_EQ(INT_MAX, hScore("2k5/5r2/8/1p6/8/K7/7R/8 w - - 0 1",
                                  "2k5/5r2/8/1p6/K7/8/7R/8 w - - 0 1"));
        int s = hScore("2k5/5r2/8/1p6/K7/8/7R/8 w - - 0 1",
                       "2k5/5r2/8/1p6/8/K7/7R/8 w - - 0 1");
        ASSERT_GE(s, 2);
        ASSERT_LE(s, 4);
    }
}

TEST(ProofGameTest, testNonAdmissible) {
    ProofGameTest::testNonAdmissible();
}

void ProofGameTest::testNonAdmissible() {
    ASSERT_EQ(13, hScore("2n2N1n/Q1B1PB2/qq2b3/1k2K1b1/R4B1r/Qpp1N1b1/P1q2rp1/Q2R4 b - - 31 72",
                         "1bn2N1n/Q1B1PB2/qq2b3/1k2K1b1/R4B1r/Qpp1N3/P1q2rp1/Q2R4 w - - 0 1", true));
    ASSERT_EQ(22, hScore("rnbqkbnr/1ppppppp/8/1p6/8/8/P1PPPPPP/RNBQKBNR w - - 0 1",
                         "rnbqkbnr/1pppp3/8/1p6/8/5ppp/N1PPPPPP/RNBQKBNR w - - 0 1", true));
    ASSERT_GE(hScore("rnbqkbnr/1p1ppppp/8/2pP4/Pp6/8/2P1PPPP/RNBQKBNR b - - 0 4",
                     "rnbqkbnr/1p1pp3/6P1/5Pp1/Pp1ppp1P/8/2P5/1NBQKBN1 w - - 0 1", true), 1);
    ASSERT_GE(hScore("rn1qkbnr/p4p1p/7P/1pPPpb2/5Pp1/8/P2PP1P1/RNBQKBNR b - - 0 1",
                     "rn1qk1nr/p4p2/7P/1pPP3p/5pP1/6p1/P2PP3/RNBQKBNR w - - 0 1", true), 1);
}

TEST(ProofGameTest, testSearch) {
    ProofGameTest::testSearch();
}

void
ProofGameTest::testSearch() {
    { // Start position without castling rights
        ProofGame ps(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", true, {});
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(16, best);
    }
    { // Start position without castling rights
        ProofGame ps(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", true, {});
        ProofGame::Result result;
        int best = ps.search(ProofGame::Options().setWeightA(1).setWeightB(9), result);
        ASSERT_EQ(16, best);
    }
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqk1nr/ppppppbp/6p1/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1", true, {});
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(4, best);
        ASSERT_EQ(4, result.proofGame.size());
        ASSERT_EQ("a2a4", TextIO::moveToUCIString(result.proofGame[0]));
        ASSERT_EQ("g7g6", TextIO::moveToUCIString(result.proofGame[1]));
        ASSERT_EQ("b1a3", TextIO::moveToUCIString(result.proofGame[2]));
        ASSERT_EQ("f8g7", TextIO::moveToUCIString(result.proofGame[3]));
    }
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkbnr/p1pppppp/p7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                     true, {});
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(INT_MAX, best);
    }
    {
        ProofGame ps1(TextIO::startPosFEN,
                      "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 1",
                      true, {});
        ProofGame::Result result;
        int best = ps1.search({}, result);
        ASSERT_EQ(4, best);

        // One extra move needed to avoid ep square in goal position
        ProofGame ps2(TextIO::startPosFEN,
                      "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1",
                      true, {});
        best = ps2.search({}, result);
        ASSERT_EQ(6, best);
    }
}

TEST(ProofGameTest, testInitPath) {
    ProofGameTest::testInitPath();
}

void
ProofGameTest::testInitPath() {
    auto getMoves = [](const std::vector<std::string>& strMoves) {
        std::vector<Move> moves;
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        UndoInfo ui;
        for (const std::string& s : strMoves) {
            Move m = TextIO::stringToMove(pos, s);
            moves.push_back(m);
            pos.makeMove(m, ui);
        }
        return moves;
    };

    {
        std::vector<Move> initPath = getMoves({"a4", "b5", "axb5", "d5", "c4", "h5", "cxd5",
                                               "e6", "g4", "hxg4", "d6", "f5", "d7+", "Kf7",
                                               "d4", "Bc5", "d5", "a5", "e4", "Be3", "d6",
                                               "c5", "Bc4", "g5", "Bd5", "exd5", "fxe3"});
        ProofGame ps(TextIO::startPosFEN,
                     "2NB4/1bk1P3/1Rq1rn1B/q1pnN3/2K1Ppb1/Q6R/QB4pQ/3Q2rq b - - 0 1", true, initPath);
        ProofGame::Result result;
        int best = ps.search(ProofGame::Options().setMaxNodes(2), result);
        ASSERT_EQ(-1, best);
    }
    {
        std::vector<Move> initPath = getMoves({"a4", "g6"});
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqk1nr/ppppppbp/6p1/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1", true, initPath);
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(4, best);
        ASSERT_EQ(4, result.proofGame.size());
        ASSERT_EQ("a2a4", TextIO::moveToUCIString(result.proofGame[0]));
        ASSERT_EQ("g7g6", TextIO::moveToUCIString(result.proofGame[1]));
        ASSERT_EQ("b1a3", TextIO::moveToUCIString(result.proofGame[2]));
        ASSERT_EQ("f8g7", TextIO::moveToUCIString(result.proofGame[3]));
    }
    {
        std::vector<Move> initPath = getMoves({"a3"});
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqk1nr/ppppppbp/6p1/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1", true, initPath);
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(6, best);
        ASSERT_EQ(6, result.proofGame.size());
        ASSERT_EQ("a2a3", TextIO::moveToUCIString(result.proofGame[0]));
        ASSERT_EQ("g7g6", TextIO::moveToUCIString(result.proofGame[1]));
        ASSERT_EQ("a3a4", TextIO::moveToUCIString(result.proofGame[2]));
        ASSERT_EQ("f8h6", TextIO::moveToUCIString(result.proofGame[3]));
        ASSERT_EQ("b1a3", TextIO::moveToUCIString(result.proofGame[4]));
        ASSERT_EQ("h6g7", TextIO::moveToUCIString(result.proofGame[5]));
    }
    { // Initial path contains a capture that is needed as last move, so no solution exists
        std::vector<Move> initPath = getMoves({"b4", "a5", "c4", "axb4", "d4", "c5", "f4", "e5",
                                               "g4", "exf4", "Nf3", "h5", "Ng1", "d5", "cxd5",
                                               "cxd4", "Bh3", "hxg4", "Bg2", "Qa5", "Bf1", "Qd8"});
        EXPECT_THROW({
            ProofGame ps(TextIO::startPosFEN,
                         "2rQR3/1pr1B2n/P1bP1k1q/KRb1n3/5q2/1R2bB1q/1n1Nq3/QN1n4 b - - 0 1",
                         true, initPath);
        }, ChessError);
    }
    {   // Forced last move overlaps with initial path
        std::vector<Move> initPath = getMoves({"d4", "Nf6", "e4", "Nxe4", "d5", "Nf6", "d6", "exd6", "Bc4",
                                               "Ng8", "Be6", "f6", "Qd2", "Qe7", "Qe2", "Qd8", "Bf7+"});
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkbnr/pppp1Bpp/3p1p2/8/8/8/PPP1QPPP/RNB1K1NR b KQkq - 0 1", true, initPath);
        ProofGame::Result result;
        auto opts = ProofGame::Options().setMaxNodes(10);
        int best = ps.search(opts, result);
        ASSERT_EQ(17, best);
        ASSERT_EQ(initPath, result.proofGame);
    }
    {   // Repetition in initial path
        std::vector<Move> initPath = getMoves({"Nf3", "Nf6", "Ng1", "Ng8", "Nf3"});
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1", true, initPath);
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(2, best);
        ASSERT_EQ(2, result.proofGame.size());
        ASSERT_EQ("g1f3", TextIO::moveToUCIString(result.proofGame[0]));
        ASSERT_EQ("g8f6", TextIO::moveToUCIString(result.proofGame[1]));
    }
}

TEST(ProofGameTest, testLastMove) {
    ProofGameTest::testLastMove();
}

void
ProofGameTest::testLastMove() {
    {
        std::string goal = "K1b1r2n/2N4p/2nBQ1b1/8/RB2Rrq1/1nQB1nr1/QpNk4/1B2b2n b - - 0 1";
        ProofGame ps1(TextIO::startPosFEN, goal, true, {});
        ASSERT_EQ(goal, TextIO::toFEN(ps1.getGoalPos()));

        ProofGame ps2(TextIO::startPosFEN, goal, true, {}, true);
        ASSERT_EQ("K1b1r2n/2N4p/2nBQ1b1/8/RBQ1Rrq1/1nqB1nr1/QpNk4/1B2b2n w - - 0 1",
                  TextIO::toFEN(ps2.getGoalPos()));

        ProofGame ps3(TextIO::startPosFEN, goal, false, {}, true);
        ASSERT_EQ(goal, TextIO::toFEN(ps3.getGoalPos()));
    }
    {
        std::string goal = "nR2RQB1/N2K4/2p1r2B/1B1pb3/2P4k/q2bQ1P1/R1n1b3/3nbBbr b - - 0 1";
        ProofGame ps1(TextIO::startPosFEN, goal, true, {});
        ASSERT_EQ(goal, TextIO::toFEN(ps1.getGoalPos()));

        ProofGame ps2(TextIO::startPosFEN, goal, true, {}, true);
        ASSERT_NE(goal, TextIO::toFEN(ps2.getGoalPos()));
    }
}

TEST(ProofGameTest, testEnPassant) {
    ProofGameTest::testEnPassant();
}

void
ProofGameTest::testEnPassant() {
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkbnr/pp1ppppp/8/8/2pPP3/7P/PPP2PP1/RNBQKBNR b KQkq d3 0 1", true, {});
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(5, best);
        ASSERT_EQ(5, result.proofGame.size());
        ASSERT_EQ("d2d4", TextIO::moveToUCIString(result.proofGame[4]));
    }
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkbnr/ppppp1pp/8/8/3PPp2/7P/PPP2PP1/RNBQKBNR b KQkq e3 0 1", true, {});
        ProofGame::Result result;
        int best = ps.search({}, result);
        ASSERT_EQ(5, best);
        ASSERT_EQ(5, result.proofGame.size());
        ASSERT_EQ("e2e4", TextIO::moveToUCIString(result.proofGame[4]));
    }
    {
        EXPECT_THROW({
            ProofGame ps(TextIO::startPosFEN, "4k3/8/8/1pP5/B7/1P6/8/4K3 w - b6 0 1", true, {});
        }, ChessError);
    }
    {
        EXPECT_THROW({
            ProofGame ps(TextIO::startPosFEN,
                         "1r2N1B1/1Np2K1R/pq2rQn1/nN4pr/k3bBpP/8/BN4N1/b4Qq1 b - h3 0 1", true, {});
        }, ChessParseError);
    }
}

TEST(ProofGameTest, testCaptureSquares) {
    ProofGameTest::testCaptureSquares();
}

void
ProofGameTest::testCaptureSquares() {
    { // Test solveAssignment
        std::vector<int> initial = {
            1, 1, 0, 0, 1, 1, 0, 1,
            0, 1, 0, 0, 0, 1, 0, 1,
            1, 1, 1, 0, 1, 1, 1, 1,
            0, 1, 0, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 1, 1, 0, 1,
            1, 1, 1, 0, 1, 1, 0, 1,
            1, 1, 1, 1, 1, 1, 1, 1,
            0, 1, 0, 0, 0, 1, 0, 0
        };
        std::vector<int> reduced = {
            1, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 1,
            0, 0, 0, 0, 0, 0, 1, 0,
            0, 1, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 1, 0, 0, 0,
            0, 0, 1, 0, 0, 0, 0, 0,
            0, 0, 0, 1, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 1, 0, 0
        };
        Matrix<int> m(8, 8);
        const int bigCost = ProofGame::bigCost;
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
                m(i,j) = initial[i*8+j] ? 1 : bigCost;
        Assignment<int> as(m);
        ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN, true, {});
        int cost = ps.solveAssignment(as);
        ASSERT_EQ(8, cost);
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
                ASSERT_EQ(reduced[i*8+j] ? 1 : bigCost, as.getCost(i, j));
    }

    {
        ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN, true, {});
        U64 m = ps.allPawnPaths(true, D3, F6, 0, 2);
        ASSERT_EQ(BitBoard::sqMask(D3,D4,E4,E5,F5,F6), m);
        m = ps.allPawnPaths(true, D3, F6, 0, 1);
        ASSERT_EQ(0, m);
        m = ps.allPawnPaths(true, D3, F6, 0, 3);
        ASSERT_EQ(BitBoard::sqMask(D3,D4,E4,E5,F5,F6), m);
        m = ps.allPawnPaths(true, D3, F6, 0, 4);
        ASSERT_EQ(BitBoard::sqMask(D3,D4,E4,E5,F5,F6), m);

        m = ps.allPawnPaths(true, D3, F6, BitBoard::sqMask(E4), 2);
        ASSERT_EQ(BitBoard::sqMask(D3,D4,E5,F6), m);

        m = ps.allPawnPaths(false, F6, D3, BitBoard::sqMask(E4), 2);
        ASSERT_EQ(BitBoard::sqMask(D3,D4,E5,F6), m);

        m = ps.allPawnPaths(true, E2, E7, 0, 0);
        ASSERT_EQ(BitBoard::sqMask(E2,E3,E4,E5,E6,E7), m);
        m = ps.allPawnPaths(true, E2, E7, BitBoard::sqMask(E4), 0);
        ASSERT_EQ(0, m);
        m = ps.allPawnPaths(true, E2, E7, 0, 1);
        ASSERT_EQ(BitBoard::sqMask(E2,E3,E4,E5,E6,E7), m);
        m = ps.allPawnPaths(true, E2, E7, 0, 2);
        ASSERT_EQ(BitBoard::sqMask(E2,D3,E3,F3,D4,E4,F4,D5,E5,F5,D6,E6,F6,E7), m);
        m = ps.allPawnPaths(true, E2, E7, 0, 3);
        ASSERT_EQ(BitBoard::sqMask(E2,D3,E3,F3,D4,E4,F4,D5,E5,F5,D6,E6,F6,E7), m);
        m = ps.allPawnPaths(true, E2, E7, BitBoard::sqMask(E4,F4), 2);
        ASSERT_EQ(BitBoard::sqMask(E2,D3,E3,D4,D5,E5,D6,E6,E7), m);
        m = ps.allPawnPaths(true, E2, E7, BitBoard::sqMask(D4,E4,F4), 4);
        ASSERT_EQ(BitBoard::sqMask(E2,D3,F3,C4,G4,C5,D5,F5,G5,D6,E6,F6,E7), m);
    }

    {
        ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN, true, {});
        U64 cutSets[16];
        int size = 0;
        bool ret = ps.computeCutSets(true, BitBoard::sqMask(C2), A4, 0, 2, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(2, size);
        ASSERT_EQ(BitBoard::sqMask(A4), cutSets[0]);
        ASSERT_EQ(BitBoard::sqMask(B3), cutSets[1]);

        size = 0;
        ret = ps.computeCutSets(true, BitBoard::sqMask(C2), A6, 0, 2, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(2, size);
        ASSERT_EQ(BitBoard::sqMask(A4,A5,A6), cutSets[0]);
        ASSERT_EQ(BitBoard::sqMask(B3,B4,B5), cutSets[1]);

        size = 0;
        ret = ps.computeCutSets(true, BitBoard::sqMask(C2), A6, 0, 1, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(0, size);

        size = 15;
        ret = ps.computeCutSets(true, BitBoard::sqMask(C2), A6, 0, 1, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(15, size);

        size = 0;
        ret = ps.computeCutSets(true, BitBoard::sqMask(C2), A6, 0, 4, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(2, size);
        ASSERT_EQ(BitBoard::sqMask(A4,A5,A6), cutSets[0]);
        ASSERT_EQ(BitBoard::sqMask(B3,B4,B5), cutSets[1]);

        size = 0;
        ret = ps.computeCutSets(true, BitBoard::sqMask(C2), C6, 0, 2, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(0, size);

        size = 0;
        ret = ps.computeCutSets(false, BitBoard::sqMask(C7,G7), E5, 0, 2, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(2, size);
        ASSERT_EQ(BitBoard::sqMask(E5), cutSets[0]);
        ASSERT_EQ(BitBoard::sqMask(D6,F6), cutSets[1]);

        ret = ps.computeCutSets(false, BitBoard::sqMask(C7,F7,G7), E5, 0, 2, cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(3, size);
        ASSERT_EQ(BitBoard::sqMask(E5), cutSets[0]);
        ASSERT_EQ(BitBoard::sqMask(D6,F6), cutSets[1]);
        ASSERT_EQ(BitBoard::sqMask(E5,E6), cutSets[2]);

        size = 0;
        ret = ps.computeCutSets(true, BitBoard::sqMask(D2,H2), F5, BitBoard::sqMask(E3), 2,
                                cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(2, size);
        ASSERT_EQ(BitBoard::sqMask(F4,F5), cutSets[0]);
        ASSERT_EQ(BitBoard::sqMask(E4,G4,G3), cutSets[1]);

        size = 0;
        ret = ps.computeCutSets(true, BitBoard::sqMask(A2), B3, BitBoard::sqMask(B4), 7,
                                cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(1, size);
        ASSERT_EQ(BitBoard::sqMask(B3), cutSets[0]);

        size = 13;
        ret = ps.computeCutSets(true, BitBoard::sqMask(D2,H2), F5, BitBoard::sqMask(E3), 2,
                                cutSets, size);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(15, size);
        ASSERT_EQ(BitBoard::sqMask(F4,F5), cutSets[13]);
        ASSERT_EQ(BitBoard::sqMask(E4,G4,G3), cutSets[14]);

        size = 14;
        ret = ps.computeCutSets(true, BitBoard::sqMask(D2,H2), F5, BitBoard::sqMask(E3), 2,
                                cutSets, size);
        ASSERT_EQ(false, ret);
    }

    {
        std::string goal("rnbqk3/pppppp1p/8/8/1P3P2/8/PPP1PPP1/RNBQKBNR w KQq - 0 1");
        ASSERT_GE(hScore(TextIO::startPosFEN, goal), 8);
        ASSERT_LE(hScore(TextIO::startPosFEN, goal), 26);
    }
    {
        std::string start("rnbqkbnr/pppppppp/8/8/8/8/1P1P1P1P/RNBQKBNR w KQkq - 0 1");
        std::string goal("rnbqk3/pppppp1p/8/8/8/P1P1P1P1/8/RNBQKBNR w KQq - 0 1");
        ASSERT_GE(hScore(start, goal), 8);
        ASSERT_LE(hScore(start, goal), 28);
    }
    ASSERT_EQ(INT_MAX, hScore("2b1kqr1/p1rp3p/1p1p1b2/3PpPp1/PpP3P1/6P1/4BN2/R1B1KQ1R w Q - 5 1",
                              "2b1kqr1/p2p3p/3p4/p2PpP2/PpP2p2/6P1/8/RRB1KQ1N w - - 0 1"));
}

TEST(ProofGameTest, testDeadlockedPieces) {
    ProofGameTest::testDeadlockedPieces();
}

void
ProofGameTest::testDeadlockedPieces() {
    {
        std::string goalFen = "1k6/1Pb4b/1P6/8/8/8/8/4K3 w - - 0 1";
        ASSERT_EQ(INT_MAX, hScore("1k6/bP5b/1P6/8/8/8/8/4K3 w - - 0 1", goalFen));
        ASSERT_EQ(4,       hScore("1k6/1Pb2b2/1P6/8/8/2K5/8/8 w - - 0 1", goalFen));
    }
    ASSERT_EQ(INT_MAX, hScore("4k3/2b1P3/4P3/8/8/2K1P3/8/8 w - - 0 1",
                              "4k3/2b1P3/4P3/8/8/2K1Q3/8/8 w - - 0 1"));
    ASSERT_EQ(INT_MAX, hScore("4k3/2b1P3/4P3/8/P7/2K5/8/8 w - - 0 1",
                              "2k5/2b1P3/4P3/8/N7/2K5/8/8 w - - 0 1"));
    {
        std::string startFen = "4k3/2b1P3/4P3/8/P7/2K5/8/8 w - - 0 1";
        std::string goalFen  = "8/2b5/3k4/8/B3B3/2K5/8/8 w - - 0 1";
        ASSERT_GE(hScore(startFen, goalFen), 12);
        ASSERT_LE(hScore(startFen, goalFen), 16);
    }

    {
        std::string startFen = "3k4/b2P4/3P4/3P4/3p4/3p3B/3p4/3K4 w - - 0 1";
        std::string goalFen  = "3k4/b2P4/3P4/3P4/1B1p4/3p4/3p4/3K4 w - - 0 1";
        ProofGame ps(startFen, goalFen, true, {});
        ASSERT_EQ(INT_MAX, hScore(startFen, goalFen));
        Position pos = TextIO::readFEN(startFen);
        U64 blocked;
        ps.computeBlocked(pos, blocked);
        U64 expected = BitBoard::maskFileD;
        ASSERT_EQ(expected, blocked);
    }
    {
        std::string startFen = "4k3/b3p3/4p3/8/1B6/5P2/5P2/5K2 w - - 0 1";
        std::string goalFen  = "4k3/4p3/1b2p3/8/8/B4P2/5P2/5K2 w - - 0 1";
        ProofGame ps(startFen, goalFen, true, {});
        ASSERT_EQ(2, hScore(startFen, goalFen));
        Position pos = TextIO::readFEN(startFen);
        U64 blocked;
        ps.computeBlocked(pos, blocked);
        ASSERT_EQ(BitBoard::sqMask(F2, F3, E6, E7), blocked);
    }
    {
        std::string startFen = "3k4/b2P4/3P4/2P5/1P6/P7/8/4K3 w - - 0 1";
        std::string goalFen  = "3k4/3P2b1/3P4/2P5/1P6/P7/8/4K3 w - - 0 1";
        ProofGame ps(startFen, goalFen, true, {});
        ASSERT_EQ(INT_MAX, hScore(startFen, goalFen));
        U64 blocked;
        Position pos = TextIO::readFEN(startFen);
        ps.computeBlocked(pos, blocked);
        ASSERT_EQ(BitBoard::sqMask(A3, B4, C5, D6, D7, D8), blocked);
    }

    {
        std::string startFen = "rBb1q3/pPPp2bQ/2RQ4/1r4kn/1N3R2/1qN1B2B/Q1nKR2n/Qr6 w - - 16 65";
        std::string goalFen  = "4q3/pB1p2bQ/2RQ4/1r1bBrkn/1N3R2/1qN1B1BB/Q1nKR2n/Qr6 b - - 0 1";
        ProofGame ps(startFen, goalFen, true, {});
        ASSERT_EQ(INT_MAX, hScore(startFen, goalFen));
        U64 blocked;
        Position pos = TextIO::readFEN(startFen);
        ps.computeBlocked(pos, blocked);
        ASSERT_EQ(BitBoard::sqMask(A7, B7, C7, D7, A8, B8, C8), blocked);
    }
    {
        std::string startFen = "3k2br/5p1P/8/8/8/7P/8/4K3 w - - 0 1";
        std::string goalFen  = "3k2br/5p1P/8/8/8/7N/8/4K3 w - - 0 1";
        ProofGame ps(startFen, goalFen, true, {});
        ASSERT_EQ(INT_MAX, hScore(startFen, goalFen));
        U64 blocked;
        Position pos = TextIO::readFEN(startFen);
        ps.computeBlocked(pos, blocked);
        ASSERT_EQ(BitBoard::sqMask(F7, H7, G8, H8), blocked);
    }
    {
        std::string startFen = "3k2br/5p1P/8/8/8/7P/8/4K3 w - - 0 1";
        std::string goalFen  = "3k2br/5p1P/8/8/7P/8/8/4K3 b - - 0 1";
        ProofGame ps(startFen, goalFen, true, {});
        ASSERT_EQ(1, hScore(startFen, goalFen));
        U64 blocked;
        Position pos = TextIO::readFEN(startFen);
        ps.computeBlocked(pos, blocked);
        ASSERT_EQ(BitBoard::sqMask(F7, H7, G8, H8), blocked);
    }
}

TEST(ProofGameTest, testFilter1a) {
    ProofGameTest::testFilter1a();
}

TEST(ProofGameTest, testFilter1b) {
    ProofGameTest::testFilter1b();
}

TEST(ProofGameTest, testFilter1c) {
    ProofGameTest::testFilter1c();
}

TEST(ProofGameTest, testFilter2) {
    ProofGameTest::testFilter2();
}

TEST(ProofGameTest, testFilter3a) {
    ProofGameTest::testFilter3a();
}

TEST(ProofGameTest, testFilter3b) {
    ProofGameTest::testFilter3b();
}

TEST(ProofGameTest, testFilter4a) {
    ProofGameTest::testFilter4a();
}

TEST(ProofGameTest, testFilter4b) {
    ProofGameTest::testFilter4b();
}

TEST(ProofGameTest, testFilter5) {
    ProofGameTest::testFilter5();
}

TEST(ProofGameTest, testFilter6) {
    ProofGameTest::testFilter6();
}

TEST(ProofGameTest, testFilter7) {
    ProofGameTest::testFilter7();
}

namespace {
    struct FilterData {
        std::string fen;
        std::string status;
        bool value;
    };

    bool contains(const std::string& str, const std::string& value) {
        return str.find(value) != std::string::npos;
    };

    void testFilterData(const std::vector<FilterData>& v) {
        for (const FilterData& d : v) {
            std::stringstream in;
            in << d.fen << std::endl;
            std::stringstream out;
            ProofGameFilter().filterFens(in, out);
            ASSERT_EQ(d.value, contains(out.str(), d.status))
                << (d.value ? "" : "!")  << d.status << " : " << out.str();
        }
    }
}

void ProofGameTest::testFilter1a() {
    std::vector<FilterData> v = {
        { TextIO::startPosFEN, "illegal", false },
        { TextIO::startPosFEN, "legal: proof", true },
        { "rnbqkbnr/p1pppppp/p7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
          "illegal:", true }, // invalid pawn capture
        { "rnbqkbnr/p1pppppp/p7/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1",
          "illegal:", false }, // a4 Nf6 a5 Ng8 a6 bxa6
        { "nnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQk - 0 1",
          "illegal:", true }, // Too many black knights
        { "8/8/8/8/8/8/8/Kk6 w - - 0 1", "illegal:", true }, // King capture possible
        { "8/8/8/8/8/8/8/KRk5 w - - 0 1", "illegal:", true }, // King capture possible
//        { "8/8/8/8/8/8/8/KRk5 b - - 0 1", "illegal:", false }, // King in check XXX Search explosion

        // All possible captures for last move rejected
        { "k1bBrR2/1B1rbN2/p2BN1Q1/2n3bP/p2bNR1p/2R1n3/P4b2/1K3b2 b - - 0 1", "illegal:", true },
        { "nB1kr3/pbnBR3/P2PpQ1Q/2N2K2/r1r1r3/qPr5/qR2P2q/1N5r w - - 1 2", "illegal:", true },
        { "4B2n/Bqp2Nbr/r1r2p1B/5b2/n3bN2/1QR1q1b1/3kr2B/QKb2B1R w - - 0 1", "illegal:", true },

        { "rnbRkbnr/p1pp1ppp/1p1p4/8/8/8/PPP2PPP/RNBQKBNR b KQkq - 0 1", "illegal:", false },
        { "Q2q4/2Bp3r/2PBRb2/R2N3p/1q3RBP/1pQnK2N/2B2Qp1/2bn1b1k w - - 0 1", "illegal:", true },

        { " 2bqk2r/1pppp1P1/6p1/p5p1/2B3p1/6P1/PPPP2P1/R1BQKB1R b KQk - 0 1 ", "unknown: kernel:", true },
        { "k1Bb1B1q/2NNp2r/1KNp1RB1/1R2qb2/BB1P3B/2N5/r2r4/RQqn4 b - - 0 1", "illegal:", true },

        { "1B1b1nr1/2N2p2/3rr1B1/r3NQQR/Bp1k1nRK/2P1N3/2r2QP1/B5q1 b - - 0 1", "illegal:", true },

        { "r1bqkbnr/pppppppp/2n5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1 legal: proof: e4 Nc6",
          "legal: proof: e4 Nc6", true },

        // Test that PATH is computed
        { "Rr1n1K2/1P2Q2B/n2Q3B/2bq1RR1/b5Nq/2k2N1r/1N1Rq1Q1/n1b1r1Q1 w - - 0 1 "
          "unknown: kernel: wPa0xPb1 wPc0xPd1 wPe0xPf1 bPg1xPh0 "
          "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 wPc2-c4 bPd7-d5 wPc4xd5 wPe2-e4 bPf7-f5 "
          "wPe4xf5 bPg7-g5 wPh2-h4 bPg5xh4",
          " path: ", true },
        { "b4R2/Pb1Q1n2/1pN2R2/3K3P/2r1P2r/B1k3nb/Q2pr1Nq/Q2B2rb b - - 0 1 "
          "unknown: kernel: bPa1xPb0 bPc1xPd0 bPe1xPf0 wPg0xPh1 wPc0xDBbQ "
          "extKernel: bPa7-a5 wPb2-b4 bPa5xb4 bPc7-c5 wPd2-d4 bPc5xd4 bPe7-e5 wPf2-f4 "
          "bPe5xf4 wPg2-g4 bPh7-h5 wPg4xh5 wPc2-c7 bDBf8-b8 wPc7xb8Q",
          " path: ", true },
        { "rnN1kbnr/p1pppppp/2b5/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1 "
          "unknown: kernel: wPa0xPb1 wPb1xQcN "
          "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 wPb5-b7 bQd8-c8 wPb7xc8N",
          " path: ", true },
        { "rnbqkbnr/1ppppppp/8/8/8/2B5/P1PPPPPP/RNb1KBNR w KQkq - 0 1 "
          "unknown: kernel: bPa1xPb0 bPb0xQcDB "
          "extKernel: bPa7-a5 wPb2-b4 bPa5xb4 bPb4-b2 wQd1-c1 bPb2xc1DB",
          " path: ", true },
        { "1R1Qr3/1pPQ2nr/nnk1BN1Q/3RB1n1/3n3N/4p2b/RKn3Q1/1n3B2 b - - 0 1 "
          "unknown: kernel: bPa1xPb0 bPc1xPd0 wPe0xPf1 wPg0xQf0 wPh0xDBg1 "
          "extKernel: bPa7-a5 wPb2-b4 bPa5xb4 bPc7-c5 wPd2-d4 bPc5xd4 wPe2-e4 bPf7-f5 "
          "wPe4xf5 wPg2-g4 wPf5-f7 wPf2-f6 bQd8-f5 wPg4xf5 wPh2-h4 bPg7-g4 bDBf8-g5 wPh4xg5",
          " path: ", true },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter1b() {
    std::vector<FilterData> v = {
        { "3n4/6b1/Bn1qN3/k1Bqq1nr/3np1KR/bb4B1/Pb1NB1pN/8 b - - 0 1 "
          "unknown: kernel: bPa1xPb0 bPc1xPd0 bPf1xPe0 bPh1xPg0 bPb0xQaQ bPb0xRaQ wPf0xRe2 "
          "extKernel: bPa7-a5 wPb2-b4 bPa5xb4 bPc7-c5 wPd2-d4 bPc5xd4 bPf7-f5 wPe2-e4 "
          "bPf5xe4 bPh7-h5 wPg2-g4 bPh5xg4 bPb4-b2 wQd1-a1 bPb2xa1Q bPb7-b2 bPb2xa1Q "
          "wPf2-f4 bPe4-e3 bPe7-e4 bRa8-e5 wPf4xe5",
          " Wrong piece color ", false },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter1c() {
    {
        std::stringstream in;
        in << "Q2q4/2Bp3r/2PBRb2/R2N3p/1q3RBP/1pQnK2N/2B2Qp1/2bn1b1k w - - 0 1" << '\n'
           << "2bqk2r/1pppp1P1/6p1/p5p1/2B3p1/6P1/PPPP2P1/R1BQKB1R b KQk - 0 1" << '\n';
        std::stringstream out;
        ProofGameFilter().filterFens(in, out);
        std::string outStr = out.str();
        size_t idx = outStr.find('\n');
        std::string line1 = outStr.substr(0, idx);
        std::string line2 = outStr.substr(idx+1);
        ASSERT_EQ(true, contains(line1, "illegal:")) << line1;
        ASSERT_EQ(true, contains(line2, "unknown:")) << line2;
    }

    {
        std::stringstream in;
        in << "4k3/8/8/8/8/8/8/4K3 w - - 0 1 abc" << std::endl;
        std::stringstream out;
        EXPECT_THROW({
            ProofGameFilter().filterFens(in, out);
        }, ChessParseError);
    }
}

void ProofGameTest::testFilter2() {
    std::vector<FilterData> v = {
        // Test that PATH is computed also in complicated cases
        { "2R4n/2rPQN1p/2BrRpR1/K1Nk4/n4Q1r/B3bn1R/5nqP/n1B2Q2 b - - 0 1 "
          "unknown: kernel: wPa0xPb1 wPc0xPd1 bPe1xPf0 wPg0xLBf2 "
          "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 wPc2-c4 bPd7-d5 wPc4xd5 bPe7-e5 wPf2-f4 "
          "bPe5xf4 wPg2-g6 bPf7-f6 bLBc8-f7 wPg6xf7",
          " path: ", true },
        { "R1b1bB2/4n1pQ/3bq1p1/3PQP1r/3K1nkq/RP1n4/bB6/1r5N w - - 0 1 "
          "unknown: kernel: wPa0xPb1 wPc0xPd1 bPf1xPe0 bPh1xPg0 bPa0xPb0 bPc0xLBbQ bPe0xNd0 "
          "extKernel: bPb7-b3 wPa2xb3 wPc2-c4 bPd7-d5 wPc4xd5 bPf7-f5 wPe2-e4 bPf5xe4 wPg2-g6 "
          "bPh7xg6 bPa7-a3 bPa3xb2 bPc7-c2 wLBf1-b1 bPc2xb1Q wPd2-d4 wNb1-d3 bPe4xd3",
          " path: ", true },
        { "nr2BK2/Q1NBb1R1/4pkb1/1R4R1/R1rq1rQ1/1n1r1Q2/3qqP1q/3Q4 w - - 0 1 "
          "unknown: kernel: wPa0xPb1 wPc0xPd1 bPf1xPe0 bPg1xDBh0 bPh2xNg0 "
          "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 wPc2-c4 bPd7-d5 wPc4xd5 bPf7-f5 wPe2-e4 "
          "bPf5xe4 bPg7-g5 wPh2-h5 wDBc1-h4 bPg5xh4 bPh7-h6 wPg2-g6 wNb1-g5 bPh6xg5",
          " path: ", true },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter3a() {
    std::vector<FilterData> v = {
        { "Bq1b3R/1Pb3N1/1BP2pNP/4pBNp/4r3/R1np4/1KN4b/3k2Bb w - - 0 1 "
          "unknown: kernel: wPa0xPb1 bPc1xPd0 wPf0xPe1 bPd0xQe0 wPg0xRh2 wPh0xNg1 "
          "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 wPd2-d6 bPc7xd6 wPf2-f4 bPe7-e5 wPf4xe5 "
          "wPe5-e7 wPe2-e6 wQd1-e5 bPd6xe5 wPg2-g5 bPh7-h5 bRa8-h6 wPg5xh6 wPh2-h4 "
          "bPg7-g4 bNb8-g5 wPh4xg5",
          " unknown: ", true },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter3b() {
    std::vector<FilterData> v = {
        { "KN2bBn1/1n1b2r1/2qQ1Q2/pp6/Pr1RPn2/r3p3/1BQ5/2Rnqk2 b - - 0 1 "
          "unknown: kernel: bPc1xPb0 bPd1xPc0 bPg1xPh0 wPd0xDBc1 bPe1xLBd0 bPf1xNe0 "
          "extKernel: bPc7-c5 wPb2-b4 bPc5xb4 bPd7-d5 wPc2-c4 bPd5xc4 bPg7-g5 wPh2-h4 "
          "bPg5xh4 wPd2-d4 bDBf8-c5 wPd4xc5 bPe7-e6 wLBf1-d5 bPe6xd5 bPf7-f4 wPe2-e4 "
          "wNb1-e3 bPf4xe3",
          " path: ", true },
        { "B2K2b1/ppp3k1/1n3NR1/1Qqp1n2/qbN2R2/1B5B/6PB/5R1R b - - 0 1 ",
          " illegal: ", true },
        { "k1q2Nr1/1bn2Q2/1bqNRbN1/5RK1/7P/2NPp2p/1q1pn2N/3R1q2 w - - 0 1 "
          "unknown: kernel: dummy "
          "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 bPf7-f5 wPe2-e4 bPf5xe4 bPc7-c5 wPd2-d4 "
          "bPc5xd4 bPd4-d1 bPd7-d2 bRa8-d3 wPc2xd3 bPg7-g4 wPh2-h4 wLBf1-h3 bPg4xh3 "
          "bPh7-h6 wPg2-g6 wDBc1-g5 bPh6xg5",
          " path: ", true },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter4a() {
    std::vector<FilterData> v = {
        // Test PATH computation when promotion piece information is missing
        { "rBbqkbnr/p1p1pppp/n1p5/8/8/8/2PPPPPP/RNBQKBNR w KQkq - 0 1 "
          "unknown: kernel: wPa0xPb1 bPd7xNc6 "
          "extKernel: bPb7-b3 wPa2xb3 wPb3-b8 wPb2-b8 wNb8-c6 bPd7xc6",
          " path: ", true },
        { "r2qkbnr/p1pppppp/n7/3p4/8/8/N2PPPPP/RNBQKBNR w KQkq - 0 1 "
          "unknown: kernel: bPb1xPc0 bPc0xbe1 wPa0xLBb0 "
          "extKernel: wPc2-c6 bPb7xc6 wPb2-b8 wNb8-d5 bPc6xd5 wPa2-a6 bLBc8-b7 wPa6xb7 wPb7-b8",
          " path: ", true },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter4b() {
    std::vector<FilterData> v = {
        { "kNN2b1Q/1R1n2P1/bB2p3/3rQB2/1q3Q2/2ppQ1r1/P1PQq3/B1K1nr2 w - - 0 1 "
          "unknown: kernel: wPb0xPa1 wPd0xPe1 wPh0xPg1 bPf1xRe0 "
          "extKernel: wPb2-b4 bPa7-a5 wPb4xa5 wPd2-d4 bPe7-e5 wPd4xe5 wPh2-h4 bPg7-g5 "
          "wPh4xg5 wPe5-e8 wPe2-e7 wRa1-e6 bPf7xe6",
          " path: ", true },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter5() {
    std::vector<FilterData> v = {
        { "5b2/5B2/pPbB3b/P1nb1p2/q4NQR/1qP2rp1/1kr2N2/R2NKRrr b - - 0 1 "
          "unknown: kernel: bPb1xPc0 bPe1xPf0 bPh1xPg0 wPd0xNc2 "
          "extKernel: bPb7-b5 wPc2-c4 bPb5xc4 bPe7-e5 wPf2-f4 bPe5xf4 bPh7-h5 wPg2-g4 "
          "bPh5xg4 bPc4-c1 bPc7-c2 bNb8-c3 wPd2xc3",
          " path: ", true },

        { "NN1b1K2/pQ3Qn1/2qQ1q1N/7k/2B4n/Nnb2rQR/r3RPr1/1qB4b b - - 0 1", "illegal:", true },
    };
    testFilterData(v);
}

void ProofGameTest::testFilter6() {
    std::vector<FilterData> v = {
        { "3b4/n1n1QK2/1p1Bpp2/1R4PB/b1bBq2R/2Rn3b/B1BQ3Q/4k1rr b - - 0 1 "
          "unknown: kernel: bPa1xPb0 wPd0xPc1 wPe0xPf1 bPg1xNf0 bPh1xNg0 "
          "extKernel: bPa7-a5 wPb2-b4 bPa5xb4 wPd2-d4 bPc7-c5 wPd4xc5 wPe2-e4 bPf7-f5 "
          "wPe4xf5 wPf5-f8 wPf2-f7 wNb1-f6 bPg7xf6 bPh7-h5 wPg2-g5 wNg1-g4 bPh5xg4",
          " path: ", true },
        { "1R2Q1Nn/1rRqB2R/1n6/pq1b4/1np2PN1/3qr2K/1b4pR/b1k2b1R w - - 0 1 "
          "unknown: kernel: dummy "
          "extKernel: bPb7-b5 wPa2-a4 bPb5xa4 bPd7-d5 wPc2-c4 bPd5xc4 bPf7-f5 "
          "wPe2-e4 bPf5xe4 bPg7-g5 wPh2-h4 bPg5xh4 bPh4-h3 wPg2-g3 wLBf1-g2 bPh3xg2",
          " unknown: ", true }, // Only test that code does not crash, finding solution too hard
        { "2rn2N1/nP1p3K/BpBP4/2kN1PBB/3bPn2/1n1n2Q1/5RP1/q1N2r2 b - - 0 1 "
          "unknown: kernel: dummy "
          "extKernel: bPe7-e5 wPf2-f5 wRa1-f4 bPe5xf4 wPb2-b4 bPa7-a5 wPb4xa5 "
          "wPh2-h4 bPg7-g5 wPh4xg5 bPf7-f6 wPg5xf6 wPc2-c6 bPb7-b6 bLBc8-b7 wPc6xb7",
          " unknown: ", true }, // Only test that code does not crash, finding solution too hard
#ifdef RUN_SLOW_TESTS
        { "2n3N1/bp4N1/B1pB1bpN/2N1p3/2q3r1/rPQn3Q/2P2pNq/Nb1k3K w - - 0 1 "
          "unknown: kernel: dummy "
          "extKernel: bPd7-d5 wPe2-e4 bPd5xe4 wPf2-f4 bPg7-g5 wPf4xg5 wPg5-g8 wPg2-g7 "
          "wRa1-g6 bPh7xg6 bPa7-a3 wPb2-b3 wRh1-b2 bPa3xb2",
          " unknown: ", true }, // Only test that code does not crash, finding solution too hard
#endif
    };
    testFilterData(v);
}


void ProofGameTest::testFilter7() {
    std::vector<FilterData> v = {
#ifdef RUN_SLOW_TESTS
        { "3rB1Br/3p1p2/bp1rnBp1/1QNnqNP1/2PP1Q2/8/2kRbP2/K4bq1 b - - 1 3 "
          "unknown: kernel: dummy "
          "extKernel: bPa7-a5 wPb2-b4 bPa5xb4 bPc7-c4 wQd1-b3 bPc4xb3 bPe7-e4 wPd2-d4 "
          "wRa1-d3 bPe4xd3 wPh2-h6 bPg7-g6 bDBf8-g7 wPh6xg7",
          " path: ", true },
#endif
    };
    testFilterData(v);
}

TEST(ProofGameTest, testFilterPath) {
    ProofGameTest::testFilterPath();
}

void ProofGameTest::testFilterPath() {
    auto getPathPos = [](const std::string& lineStr, Position& pos) {
        std::stringstream out;
        {
            std::stringstream in;
            in << lineStr << std::endl;
            ProofGameFilter().filterFens(in, out);
        }

        std::stringstream in;
        in << out.str();
        ProofGameFilter::Line line;
        bool res = line.read(in);
        ASSERT_TRUE(res);
        ASSERT_TRUE(line.hasToken(ProofGameFilter::PATH));
        pos = TextIO::readFEN(TextIO::startPosFEN);
        UndoInfo ui;
        for (const std::string& s : line.tokenData(ProofGameFilter::PATH)) {
            Move m = TextIO::stringToMove(pos, s);
            pos.makeMove(m, ui);
        }
    };

    {
        Position pos;
        getPathPos("Qnb1kbnr/p1pppppp/8/Q7/8/8/2PPPPPP/RNBQKBNR b KQk - 0 1 "
                   "unknown: kernel: dummy "
                   "extKernel: wPa2-a6 wPa6xb7 wPb7xa8Q wPb2-b7 bQd8-a8 wPb7xa8Q", pos);
        ASSERT_EQ(3, BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN)));
        ASSERT_EQ(0, BitBoard::bitCount(pos.pieceTypeBB(Piece::BQUEEN)));
        ASSERT_EQ(1, BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK)));
        ASSERT_EQ(6, BitBoard::bitCount(pos.pieceTypeBB(Piece::WPAWN)));
        ASSERT_EQ(7, BitBoard::bitCount(pos.pieceTypeBB(Piece::BPAWN)));
    }
    {
        Position pos;
        getPathPos("rnbqNbn1/ppppp2p/4k1p1/8/8/8/PPPP1PPP/RNBQKBNR w KQ - 0 1 "
                   "unknown: kernel: dummy "
                   "extKernel: wPe2-e6 wPe6xf7 bPg7-g6 bRh8-e8 wPf7xe8N", pos);
        ASSERT_EQ(1, BitBoard::bitCount(pos.pieceTypeBB(Piece::BKING)));
        ASSERT_EQ(1, BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK)));
        ASSERT_EQ(3, BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT)));
        ASSERT_EQ(7, BitBoard::bitCount(pos.pieceTypeBB(Piece::WPAWN)));
        ASSERT_EQ(7, BitBoard::bitCount(pos.pieceTypeBB(Piece::BPAWN)));
    }
    {
        Position pos;
        std::string goalFen  = "N1BR1Bk1/1n1R4/K1Bn1PR1/1QP2pr1/nP3n1p/1r6/1BqbNq2/6N1 b - - 0 1";
        getPathPos(goalFen + " "
                   "unknown: kernel: dummy "
                   "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 bPc7-c5 wPd2-d4 bPc5xd4 wPh2-h4 "
                   "bPg7-g5 wPh4xg5 wPe2-e5 bPf7-f5 bQd8-f6 wPe5xf6 wPf2-f3 bPe7-e3 "
                   "bLBc8-e4 wPf3xe4", pos);
        int s = hScore(TextIO::toFEN(pos), goalFen);
        ASSERT_GE(s, 70);
        ASSERT_LE(s, 149);
    }
    {
        Position pos;
        std::string goalFen  = "B1rb1RN1/B4Np1/rP5b/3qb1P1/1k1n3P/3QrPK1/1BbPB3/2n3Q1 w - - 0 1";
        getPathPos(goalFen + " "
                   "unknown: kernel: dummy "
                   "extKernel: wPa2-a4 bPb7-b5 wPa4xb5 wPc2-c4 bPd7-d5 wPc4xd5 wPe2-e4 "
                   "bPf7-f5 wPe4xf5 wPb2-b4 bPa7-a5 wPb4xa5 bPh7-h5 wPg2-g5 wRa1-g4 bPh5xg4 ",
                   pos);
        int fromSq = -1, toSq = -1;
        ProofGame pg(TextIO::toFEN(pos), goalFen, true, {}, true);
        bool infeasible = pg.isInfeasible(fromSq, toSq);
        ASSERT_FALSE(infeasible);
    }
    {
        Position pos;
        std::string goalFen  = "rnbqkbnr/1ppppppp/8/8/8/2B5/PpPPPPPP/RN1QKBNR w KQkq - 0 1";
        getPathPos(goalFen + " "
                   "unknown: kernel: dummy "
                   "extKernel: bPa7-a4 wPb2-b3 bPa4xb3 bPb3-b2 ", pos);
        int fromSq = -1, toSq = -1;
        ProofGame pg(TextIO::toFEN(pos), goalFen, true, {}, true);
        bool infeasible = pg.isInfeasible(fromSq, toSq);
        ASSERT_FALSE(infeasible);
    }
    {
        Position pos;
        std::string goalFen  = "n1n2R2/3b4/Q1q1Bp2/2q5/RBrPPN1P/QNP1P1p1/4p3/1k2K1QR w K - 0 1";
        getPathPos(goalFen + " "
                   "unknown: kernel: dummy "
                   "extKernel: wPd2-d3 bPe7-e3 bRa8-e4 wPd3xe4 wxa7 wPg2-g4 bPh7-h5 wPg4xh5 "
                   "wPb2-b4 bPc7-c5 wPb4xc5 bPd7-d3 wPe2xd3 bPe3-e2 bDBf8-e3 wPf2xe3", pos);
        ASSERT_EQ(E1, pos.getKingSq(true));
        ASSERT_TRUE(pos.h1Castle());
    }
}

TEST(ProofGameTest, testPkSequence) {
    ProofGameTest::testPkSequence();
}

void ProofGameTest::testPkSequence() {
    using PieceColor = ProofKernel::PieceColor;
    using PieceType = ProofKernel::PieceType;
    using ExtPkMove = ProofKernel::ExtPkMove;
    auto toExtKernel = [](const std::string& str) {
        std::vector<std::string> strMoves;
        splitString(str, strMoves);
        std::vector<ExtPkMove> extKernel;
        for (const std::string& s : strMoves)
            extKernel.push_back(strToExtPkMove(s));
        return extKernel;
    };
    auto toStr = [](const std::vector<ExtPkMove>& extKernel) -> std::string {
        std::string ret;
        for (const ExtPkMove& m : extKernel) {
            if (!ret.empty())
                ret += ' ';
            ret += toString(m);
        }
        return ret;
    };

    auto mirrorPieceType = [](PieceType pt) {
        if (pt == PieceType::DARK_BISHOP)
            return PieceType::LIGHT_BISHOP;
        if (pt == PieceType::LIGHT_BISHOP)
            return PieceType::DARK_BISHOP;
        return pt;
    };

    auto mirrorExtKernel = [&](const std::string& extKernel) {
        std::vector<std::string> strMoves;
        splitString(extKernel, strMoves);

        std::string ret;
        for (const std::string& strMove : strMoves) {
            ExtPkMove m = strToExtPkMove(strMove);
            m.color = m.color == PieceColor::WHITE ? PieceColor::BLACK : PieceColor::WHITE;
            m.movingPiece = mirrorPieceType(m.movingPiece);
            m.fromSquare = Square::mirrorY(m.fromSquare);
            m.toSquare = Square::mirrorY(m.toSquare);
            m.promotedPiece = mirrorPieceType(m.promotedPiece);
            if (!ret.empty())
                ret += ' ';
            ret += toString(m);
        }
        return ret;
    };

    auto test0 = [&](const std::string& initFen, const std::string& goalFen,
                     const std::string& extKernel, const std::string& expected) {
        PkSequence pkSeq(toExtKernel(extKernel), TextIO::readFEN(initFen),
                         TextIO::readFEN(goalFen), std::cout);
        pkSeq.improve();
        ASSERT_EQ(expected, toStr(pkSeq.getSeq()));
    };
    auto test = [&](const std::string& initFen, const std::string& goalFen,
                    const std::string& extKernel, const std::string& expected,
                    bool testMirrorY = true) {
        test0(initFen, goalFen, extKernel, expected);
        if (testMirrorY) {
            test0(mirrorFenY(initFen), mirrorFenY(goalFen),
                  mirrorExtKernel(extKernel), mirrorExtKernel(expected));
        }
    };

    test(TextIO::startPosFEN, "rnbqkbnr/1pp2ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1",
         "wxe7", "wNb1-c3 wNc3-d5 wNd5xe7");
    test(TextIO::startPosFEN, "rnbqkbnr/1pp2ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1",
         "wxd7", "wNg1-f3 wNf3-e5 wNe5xd7");
    test("rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR w - - 0 1",
         "rnbqkbnr/1pp2ppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR w - - 0 1",
         "wxa7", "wDBc1-e3 wDBe3xa7");
    test("rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR w - - 0 1",
         "rnbqkbnr/ppp2pp1/8/8/8/4P3/PPPP1PPP/RNBQKBNR w - - 0 1",
         "wxh7", "wQd1-h5 wQh5xh7");
    test("rnbqkbnr/pppppppp/8/8/8/6P1/PPPPPP1P/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/8/6P1/PPPPPP1P/RNBQKBNR w KQkq - 0 1",
         "wxb7", "wLBf1-g2 wLBg2xb7");
    test("rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR w - - 0 1",
         "rnbqkbnr/p1pppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR w - - 0 1",
         "wxb7", "wRa1-a3 wRa3-b3 wRb3xb7");
    test("rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1",
         "wxb7", "wNb1-a3 wNa3-c4 wNc4-a5 wNa5xb7", false);

    test(TextIO::startPosFEN, "rnbqkbnr/1pp2ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1",
         "wxe7 wNb1-c3", "wNb1-c3 wNc3-d5 wNd5xe7 wNe7-d5 wNd5-c3");
    test(TextIO::startPosFEN, "rnbqkbnr/1pp2ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1",
         "wxe7 wNb1-e7", "wNb1-c3 wNc3-d5 wNd5xe7 wNe7-e7");

    test(TextIO::startPosFEN, "rnbqkbnr/pppp1ppp/5p2/8/8/1P6/P1PPPPPP/RN1QKBNR w KQkq - 0 1",
         "wDBc1-f6 bPe7xf6 wPb2-b3", "wPb2-b3 wDBc1-b2 wDBb2-f6 bPe7xf6");
    test("rnbqkbnr/p1pppppp/8/8/8/1p6/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/p1pp1ppp/5p2/8/8/1P1P4/P1p1PPPP/RN1QKBNR w KQkq - 0 1",
         "wDBc1-f6 bPe7xf6 bPb3xc2 wPb2-b3 wPd2-d3", "bPb3xc2 wPb2-b3 wDBc1-b2 wDBb2-f6 bPe7xf6 wPd2-d3");
    test(TextIO::startPosFEN, "rnbqk1nr/pppp1ppp/5p2/8/8/P7/P1PPPPPP/RN1QKBNR w KQkq - 0 1",
         "wDBc1-f6 bPe7xf6 bDBf8-a3 wPb2xa3", "wDBc1-f6 bPe7xf6 bDBf8-a3 wPb2xa3"); // No valid reordering
    test(TextIO::startPosFEN, "rnbqkbnr/pppppppp/5R2/8/P7/8/1PPPPPPP/1NBQKBNR w - - 0 1",
         "wRa1-f6 wPa2-a4", "wPa2-a4 wRa1-a3 wRa3-f3 wRf3-f6");

    test(TextIO::startPosFEN, "rnbqkbnr/p1pppppp/5R2/8/1P6/8/1PPPPPPP/1NBQKBNR w Kkq - 0 1",
         "wRa1-f6 wPa2-a3 bPb7-b4 wPa3xb4", "bPb7-b5 bPb5-b4 wPa2-a3 wPa3xb4 wRa1-a6 wRa6-f6");
    test(TextIO::startPosFEN, "rnbqkbnr/pppppppp/5R2/8/P7/8/1PPPPPPP/1NBQKBNR w - - 0 1",
         "wRa1-f6 wPa2-a3", "wPa2-a4 wRa1-a3 wRa3-f3 wRf3-f6");

    test("rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppppppp/7B/6P1/8/3P4/PPP1PP1P/RN1QKBNR w KQkq - 0 1",
         "wPg2-g5 wDBc1-h6", "wPg2-g4 wDBc1-h6 wPg4-g5");
    test(TextIO::startPosFEN, "rnbqkbnr/pppppppp/7B/6P1/8/3P4/PPP1PP1P/RN1QKBNR w KQkq - 0 1",
         "wPg2-g5 wDBc1-h6 wPd2-d3", "wPg2-g4 wPd2-d3 wDBc1-h6 wPg4-g5");

    test(TextIO::startPosFEN, "rQbqkbnr/p1pppppp/n7/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1",
         "wPa2-a6 wPa6xb7 wPb7-b8Q", "wPa2-a4 wPa4-a5 wPa5-a6 wPa6xb7 bNb8-a6 wPb7-b8Q");
    test(TextIO::startPosFEN, "rnNqkbnr/p2ppppp/bp6/8/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "wPb2-b6 wPb6xc7 wPc7-c8N", "wPb2-b4 wPb4-b5 wPb5-b6 wPb6xc7 bPb7-b6 bLBc8-a6 wPc7-c8N");

    test(TextIO::startPosFEN, "rnbqRbnr/ppp2ppp/2kp4/8/8/8/PPP1PPPP/RNBQKBNR w KQ - 0 1",
         "wPd2-d6 wPd6xe7 wPe7-e8R", "wPd2-d4 wPd4-d5 wPd5-d6 wPd6xe7 bPd7-d6 bKe8-d7 bKd7-c6 wPe7-e8R");

    test(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/1P6/N1R5/P1PPPPPP/2BQKBNR w Kkq - 0 1",
         "wPb2-b4 wRa1-c3", "wPb2-b4 wNb1-a3 wRa1-b1 wRb1-b3 wRb3-c3");
    test(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/1P6/N1R5/P1PPPPPP/2BQKBNR w Kkq - 0 1",
         "wRa1-c3 wPb2-b4", "wNb1-a3 wPb2-b4 wRa1-b1 wRb1-b3 wRb3-c3");

    test("rnbqkbnr/p1pppppp/1p6/8/8/8/PPPPPPPP/RNBQKBNR w KQk a6 0 1",
         "1nbqkbnr/2pppppp/1p6/2r5/8/p7/PPPPPPPP/RNBQKBNR w KQk - 0 1",
         "bRa8-c5", "bPa7-a5 bPa5-a4 bRa8-a5 bRa5-c5");
    test("rnbqkbnr/pppppppp/8/8/8/1P6/P1PPPPPP/RNBQKBNR w KQkq - 0 1",
         "rnbqkbnr/pppppppp/P7/8/2R5/1P6/2PPPPPP/1NBQKBNR w Kkq - 0 1",
         "wRa1-c4 wPa2-a4 wPa4-a5", "wPa2-a4 wPa4-a5 wRa1-a4 wRa4-c4");
}

TEST(ProofGameTest, testMultiBoard) {
    ProofGameTest::testMultiBoard();
}

void ProofGameTest::testMultiBoard() {
    {
        MultiBoard brd;
        for (int i = 0; i < 64; i++) {
            ASSERT_EQ(0, brd.nPieces(i));
        }
        brd.addPiece(A2, Piece::WPAWN);
        ASSERT_EQ(1, brd.nPieces(A2));
        ASSERT_TRUE(brd.hasPiece(A2, Piece::WPAWN));
        ASSERT_FALSE(brd.hasPiece(A2, Piece::BPAWN));
        ASSERT_FALSE(brd.hasPiece(A2, Piece::EMPTY));
        ASSERT_EQ(Piece::WPAWN, brd.getPiece(A2, 0));

        brd.removePieceType(A2, Piece::WPAWN);
        ASSERT_EQ(0, brd.nPieces(A2));

        brd.addPiece(B5, Piece::BPAWN);
        brd.addPiece(B5, Piece::WPAWN);
        ASSERT_EQ(2, brd.nPieces(B5));
        ASSERT_TRUE(brd.hasPiece(B5, Piece::WPAWN));
        ASSERT_TRUE(brd.hasPiece(B5, Piece::BPAWN));
        ASSERT_EQ(Piece::BPAWN, brd.getPiece(B5, 0));
        ASSERT_EQ(Piece::WPAWN, brd.getPiece(B5, 1));

        brd.removePieceNo(B5, 0);
        ASSERT_EQ(1, brd.nPieces(B5));
        ASSERT_TRUE(brd.hasPiece(B5, Piece::WPAWN));
        ASSERT_FALSE(brd.hasPiece(B5, Piece::BPAWN));
    }
    {
        Position initPos = TextIO::readFEN(TextIO::startPosFEN);
        MultiBoard brd(initPos);
        Position pos;
        brd.toPos(pos);
        for (int sq = 0; sq < 64; sq++) {
            ASSERT_EQ(pos.getPiece(sq), initPos.getPiece(sq));
        }
    }
}

TEST(ProofGameTest, testAttackedSq) {
    ProofGameTest::testAttackedSq();
}

void ProofGameTest::testAttackedSq() {
    auto test = [](const std::string& fen, U64 wAttacked, U64 bAttacked) {
        Position pos = TextIO::readFEN(fen);
        ASSERT_EQ(wAttacked, PosUtil::attackedSquares(pos, true)) << fen;
        ASSERT_EQ(bAttacked, PosUtil::attackedSquares(pos, false)) << fen;

        Position mirrorPos = PosUtil::swapColors(pos);
        ASSERT_EQ(BitBoard::mirrorY(bAttacked), PosUtil::attackedSquares(mirrorPos, true)) << fen;
        ASSERT_EQ(BitBoard::mirrorY(wAttacked), PosUtil::attackedSquares(mirrorPos, false)) << fen;
    };

    test("rnbk2nr/1p1ppP1p/5P2/5PP1/1p1p2p1/8/P1P5/RNBQKBNR w KQkq - 0 1",
         BitBoard::sqMask(B1,C1,D1,E1,F1,G1,
                          A2,B2,C2,D2,E2,F2,G2,H2,
                          A3,B3,C3,D3,E3,F3,H3,
                          C4,D4,F4,G4,H4,
                          B5,G5,H5,
                          A6,E6,F6,G6,H6,
                          E7,G7,H7,
                          E8,G8),
         BitBoard::sqMask(A2,
                          A3,C3,E3,F3,H3,
                          A4,
                          A5,
                          A6,C6,D6,E6,F6,G6,H6,
                          A7,B7,C7,D7,E7,H7,
                          B8,C8,E8,G8));
}
