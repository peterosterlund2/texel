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
 * revmovegen.hpp
 *
 *  Created on: Oct 3, 2021
 *      Author: petero
 */

#ifndef REVMOVEGEN_HPP_
#define REVMOVEGEN_HPP_

#include "move.hpp"
#include "undoInfo.hpp"
#include "moveGen.hpp"
#include "textio.hpp"


/** Contains information needed to go to the previous position from the
 *  current position using Position::unMakeMove(). */
struct UnMove {
    /** The move leading from the previous position to this position. */
    Move move;
    /** Information needed to reconstruct the previous position.
     *  The halfMoveClock is always set to 0. */
    UndoInfo ui;
};

/** Implementation of a reverse move generator. Given a position, calculate a set
 *  of moves that when played from some other position would result in this position.
 *  All moves are legal from the "other position".
 *  Some but not all cases of "other position" being invalid are detected and
 *  excluded from the result. */
class RevMoveGen {
public:
    /** Generate list of moves leading to this position from other positions.
     *  @param includeAllEpSquares  If true, also generate all possible en passant
     *                              possibilities for the previous positions. */
    static void genMoves(const Position& pos, std::vector<UnMove>& moves,
                         bool includeAllEpSquare);

private:
    /** Generate possible reverse moves from "pos". */
    static void genMovesNoUndoInfo(const Position& pos, MoveList& moveList);

    /** Add all moves from squares in "fromMask" to square "toSq". */
    static void addMovesByMask(MoveList& moveList, U64 fromMask, int toSq,
                               int promoteTo = Piece::EMPTY);

    /** Return true if the position reached by undoing "m" is known to be invalid. */
    static bool knownInvalid(const Position& pos, const Move& m, const UndoInfo& ui);

    /** Non-template version of MoveGen::sqAttacked(). */
    static bool sqAttacked(bool wtm, const Position& pos, int sq, U64 occupied);
};


inline void
RevMoveGen::addMovesByMask(MoveList& moveList, U64 fromMask, int toSq, int promoteTo) {
    assert(moveList.size + BitBoard::bitCount(fromMask) < 256);
    while (fromMask != 0) {
        int sq0 = BitBoard::extractSquare(fromMask);
        moveList.addMove(sq0, toSq, promoteTo);
    }
}

inline bool
RevMoveGen::sqAttacked(bool wtm, const Position& pos, int sq, U64 occupied) {
    if (wtm)
        return MoveGen::sqAttacked<true>(pos, sq, occupied);
    else
        return MoveGen::sqAttacked<false>(pos, sq, occupied);
}

inline std::ostream&
operator<<(std::ostream& os, const UnMove& um) {
    os << um.move << " " << um.ui;
    return os;
};

inline bool
operator==(const UnMove& um1, const UnMove& um2) {
    return um1.move == um2.move && um1.ui == um2.ui;
}

#endif /* REVMOVGEN_HPP_ */
