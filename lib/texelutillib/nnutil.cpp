/*
    Texel - A UCI chess engine.
    Copyright (C) 2022  Peter Österlund, peterosterlund2@gmail.com

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
 * nnutil.cpp
 *
 *  Created on: Jul 14, 2022
 *      Author: petero
 */

#include "nnutil.hpp"
#include "position.hpp"
#include "posutil.hpp"

static Piece::Type ptVec[] = { Piece::WQUEEN, Piece::WROOK, Piece::WBISHOP, Piece::WKNIGHT, Piece::WPAWN,
                               Piece::BQUEEN, Piece::BROOK, Piece::BBISHOP, Piece::BKNIGHT, Piece::BPAWN};

void
NNUtil::posToRecord(Position& pos, int searchScore, Record& r) {
    r.searchScore = searchScore;

    if (!pos.isWhiteMove()) {
        pos = PosUtil::swapColors(pos);
        r.searchScore *= -1;
    }

    auto castleSquare = [](Square square, int castleMask, bool wtm) -> int {
        int mask = wtm ? (castleMask & 3) : (castleMask >> 2);
        return mask ? (63 + mask) : square.asInt();
    };
    int castleMask = pos.getCastleMask();
    r.wKing = castleSquare(pos.getKingSq(true), castleMask, true);
    r.bKing = castleSquare(pos.getKingSq(false), castleMask, false);
    r.halfMoveClock = pos.getHalfMoveClock();

    int p = 0;
    int i = 0;
    for (Piece::Type pt : ptVec) {
        U64 mask = pos.pieceTypeBB(pt);
        while (mask) {
            Square sq = BitBoard::extractSquare(mask);
            r.squares[i++] = sq.asInt();
        }
        if (p < 9)
            r.nPieces[p++] = i;
    }
    while (i < 30)
        r.squares[i++] = -1;
}

void
NNUtil::recordToPos(const Record& r, Position& pos, int& searchScore) {
    for (int sq = 0; sq < 64; sq++)
        pos.clearPiece(Square(sq));

    int castleMask = 0;
    int wk = r.wKing;
    int bk = r.bKing;
    if (wk >= 64) {
        castleMask |= (wk - 63);
        wk = E1;
    }
    if (bk >= 64) {
        castleMask |= (bk - 63) << 2;
        bk = E8;
    }
    pos.setPiece(Square(wk), Piece::WKING);
    pos.setPiece(Square(bk), Piece::BKING);
    pos.setCastleMask(castleMask);

    int pieceType = 0;
    for (int i = 0; i < 30; i++) {
        while (pieceType < 9 && i >= r.nPieces[pieceType])
            pieceType++;
        int sq = r.squares[i];
        if (sq == -1)
            continue;
        pos.setPiece(Square(sq), ptVec[pieceType]);
    }

    pos.setWhiteMove(true);
    pos.setEpSquare(Square(-1));
    pos.setHalfMoveClock(r.halfMoveClock);
    pos.setFullMoveCounter(1);

    searchScore = r.searchScore;
}
