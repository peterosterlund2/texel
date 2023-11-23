/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * undoInfo.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef UNDOINFO_HPP_
#define UNDOINFO_HPP_

#include "square.hpp"


/**
 * Contains enough information to undo a previous move.
 * Set by makeMove(). Used by unMakeMove().
 */
struct UndoInfo {
    int capturedPiece;
    int castleMask;
    Square epSquare;
    int halfMoveClock;
};

inline bool operator==(const UndoInfo& ui1, const UndoInfo& ui2) {
    return ui1.capturedPiece == ui2.capturedPiece &&
           ui1.castleMask == ui2.castleMask &&
           ui1.epSquare == ui2.epSquare &&
           ui1.halfMoveClock == ui2.halfMoveClock;
}

#endif /* UNDOINFO_HPP_ */
