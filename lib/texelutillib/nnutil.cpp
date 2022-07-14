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

void
NNUtil::posToRecord(Position& pos, int searchScore, Record& r) {
    r.searchScore = searchScore;

    if (!pos.isWhiteMove()) {
        pos = PosUtil::swapColors(pos);
        r.searchScore *= -1;
    }

    r.wKing = pos.getKingSq(true);
    r.bKing = pos.getKingSq(false);
    r.halfMoveClock = pos.getHalfMoveClock();

    int p = 0;
    int i = 0;
    for (Piece::Type pt : {Piece::WQUEEN, Piece::WROOK, Piece::WBISHOP, Piece::WKNIGHT, Piece::WPAWN,
                           Piece::BQUEEN, Piece::BROOK, Piece::BBISHOP, Piece::BKNIGHT, Piece::BPAWN}) {
        U64 mask = pos.pieceTypeBB(pt);
        while (mask) {
            int sq = BitBoard::extractSquare(mask);
            r.squares[i++] = sq;
        }
        if (p < 9)
            r.nPieces[p++] = i;
    }
    while (i < 30)
        r.squares[i++] = -1;
}
