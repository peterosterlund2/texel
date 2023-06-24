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
 * posutil.hpp
 *
 *  Created on: Oct 3, 2021
 *      Author: petero
 */

#ifndef POSUTIL_HPP_
#define POSUTIL_HPP_

#include "piece.hpp"
#include "util.hpp"

class Position;

class PosUtil {
public:
    /** Swap white/black pieces, creating a mirrored position with the
     *  same game-theoretical value as the original. */
    static Position swapColors(const Position& pos);

    /** Swap white/black castle rights. */
    static int swapCastleMask(int mask);

    /** Mirror position in X direction, remove castling rights. */
    static Position mirrorX(const Position& pos);

    /** Change color of a piece. */
    static int swapPieceColor(int p);

    /** Return mask of squares attacked by white/black pieces. */
    static U64 attackedSquares(const Position& pos, bool whitePieces);
};

inline int
PosUtil::swapPieceColor(int p) {
    return Piece::isWhite(p) ? Piece::makeBlack(p) : Piece::makeWhite(p);
}


#endif /* POSUTIL_HPP_ */
