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
 * nnutil.hpp
 *
 *  Created on: Jul 14, 2022
 *      Author: petero
 */

#ifndef NNUTIL_HPP_
#define NNUTIL_HPP_

#include "util.hpp"

class Position;

class NNUtil {
public:
    struct Record {
        S8 wKing;         // 64,65,66 = Ke1 with castling flags K, Q, KQ
        S8 bKing;
        S8 nPieces[9];    // No of pieces of type WQ, WR, WB, WN, WP, BQ, BR, BB, BN (cumulative)
        S8 squares[30];   // Position for each piece, -1 for captured pieces
        S8 halfMoveClock;
        S16 searchScore;
    };
    static_assert(sizeof(Record) == 44, "Unsupported struct packing");

    /** Convert a Position object to Record format. "pos" may get modified. */
    static void posToRecord(Position& pos, int searchScore, Record& r);

    /** Convert a Record to Position format. */
    static void recordToPos(const Record& r, Position& pos, int& searchScore);

    /** Return number of pieces, including kings, in the position
     *  corresponding to Record "r". */
    static int nPieces(const Record& r);
};

#endif /* NNUTIL_HPP_ */
