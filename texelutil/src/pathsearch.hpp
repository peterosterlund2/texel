/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * pathsearch.hpp
 *
 *  Created on: Aug 15, 2015
 *      Author: petero
 */

#ifndef PATHSEARCH_HPP_
#define PATHSEARCH_HPP_

#include "util/util.hpp"
#include "position.hpp"
#include "assignment.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

/**
 * Search for a sequence of legal moves leading from a start to an end position.
 */
class PathSearch {
    friend class PathSearchTest;
public:
    /** Create object to find a move path to a goal position.
     * A position is considered to match the goal position even if move
     * numbers, en passant square, and/or castling flags are different.
     * Use scale a for ply and scale b for bound when ordering nodes to search. */
    PathSearch(const std::string& goal, int a = 1, int b = 1);

    /** Search for shortest solution. Print solutions to standard output.
     * Return length of shortest path found.
     */
    int search(const std::string& initialFen, std::vector<Move>& movePath);

    /** Return goal position. */
    const Position& getGoalPos() const;

private:
    /** Initialize static data if not already done. */
    void staticInit();

    /** Check that there are not too many pieces present. */
    static void validatePieceCounts(const Position& pos);

    /** Queue a new position to be searched. */
    void addPosition(const Position& pos, U32 parent, bool isRoot);

    /** Return true if pos is equal to the goal position. */
    bool isSolution(const Position& pos) const;

    /** Get move sequence leading to position given by index "idx".
     * Also print move sequence to standard output. */
    void getSolution(const Position& startPos, int idx, std::vector<Move>& movePath) const;

    /** Compute a lower bound for the minimum number of plies from position pos
     * to position goalPos. If INT_MAX is returned, it means that goalPos can not be
     * reached from pos. */
    int distLowerBound(const Position& pos);

    /** Return true if there are enough remaining pieces to reach goalPos. */
    bool enoughRemainingPieces(int pieceCnt[]) const;

    /** Compute number of required captures to get pawns into correct files.
     * Check that excess material can satisfy both required captures and
     * required pawn promotions. */
    bool capturesFeasible(const Position& pos, int pieceCnt[],
                          int numWhiteExtraPieces, int numBlackExtraPieces,
                          int excessWPawns, int excessBPawns);

    struct ShortestPathData {
        S8 pathLen[64];    // Distance to target square
        U64 fromSquares;   // Bitboard corresponding to pathLen[i] >= 0
    };

    struct SqPathData {
        SqPathData() : square(-1) {}
        SqPathData(int s, const std::shared_ptr<ShortestPathData>& d)
            : square(s), spd(d) {}
        int square;
        std::shared_ptr<ShortestPathData> spd;
    };

    /** Compute lower bound of number of needed moves for white/black.
     * Return false if it is discovered that goalPos is not reachable. */
    bool computeNeededMoves(const Position& pos, U64 blocked,
                            int numWhiteExtraPieces, int numBlackExtraPieces,
                            int excessWPawns, int excessBPawns,
                            int neededMoves[]);

    /** Compute shortest path data to all non-blocked goal squares. Update
     * blocked if it is discovered that more pieces are blocked.
     * Return false if it is discovered the goalPos is not reachable. */
    bool computeShortestPathData(const Position& pos,
                                 int numWhiteExtraPieces, int numBlackExtraPieces,
                                 SqPathData promPath[][8],
                                 std::vector<SqPathData>& sqPathData, U64& blocked);

    /** Compute blocked pieces in a position. A block piece is a piece that
     *  can not move without making it impossible to reach the goal position.
     *  If false is returned, it is impossible to reach goalPos from pos. */
    bool computeBlocked(const Position& pos, U64& blocked) const;


    /** Compute shortest path for a piece p to toSq from all possible start squares,
     *  taking blocked squares into account. For squares that can not reach toSq,
     *  the shortest path is set to 1. For pawns the maximum number of available
     *  captures is taken into account. */
    std::shared_ptr<ShortestPathData> shortestPaths(Piece::Type p, int toSq, U64 blocked,
                                                    int maxCapt);

    /** Compute all squares that can reach toSquares in one move while
     *  taking blocked squares into account. */
    static U64 computeNeighbors(Piece::Type p, U64 toSquares, U64 blocked);


    Position goalPos;
    int goalPieceCnt[Piece::nPieceTypes];
    Move epMove; // Move that sets up the EP square to get to the original goalPos

    struct TreeNode {
        Position::SerializeData psd; // Position
        U32 parent;                  // Parent index, not used for root position
        U16 ply;                     // Number of moves already made, 0 for root node
        U16 bound;                   // Lower bound on number of moves to a solution

        int sortWeight(int a, int b) const { return a * ply + b * bound; }
    };

    // All nodes encountered so far
    std::vector<TreeNode> nodes;

    // Hash table of already seen nodes, to avoid duplicate work after transpositions
    std::unordered_map<U64,int> nodeHash;

    class TreeNodeCompare {
    public:
        TreeNodeCompare(const std::vector<TreeNode>& nodes0, int a0, int b0)
            : nodes(nodes0), k0(a0), k1(b0) {}
        bool operator()(int a, int b) const {
            const TreeNode& n1 = nodes[a];
            const TreeNode& n2 = nodes[b];
            int min1 = n1.sortWeight(k0, k1);
            int min2 = n2.sortWeight(k0, k1);
            if (min1 != min2)
                return min1 > min2;
            if (n1.ply != n2.ply)
                return n1.ply < n2.ply;
            return n1.parent < n2.parent;
        }
    private:
        const std::vector<TreeNode>& nodes;
        int k0, k1;
    };

    // Nodes ordered by "ply+bound". Elements are indices in the nodes vector.
    std::priority_queue<int, std::vector<int>, TreeNodeCompare> queue;

    // Cache of recently used ShortestPathData objects
    static const int PathCacheSize = 1024*1024;
    struct PathCacheEntry {
        PathCacheEntry() : piece(-1), toSq(-1), blocked(0) {}
        U8 piece;
        U8 toSq;
        U8 maxCapt;
        U64 blocked;
        std::shared_ptr<ShortestPathData> spd;
    };
    std::vector<PathCacheEntry> pathDataCache;

    // Assignment objects used to compute number of required captures for white/black.
    Assignment<int> captureAP[2];
    // Assignment objects used to compute piece movements.
    static const int maxMoveAPSize = 16;
    Assignment<int> moveAP[2][maxMoveAPSize + 1];

    // Squares reachable by a pawn on an empty board, starting at a given square.
    static U64 wPawnReachable[64];
    static U64 bPawnReachable[64];
    static bool staticInitDone;
};


inline bool
PathSearch::isSolution(const Position& pos) const {
    if (pos.zobristHash() != goalPos.zobristHash())
        return false;
    return pos.drawRuleEquals(goalPos);
}

inline const Position&
PathSearch::getGoalPos() const {
    return goalPos;
}

#endif /* PATHSEARCH_HPP_ */
