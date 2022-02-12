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
 * pkseq.hpp
 *
 *  Created on: Feb 5, 2022
 *      Author: petero
 */

#ifndef PKSEQ_HPP_
#define PKSEQ_HPP_

#include "proofkernel.hpp"


/**
 * A sequence of ProofKernel::ExtPkMove that can be transformed in various ways
 * to make it closer to a sequence of real chess moves.
 */
class PkSequence {
    friend class ProofGameTest;
    using ExtPkMove = ProofKernel::ExtPkMove;
public:
    /** Constructor. */
    PkSequence(const std::vector<ExtPkMove>& extKernel,
               const Position& initPos, const Position& goalPos,
               std::ostream& log);

    /** Make the move sequence more suitable to be converted to a sequence
     *  of real chess moves. */
    void improve();

    /** Return the improved move sequence. */
    std::vector<ExtPkMove> getSeq() const;

private:
    struct MoveData {
        ExtPkMove move;
        U64 squares = 0; // If pawn move, squares affected by move
        Position pos;    // Position before move has been made
        Move m;          // "move" converted to real chess move

        MoveData(const ExtPkMove& m) : move(m) {}
    };

    /** Split pawn moves into several shorter moves, eg "a2a5 -> "a2a3, a3a4, a4a5". */
    void splitPawnMoves();

    /** Combine pawn moves into double pawn moves, eg "a2a3, a3a4" -> "a2a4". */
    void combinePawnMoves();

    /** For all piece moves in moves, expand them using expandPieceMove(). */
    void expandPieceMoves(std::vector<MoveData>& moves);

    /** Add piece and fromSquare info to an unspecified capture move. */
    void assignPiece(const Position& pos, ExtPkMove& move) const;

    /** Convert "move" to a sequence of moves that correspond to how chess pieces
     *  are allowed to move. For example "wRh1-f6" can be converted to "wRh1-h6, wRh6-f6".
     *  All resulting moves are legal regarding how the pieces move, but it is possible
     *  that a move allows for an illegal king capture. */
    void expandPieceMove(const Position& pos, const ExtPkMove& move,
                         std::vector<ExtPkMove>& moves) const;

    std::vector<ExtPkMove> extKernel;
    const Position initPos;
    const Position goalPos;
    std::ostream& log;
};


inline std::vector<ProofKernel::ExtPkMove>
PkSequence::getSeq() const {
    return extKernel;
}

#endif /* PKSEQ_HPP_ */
