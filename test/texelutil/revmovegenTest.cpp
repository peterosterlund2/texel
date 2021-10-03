/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * revmovegenTest.hpp
 *
 *  Created on: Oct 3, 2021
 *      Author: petero
 */

#include "revmovegenTest.hpp"
#include "revmovegen.hpp"
#include "textio.hpp"
#include "posutil.hpp"
#include "stloutput.hpp"
#include <set>

#include "gtest/gtest.h"

class UnMoveCompare {
public:
    bool operator()(const UnMove& um1, const UnMove& um2) const {
        U16 cm1 = um1.move.getCompressedMove();
        U16 cm2 = um2.move.getCompressedMove();
        if (cm1 != cm2)
            return cm1 < cm2;
        return compare(um1.ui, um2.ui);
    }

private:
    bool compare(const UndoInfo& ui1, const UndoInfo& ui2) const {
        if (ui1.capturedPiece != ui2.capturedPiece)
            return ui1.capturedPiece < ui2.capturedPiece;
        if (ui1.castleMask != ui2.castleMask)
            return ui1.castleMask < ui2.castleMask;
        return ui1.epSquare < ui2.epSquare;
    }
};
using UnMoveSet = std::set<UnMove, UnMoveCompare>;

static std::string
mirrorFenY(const std::string& fen) {
    Position pos = TextIO::readFEN(fen);
    pos = PosUtil::swapColors(pos);
    return TextIO::toFEN(pos);
}

static Move
mirrorMoveY(const Move& m) {
    int from = Square::mirrorY(m.from());
    int to = Square::mirrorY(m.to());
    int promoteTo = PosUtil::swapPieceColor(m.promoteTo());
    return Move(from, to, promoteTo);
}

static std::string
mirrorUciMoveY(const std::string& move) {
    Move m = TextIO::uciStringToMove(move);
    m = mirrorMoveY(m);
    return TextIO::moveToUCIString(m);
}

static UndoInfo
mirrorUndoInfoY(const UndoInfo& ui) {
    UndoInfo ret;
    ret.capturedPiece = PosUtil::swapPieceColor(ui.capturedPiece);
    ret.castleMask = PosUtil::swapCastleMask(ui.castleMask);
    ret.epSquare = ui.epSquare == -1 ? -1 : Square::mirrorY(ui.epSquare);
    ret.halfMoveClock = ui.halfMoveClock;
    return ret;
}


TEST(RevMoveGenTest, testMoves) {
    RevMoveGenTest::testMoves();
}

void
RevMoveGenTest::testMoves() {
    auto extractMoves = [](const std::vector<UnMove>& unMoves) {
        std::set<std::string> ret;
        for (const UnMove& um : unMoves)
            ret.insert(TextIO::moveToUCIString(um.move));
        return ret;
    };

    struct Data {
        std::string fen;
        std::set<std::string> unmoves;
    };
    std::vector<Data> v = {
        { TextIO::startPosFEN, {"a6b8", "c6b8", "f6g8", "h6g8"} }, // knights
        { "8/4k3/8/8/4P3/4K3/8/8 w - - 0 1", {"d6e7", "d7e7", "d8e7",
                                              "e6e7",         "e8e7",
                                              "f6e7", "f7e7", "f8e7"} }, // king
        { "8/4k3/8/8/4P3/4K3/8/8 b - - 0 1", {"d2e3", "d3e3", "d3e4", "d4e3",
                                              "e2e3",
                                              "f2e3", "f3e3", "f3e4", "f4e3"} },
        { "k7/p2p4/1p5p/5p2/4P3/4K3/8/8 w - - 0 1", // king, pawns
          { "b7a8", "b8a8",
            "b7b6", "c7b6",
            "e6f5", "f6f5", "f7f5", "g6f5",
            "g7h6", "h7h6",
          }
        },
        { "3kn3/4p3/3p4/8/5Q2/3R4/8/2B1K3 b - - 0 1",
          { "a3c1", "b2c1", "e3c1", "d2c1",                         // bishop
            "d1e1", "d2e1", "e2e1", "f1e1", "f2e1",                 // king
            "a3d3", "b3d3", "c3d3", "e3d3", "f3d3", "g3d3", "h3d3", // rook
            "d1d3", "d2d3", "d4d3", "d5d3",                         // rook
            "d2f4", "e3f4", "g5f4", "h6f4", "e5f4", "g3f4", "h2f4", // queen
            "a4f4", "b4f4", "c4f4", "d4f4", "e4f4", "g4f4", "h4f4", // queen
            "f1f4", "f2f4", "f3f4", "f5f4", "f6f4", "f7f4", "f8f4", // queen
          }
        },
        // Short castling
        { "4k3/4p3/8/8/8/8/5P2/3n1RK1 b - - 0 1", {"e1g1", "g2g1", "h1g1", "h2g1", "e1f1"} },
        { "4k3/4p3/8/8/8/7b/5P2/3n1RK1 b - - 0 1", {"g2g1", "h1g1", "h2g1", "e1f1"} },
        { "4k3/4r3/8/8/8/8/5P2/3n1RK1 b - - 0 1", {"g2g1", "h1g1", "h2g1", "e1f1"} },
        { "4k3/8/8/8/8/8/5P2/4nRK1 b - - 0 1", {"g2g1", "h1g1", "h2g1"} },
        { "4k3/4p3/8/8/8/8/5P2/3n1RKn b - - 0 1", {"g2g1", "h2g1", "e1f1"} },
        { "4k3/4p3/8/8/8/8/5P2/3n1RKN b - - 0 1", {"g2g1", "h2g1", "e1f1", "g3h1"} },
        // Long castling
        { "4k3/4p3/8/8/8/8/3P4/2KR1n2 b - - 0 1", {"b1c1", "b2c1", "c2c1", "e1c1", "e1d1"} },
        { "4k3/8/8/8/8/4n3/3P4/2KR1n2 b - - 0 1", {"b1c1", "b2c1", "c2c1", "e1d1"} },
        { "4k3/4p3/8/8/8/5n2/3P4/2KR1n2 b - - 0 1", {"b1c1", "b2c1", "c2c1", "e1d1"} },
        { "4k3/4p3/8/8/8/8/3P4/N1KR1n2 b - - 0 1", {"b1c1", "b2c1", "c2c1", "e1d1", "b3a1", "c2a1"} },
        { "4k3/4p3/8/8/8/8/3P4/1nKR1n2 b - - 0 1", {"b2c1", "c2c1", "e1d1"} },
        // En passant square
        { "rn1qkbnr/pb2pppp/8/2p5/3pP3/6P1/P1P2P1P/RNBQKBNR b KQkq e3 0 1", {"e2e4"} },
        { "4k3/8/1P6/8/8/8/8/4K3 b - - 0 1",
          { "a5b6", "b5b6", "c5b6",                 // pawn
            "d1e1", "d2e1", "e2e1", "f1e1", "f2e1", // king
          }
        },
        // Promotion
        { "1KN5/8/8/8/5k2/8/8/8 b - - 0 1",
          { "a7b8", "a8b8", "b7b8", "c7b8", // king
            "a7c8", "b6c8", "d6c8", "e7c8", // knight
            "b7c8n", "c7c8n", "d7c8n",      // promotion to knight
          }
        },
        // Immovable pieces because of castling rights
        { "4k3/8/8/8/8/P7/7P/R3K2R b KQ - 0 1", {"a2a3", "b2a3"} },
        { "R1n1kn1R/P6P/pp4pp/8/8/P7/7P/R3K2R b KQ - 0 1", {"a2a3", "b2a3", "b8a8", "b7a8r", "g8h8", "g7h8r"} },
        { "4k3/8/8/8/8/P7/7P/R3K2R b K - 0 1", {"a2a3", "b2a3", "a2a1", "b1a1", "c1a1", "d1a1"} },
        { "4k3/8/8/8/8/P7/7P/R3K2R b Q - 0 1", {"a2a3", "b2a3", "f1h1", "g1h1"} },
        { "r3k2r/8/8/8/8/P7/7P/R3K2R b kq - 0 1",
          { "a2a3", "b2a3",                                 // pawn
            "a2a1", "b1a1", "c1a1", "d1a1", "f1h1", "g1h1", // rooks
            "d1e1", "d2e1", "e2e1", "f1e1", "f2e1",         // king
          }, // rook
        },

        // Reduced list of moves because of possible king captures
        { "4k3/8/3p4/8/5Q2/3R4/8/2B1K3 b - - 0 1",
          { "a3c1", "b2c1", "e3c1", "d2c1",                 // bishop
            "d1e1", "d2e1", "e2e1", "f1e1", "f2e1",         // king
            "a3d3", "b3d3", "c3d3", "f3d3", "g3d3", "h3d3", // rook
            "d1d3", "d2d3", "d4d3", "d5d3",                 // rook
            "d2f4", "g5f4", "h6f4", "g3f4", "h2f4",         // queen
            "b4f4", "c4f4", "d4f4", "g4f4", "h4f4",         // queen
            "f1f4", "f2f4", "f3f4", "f5f4", "f6f4",         // queen
          }
        },
        { "4k3/8/8/8/8/8/5P2/3n1RK1 b - - 0 1", {"e1g1", "g2g1", "h1g1", "h2g1"} },
        { "4k3/8/8/8/8/7b/5P2/3n1RK1 b - - 0 1", {"g2g1", "h1g1", "h2g1",} },
        { "4k3/8/8/8/8/8/5P2/3n1RKn b - - 0 1", {"g2g1", "h2g1"} },
        { "4k3/8/8/8/8/8/5P2/3n1RKN b - - 0 1", {"g2g1", "h2g1", "g3h1"} },
        { "4k3/8/8/8/8/8/3P4/2KR1n2 b - - 0 1", {"b1c1", "b2c1", "c2c1", "e1c1"} },
        { "4k3/8/8/8/8/5n2/3P4/2KR1n2 b - - 0 1", {"b1c1", "b2c1", "c2c1"} },
        { "4k3/8/8/8/8/8/3P4/N1KR1n2 b - - 0 1", {"b1c1", "b2c1", "c2c1", "b3a1", "c2a1"} },
        { "4k3/8/8/8/8/8/3P4/1nKR1n2 b - - 0 1", {"b2c1", "c2c1"} },
    };

    auto mirrorDataY = [](const Data& d) {
        Data mirror;
        mirror.fen = mirrorFenY(d.fen);
        for (std::string moveStr : d.unmoves)
            mirror.unmoves.insert(mirrorUciMoveY(moveStr));
        return mirror;
    };

    std::vector<UnMove> unMoves;
    for (const Data& data : v) {
        for (int i = 0; i < 2; i++) {
            Data d = i == 0 ? data : mirrorDataY(data);
            Position pos = TextIO::readFEN(d.fen);
            genMoves(pos, unMoves, false);
            std::set<std::string> strs = extractMoves(unMoves);
            ASSERT_EQ(d.unmoves, strs) << d.fen;
        }
    }
}

TEST(RevMoveGenTest, testCastleMask) {
    RevMoveGenTest::testCastleMask();
}

void
RevMoveGenTest::testCastleMask() {
    auto extractMasks = [](const std::vector<UnMove>& unMoves, const Move& move) {
        std::set<int> ret;
        for (const UnMove& um : unMoves)
            if (um.move == move)
                ret.insert(um.ui.castleMask);
        return ret;
    };

    struct Data {
        std::string fen;
        std::string move;
        std::set<int> masks;
    };

    const int a1 = 1 << Position::A1_CASTLE;
    const int h1 = 1 << Position::H1_CASTLE;
    const int a8 = 1 << Position::A8_CASTLE;
    const int h8 = 1 << Position::H8_CASTLE;
    std::vector<Data> v = {
        { TextIO::startPosFEN, "f6g8", { a1|h1|a8|h8 } },

        { "4k3/8/8/8/8/8/5P2/3n1RK1 b - - 0 1", "e1g1", { h1 } },
        { "4k3/8/8/8/8/8/5P2/R2n1RK1 b - - 0 1", "e1g1", { h1, a1|h1 } },
        { "4k3/8/8/8/8/8/5P2/r2n1RK1 b - - 0 1", "e1g1", { h1 } },
        { "r3k3/8/8/8/8/8/5P2/3n1RK1 b q - 0 1", "e1g1", { h1|a8 } },
        { "r3k2r/8/8/8/8/8/5P2/3n1RK1 b kq - 0 1", "e1g1", { h1|a8|h8 } },
        { "r3k3/8/8/8/8/8/5P2/R2n1RK1 b q - 0 1", "e1g1", { h1|a8, a1|h1|a8 } },
        { "r3k3/8/8/8/8/8/5P2/R2n1RK1 b - - 0 1", "e1g1", { h1, a1|h1 } },

        { "4k3/8/8/8/8/8/5P2/2KR2n1 b - - 0 1", "e1c1", { a1 } },
        { "4k3/8/8/8/8/8/5P2/2KR2nR b - - 0 1", "e1c1", { a1, a1|h1 } },
        { "4k3/8/8/8/8/8/5P2/2KR2nr b - - 0 1", "e1c1", { a1 } },
        { "r3k3/8/8/8/8/8/5P2/2KR2n1 b q - 0 1", "e1c1", { a1|a8 } },
        { "r3k2r/8/8/8/8/8/5P2/2KR2n1 b kq - 0 1", "e1c1", { a1|a8|h8 } },
        { "r3k3/8/8/8/8/8/5P2/2KR2nR b q - 0 1", "e1c1", { a1|a8, a1|h1|a8 } },
        { "r3k3/8/8/8/8/8/5P2/2KR2nR b - - 0 1", "e1c1", { a1, a1|h1 } },

        { "r3k2r/8/2b5/8/8/2N5/8/R3K2R b KQkq - 0 1", "a2a1", { } }, // Invalid move
        { "r3k2r/8/2b5/8/8/2N5/8/R3K2R b Kkq - 0 1", "a2a1", { h1|a8|h8 } },
        { "r3k2r/8/2b5/8/8/2N5/8/R3K2R b Qkq - 0 1", "h2h1", { a1|a8|h8 } },
        { "r3k2r/8/2b5/8/8/2N5/8/R3K2R b kq - 0 1", "e2e1", { a8|h8 } },
        { "r3k2r/8/2b5/8/8/2N5/8/R3K2R b q - 0 1", "e2e1", { a8 } },

        { "r3k2r/8/2b5/8/8/2N5/4K3/R6R b - - 0 1", "e1e2", { 0, a1, h1, a1|h1 } },
        { "r3k2r/8/2b5/8/8/2N5/4K3/7R b - - 0 1", "e1e2", { 0, h1 } },
        { "r3k2r/8/2b5/8/8/2N5/4K3/R7 b - - 0 1", "e1e2", { 0, a1 } },
        { "r3k2r/8/2b5/8/8/2N5/R7/4K2R b - - 0 1", "a1a2", { 0, a1 } },
        { "r3k2r/8/2b5/8/8/2N5/R7/4K2R b Kkq - 0 1", "a1a2", { h1|a8|h8, a1|h1|a8|h8 } },
        { "R3k2r/8/2b5/8/8/2N5/8/4K2R b - - 0 1", "a1a8", { 0, a1, a8, a1|a8 } },
        { "R3k2r/8/2b5/8/8/2N5/8/4K2R b Kk - 0 1", "a1a8", { h1|h8, a1|h1|h8, a8|h1|h8, a1|a8|h1|h8 } },
        { "r3k2r/8/2b5/8/8/2N5/8/R3KR2 b - - 0 1", "h1f1", { 0, h1 } },
        { "r3k2r/8/2b5/8/8/2N5/8/R3KR2 b kQq - 0 1", "h1f1", { a1|a8|h8, a1|a8|h1|h8 } },
        { "r3k2R/8/2b5/8/8/2N5/8/R3K3 b - - 0 1", "h1h8", { 0, h1, h8, h1|h8 } },
        { "r3k2R/8/2b5/8/8/2N5/8/R3K3 b Q - 0 1", "h1h8", { a1, a1|h1, a1|h8, a1|h1|h8 } },
        { "r3k2R/8/2b5/8/8/2N5/8/R3K3 b q - 0 1", "h1h8", { a8, a8|h1, a8|h8, a8|h1|h8 } },

        { "r3k2R/8/2b5/8/8/2N5/8/R3K3 b - - 0 1", "g7h8r", { 0, h8 } },
        { "r3k2R/8/2b5/8/8/2N5/8/R3K3 b Q - 0 1", "g7h8r", { a1, a1|h8 } },
        { "r3k2Q/8/2b5/8/8/2N5/8/R3K3 b - - 0 1", "g7h8q", { 0, h8 } },
        { "r3k2Q/8/2b5/8/8/2N5/8/R3K3 b q - 0 1", "g7h8q", { a8, a8|h8 } },
    };

    auto mirrorDataY = [](const Data& d) {
        Data mirror;
        mirror.fen = mirrorFenY(d.fen);
        mirror.move = mirrorUciMoveY(d.move);

        std::set<int> mirrorMasks;
        for (int mask : d.masks) {
            int mirrorMask = 0;
            if (mask & a1) mirrorMask |= a8;
            if (mask & h1) mirrorMask |= h8;
            if (mask & a8) mirrorMask |= a1;
            if (mask & h8) mirrorMask |= h1;
            mirrorMasks.insert(mirrorMask);
        }
        mirror.masks = mirrorMasks;

        return mirror;
    };

    std::vector<UnMove> unMoves;
    for (const Data& data : v) {
        for (int i = 0; i < 2; i++) {
            Data d = i == 0 ? data : mirrorDataY(data);
            Position pos = TextIO::readFEN(d.fen);
            genMoves(pos, unMoves, false);
            std::set<int> masks = extractMasks(unMoves, TextIO::uciStringToMove(d.move));
            ASSERT_EQ(d.masks, masks) << d.fen << " um:" << unMoves;
        }
    }
}

TEST(RevMoveGenTest, testEpSquare) {
    RevMoveGenTest::testEpSquare();
}

const int a1C = 1 << Position::A1_CASTLE;
const int h1C = 1 << Position::H1_CASTLE;
const int a8C = 1 << Position::A8_CASTLE;
const int h8C = 1 << Position::H8_CASTLE;

void
RevMoveGenTest::testEpSquare() {
    auto extractEP = [](const std::vector<UnMove>& unMoves, int fromSq, int toSq) {
        UnMoveSet ret;
        for (const UnMove& um : unMoves)
            if (um.ui.epSquare != -1)
                if ((toSq == -1 || um.move.to() == toSq) &&
                        (fromSq == -1 || um.move.from() == fromSq))
                    ret.insert(um);
        return ret;
    };

    struct Data {
        std::string fen;
        int fromSq;
        int toSq;
        int allEp; // 0=false, 1=true, 2=both
        UnMoveSet epMoves;
    };
    const int both = 2;

    auto UM = [](const std::string& move, int captPiece, int castleMask, int epSq) -> UnMove {
        UnMove um;
        um.move = TextIO::uciStringToMove(move);
        um.ui = { captPiece, castleMask, epSq, 0 };
        return um;
    };

    const int empty = Piece::EMPTY;
    const int bpawn = Piece::BPAWN;

    std::vector<Data> v = {
        { TextIO::startPosFEN, G8, { } },
        { "4k3/8/1P6/8/8/8/8/4K3 b - - 0 1", -1, B6, both, { UM("a5b6", empty, 0, B6), UM("c5b6", empty, 0, B6) } },
        { "4k3/8/P7/8/8/8/8/4K3 b - - 0 1", -1, A6, both, { UM("b5a6", empty, 0, A6) } },
        { "4k3/8/7P/8/8/8/8/4K3 b - - 0 1", -1, H6, both, { UM("g5h6", empty, 0, H6) } },
        { "4k3/8/8/pPpP4/P1P5/8/8/4K3 b - - 0 1", -1, B5, false, { } },
        { "4k3/8/8/pPpP4/P1P5/8/8/4K3 b - - 0 1", -1, B5, true, { UM("b4b5", empty, 0, C6) } },
        { "4k3/8/8/1pPpP3/1P1P4/8/8/4K3 b - - 0 1", -1, C5, true, { UM("c4c5", empty, 0, D6) } },
        { "4k3/8/8/pPpP4/P1P5/7P/6P1/4K3 b - - 0 1", -1, H3, true, { UM("h2h3", empty, 0, A6), UM("h2h3", empty, 0, C6) } },
        { "4k3/8/1P6/Pp6/8/8/8/4K3 b - - 0 1", -1, B6, both, { } },
        { "4k3/1p6/1P6/P7/8/8/8/4K3 b - - 0 1", -1, B6, both, { } },
        { "3nk3/8/8/PR6/8/8/8/4K3 b - - 0 1", B4, B5, false, { } },
        { "3nk3/8/8/PR6/8/8/8/4K3 b - - 0 1", B4, B5, true, { UM("b4b5", bpawn, 0, B6) } },
        { "3nk3/8/8/PR6/8/8/8/4K3 b - - 0 1", C5, B5, true, { UM("c5b5", bpawn, 0, B6) } },
        { "3nk3/8/8/PR6/8/8/8/4K3 b - - 0 1", B6, B5, true, { } },
        { "3nk3/8/8/PR6/8/8/8/4K3 b - - 0 1", B7, B5, true, { } },
        { "3nk3/8/8/PR6/8/8/8/4K3 b - - 0 1", B8, B5, true, { UM("b8b5", bpawn, 0, B6) } },
        { "4k3/8/8/pPpP4/P1P5/7P/8/4K3 b - - 0 1", G2, H3, true,
          { UM("g2h3", Piece::BQUEEN , 0, A6), UM("g2h3", Piece::BQUEEN , 0, C6),
            UM("g2h3", Piece::BROOK  , 0, A6), UM("g2h3", Piece::BROOK  , 0, C6),
            UM("g2h3", Piece::BBISHOP, 0, A6), UM("g2h3", Piece::BBISHOP, 0, C6),
            UM("g2h3", Piece::BKNIGHT, 0, A6), UM("g2h3", Piece::BKNIGHT, 0, C6),
            UM("g2h3", Piece::BPAWN  , 0, A6), UM("g2h3", Piece::BPAWN  , 0, C6),
          }
        },
        { "4k3/8/8/pPpP4/P1P5/8/8/R4RK1 b - - 0 1", E1, G1, false, { } },
        { "4k3/8/8/pPpP4/P1P5/8/8/R4RK1 b - - 0 1", E1, G1, true,
          { UM("e1g1", empty, h1C, A6), UM("e1g1", empty, a1C|h1C, A6),
            UM("e1g1", empty, h1C, C6), UM("e1g1", empty, a1C|h1C, C6),
          }
        },
        { "4k3/8/3R4/Pp6/8/8/8/4K3 b - - 0 1", B6, D6, true, { } },
        { "4k3/8/3R4/Pp6/8/8/8/4K3 b - - 0 1", A6, D6, true,
          { UM("a6d6", empty, 0, B6),
            UM("a6d6", Piece::BQUEEN , 0, B6),
            UM("a6d6", Piece::BROOK  , 0, B6),
            UM("a6d6", Piece::BBISHOP, 0, B6),
            UM("a6d6", Piece::BKNIGHT, 0, B6),
            UM("a6d6", Piece::BPAWN  , 0, B6),
          }
        },
        { "2Rnk3/8/8/Pp6/8/8/8/4K3 b - - 0 1", B7, C8, true, { } },
        { "1R1nk3/8/8/Pp6/8/8/8/4K3 b - - 0 1", A7, B8, true,
          { UM("a7b8r", Piece::BQUEEN, 0, B6),
            UM("a7b8r", Piece::BROOK, 0, B6),
            UM("a7b8r", Piece::BBISHOP, 0, B6),
            UM("a7b8r", Piece::BKNIGHT, 0, B6),
          }
        },
        { "rn1qkbnr/pb2pppp/8/1Pp5/3pP3/6P1/P1P2P1P/RNBQKBNR b KQkq e3 0 1", E2, E4, true,
          { UM("e2e4", empty, a1C|h1C|a8C|h8C, C6) }
        },
        { "rn1qkbnr/pb2pp1p/8/1Pp5/3pP1pP/6P1/P1P2P2/RNBQKBNR b KQkq h3 0 1", E2, E4, true, { } },
        { "rn1qkbnr/pb2pp1p/8/1Pp5/3pP1pP/6P1/P1P2P2/RNBQKBNR b KQkq h3 0 1", H2, H4, true,
          { UM("h2h4", empty, a1C|h1C|a8C|h8C, C6) } },
        { "rn2kbnr/pb2pppp/1q6/1Pp5/3pP3/6P1/PKP2P1P/RNBQ1BNR b kq e3 0 1", E2, E4, true, { } },
    };

    auto mirrorDataY = [](const Data& d) {
        Data mirror;
        mirror.fen = mirrorFenY(d.fen);
        mirror.fromSq = d.fromSq == -1 ? -1 : Square::mirrorY(d.fromSq);
        mirror.toSq = d.toSq == -1 ? -1 : Square::mirrorY(d.toSq);
        mirror.allEp = d.allEp;
        for (const UnMove& um : d.epMoves) {
            UnMove mirrorUm;
            mirrorUm.move = mirrorMoveY(um.move);
            mirrorUm.ui = mirrorUndoInfoY(um.ui);
            mirror.epMoves.insert(mirrorUm);
        }
        EXPECT_EQ(d.epMoves.size(), mirror.epMoves.size());
        return mirror;
    };

    std::vector<UnMove> unMoves;
    for (const Data& data : v) {
        for (int i = 0; i < 2; i++) {
            Data d = i == 0 ? data : mirrorDataY(data);
            for (int allEp = 0; allEp < 2; allEp++) {
                if (d.allEp == allEp || d.allEp == both) {
                    Position pos = TextIO::readFEN(d.fen);
                    genMoves(pos, unMoves, allEp);
                    UnMoveSet epMoves = extractEP(unMoves, d.fromSq, d.toSq);
                    ASSERT_EQ(d.epMoves, epMoves) << d.fen << " allEp:" << allEp << " um:" << unMoves;
                }
            }
        }
    }
}

TEST(RevMoveGenTest, testInvalidMoves) {
    RevMoveGenTest::testInvalidMoves();
}

void
RevMoveGenTest::testInvalidMoves() {
    // Return subset of "unMoves" having matching from and/or to squares
    auto filter = [](const std::vector<UnMove>& unMoves, int fromSq, int toSq) {
        UnMoveSet ret;
        for (const UnMove& um : unMoves)
            if ((toSq == -1 || um.move.to() == toSq) &&
                    (fromSq == -1 || um.move.from() == fromSq))
                ret.insert(um);
        return ret;
    };

    struct Data {
        std::string fen;
        int fromSq;
        int toSq;
        UnMoveSet expected;
    };

    auto UM = [](const std::string& move, int captPiece, int castleMask, int epSq) -> UnMove {
        UnMove um;
        um.move = TextIO::uciStringToMove(move);
        um.ui = { captPiece, castleMask, epSq, 0 };
        return um;
    };

    const int empty = Piece::EMPTY;
    const int bpawn = Piece::BPAWN;
    const int brook   = Piece::BROOK;
    const int bknight = Piece::BKNIGHT;
    const int bbishop = Piece::BBISHOP;
    const int bqueen  = Piece::BQUEEN;

    std::vector<Data> v = {
        // Piece counts limit possible captured pieces
        { "rnbqkbnr/1ppp1ppp/1P2p3/p7/8/8/8/4K3 b - - 0 1", C5, B6, { } },
        { "1nbqkbnr/1ppp1ppp/1P2p3/p7/8/8/8/4K3 b - - 0 1", C5, B6, { UM("c5b6", brook, 0, -1) } },
        { "r1bqkbnr/1ppp1ppp/1P2p3/p7/8/8/8/4K3 b - - 0 1", C5, B6, { UM("c5b6", bknight, 0, -1) } },
        { "rnbqk1nr/1ppp1ppp/1P2p3/p7/8/8/8/4K3 b - - 0 1", C5, B6, { UM("c5b6", bbishop, 0, -1) } },
        { "rnb1kbnr/1ppp1ppp/1P2p3/p7/8/8/8/4K3 b - - 0 1", C5, B6, { UM("c5b6", bqueen, 0, -1) } },
        { "rnbqkbnr/1ppp1ppp/1P2p3/8/8/8/8/4K3 b - - 0 1", C5, B6,
          { UM("c5b6", bqueen, 0, -1), UM("c5b6", brook, 0, -1), UM("c5b6", bbishop, 0, -1),
            UM("c5b6", bknight, 0, -1), UM("c5b6", bpawn, 0, -1) }
        },

        // Illegal EP square
        { "rn1qkbnr/pb2pppp/8/1Pp5/3pP3/6P1/P1P2P1P/RNBQKBNR b KQkq - 0 1", E2, E4, { } },
        { "rn1qkbnr/pb2pppp/8/1Pp5/3pP3/6P1/P1P2P1P/RNBQKBNR b KQkq e3 0 1", E2, E4, { UM("e2e4", empty, a1C|h1C|a8C|h8C, -1) } },
        { "rn1q1bnr/pb1kpppp/8/1Pp5/3pP3/6P1/P1P2P1P/RNBQKBNR b KQ - 0 1", E2, E4, { UM("e2e4", empty, a1C|h1C, -1) } },
        { "4k3/1q6/4K3/3pP3/8/8/8/8 w - d6 0 1", -1, -1, { } },
        { "4k3/1q6/5K2/3pP3/8/8/8/8 w - d6 0 1", -1, -1, { UM("d7d5", empty, 0, -1)} },

        // Discovered checks
        { "8/8/1Pk5/8/B7/2R5/8/4K3 b - - 0 1", -1, -1, { UM("c5b6", empty, 0, B6) } },
        { "4k3/8/1P6/8/B7/1R6/8/4K3 b - - 0 1", C5, B6, { UM("c5b6", empty, 0, B6) } },
        { "4k3/8/8/1pP5/B7/1R6/8/4K3 w - b6 0 1", -1, -1, { UM("b7b5", empty, 0, -1) } },
        { "4k3/1p6/8/2P5/B7/1R6/8/4K3 b - - 0 1", -1, -1,
          { UM("b5b3", empty, 0, -1), UM("b5b3", bqueen, 0, -1), UM("b5b3", brook, 0, -1),
            UM("b5b3", bbishop, 0, -1), UM("b5b3", bknight, 0, -1), UM("b5b3", bpawn, 0, -1) }
        },
        { "8/3Rk3/8/8/4R3/8/8/4K3 b - - 0 1", -1, -1, { } },
        { "3Rk3/8/8/8/4R3/8/8/4K3 b - - 0 1", -1, -1,
          { UM("e7d8r", bqueen, 0, -1), UM("e7d8r", brook, 0, -1), UM("e7d8r", bbishop, 0, -1), UM("e7d8r", bknight, 0, -1) }
        },
    };

    auto mirrorDataY = [](const Data& d) {
        Data mirror;
        mirror.fen = mirrorFenY(d.fen);
        mirror.fromSq = d.fromSq == -1 ? -1 : Square::mirrorY(d.fromSq);
        mirror.toSq = d.toSq == -1 ? -1 : Square::mirrorY(d.toSq);
        for (const UnMove& um : d.expected) {
            UnMove mirrorUm;
            mirrorUm.move = mirrorMoveY(um.move);
            mirrorUm.ui = mirrorUndoInfoY(um.ui);
            mirror.expected.insert(mirrorUm);
        }
        EXPECT_EQ(d.expected.size(), mirror.expected.size());
        return mirror;
    };

    std::vector<UnMove> unMoves;
    for (const Data& data : v) {
        for (int i = 0; i < 2; i++) {
            Data d = i == 0 ? data : mirrorDataY(data);
            Position pos = TextIO::readFEN(d.fen);
            genMoves(pos, unMoves, false);
            UnMoveSet moves = filter(unMoves, d.fromSq, d.toSq);
            ASSERT_EQ(d.expected, moves) << d.fen << " um:" << unMoves;
        }
    }
}


/** Check that "pos" does not have impossible castle rights. */
static void checkCastleRights(const Position& pos, const Move& move) {
    if (pos.a1Castle() || pos.h1Castle()) {
        ASSERT_EQ(E1, pos.getKingSq(true))
                << TextIO::toFEN(pos) << " m:" << move;
    }
    if (pos.a1Castle()) {
        ASSERT_TRUE((pos.pieceTypeBB(Piece::WROOK) & BitBoard::sqMask(A1)) != 0)
                << TextIO::toFEN(pos) << " m:" << move;
    }
    if (pos.h1Castle()) {
        ASSERT_TRUE((pos.pieceTypeBB(Piece::WROOK) & BitBoard::sqMask(H1)) != 0)
                        << TextIO::toFEN(pos) << " m:" << move;
    }

    if (pos.a8Castle() || pos.h8Castle()) {
        ASSERT_EQ(E8, pos.getKingSq(false))
                << TextIO::toFEN(pos) << " m:" << move;
    }
    if (pos.a8Castle()) {
        ASSERT_TRUE((pos.pieceTypeBB(Piece::BROOK) & BitBoard::sqMask(A8)) != 0)
                << TextIO::toFEN(pos) << " m:" << move;
    }
    if (pos.h8Castle()) {
        ASSERT_TRUE((pos.pieceTypeBB(Piece::BROOK) & BitBoard::sqMask(H8)) != 0)
                << TextIO::toFEN(pos) << " m:" << move;
    }
}

static void
checkEPSquare(const Position& pos, const Move& move) {
    int epSquare = pos.getEpSquare();
    if (epSquare == -1)
        return;

    ASSERT_EQ(Piece::EMPTY, pos.getPiece(epSquare)) << TextIO::toFEN(pos) << " m:" << move;

    bool wtm = pos.isWhiteMove();
    const int pawn  = wtm ? Piece::WPAWN : Piece::BPAWN;
    const int oPawn = wtm ? Piece::BPAWN : Piece::WPAWN;
    int x = Square::getX(epSquare);
    int y = Square::getY(epSquare);
    int dy = wtm ? 1 : -1;

    ASSERT_TRUE(y == 2 || y == 5) << TextIO::toFEN(pos) << " m:" << move;
    ASSERT_EQ(wtm, y == 5) << TextIO::toFEN(pos) << " m:" << move;
    ASSERT_EQ(Piece::EMPTY, pos.getPiece(Square::getSquare(x, y + dy))) << TextIO::toFEN(pos) << " m:" << move;
    ASSERT_EQ(oPawn, pos.getPiece(Square::getSquare(x, y - dy))) << TextIO::toFEN(pos) << " m:" << move;

    bool leftPawn  = x > 0 && pos.getPiece(Square::getSquare(x - 1, y - dy)) == pawn;
    bool rightPawn = x < 7 && pos.getPiece(Square::getSquare(x + 1, y - dy)) == pawn;
    ASSERT_TRUE(leftPawn || rightPawn) << TextIO::toFEN(pos) << " m:" << move;
}

void
RevMoveGenTest::genMoves(const Position& pos, std::vector<UnMove>& unMoves, bool includeEpSquare) {
    RevMoveGen::genMoves(pos, unMoves, includeEpSquare);

    UnMoveSet umSet;
    umSet.insert(unMoves.begin(), unMoves.end());
    ASSERT_EQ(unMoves.size(), umSet.size()) << TextIO::toFEN(pos) << " um: " << unMoves;

    for (const UnMove& um : unMoves) {
        Position tmpPos(pos);
        tmpPos.unMakeMove(um.move, um.ui);

        ASSERT_EQ(1, BitBoard::bitCount(tmpPos.pieceTypeBB(Piece::WKING)));
        ASSERT_EQ(1, BitBoard::bitCount(tmpPos.pieceTypeBB(Piece::BKING)));
        ASSERT_EQ(0, tmpPos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskRow1Row8);
        ASSERT_EQ(0, tmpPos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskRow1Row8);

        ASSERT_NE(Piece::WKING, um.move.promoteTo());
        ASSERT_NE(Piece::BKING, um.move.promoteTo());
        ASSERT_NE(Piece::WPAWN, um.move.promoteTo());
        ASSERT_NE(Piece::BPAWN, um.move.promoteTo());
        if (um.move.promoteTo() != Piece::EMPTY) {
            ASSERT_EQ(tmpPos.isWhiteMove(), Piece::isWhite(um.move.promoteTo()));
        }

        checkCastleRights(tmpPos, um.move);
        checkEPSquare(tmpPos, um.move);

        MoveList moves;
        MoveGen::pseudoLegalMoves(tmpPos, moves);
        MoveGen::removeIllegal(tmpPos, moves);
        bool valid = false;
        for (int i = 0; i < moves.size; i++)
            if (moves[i] == um.move)
                valid = true;
        ASSERT_TRUE(valid) << TextIO::toFEN(pos) << " invalid move:" << um.move
                << " captP:" << um.ui.capturedPiece << " castleM:" << um.ui.castleMask
                << " epSq:" << um.ui.epSquare;

        UndoInfo ui;
        tmpPos.makeMove(um.move, ui);
        tmpPos.setHalfMoveClock(pos.getHalfMoveClock());
        TextIO::fixupEPSquare(tmpPos);
        ASSERT_EQ(TextIO::toFEN(pos), TextIO::toFEN(tmpPos)) << " m:" << um.move;
        ASSERT_EQ(pos, tmpPos);
        ASSERT_EQ(ui.capturedPiece, um.ui.capturedPiece);
        ASSERT_EQ(ui.castleMask, um.ui.castleMask);
        ASSERT_EQ(ui.epSquare, um.ui.epSquare);
    }
}
