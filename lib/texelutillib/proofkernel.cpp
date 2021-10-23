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
 * proofkernel.cpp
 *
 *  Created on: Oct 16, 2021
 *      Author: petero
 */

#include "proofkernel.hpp"




ProofKernel::PawnColumn::PawnColumn(int x) {
    bool even = (x % 2) == 0;
    promSquare[WHITE] = even ? SquareColor::LIGHT : SquareColor::DARK;
    promSquare[BLACK] = even ? SquareColor::DARK  : SquareColor::LIGHT;
}

int
ProofKernel::PawnColumn::nPromotions(PieceColor c) const {
    int np = nPawns();
    if (c == WHITE) {
        int cnt = 0;
        for (int i = np - 1; i >= 0; i--) {
            if (getPawn(i) == BLACK)
                break;
            cnt++;
        }
        return cnt;
    } else {
        int cnt = 0;
        for (int i = 0; i < np; i++) {
            if (getPawn(i) == WHITE)
                break;
            cnt++;
        }
        return cnt;
    }
}
