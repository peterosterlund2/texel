/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * textioTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "textio.hpp"

#include "gtest/gtest.h"


/** Tests if trying to parse a FEN string causes an error. */
static bool
testFENParseError(const std::string& fen) {
    bool wasError = false;
    try {
        TextIO::readFEN(fen);
    } catch (const ChessParseError&) {
        wasError = true;
    }
    return wasError;
}

TEST(TextIOTest, testReadFEN) {
    std::string fen = "rnbqk2r/1p3ppp/p7/1NpPp3/QPP1P1n1/P4N2/4KbPP/R1B2B1R b kq - 0 1";
    Position pos = TextIO::readFEN(fen);
    EXPECT_EQ(fen, TextIO::toFEN(pos));
    EXPECT_EQ(Piece::WQUEEN, pos.getPiece(Square::getSquare(0, 3)));
    EXPECT_EQ(Piece::BKING, pos.getPiece(Square::getSquare(4, 7)));
    EXPECT_EQ(Piece::WKING, pos.getPiece(Square::getSquare(4, 1)));
    EXPECT_EQ(false, pos.isWhiteMove());
    EXPECT_EQ(false, pos.a1Castle());
    EXPECT_EQ(false, pos.h1Castle());
    EXPECT_EQ(true, pos.a8Castle());
    EXPECT_EQ(true, pos.h8Castle());

    fen = "8/3k4/8/5pP1/1P6/1NB5/2QP4/R3K2R w KQ f6 1 2";
    pos = TextIO::readFEN(fen);
    EXPECT_EQ(fen, TextIO::toFEN(pos));
    EXPECT_EQ(1, pos.getHalfMoveClock());
    EXPECT_EQ(2, pos.getFullMoveCounter());

    // Must have exactly one king
    bool wasError = testFENParseError("8/8/8/8/8/8/8/kk1K4 w - - 0 1");
    EXPECT_EQ(true, wasError);

    // Must not be possible to capture the king
    wasError = testFENParseError("8/8/8/8/8/8/8/k1RK4 w - - 0 1");
    EXPECT_EQ(true, wasError);

    // Make sure bogus en passant square information is removed
    fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
    pos = TextIO::readFEN(fen);
    EXPECT_EQ(-1, pos.getEpSquare());

    // Test for too many rows (slashes)
    wasError = testFENParseError("8/8/8/8/4k3/8/8/8/KBN5 w - - 0 1");
    EXPECT_EQ(true, wasError);

    // Test for too many columns
    wasError = testFENParseError("8K/8/8/8/4k3/8/8/8 w - - 0 1");
    EXPECT_EQ(true, wasError);

    // Pawns must not be on first/last rank
    wasError = testFENParseError("kp6/8/8/8/8/8/8/K7 w - - 0 1");
    EXPECT_EQ(true, wasError);

    wasError = testFENParseError("kr/pppp/8/8/8/8/8/KBR w");
    EXPECT_EQ(false, wasError);  // OK not to specify castling flags and ep square

    wasError = testFENParseError("k/8/8/8/8/8/8/K");
    EXPECT_EQ(true, wasError);   // Error side to move not specified

    wasError = testFENParseError("");
    EXPECT_EQ(true, wasError);

    wasError = testFENParseError("    |");
    EXPECT_EQ(true, wasError);

    wasError = testFENParseError("1B1B4/6k1/7r/7P/6q1/r7/q7/7K b - - acn 6; acs 0;");
    EXPECT_EQ(false, wasError);  // Extra stuff after FEN string is allowed

    // Test invalid en passant square detection
    pos = TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/8/PPP2PPP/RNBQKBNR b KQkq d3 0 1");
    EXPECT_EQ(TextIO::getSquare("d3"), pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/8/PPP2PPP/RNBQKBNR w KQkq d3 0 1");
    EXPECT_EQ(pos, TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1"));

    pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/8/2Ppp3/8/8/PP1PPPPP/RNBQKBNR w KQkq d6 0 1");
    EXPECT_EQ(TextIO::getSquare("d6"), pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/8/2Ppp3/8/8/PP1PPPPP/RNBQKBNR b KQkq d6 0 1");
    EXPECT_EQ(pos, TextIO::readFEN("rnbqkbnr/ppp2ppp/8/2Ppp3/8/8/PP1PPPPP/RNBQKBNR b KQkq - 0 1"));

    pos = TextIO::readFEN("rnbqkbnr/pppppppp/8/8/3PP3/8/PPP2PPP/RNBQKBNR b KQkq d3 0 1");
    EXPECT_EQ(-1, pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/8/3pp3/8/8/PPPPPPPP/RNBQKBNR w KQkq e6 0 1");
    EXPECT_EQ(-1, pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/3P4/PP3PPP/RNBQKBNR b KQkq d3 0 1");
    EXPECT_EQ(pos, TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/3P4/PP3PPP/RNBQKBNR b KQkq - 0 1"));
}

TEST(TextIOTest, testMoveToString) {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    EXPECT_EQ(TextIO::startPosFEN, TextIO::toFEN(pos));
    Move move(Square::getSquare(4, 1), Square::getSquare(4, 3), Piece::EMPTY);
    bool longForm = true;
    std::string result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("e2-e4", result);

    move = Move(Square::getSquare(6, 0), Square::getSquare(5, 2), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("Ng1-f3", result);

    move = Move(Square::getSquare(4, 7), Square::getSquare(2, 7), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("O-O-O", result);

    std::string fen = "1r3k2/2P5/8/8/8/4K3/8/8 w - - 0 1";
    pos = TextIO::readFEN(fen);
    EXPECT_EQ(fen, TextIO::toFEN(pos));
    move = Move(Square::getSquare(2,6), Square::getSquare(1,7), Piece::WROOK);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("c7xb8R+", result);

    move = Move(Square::getSquare(2,6), Square::getSquare(2,7), Piece::WKNIGHT);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("c7-c8N", result);

    move = Move(Square::getSquare(2,6), Square::getSquare(2,7), Piece::WQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("c7-c8Q+", result);
}

TEST(TextIOTest, testMoveToStringMate) {
    Position pos = TextIO::readFEN("3k4/1PR5/3N4/8/4K3/8/8/8 w - - 0 1");
    bool longForm = true;

    Move move(Square::getSquare(1, 6), Square::getSquare(1, 7), Piece::WROOK);
    std::string result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("b7-b8R+", result);    // check

    move = Move(Square::getSquare(1, 6), Square::getSquare(1, 7), Piece::WQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("b7-b8Q#", result);    // check mate

    move = Move(Square::getSquare(1, 6), Square::getSquare(1, 7), Piece::WKNIGHT);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("b7-b8N", result);

    move = Move(Square::getSquare(1, 6), Square::getSquare(1, 7), Piece::WBISHOP);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("b7-b8B", result);     // stalemate
}

TEST(TextIOTest, testMoveToStringShortForm) {
    std::string fen = "r4rk1/2pn3p/2q1q1n1/8/2q2p2/6R1/p4PPP/1R4K1 b - - 0 1";
    Position pos = TextIO::readFEN(fen);
    EXPECT_EQ(fen, TextIO::toFEN(pos));
    bool longForm = false;

    Move move = Move(Square::getSquare(4,5), Square::getSquare(4,3), Piece::EMPTY);
    std::string result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("Qee4", result);   // File disambiguation needed

    move = Move(Square::getSquare(2,5), Square::getSquare(4,3), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("Qc6e4", result);  // Full disambiguation needed

    move = Move(Square::getSquare(2,3), Square::getSquare(4,3), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("Q4e4", result);   // Row disambiguation needed

    move = Move(Square::getSquare(2,3), Square::getSquare(2,0), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("Qc1+", result);   // No disambiguation needed

    move = Move(Square::getSquare(0,1), Square::getSquare(0,0), Piece::BQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("a1Q", result);    // Normal promotion

    move = Move(Square::getSquare(0,1), Square::getSquare(1,0), Piece::BQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("axb1Q#", result); // Capture promotion and check mate

    move = Move(Square::getSquare(0,1), Square::getSquare(1,0), Piece::BKNIGHT);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("axb1N", result);  // Capture promotion

    move = Move(Square::getSquare(3,6), Square::getSquare(4,4), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("Ne5", result);    // Other knight pinned, no disambiguation needed

    move = Move(Square::getSquare(7,6), Square::getSquare(7,4), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("h5", result);     // Regular pawn move

    move = Move(Square::getSquare(5,7), Square::getSquare(3,7), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    EXPECT_EQ("Rfd8", result);     // File disambiguation needed
}

TEST(TextIOTest, testStringToMove) {
    Position pos = TextIO::readFEN("r4rk1/2pn3p/2q1q1n1/8/2q2p2/6R1/p4PPP/1R4K1 b - - 0 1");

    Move mNe5(Square::getSquare(3, 6), Square::getSquare(4, 4), Piece::EMPTY);
    Move m = TextIO::stringToMove(pos, "Ne5");
    EXPECT_EQ(mNe5, m);
    m = TextIO::stringToMove(pos, "ne");
    EXPECT_EQ(mNe5, m);
    m = TextIO::stringToMove(pos, "N");
    EXPECT_TRUE(m.isEmpty());

    Move mQc6e4(Square::getSquare(2, 5), Square::getSquare(4, 3), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "Qc6-e4");
    EXPECT_EQ(mQc6e4, m);
    m = TextIO::stringToMove(pos, "Qc6e4");
    EXPECT_EQ(mQc6e4, m);
    m = TextIO::stringToMove(pos, "Qce4");
    EXPECT_TRUE(m.isEmpty());
    m = TextIO::stringToMove(pos, "Q6e4");
    EXPECT_TRUE(m.isEmpty());

    Move maxb1Q(Square::getSquare(0, 1), Square::getSquare(1, 0), Piece::BQUEEN);
    m = TextIO::stringToMove(pos, "axb1Q");
    EXPECT_EQ(maxb1Q, m);
    m = TextIO::stringToMove(pos, "axb1Q#");
    EXPECT_EQ(maxb1Q, m);
    m = TextIO::stringToMove(pos, "axb1Q+");
    EXPECT_EQ(maxb1Q, m);

    Move mh5(Square::getSquare(7, 6), Square::getSquare(7, 4), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "h5");
    EXPECT_EQ(mh5, m);
    m = TextIO::stringToMove(pos, "h7-h5");
    EXPECT_EQ(mh5, m);
    m = TextIO::stringToMove(pos, "h");
    EXPECT_TRUE(m.isEmpty());

    pos = TextIO::readFEN("r1b1k2r/1pqpppbp/p5pn/3BP3/8/2pP4/PPPBQPPP/R3K2R w KQkq - 0 12");
    m = TextIO::stringToMove(pos, "bxc3");
    EXPECT_EQ(TextIO::getSquare("b2"), m.from());
    m = TextIO::stringToMove(pos, "Bxc3");
    EXPECT_EQ(TextIO::getSquare("d2"), m.from());
    m = TextIO::stringToMove(pos, "bxc");
    EXPECT_EQ(TextIO::getSquare("b2"), m.from());
    m = TextIO::stringToMove(pos, "Bxc");
    EXPECT_EQ(TextIO::getSquare("d2"), m.from());

    // Test castling. o-o is a substring of o-o-o, which could cause problems.
    pos = TextIO::readFEN("5k2/p1pQn3/1p2Bp1r/8/4P1pN/2N5/PPP2PPP/R3K2R w KQ - 0 16");
    Move kCastle(Square::getSquare(4,0), Square::getSquare(6,0), Piece::EMPTY);
    Move qCastle(Square::getSquare(4,0), Square::getSquare(2,0), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "o");
    EXPECT_TRUE(m.isEmpty());
    m = TextIO::stringToMove(pos, "o-o");
    EXPECT_EQ(kCastle, m);
    m = TextIO::stringToMove(pos, "O-O");
    EXPECT_EQ(kCastle, m);
    m = TextIO::stringToMove(pos, "o-o-o");
    EXPECT_EQ(qCastle, m);

    // Test 'o-o+'
    pos.setPiece(Square::getSquare(5,1), Piece::EMPTY);
    pos.setPiece(Square::getSquare(5,5), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "o");
    EXPECT_TRUE(m.isEmpty());
    m = TextIO::stringToMove(pos, "o-o");
    EXPECT_EQ(kCastle, m);
    m = TextIO::stringToMove(pos, "o-o-o");
    EXPECT_EQ(qCastle, m);
    m = TextIO::stringToMove(pos, "o-o+");
    EXPECT_EQ(kCastle, m);

    // Test d8=Q+ syntax
    pos = TextIO::readFEN("1r3r2/2kP2Rp/p1bN1p2/2p5/5P2/2P5/P5PP/3R2K1 w - -");
    m = TextIO::stringToMove(pos, "d8=Q+");
    Move m2 = TextIO::stringToMove(pos, "d8Q");
    EXPECT_EQ(m2, m);

    // Test non-standard castling syntax
    pos = TextIO::readFEN("r3k2r/pppqbppp/2npbn2/4p3/2B1P3/2NPBN2/PPPQ1PPP/R3K2R w KQkq - 0 1");
    m = TextIO::stringToMove(pos, "0-0");
    EXPECT_EQ(Move(TextIO::getSquare("e1"), TextIO::getSquare("g1"), Piece::EMPTY), m);
    m = TextIO::stringToMove(pos, "0-0-0");
    EXPECT_EQ(Move(TextIO::getSquare("e1"), TextIO::getSquare("c1"), Piece::EMPTY), m);
    pos.setWhiteMove(false);
    m = TextIO::stringToMove(pos, "0-0");
    EXPECT_EQ(Move(TextIO::getSquare("e8"), TextIO::getSquare("g8"), Piece::EMPTY), m);
    m = TextIO::stringToMove(pos, "0-0-0");
    EXPECT_EQ(Move(TextIO::getSquare("e8"), TextIO::getSquare("c8"), Piece::EMPTY), m);

    // Test non-standard disambiguation
    pos = TextIO::readFEN("1Q6/1K2q2k/1QQ5/8/7P/8/8/8 w - - 3 88");
    m = TextIO::stringToMove(pos, "Qb8c7");
    EXPECT_EQ(Move(TextIO::getSquare("b8"), TextIO::getSquare("c7"), Piece::EMPTY), m);
    m2 = TextIO::stringToMove(pos, "Q8c7");
    EXPECT_EQ(m2, m);

    // Test extra characters
    pos = TextIO::readFEN(TextIO::startPosFEN);
    Move mNf3(TextIO::getSquare("g1"), TextIO::getSquare("f3"), Piece::EMPTY);
    EXPECT_EQ(mNf3, TextIO::stringToMove(pos, "Ngf3"));
    EXPECT_EQ(mNf3, TextIO::stringToMove(pos, "Ng1f3"));
    EXPECT_EQ(mNf3, TextIO::stringToMove(pos, "Ng1-f3"));
    EXPECT_EQ(mNf3, TextIO::stringToMove(pos, "g1f3"));
    EXPECT_EQ(mNf3, TextIO::stringToMove(pos, "N1f3"));
    EXPECT_EQ(mNf3, TextIO::stringToMove(pos, "Ngf"));
    EXPECT_EQ(mNf3, TextIO::stringToMove(pos, "Nf"));
}

TEST(TextIOTest, testGetSquare) {
    EXPECT_EQ(Square::getSquare(0, 0), TextIO::getSquare("a1"));
    EXPECT_EQ(Square::getSquare(1, 7), TextIO::getSquare("b8"));
    EXPECT_EQ(Square::getSquare(3, 3), TextIO::getSquare("d4"));
    EXPECT_EQ(Square::getSquare(4, 3), TextIO::getSquare("e4"));
    EXPECT_EQ(Square::getSquare(3, 1), TextIO::getSquare("d2"));
    EXPECT_EQ(Square::getSquare(7, 7), TextIO::getSquare("h8"));
}

TEST(TextIOTest, testSquareToString) {
    EXPECT_EQ("a1", TextIO::squareToString(Square::getSquare(0, 0)));
    EXPECT_EQ("h6", TextIO::squareToString(Square::getSquare(7, 5)));
    EXPECT_EQ("e4", TextIO::squareToString(Square::getSquare(4, 3)));
}

static int countSubStr(const std::string& str, const std::string& sub) {
    int N = sub.length();
    int ret = 0;
    size_t pos = 0;
    while (pos < str.length()) {
        pos = str.find(sub, pos);
        if (pos == std::string::npos)
            break;
        else {
            pos += N;
            ret++;
        }
    }
    return ret;
}

TEST(TextIOTest, testAsciiBoard) {
    Position pos = TextIO::readFEN("r4rk1/2pn3p/2q1q1n1/8/2q2p2/6R1/p4PPP/1R4K1 b - - 0 1");
    std::string aBrd = TextIO::asciiBoard(pos);
    //        printf("%s\n", aBrd.c_str());
    EXPECT_EQ(12, countSubStr(aBrd, "*")); // 12 black pieces
    EXPECT_EQ(3, countSubStr(aBrd, "*Q")); // 3 black queens
    EXPECT_EQ(3, countSubStr(aBrd, " P")); // 3 white pawns

    aBrd = TextIO::asciiBoard(BitBoard::sqMask(A1,C2,D4));
    EXPECT_EQ(3, countSubStr(aBrd, "1"));
    std::string sqList = TextIO::squareList(BitBoard::sqMask(A1,C2,D4));
    EXPECT_EQ("a1,c2,d4", sqList);
}

TEST(TextIOTest, testUciStringToMove) {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    Move m = TextIO::uciStringToMove("e2e4");
    EXPECT_EQ(TextIO::stringToMove(pos, "e4"), m);
    m = TextIO::uciStringToMove("e2e5");
    EXPECT_EQ(Move(12, 12+8*3, Piece::EMPTY), m);

    m = TextIO::uciStringToMove("e2e5q");
    EXPECT_TRUE(m.isEmpty());

    m = TextIO::uciStringToMove("e7e8q");
    EXPECT_EQ(Piece::WQUEEN, m.promoteTo());
    m = TextIO::uciStringToMove("e7e8r");
    EXPECT_EQ(Piece::WROOK, m.promoteTo());
    m = TextIO::uciStringToMove("e7e8b");
    EXPECT_EQ(Piece::WBISHOP, m.promoteTo());
    m = TextIO::uciStringToMove("e2e1n");
    EXPECT_EQ(Piece::BKNIGHT, m.promoteTo());
    m = TextIO::uciStringToMove("e7e8x");
    EXPECT_TRUE(m.isEmpty());  // Invalid promotion piece
    m = TextIO::uciStringToMove("i1i3");
    EXPECT_TRUE(m.isEmpty());  // Outside board
    m = TextIO::uciStringToMove("h8h9");
    EXPECT_TRUE(m.isEmpty());  // Outside board
    m = TextIO::uciStringToMove("c1c0");
    EXPECT_TRUE(m.isEmpty());  // Outside board
}
