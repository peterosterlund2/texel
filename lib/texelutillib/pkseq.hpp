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
    using ExtPkMove = ProofKernel::ExtPkMove;
public:
    /** Constructor. */
    PkSequence(std::vector<ExtPkMove>& extKernel,
               const Position& initPos, const Position& goalPos,
               std::ostream& log);

    /** Make the move sequence more suitable to be converted to a sequence
     *  of real chess moves. */
    void improve();

    /** Return the improved move sequence. */
    std::vector<ExtPkMove> getSeq() const;

private:
    /** Split pawn moves into several shorter moves, eg "a2a5 -> "a2a3, a3a4, a4a5". */
    void splitPawnMoves();

    /** Combine pawn moves into double pawn moves, eg "a2a3, a3a4" -> "a2a4". */
    void combinePawnMoves();

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
