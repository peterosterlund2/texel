/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * moveGenTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "moveGen.hpp"
#include "textio.hpp"
#include "posutil.hpp"

#include <vector>
#include <iostream>

#include "gtest/gtest.h"

template <typename T>
static bool
containsAll(const std::vector<T>& v, const std::vector<T>& e) {
    for (size_t i = 0; i < e.size(); i++)
        if (!contains(v, e[i]))
            return false;
    return true;
}

static void removeIllegal(Position& pos, MoveList& moveList) {
    MoveList ml2;
    for (int i = 0; i < moveList.size; i++) {
        const Move& m = moveList[i];
        if (MoveGen::isLegal(pos, moveList[i], MoveGen::inCheck(pos)))
            ml2.addMove(m.from(), m.to(), m.promoteTo());
    }
    MoveGen::removeIllegal(pos, moveList);
    ASSERT_EQ(moveList.size, ml2.size);
}

static std::vector<std::string>
getCaptureList(Position& pos, bool includeChecks, bool onlyLegal) {
    MoveList moves;
    if (includeChecks)
        MoveGen::pseudoLegalCapturesAndChecks(pos, moves);
    else
        MoveGen::pseudoLegalCaptures(pos, moves);
    if (onlyLegal)
        removeIllegal(pos, moves);
    std::vector<std::string> strMoves;
    for (int mi = 0; mi < moves.size; mi++) {
        const Move& m = moves[mi];
        std::string mStr = TextIO::moveToUCIString(m);
        strMoves.push_back(mStr);
    }
    return strMoves;
}

static std::vector<std::string>
getCheckEvasions(Position& pos, bool onlyLegal) {
    std::vector<std::string> strMoves;
    if (!MoveGen::inCheck(pos))
        return strMoves;
    MoveList moves;
    MoveGen::checkEvasions(pos, moves);
    if (onlyLegal)
        removeIllegal(pos, moves);
    for (int mi = 0; mi < moves.size; mi++) {
        const Move& m = moves[mi];
        std::string mStr = TextIO::moveToUCIString(m);
        strMoves.push_back(mStr);
    }
    return strMoves;
}

static std::vector<std::string>
getMoveList0(Position& pos, bool onlyLegal) {
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    if (onlyLegal)
        removeIllegal(pos, moves);
    std::vector<std::string> strMoves;
    for (int mi = 0; mi < moves.size; mi++) {
        const Move& m = moves[mi];
        std::string mStr = TextIO::moveToUCIString(m);
        strMoves.push_back(mStr);
    }

    std::vector<std::string> capList1 = getCaptureList(pos, false, onlyLegal);
    EXPECT_TRUE(containsAll(strMoves, capList1));

    std::vector<std::string> capList2 = getCaptureList(pos, true, onlyLegal);
    EXPECT_TRUE(containsAll(strMoves, capList2));

    bool inCheck = MoveGen::inCheck(pos);
    std::vector<std::string> evList = getCheckEvasions(pos, onlyLegal);
    if (inCheck) {
        EXPECT_TRUE(containsAll(strMoves, evList));
    }
    for (size_t i = 0; i < strMoves.size(); i++) {
        const std::string& sm = strMoves[i];
        Move m = TextIO::uciStringToMove(sm);
        if (!m.isEmpty() && !MoveGen::isLegal(pos, m, MoveGen::inCheck(pos)))
            m.setMove(A1,A1,0,0);
        if (m.isEmpty()) // Move was illegal (but pseudo-legal)
            continue;
        bool qProm = false; // Promotion types considered in qsearch
        switch (m.promoteTo()) {
        case Piece::WQUEEN:  case Piece::BQUEEN:
        case Piece::WKNIGHT: case Piece::BKNIGHT:
        case Piece::EMPTY:
            qProm = true;
            break;
        default:
            break;
        }
        if (!MoveGen::canTakeKing(pos) && MoveGen::givesCheck(pos, m)) {
            if (qProm) {
                EXPECT_TRUE(contains(capList2, sm));
            }
        } else {
            switch (m.promoteTo()) {
            case Piece::WQUEEN: case Piece::BQUEEN:
            case Piece::WKNIGHT: case Piece::BKNIGHT:
                EXPECT_TRUE(contains(capList1, sm)); // All queen/knight promotions
                EXPECT_TRUE(contains(capList2, sm)); // All queen/knight promotions
                break;
            case Piece::EMPTY:
                break;
            default:
                EXPECT_TRUE(!contains(capList1, sm)); // No rook/bishop promotions
                EXPECT_TRUE(!contains(capList2, sm)); // No rook/bishop promotions
                break;
            }
        }
        if (pos.getPiece(m.to()) != Piece::EMPTY) {
            if (qProm) {
                EXPECT_TRUE(contains(capList1, sm));
                EXPECT_TRUE(contains(capList2, sm));
            }
        }
        if (inCheck) {
            EXPECT_TRUE(contains(evList, sm));
        }
    }

    return strMoves;
}

static std::vector<std::string>
getMoveList(Position& pos, bool onlyLegal) {
    Position swap = PosUtil::swapColors(pos);
    std::vector<std::string> swapList = getMoveList0(swap, onlyLegal);
    std::vector<std::string> ret = getMoveList0(pos, onlyLegal);
    EXPECT_EQ(swapList.size(), ret.size());

    std::vector<std::string> retSwapped;
    for (const auto& ms : ret) {
        Move m = TextIO::uciStringToMove(ms);
        int promoteTo = m.promoteTo();
        if (promoteTo != Piece::EMPTY)
            promoteTo = Piece::isWhite(promoteTo) ?
                        Piece::makeBlack(promoteTo) :
                        Piece::makeWhite(promoteTo);
        m.setMove(Square::mirrorY(m.from()), Square::mirrorY(m.to()), promoteTo, 0);
        std::string msSwapped = TextIO::moveToUCIString(m);
        retSwapped.push_back(msSwapped);
    }
    std::sort(swapList.begin(), swapList.end());
    std::sort(retSwapped.begin(), retSwapped.end());
    for (size_t i = 0; i < swapList.size(); i++)
        EXPECT_EQ(swapList[i], retSwapped[i]);

    return ret;
}

TEST(MoveGenTest, testPseudoLegalMoves) {
    std::string fen = "8/3k4/8/2n2pP1/1P6/1NB5/2QP4/R3K2R w KQ f6 0 2";
    Position pos = TextIO::readFEN(fen);
    EXPECT_EQ(fen, TextIO::toFEN(pos));
    std::vector<std::string> strMoves = getMoveList(pos, false);
    EXPECT_TRUE(contains(strMoves, "a1d1"));
    EXPECT_TRUE(!contains(strMoves, "a1e1"));
    EXPECT_TRUE(!contains(strMoves, "a1f1"));
    EXPECT_TRUE(contains(strMoves, "a1a7"));
    EXPECT_TRUE(contains(strMoves, "e1f2"));
    EXPECT_TRUE(!contains(strMoves, "e1g3"));
    EXPECT_TRUE(contains(strMoves, "c3f6"));
    EXPECT_TRUE(!contains(strMoves, "b3d2"));

    // Test castling
    EXPECT_TRUE(contains(strMoves, "e1g1"));
    EXPECT_TRUE(contains(strMoves, "e1c1"));
    EXPECT_EQ(49, strMoves.size());

    pos.setPiece(Square::getSquare(4,3), Piece::BROOK);
    strMoves = getMoveList(pos, false);
    EXPECT_TRUE(!contains(strMoves, "e1g1"));      // In check, no castling possible
    EXPECT_TRUE(!contains(strMoves, "e1c1"));

    pos.setPiece(Square::getSquare(4, 3), Piece::EMPTY);
    pos.setPiece(Square::getSquare(5, 3), Piece::BROOK);
    strMoves = getMoveList(pos, false);
    EXPECT_TRUE(!contains(strMoves, "e1g1"));      // f1 attacked, short castle not possible
    EXPECT_TRUE(contains(strMoves, "e1c1"));

    pos.setPiece(Square::getSquare(5, 3), Piece::EMPTY);
    pos.setPiece(Square::getSquare(6, 3), Piece::BBISHOP);
    strMoves = getMoveList(pos, false);
    EXPECT_TRUE(contains(strMoves, "e1g1"));      // d1 attacked, long castle not possible
    EXPECT_TRUE(!contains(strMoves, "e1c1"));

    pos.setPiece(Square::getSquare(6, 3), Piece::EMPTY);
    pos.setCastleMask(1 << Position::A1_CASTLE);
    strMoves = getMoveList(pos, false);
    EXPECT_TRUE(!contains(strMoves, "e1g1"));      // short castle right has been lost
    EXPECT_TRUE(contains(strMoves, "e1c1"));
}

TEST(MoveGenTest, testPawnMoves) {
    std::string fen = "1r2k3/P1pppp1p/8/1pP3p1/1nPp2P1/n4p1P/1P2PP2/4KBNR w K b6 0 1";
    Position pos = TextIO::readFEN(fen);
    EXPECT_EQ(fen, TextIO::toFEN(pos));
    std::vector<std::string> strMoves = getMoveList(pos, false);
    EXPECT_TRUE(contains(strMoves, "c5b6"));     // En passant capture
    EXPECT_TRUE(contains(strMoves, "a7a8q"));    // promotion
    EXPECT_TRUE(contains(strMoves, "a7a8n"));    // under promotion
    EXPECT_TRUE(contains(strMoves, "a7b8r"));    // capture promotion
    EXPECT_TRUE(contains(strMoves, "b2b3"));     // pawn single move
    EXPECT_TRUE(contains(strMoves, "b2a3"));     // pawn capture to the left
    EXPECT_TRUE(contains(strMoves, "e2e4"));     // pawn double move
    EXPECT_TRUE(contains(strMoves, "e2f3"));     // pawn capture to the right
    EXPECT_EQ(22, strMoves.size());

    pos.setEpSquare(-1);
    strMoves = getMoveList(pos, false);
    EXPECT_EQ(21, strMoves.size());          // No ep, one less move possible

    // Check black pawn moves
    pos.setWhiteMove(false);
    strMoves = getMoveList(pos, false);
    EXPECT_TRUE(contains(strMoves, "f3e2"));
    EXPECT_TRUE(contains(strMoves, "d4d3"));
    EXPECT_TRUE(contains(strMoves, "e7e6"));
    EXPECT_TRUE(contains(strMoves, "e7e5"));
    EXPECT_EQ(28, strMoves.size());

    // Check black pawn promotion
    pos.setPiece(Square::getSquare(0,1), Piece::BPAWN);
    strMoves = getMoveList(pos, false);
    EXPECT_TRUE(contains(strMoves, "a2a1q"));
    EXPECT_TRUE(contains(strMoves, "a2a1r"));
    EXPECT_TRUE(contains(strMoves, "a2a1n"));
    EXPECT_TRUE(contains(strMoves, "a2a1b"));
}

TEST(MoveGenTest, testInCheck) {
    Position pos;
    pos.setPiece(Square::getSquare(4,2), Piece::WKING);
    pos.setPiece(Square::getSquare(4,7), Piece::BKING);
    EXPECT_EQ(false, MoveGen::inCheck(pos));

    pos.setPiece(Square::getSquare(3,3), Piece::BQUEEN);
    EXPECT_EQ(true, MoveGen::inCheck(pos));
    pos.setPiece(Square::getSquare(3,3), Piece::BROOK);
    EXPECT_EQ(false, MoveGen::inCheck(pos));
    pos.setPiece(Square::getSquare(3,3), Piece::BPAWN);
    EXPECT_EQ(true, MoveGen::inCheck(pos));

    pos.setPiece(Square::getSquare(3,3), Piece::EMPTY);
    pos.setPiece(Square::getSquare(5,3), Piece::WQUEEN);
    EXPECT_EQ(false, MoveGen::inCheck(pos));

    pos.setPiece(Square::getSquare(4, 6), Piece::BROOK);
    EXPECT_EQ(true, MoveGen::inCheck(pos));
    pos.setPiece(Square::getSquare(4, 4), Piece::WPAWN);
    EXPECT_EQ(false, MoveGen::inCheck(pos));

    pos.setPiece(Square::getSquare(2, 3), Piece::BKNIGHT);
    EXPECT_EQ(true, MoveGen::inCheck(pos));

    pos.setPiece(Square::getSquare(2, 3), Piece::EMPTY);
    pos.setPiece(Square::getSquare(0, 4), Piece::BKNIGHT);
    EXPECT_EQ(false, MoveGen::inCheck(pos));
}

TEST(MoveGenTest, testGivesCheck) {
    Position pos;
    UndoInfo ui;
    pos.setPiece(TextIO::getSquare("e3"), Piece::WKING);
    pos.setPiece(TextIO::getSquare("e8"), Piece::BKING);
    pos.setPiece(TextIO::getSquare("c2"), Piece::WROOK);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Rc8")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Rc6")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Rc7")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Re2")));

    pos.setPiece(TextIO::getSquare("c2"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("e2"), Piece::WROOK);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kd3")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kd4")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Ke4")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kf2")));

    pos.setPiece(TextIO::getSquare("e4"), Piece::WBISHOP);
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Bd5")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Bc6")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kd3")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Re1")));

    pos = TextIO::readFEN("4k3/3p4/8/8/4B3/2K5/4R3/8 w - - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Bc6")));
    pos = TextIO::readFEN("4k3/8/5K2/8/6N1/8/8/8 w - - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Ke6")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, Move(TextIO::getSquare("f6"),
                                          TextIO::getSquare("e7"),
                                          Piece::EMPTY)));

    pos = TextIO::readFEN("8/2k5/8/4N3/8/2K3B1/8/8 w - - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kc4")));
    pos.setPiece(TextIO::getSquare("g3"), Piece::WROOK);
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
    pos.setPiece(TextIO::getSquare("g3"), Piece::WQUEEN);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
    pos.setPiece(TextIO::getSquare("g3"), Piece::WKNIGHT);
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
    pos.setPiece(TextIO::getSquare("g3"), Piece::WPAWN);
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
    pos.setPiece(TextIO::getSquare("c3"), Piece::EMPTY);
    pos.setPiece(TextIO::getSquare("g3"), Piece::WKING);
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));

    pos = TextIO::readFEN("8/2k5/3p4/4N3/8/2K3B1/8/8 w - - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));

    pos = TextIO::readFEN("8/2k5/8/4N3/8/6q1/2K5/8 w - - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
    pos = TextIO::readFEN("8/2k5/8/4N3/8/8/2K5/8 w - - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
    pos = TextIO::readFEN("2nk4/3P4/8/8/3R4/8/2K5/8 w - - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc8N")));

    pos = TextIO::readFEN("8/2k5/2p5/1P1P4/8/2K5/8/8 w - - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "d6")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "bxc6")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b6")));

    pos = TextIO::readFEN("8/8/R1PkP2R/8/8/2K5/8/8 w - - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c7")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e7")));

    // Test pawn promotion
    pos = TextIO::readFEN("8/1P6/2kP4/8/8/2K5/8/8 w - - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "d7")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8Q")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8N")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8R")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8B")));

    pos = TextIO::readFEN("8/2P1P3/2k5/8/8/2K5/8/8 w - - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8Q")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8N")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8R")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8B")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8Q")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8N")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8R")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8B")));

    // Test castling
    pos = TextIO::readFEN("8/8/8/8/5k2/8/8/R3K2R w KQ - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
    pos = TextIO::readFEN("8/8/8/8/6k1/8/8/R3K2R w KQ - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
    pos = TextIO::readFEN("8/8/8/8/3k4/8/8/R3K2R w KQ - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
    pos = TextIO::readFEN("8/8/8/8/5k2/8/5P2/R3K2R w KQ - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
    pos = TextIO::readFEN("8/8/8/8/8/8/8/R3K2k w Q - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
    pos = TextIO::readFEN("8/8/8/8/8/8/8/2k1K2R w K - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
    pos.setPiece(TextIO::getSquare("d1"), Piece::WKNIGHT);
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));

    // Test en passant
    pos = TextIO::readFEN("8/1kp5/8/3P4/8/8/8/4K3 b - - 0 1");
    pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

    pos = TextIO::readFEN("3k4/2p5/8/3P4/8/8/3R4/4K3 b - - 0 1");
    pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

    pos = TextIO::readFEN("5k2/2p5/8/3P4/8/B7/8/4K3 b - - 0 1");
    pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

    pos = TextIO::readFEN("5k2/2p5/8/3P4/1P6/B7/8/4K3 b - - 0 1");
    pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

    pos = TextIO::readFEN("8/2p5/8/R2P1k2/8/8/8/4K3 b - - 0 1");
    pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

    // Black pawn moves
    pos = TextIO::readFEN("8/2p5/8/R4k2/1K6/8/8/8 b - - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c5")));
    pos = TextIO::readFEN("8/2p5/8/R4k2/2K5/8/8/8 b - - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c5")));
    pos = TextIO::readFEN("8/2p5/8/R4k2/3K4/8/8/8 b - - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c5")));

    // Black castling
    pos = TextIO::readFEN("r3k2r/8/8/5K2/8/8/8/8 b kq - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
    pos = TextIO::readFEN("r3k2r/8/8/6K1/8/8/8/8 b kq - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
    pos = TextIO::readFEN("r3k2r/8/8/2K5/8/8/8/8 b kq - 0 1");
    EXPECT_TRUE(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
    pos = TextIO::readFEN("r3k2r/8/8/3K4/8/8/8/8 b kq - 0 1");
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));

    // Black en passant
    pos = TextIO::readFEN("8/8/4k3/8/4p3/8/5PK1/8 w - - 0 1");
    pos.makeMove(TextIO::stringToMove(pos, "f4"), ui);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "exf3")));

    pos = TextIO::readFEN("8/8/4k3/8/K3p1r1/8/5P2/8 w - - 0 1");
    pos.makeMove(TextIO::stringToMove(pos, "f4"), ui);
    EXPECT_TRUE(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "exf3")));
}

TEST(MoveGenTest, testRemoveIllegal) {
    Position pos = TextIO::readFEN("8/3k4/8/2n1rpP1/1P6/1NB5/2QP4/R3K2R w KQ f6 0 1");
    std::vector<std::string> strMoves = getMoveList(pos, true);
    EXPECT_TRUE(contains(strMoves, "c2e4"));
    EXPECT_TRUE(contains(strMoves, "c3e5"));
    EXPECT_TRUE(contains(strMoves, "e1d1"));
    EXPECT_TRUE(contains(strMoves, "e1f1"));
    EXPECT_TRUE(contains(strMoves, "e1f2"));
    EXPECT_EQ(5, strMoves.size());

    pos = TextIO::readFEN("4k3/8/8/2KPp1r1/8/8/8/8 w - e6 0 2");
    strMoves = getMoveList(pos, true);
    EXPECT_TRUE(!contains(strMoves, "d5e6"));
    EXPECT_EQ(7, strMoves.size());

    pos = TextIO::readFEN("8/6p1/4p3/2k1Pp1B/4KP1p/6rP/8/8 w - f6 0 55");
    strMoves = getMoveList(pos, true);
    EXPECT_TRUE(contains(strMoves, "e5f6"));
    EXPECT_EQ(1, strMoves.size());
}

/** Test that captureList and captureAndcheckList are generated correctly. */
TEST(MoveGenTest, testCaptureList) {
    Position pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/3p1p2/R7/4N3/8/PPPPQPPP/2B1KB1R w Kkq - 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("rnb1kbn1/ppp1qppp/5p2/4p3/3N3r/3P4/PPP2PPP/R1BQKB1R b KQq - 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("rnb1k1n1/ppp1qppp/5p2/b3p3/1r1N4/3P4/PPP2PPP/R1BQKB1R b KQq - 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("8/8/8/8/3k4/8/4P3/4K3 w - - 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("8/8/8/3k4/8/8/4P3/4K3 w - - 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("8/8/8/3k4/4p3/8/3KP3/8 b - - 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("3k4/r2p2K1/8/8/8/8/8/8 b - - 0 1");
    getMoveList(pos, false);
}

TEST(MoveGenTest, testCheckEvasions) {
    Position pos = TextIO::readFEN("n7/8/8/7k/5pP1/5K2/8/8 b - g3 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("rn1qkbnr/pppB1ppp/3p4/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1");
    getMoveList(pos, false);

    pos = TextIO::readFEN("1R6/1brk2p1/2P1p2p/p3Pp2/P7/6P1/1P4P1/2R3K1 b - - 0 1");
    getMoveList(pos, false);
    std::vector<std::string> evList = getCheckEvasions(pos, false);
    EXPECT_TRUE(contains(evList, "b7c6"));
}
