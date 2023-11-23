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
#include <deque>

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
        bool pseudoLegal = false;   // True if move is known to be or have been pseudo legal
        bool movedEarly = false;    // True if piece move has already been moved to
                                    // an earlier point in the sequence
        std::vector<int> dependsOn; // IDs of moves that must be played before this move

        MoveData(int id, const ExtPkMove& m) : id(id), move(m) {}
    };

    struct Graph {
        std::vector<MoveData> nodes;
        int nextId = 1;

        /** Add "m" last in "nodes". If the move is a pawn move, add
         *  dependencies to relevant previous moves. */
        void addNode(const ExtPkMove& m);

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

        /** After a move has been inserted at position "idx", adjust the next
         *  move of the same piece (if any) so that its from square matches the
         *  to square of the inserted move. Also add dependencies between this
         *  move and the previous/next move of the same piece. */
        void adjustPrevNextMove(int idx);

        /** Print kernel move sequence, highlighting the move at "idx". */
        void print(std::ostream& os, int idx) const;

    private:
        bool sortRecursive(int i,
                           std::vector<bool>& visited,
                           std::vector<bool>& currPath,
                           const std::vector<int>& idToIdx,
                           std::vector<MoveData>& result);
    };

    struct SearchLimits {
        int level = 0;
        int d1 = 2;
        int d2 = 3;
        int d3 = 5;
        U64 maxNodes = 0;

        SearchLimits nextLev() { level++; return *this; }
        SearchLimits decD1() { d1--; return *this; }
        SearchLimits decD2() { d2--; return *this; }
        SearchLimits decD3() { d3--; return *this; }
    };

    /** Try to improve the proof kernel move sequence starting from position "idx".
     *  "pos" must correspond to the position before the move at position "idx"
     *  has been played.
     *  Returns true if improving the sequence succeeded. */
    bool improveKernel(Graph& kernel, int idx, const Position& pos, const SearchLimits lim);

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
     *  No output moves interfere with the "occupied" squares.
     *  Return false if conversion is not possible. */
    static bool expandPieceMove(const ExtPkMove& move, U64 occupied,
                                std::vector<ExtPkMove>& outMoves);

    /** Compute possible pawn moves in the position corresponding to the end of
     *  the proof kernel. */
    void getPawnMoves(const Graph& kernel, int idx, const Position& inPos,
                      std::vector<ExtPkMove>& pawnMoves) const;

    /** Compute ordered list of evasion squares for a non-pawn piece, attempting
     * to put "good" squares early in the list. */
    void getPieceEvasions(const Position& pos, Square fromSq,
                          std::vector<Square>& toSquares) const;

    std::vector<ExtPkMove> extKernel;
    const Position initPos;
    const Position goalPos;
    std::ostream& log;

    struct Data {
        Graph graph;
        Position pos;
    };
    std::deque<Data> searchData;
    U64 nodes;
};


inline std::vector<ProofKernel::ExtPkMove>
PkSequence::getSeq() const {
    return extKernel;
}

#endif /* PKSEQ_HPP_ */
