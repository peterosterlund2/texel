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
 * posutil.cpp
 *
 *  Created on: Oct 3, 2021
 *      Author: petero
 */

#include "posutil.hpp"
#include "position.hpp"


Position
PosUtil::swapColors(const Position& pos) {
    Position sym;
    sym.setWhiteMove(!pos.isWhiteMove());
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            Square sq(x, y);
            int p = pos.getPiece(sq);
            p = swapPieceColor(p);
            sym.setPiece(sq.mirrorY(), p);
        }
    }

    sym.setCastleMask(swapCastleMask(pos.getCastleMask()));

    if (pos.getEpSquare().isValid())
        sym.setEpSquare(pos.getEpSquare().mirrorY());

    sym.setHalfMoveClock(pos.getHalfMoveClock());
    sym.setFullMoveCounter(pos.getFullMoveCounter());

    return sym;
}

int
PosUtil::swapCastleMask(int mask) {
    const int a1 = 1 << Position::A1_CASTLE;
    const int h1 = 1 << Position::H1_CASTLE;
    const int a8 = 1 << Position::A8_CASTLE;
    const int h8 = 1 << Position::H8_CASTLE;

    int ret = 0;
    if (mask & a1) ret |= a8;
    if (mask & h1) ret |= h8;
    if (mask & a8) ret |= a1;
    if (mask & h8) ret |= h1;
    return ret;
}

Position
PosUtil::mirrorX(const Position& pos) {
    Position mir;
    mir.setWhiteMove(pos.isWhiteMove());
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            Square sq(x, y);
            int p = pos.getPiece(sq);
            mir.setPiece(sq.mirrorX(), p);
        }
    }

    if (pos.getEpSquare().isValid())
        mir.setEpSquare(pos.getEpSquare().mirrorX());

    mir.setHalfMoveClock(pos.getHalfMoveClock());
    mir.setFullMoveCounter(pos.getFullMoveCounter());

    return mir;
}


U64
PosUtil::attackedSquares(const Position& pos, bool whitePieces) {
    U64 attacked = 0;

    U64 m = pos.pieceTypeBB(whitePieces ? Piece::WKNIGHT : Piece::BKNIGHT);
    while (m) {
        Square sq = BitBoard::extractSquare(m);
        attacked |= BitBoard::knightAttacks(sq);
    }

    m = pos.pieceTypeBB(whitePieces ? Piece::WKING : Piece::BKING);
    while (m) {
        Square sq = BitBoard::extractSquare(m);
        attacked |= BitBoard::kingAttacks(sq);
    }

    if (whitePieces)
        attacked |= BitBoard::wPawnAttacksMask(pos.pieceTypeBB(Piece::WPAWN));
    else
        attacked |= BitBoard::bPawnAttacksMask(pos.pieceTypeBB(Piece::BPAWN));

    auto q = whitePieces ? Piece::WQUEEN  : Piece::BQUEEN;
    auto r = whitePieces ? Piece::WROOK   : Piece::BROOK;
    auto b = whitePieces ? Piece::WBISHOP : Piece::BBISHOP;

    U64 occupied = pos.occupiedBB();
    m = pos.pieceTypeBB(q, r);
    while (m) {
        Square sq = BitBoard::extractSquare(m);
        attacked |= BitBoard::rookAttacks(sq, occupied);
    }

    m = pos.pieceTypeBB(q, b);
    while (m) {
        Square sq = BitBoard::extractSquare(m);
        attacked |= BitBoard::bishopAttacks(sq, occupied);
    }

    return attacked;
}
