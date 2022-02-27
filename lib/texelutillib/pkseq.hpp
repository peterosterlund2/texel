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
    /** Split pawn moves into several shorter moves, eg "a2a5 -> "a2a3, a3a4, a4a5". */
    void splitPawnMoves();

    /** Combine pawn moves into double pawn moves, eg "a2a3, a3a4" -> "a2a4". */
    void combinePawnMoves();


    struct MoveData {
        int id;                     // Graph node ID
        ExtPkMove move;
        bool pseudoLegal;           // True if move is known to be pseudo legal
        std::vector<int> dependsOn; // IDs of moves that must be played before this move

        MoveData(int id, const ExtPkMove& m) : id(id), move(m), pseudoLegal(false) {}
    };

    struct Graph {
        std::vector<MoveData> nodes;
        int nextId = 1;

        /** Add "move" last in "nodes". */
        void addNode(const ExtPkMove& move);

        /** Replace nodes[idx] with a sequence of nodes given by "moves".
         *  Dependencies are added to the first/last move in "moves"
         *  corresponding to the out/in edges for nodes[idx]. Dependencies
         *  are also added to prevent reordering within the "moves" sequence.
         *  "moves" must be pseudo legal. */
        void replaceNode(int idx, const std::vector<ExtPkMove>& moves);

        /** Topologically sort the nodes. The initial sequence of moves that
         *  is already in sorted order is guaranteed to not be modified.
         *  Return false if not a DAG. */
        bool topoSort();

    private:
        bool sortRecursive(int i,
                           std::vector<bool>& visited,
                           std::vector<bool>& currPath,
                           const std::vector<int>& idToIdx,
                           std::vector<MoveData>& result);
    };

    /** Try to improve the proof kernel move sequence starting from position "idx".
     *  "pos" must correspond to the position before the move at position "idx"
     *  has been played.
     *  Returns true if improving the sequence succeeded. */
    bool improveKernel(Graph& kernel, int idx, const Position& pos) const;

    /** Try to apply "move" to "pos". Return false if not possible, eg because
     *  the target position is already occupied for a non-capture move. */
    static bool makeMove(Position& pos, UndoInfo& ui, const ExtPkMove& move);


    /** Add piece and fromSquare info to an unspecified capture move. */
    bool assignPiece(Graph& kernel, int idx, const Position& pos) const;

    /** Convert move to a sequence of moves that correspond to how chess pieces
     *  are allowed to move. For example "wRh1-f6" can be converted to "wRh1-h6,
     *  wRh6-f6". All resulting moves are legal regarding how the pieces move,
     *  but it is possible that a move allows for an illegal king capture. That
     *  is, the moves are pseudo legal.
     *  No output moves interfere with the "blocked" squares.
     *  Return false if conversion is not possible. */
    static bool expandPieceMove(const ExtPkMove& move, U64 blocked,
                                std::vector<ExtPkMove>& outMoves);

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
