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
 * revmovegen.cpp
 *
 *  Created on: Oct 3, 2021
 *      Author: petero
 */

#include "revmovegen.hpp"


void RevMoveGen::genMoves(const Position& pos, std::vector<UnMove>& moves,
                          bool includeAllEpSquares) {
    const int wtm = !pos.isWhiteMove(); // Other side makes the un-moves
    MoveList moveList;
    if (!pos.getEpSquare().isValid()) {
        genMovesNoUndoInfo(pos, moveList);
    } else {
        Square epSq = pos.getEpSquare();
        int delta = wtm ? 8 : -8;
        addMovesByMask(moveList, 1ULL << (epSq - delta), epSq + delta);
    }

    // Return true if "piece" is a valid captured piece (ignoring piece color) for a move
    auto validCapturePiece = [](const Position& pos, const Move& m, int movingPiece, int captPiece) -> bool {
        if (captPiece == Piece::EMPTY)
            return true; // Invalid diagonal pawn moves are detected later, when EP mask is known
        if (captPiece == Piece::WKING)
            return false; // Cannot capture king
        switch (movingPiece) {
        case Piece::WKING: case Piece::BKING:
            if (BitBoard::getKingDistance(m.from(), m.to()) > 1)
                return false; // Castling moves cannot capture
            break;
        case Piece::WPAWN: case Piece::BPAWN:
            if (m.from().getX() == m.to().getX())
                return false; // Only diagonal pawn moves can capture
        default:
            break;
        }
        if (captPiece == Piece::WPAWN && (1ULL << m.to()) & BitBoard::maskRow1Row8)
            return false; // Pawn not allowed on first/last row
        return true;
    };

    // Return minimum valid castle mask given a position and a move to be undone
    auto getBaseCastleMask = [](const Position& pos, const Move& m, int movingPiece) -> int {
        int mask = pos.getCastleMask();
        if ((movingPiece == Piece::WKING || movingPiece == Piece::BKING) &&
                BitBoard::getKingDistance(m.from(), m.to()) > 1) {
            switch (m.to().asInt()) {
            case G1: mask |= 1 << Position::H1_CASTLE; break;
            case G8: mask |= 1 << Position::H8_CASTLE; break;
            case C1: mask |= 1 << Position::A1_CASTLE; break;
            case C8: mask |= 1 << Position::A8_CASTLE; break;
            default: break;
            }
        }
        return mask;
    };

    // Return mask of possible additional castling flags for a move
    auto getCastleAddMask = [](const Position& pos, const Move& m,
                               int movingPiece, int capturedPiece) -> int {
        int board[64];
        U64 squares = BitBoard::sqMask(A1, E1, H1, A8, E8, H8);
        while (squares) {
            Square sq = BitBoard::extractSquare(squares);
            board[sq.asInt()] = pos.getPiece(sq);
        }

        auto maxMask = [&board]() -> int { // Return maximum castle mask for board state
            int castleMask = 0;
            if (board[E1] == Piece::WKING) {
                if (board[A1] == Piece::WROOK)
                    castleMask |= 1 << Position::A1_CASTLE;
                if (board[H1] == Piece::WROOK)
                    castleMask |= 1 << Position::H1_CASTLE;
            }
            if (board[E8] == Piece::BKING) {
                if (board[A8] == Piece::BROOK)
                    castleMask |= 1 << Position::A8_CASTLE;
                if (board[H8] == Piece::BROOK)
                    castleMask |= 1 << Position::H8_CASTLE;
            }
            return castleMask;
        };

        int maxMask0 = maxMask();

        board[m.from().asInt()] = movingPiece;
        board[m.to().asInt()] = capturedPiece;
        if (movingPiece == Piece::WKING && m.from() == E1) {
            if (m.to() == G1)
                board[H1] = Piece::WROOK;
            else if (m.to() == C1)
                board[A1] = Piece::WROOK;
        }
        if (movingPiece == Piece::BKING && m.from() == E8) {
            if (m.to() == G8)
                board[H8] = Piece::BROOK;
            else if (m.to() == C8)
                board[A8] = Piece::BROOK;
        }

        return maxMask() & ~maxMask0;
    };

    // Return true if "m" can only be valid if it is an en passant capture
    auto mustBeEpCapture = [](const Move& m, int movingPiece, int capturedPiece) {
        return Piece::makeWhite(movingPiece) == Piece::WPAWN &&
               m.from().getX() != m.to().getX() &&
               capturedPiece == Piece::EMPTY;
    };

    // Return possible en passant files as a bit mask. Bit 8 corresponds to "no en passant file"
    auto getEpMask = [&mustBeEpCapture](const Position& pos, const Move& m, int movingPiece,
                                        int capturedPiece, bool includeAllEpSquares) -> int {
        const int wtm = !pos.isWhiteMove(); // Other side makes the un-moves
        const int y = wtm ? 5 : 2; // En passant row
        const int dy = wtm ? 1 : -1;
        const int pawn  = wtm ? Piece::WPAWN : Piece::BPAWN;
        const int oPawn = wtm ? Piece::BPAWN : Piece::WPAWN;

        U64 epFileMask = includeAllEpSquares ? 0xff : 0;
        bool isEp = false;
        int x = m.to().getX();
        if (mustBeEpCapture(m, movingPiece, capturedPiece) && m.to().getY() == y) {
            if (pos.getPiece(Square(x, y + dy)) == Piece::EMPTY &&
                pos.getPiece(Square(x, y - dy)) == Piece::EMPTY) {
                epFileMask |= 1 << m.to().getX();
                isEp = true;
            } else
                return 0; // Move invalid regardless of EP square
        }

        int mask = 1 << 8;
        if (epFileMask != 0) {
            int board[64];
            for (int i = 0; i < 64; i++)
                board[i] = pos.getPiece(Square(i));
            board[m.from().asInt()] = movingPiece;
            board[m.to().asInt()] = capturedPiece;
            if (isEp)
                board[Square(x, y - dy).asInt()] = oPawn;
            while (epFileMask != 0) {
                x = BitBoard::extractSquare(epFileMask).asInt();
                if (board[Square(x, y + dy).asInt()] != Piece::EMPTY ||
                    board[Square(x, y     ).asInt()] != Piece::EMPTY ||
                    board[Square(x, y - dy).asInt()] != oPawn)
                    continue;
                if ((x > 0 && board[Square(x - 1, y - dy).asInt()] == pawn) ||
                    (x < 7 && board[Square(x + 1, y - dy).asInt()] == pawn))
                    mask |= 1 << x;
            }
        }
        return mask;
    };

    moves.clear();
    for (int i = 0; i < moveList.size; i++) {
        const Move& m = moveList[i];
        const int movingPiece = m.promoteTo() == Piece::EMPTY ? pos.getPiece(m.to()) :
                wtm ? Piece::WPAWN : Piece::BPAWN;
        UndoInfo ui { Piece::EMPTY, 0, Square(-1), 0 };
        for (int p0 = Piece::EMPTY; p0 <= Piece::WPAWN; p0++) {
            if (!validCapturePiece(pos, m, movingPiece, p0))
                continue;
            ui.capturedPiece = wtm ? Piece::makeBlack(p0) : p0;

            int baseCastleMask = getBaseCastleMask(pos, m, movingPiece);
            int castleAddMask = getCastleAddMask(pos, m, movingPiece, ui.capturedPiece) & ~baseCastleMask;
            for (int castle = castleAddMask; ; castle = (castle - 1) & castleAddMask) {
                ui.castleMask = baseCastleMask | castle;

                U64 epMask = getEpMask(pos, m, movingPiece, ui.capturedPiece, includeAllEpSquares);
                while (epMask != 0) {
                    int epFile = BitBoard::extractSquare(epMask).asInt();
                    ui.epSquare = epFile == 8 ? Square(-1) : Square(epFile, wtm ? 5 : 2);
                    if (mustBeEpCapture(m, movingPiece, ui.capturedPiece) && m.to() != ui.epSquare)
                        continue;
                    if (!knownInvalid(pos, m, ui))
                        moves.push_back({m, ui});
                }

                if (castle == 0)
                    break;
            }
        }
    }
}

void RevMoveGen::genMovesNoUndoInfo(const Position& pos, MoveList& moveList) {
    const U64 occupied = pos.occupiedBB();
    const int wtm = !pos.isWhiteMove(); // Other side makes the un-moves
    const Piece::Type q = wtm ? Piece::WQUEEN  : Piece::BQUEEN;
    const Piece::Type r = wtm ? Piece::WROOK   : Piece::BROOK;
    const Piece::Type b = wtm ? Piece::WBISHOP : Piece::BBISHOP;
    const Piece::Type n = wtm ? Piece::WKNIGHT : Piece::BKNIGHT;
    const Piece::Type p = wtm ? Piece::WPAWN   : Piece::BPAWN;

    // Queen moves
    U64 squares = pos.pieceTypeBB(q);
    while (squares != 0) {
        Square sq = BitBoard::extractSquare(squares);
        U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & ~occupied;
        addMovesByMask(moveList, m, sq);
    }

    // Rook moves
    squares = pos.pieceTypeBB(r);
    if (pos.a1Castle()) squares &= ~BitBoard::sqMask(A1);
    if (pos.h1Castle()) squares &= ~BitBoard::sqMask(H1);
    if (pos.a8Castle()) squares &= ~BitBoard::sqMask(A8);
    if (pos.h8Castle()) squares &= ~BitBoard::sqMask(H8);
    while (squares != 0) {
        Square sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::rookAttacks(sq, occupied) & ~occupied;
        addMovesByMask(moveList, m, sq);
    }

    // Bishop moves
    squares = pos.pieceTypeBB(b);
    while (squares != 0) {
        Square sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::bishopAttacks(sq, occupied) & ~occupied;
        addMovesByMask(moveList, m, sq);
    }

    // Knight moves
    squares = pos.pieceTypeBB(n);
    while (squares != 0) {
        Square sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::knightAttacks(sq) & ~occupied;
        addMovesByMask(moveList, m, sq);
    }

    // King moves
    Square sq = pos.getKingSq(wtm);
    if (!((sq == E1 && (pos.a1Castle() || pos.h1Castle())) ||
          (sq == E8 && (pos.a8Castle() || pos.h8Castle())))) {
        U64 m = BitBoard::kingAttacks(sq) & ~occupied;
        addMovesByMask(moveList, m, sq);

        Square k0Sq = wtm ? E1 : E8;
        Square kSq = wtm ? G1 : G8;
        Square rSq = wtm ? F1 : F8;
        if (sq == kSq && pos.getPiece(rSq) == r) { // Short castle
            U64 OO_SQ = wtm ? BitBoard::sqMask(E1,H1) : BitBoard::sqMask(E8,H8);
            if ((OO_SQ & occupied) == 0 &&
                    !sqAttacked(wtm, pos, k0Sq, occupied) &&
                    !sqAttacked(wtm, pos, rSq, occupied))
                addMovesByMask(moveList, 1ULL << k0Sq, kSq);
        }

        kSq = wtm ? C1 : C8;
        rSq = wtm ? D1 : D8;
        if (sq == kSq && pos.getPiece(rSq) == r) { // Long castle
            U64 OOO_SQ = wtm ? BitBoard::sqMask(A1,B1,E1) : BitBoard::sqMask(A8,B8,E8);
            if ((OOO_SQ & occupied) == 0 &&
                    !sqAttacked(wtm, pos, k0Sq, occupied) &&
                    !sqAttacked(wtm, pos, rSq, occupied))
                addMovesByMask(moveList, 1ULL << k0Sq, kSq);
        }
    }

    // Pawn moves
    squares = pos.pieceTypeBB(p);
    while (squares != 0) {
        Square sq = BitBoard::extractSquare(squares);
        U64 m = wtm ? (BitBoard::bPawnAttacks(sq) | (1ULL << (sq - 8))) & ~BitBoard::maskRow1
                    : (BitBoard::wPawnAttacks(sq) | (1ULL << (sq + 8))) & ~BitBoard::maskRow8;
        if (sq.getY() == (wtm ? 3 : 4))
            if (((1ULL << (wtm ? sq - 8 : sq + 8)) & occupied) == 0)
                m |= 1ULL << (wtm ? sq - 16 : sq + 16);
        m &= ~occupied;
        addMovesByMask(moveList, m, sq);
    }

    // Promotions
    squares = pos.colorBB(wtm) & (wtm ? BitBoard::maskRow8 : BitBoard::maskRow1);
    while (squares != 0) {
        Square sq = BitBoard::extractSquare(squares);
        int promoteTo = pos.getPiece(sq);
        if (promoteTo == Piece::WKING || promoteTo == Piece::BKING)
            continue;
        U64 m = wtm ? (BitBoard::bPawnAttacks(sq) | (1ULL << (sq - 8)))
                    : (BitBoard::wPawnAttacks(sq) | (1ULL << (sq + 8)));
        m &= ~occupied;
        addMovesByMask(moveList, m, sq, promoteTo);
    }
}

/** Return false if "pos" is not legal because the piece counts are known
 *  to be impossible to obtain from the starting position using promotions
 *  and captures. */
static bool pieceCountsValid(const Position& pos) {
    int pieceCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieceCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    // White must not have too many pieces
    int maxWPawns = 8;
    maxWPawns -= std::max(0, pieceCnt[Piece::WKNIGHT] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WBISHOP] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WROOK  ] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WQUEEN ] - 1);
    if (pieceCnt[Piece::WPAWN] > maxWPawns)
        return false;

    // Black must not have too many pieces
    int maxBPawns = 8;
    maxBPawns -= std::max(0, pieceCnt[Piece::BKNIGHT] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BBISHOP] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BROOK  ] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BQUEEN ] - 1);
    if (pieceCnt[Piece::BPAWN] > maxBPawns)
        return false;

    return true;
}

bool
RevMoveGen::knownInvalid(const Position& pos, const Move& move, const UndoInfo& ui) {
    Position tmpPos(pos);
    tmpPos.unMakeMove(move, ui);

    if (!pieceCountsValid(tmpPos))
        return true;

    if (MoveGen::canTakeKing(tmpPos))
        return true;

    Square epSquare = tmpPos.getEpSquare();
    TextIO::fixupEPSquare(tmpPos);
    if (epSquare != tmpPos.getEpSquare())
        return true;

    UndoInfo ui2;
    tmpPos.makeMove(move, ui2);
    TextIO::fixupEPSquare(tmpPos);
    if (tmpPos.getEpSquare() != pos.getEpSquare())
        return true;

    return false;
}
