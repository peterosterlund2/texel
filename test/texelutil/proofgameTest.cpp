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
#include "moveGen.hpp"
#include "textio.hpp"
#include "posutil.hpp"
#include <climits>

#include "gtest/gtest.h"


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
ProofGameTest::hScore(const std::string& initFen, const std::string& goalFen, bool testMirrorY) {
    Position initPos = TextIO::readFEN(initFen);
    ProofGame ps(initFen, goalFen);
    {
        Position pos0 = TextIO::readFEN(TextIO::startPosFEN);
        checkBlockedConsistency(ps, pos0);
    }
    checkBlockedConsistency(ps, initPos);
    const int score = ps.distLowerBound(initPos);
    EXPECT_GE(score, 0);

    if (testMirrorY) {
        int score2 = hScore(mirrorFenY(initFen), mirrorFenY(goalFen), false);
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
    ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN);
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
    ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN);
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
        } catch (const ChessParseError&) {
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
        ASSERT_EQ(BitBoard::sqMask(A1), ProofGame::bPawnReachable[A1]);
        ASSERT_EQ(BitBoard::sqMask(A2,A1,B1), ProofGame::bPawnReachable[A2]);
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/pppppppp/8/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(   B2,C2,D2,E2,F2,G2,H2,
                                      A7,B7,C7,D7,E7,F7,G7,H7,
                                         A1,E1,H1,A8,E8,H8),
                     blocked);
    }
    {
        std::string start("4k3/1p6/2P5/3P4/4P1B1/3P4/2P2PP1/4K3 w - - 0 1");
        std::string goal("4k3/1p6/2P5/3P4/B3P3/3P1P2/2P3P1/4K3 w - - 0 1");
        Position pos = TextIO::readFEN(start);
        ProofGame ps(start, goal);
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
        ProofGame ps(start, goal);
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

#if 0
    // Reachable by en passant capture
    ASSERT_LE(hScore("rnbqkbnr/p1pppppp/8/8/1pP5/8/PP1PPPPP/RNBQKBNR b KQkq c3 0 1",
                     "rnbqkbnr/p1pppppp/8/8/8/2p5/PP1PPPPP/RNBQKBNR w KQkq - 0 1"), 1);
#endif
}

TEST(ProofGameTest, testBlocked) {
    ProofGameTest::testBlocked();
}

void
ProofGameTest::testBlocked() {
    {
        std::string start("5Nkr/1bpnbpp1/2P1pq1p/p7/1p2PBP1/P2P1P2/1PQ1B2P/RN1K3R b - - 0 20");
        std::string goal("2r2rk1/1bPn1pp1/4pq1p/p7/1p2PBPb/P4P2/1PQNB2P/R2K3R w - - 1 21");
        ProofGame ps(start, goal);
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(B2,H2,A3,F3,G4,E6,H6,F7,G7), blocked);
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
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w KQkq - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,A1,E1,H1,A8,E8,H8), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w K - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E1,H1), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w Q - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E1,A1), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w k - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E8,H8), blocked);
    }
    {
        std::string start(TextIO::startPosFEN);
        ProofGame ps(start, "rnbqkbnr/4p3/pppp1ppp/8/8/PPPP1PPP/4P3/RNBQKBNR w q - 0 1");
        U64 blocked;
        bool res = ps.computeBlocked(TextIO::readFEN(start), blocked);
        ASSERT_TRUE(res);
        ASSERT_EQ(BitBoard::sqMask(E2,E7,E8,A8), blocked);
    }
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
}

TEST(ProofGameTest, testSearch) {
    ProofGameTest::testSearch();
}

void
ProofGameTest::testSearch() {
    { // Start position without castling rights
        ProofGame ps(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
        std::vector<Move> movePath;
        int best = ps.search({}, movePath);
        ASSERT_EQ(16, best);
    }
    { // Start position without castling rights
        ProofGame ps(TextIO::startPosFEN, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", 1, 9);
        std::vector<Move> movePath;
        int best = ps.search({}, movePath);
        ASSERT_EQ(16, best);
    }
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqk1nr/ppppppbp/6p1/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        std::vector<Move> movePath;
        int best = ps.search({}, movePath);
        ASSERT_EQ(4, best);
        ASSERT_EQ(4, movePath.size());
        ASSERT_EQ("a2a4", TextIO::moveToUCIString(movePath[0]));
        ASSERT_EQ("g7g6", TextIO::moveToUCIString(movePath[1]));
        ASSERT_EQ("b1a3", TextIO::moveToUCIString(movePath[2]));
        ASSERT_EQ("f8g7", TextIO::moveToUCIString(movePath[3]));
    }
    auto toM = [](const std::string& s) {
        return TextIO::uciStringToMove(s);
    };
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqk1nr/ppppppbp/6p1/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        std::vector<Move> initPath { toM("a2a4"), toM("g7g6") };
        std::vector<Move> movePath;
        int best = ps.search(initPath, movePath);
        ASSERT_EQ(4, best);
        ASSERT_EQ(4, movePath.size());
        ASSERT_EQ("a2a4", TextIO::moveToUCIString(movePath[0]));
        ASSERT_EQ("g7g6", TextIO::moveToUCIString(movePath[1]));
        ASSERT_EQ("b1a3", TextIO::moveToUCIString(movePath[2]));
        ASSERT_EQ("f8g7", TextIO::moveToUCIString(movePath[3]));
    }
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqk1nr/ppppppbp/6p1/8/P7/N7/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        std::vector<Move> initPath { toM("a2a3") };
        std::vector<Move> movePath;
        int best = ps.search(initPath, movePath);
        ASSERT_EQ(6, best);
        ASSERT_EQ(6, movePath.size());
        ASSERT_EQ("a2a3", TextIO::moveToUCIString(movePath[0]));
        ASSERT_EQ("g7g6", TextIO::moveToUCIString(movePath[1]));
        ASSERT_EQ("a3a4", TextIO::moveToUCIString(movePath[2]));
        ASSERT_EQ("f8h6", TextIO::moveToUCIString(movePath[3]));
        ASSERT_EQ("b1a3", TextIO::moveToUCIString(movePath[4]));
        ASSERT_EQ("h6g7", TextIO::moveToUCIString(movePath[5]));
    }
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkbnr/p1pppppp/p7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::vector<Move> movePath;
        int best = ps.search({}, movePath);
        ASSERT_EQ(INT_MAX, best);
    }
}

TEST(ProofGameTest, testEnPassant) {
    ProofGameTest::testEnPassant();
}

void
ProofGameTest::testEnPassant() {
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkbnr/pp1ppppp/8/8/2pPP3/7P/PPP2PP1/RNBQKBNR b KQkq d3 0 1");
        std::vector<Move> movePath;
        int best = ps.search({}, movePath);
        ASSERT_EQ(5, best);
        ASSERT_EQ(5, movePath.size());
        ASSERT_EQ("d2d4", TextIO::moveToUCIString(movePath[4]));
    }
    {
        ProofGame ps(TextIO::startPosFEN,
                     "rnbqkbnr/ppppp1pp/8/8/3PPp2/7P/PPP2PP1/RNBQKBNR b KQkq e3 0 1");
        std::vector<Move> movePath;
        int best = ps.search({}, movePath);
        ASSERT_EQ(5, best);
        ASSERT_EQ(5, movePath.size());
        ASSERT_EQ("e2e4", TextIO::moveToUCIString(movePath[4]));
    }
    {
        EXPECT_THROW({
            ProofGame ps(TextIO::startPosFEN,
                         "4k3/8/8/1pP5/B7/1P6/8/4K3 w - b6 0 1");
        }, ChessParseError);
    }
    {
        EXPECT_THROW({
            ProofGame ps(TextIO::startPosFEN,
                         "1r2N1B1/1Np2K1R/pq2rQn1/nN4pr/k3bBpP/8/BN4N1/b4Qq1 b - h3 0 1");
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
        ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN);
        int cost = ps.solveAssignment(as);
        ASSERT_EQ(8, cost);
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
                ASSERT_EQ(reduced[i*8+j] ? 1 : bigCost, as.getCost(i, j));
    }

    {
        ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN);
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
        ProofGame ps(TextIO::startPosFEN, TextIO::startPosFEN);
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

TEST(ProofGameTest, testKingPawnsTrap) {
    ProofGameTest::testKingPawnsTrap();
}

void
ProofGameTest::testKingPawnsTrap() {
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
        ProofGame ps(startFen, goalFen);
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
        ProofGame ps(startFen, goalFen);
        ASSERT_EQ(2, hScore(startFen, goalFen));
        Position pos = TextIO::readFEN(startFen);
        U64 blocked;
        ps.computeBlocked(pos, blocked);
        ASSERT_EQ(BitBoard::sqMask(F2, F3, E6, E7), blocked);
    }
    {
        std::string startFen = "3k4/b2P4/3P4/2P5/1P6/P7/8/4K3 w - - 0 1";
        std::string goalFen  = "3k4/3P2b1/3P4/2P5/1P6/P7/8/4K3 w - - 0 1";
        ProofGame ps(startFen, goalFen);
        ASSERT_EQ(INT_MAX, hScore(startFen, goalFen));
        U64 blocked;
        Position pos = TextIO::readFEN(startFen);
        ps.computeBlocked(pos, blocked);
        ASSERT_EQ(BitBoard::sqMask(A3, B4, C5, D6, D7, D8), blocked);
    }
}

TEST(ProofGameTest, testFilter) {
    ProofGameTest::testFilter();
}

void ProofGameTest::testFilter() {
    auto contains = [](const std::string& str, const std::string value) {
        return str.find(value) != std::string::npos;
    };
    struct Data {
        std::string fen;
        std::string status;
        bool value;
    };
    std::vector<Data> v = {
        { TextIO::startPosFEN, "illegal", false },
        { "rnbqkbnr/p1pppppp/p7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "illegal", true }, // invalid pawn capture
        { "rnbqkbnr/p1pppppp/p7/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 1", "illegal", false }, // a4 Nf6 a5 Ng8 a6 bxa6
        { "nnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQk - 0 1", "illegal", true }, // Too many black knights
        { "8/8/8/8/8/8/8/Kk6 w - - 0 1", "illegal", true }, // King capture possible
        { "8/8/8/8/8/8/8/KRk5 w - - 0 1", "illegal", true }, // King capture possible
        { "8/8/8/8/8/8/8/KRk5 b - - 0 1", "illegal", false }, // King in check

        // All possible captures for last move rejected
        { "k1bBrR2/1B1rbN2/p2BN1Q1/2n3bP/p2bNR1p/2R1n3/P4b2/1K3b2 b - - 0 1", "illegal", true },
        { "nB1kr3/pbnBR3/P2PpQ1Q/2N2K2/r1r1r3/qPr5/qR2P2q/1N5r w - - 1 2", "illegal", true },
        { "4B2n/Bqp2Nbr/r1r2p1B/5b2/n3bN2/1QR1q1b1/3kr2B/QKb2B1R w - - 0 1", "illegal", true },
    };
    for (const Data& d : v) {
        std::stringstream in;
        in << d.fen << std::endl;
        std::stringstream out;
        ProofGame::filterFens(in, out);
        ASSERT_EQ(d.value, contains(out.str(), d.status))
            << (d.value ? "" : "!")  << d.status << ": " << d.fen;
    }
}
