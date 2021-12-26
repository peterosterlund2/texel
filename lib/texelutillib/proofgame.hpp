/*
    Texel - A UCI chess engine.
    Copyright (C) 2015-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * proofgame.hpp
 *
 *  Created on: Aug 15, 2015
 *      Author: petero
 */

#ifndef PROOFGAME_HPP_
#define PROOFGAME_HPP_

#include "util/util.hpp"
#include "position.hpp"
#include "assignment.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>

/**
 * Search for a sequence of legal moves leading from a start to an end position.
 */
class ProofGame {
    friend class ProofGameTest;
public:
    /** Create object to find a move path from a start to a goal position.
     * A position is considered to match the goal position even if move
     * numbers are different.
     * Use scale a for ply and scale b for bound when ordering nodes to search.
     * If "dynamic" is true, dynamic weighting A* search is used. */
    ProofGame(const std::string& start, const std::string& goal,
              int a = 1, int b = 1, bool dynamic = false, bool smallCache = false);
    ProofGame(std::ostream& log, const std::string& start, const std::string& goal,
              int a = 1, int b = 1, bool dynamic = false, bool smallCache = false);
    ProofGame(const ProofGame&) = delete;
    ProofGame& operator=(const ProofGame&) = delete;

    /** Search for shortest solution. Print solutions to standard output.
     * @param initialPath  Only search for solutions starting with this path.
     * @param movePath     Set to shortest found path.
     * @param maxNodes     Maximum number of search nodes before giving up,
     *                     or -1 to never give up.
     * @param verbose      If true, print path every time distance to goal decreases.
     * @return             Length of shortest path found,
     *                     or INT_MAX if no solution exists,
     *                     or -1 if unknown whether a solution exists. */
    int search(const std::vector<Move>& initialPath,
               std::vector<Move>& movePath, S64 maxNodes = -1, bool verbose = false);

    /** Compute blocked pieces in a position. A blocked piece is a piece that
     *  can not move without making it impossible to reach the goal position.
     *  If false is returned, it is impossible to reach goalPos from pos. */
    bool computeBlocked(const Position& pos, U64& blocked) const;

    /** Return the goal position. */
    const Position& getGoalPos() const;

private:
    /** Initialize static data if not already done. */
    void staticInit();

    /** Check that there are not too many pieces present. */
    static void validatePieceCounts(const Position& pos);

    /** Get moves from a PGN file. Only solutions starting with
     *  this move sequence will be considered during search. */
    static void getInitialPath(const std::string& initialPgnFile, Position& startPos,
                               std::vector<Move>& initialPath);

    /** Queue a new position to be searched. Return true if position was queued. */
    bool addPosition(const Position& pos, U32 parent, bool isRoot, bool checkBound);

    /** Return true if pos is equal to the goal position. */
    bool isSolution(const Position& pos) const;

    /** Get move sequence leading to position given by index "idx".
     * Also print move sequence to standard output. */
    void getMoves(const Position& startPos, int idx, bool includeLastMoves,
                  std::vector<Move>& movePath) const;

    /** Compute a lower bound for the minimum number of plies from position pos
     * to position goalPos. If INT_MAX is returned, it means that goalPos can not be
     * reached from pos. */
    int distLowerBound(const Position& pos);

    /** Return true if there are enough remaining pieces to reach goalPos. */
    bool enoughRemainingPieces(int pieceCnt[]) const;

    /** Print required piece count changes to standard output. */
    void showPieceStats(const Position& pos) const;

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
                                 SqPathData promPath[2][8],
                                 std::vector<SqPathData>& sqPathData, U64& blocked);

    /** Compute shortest path for a pawn on fromSq to a target square after
     * a suitable promotion. */
    int promPathLen(bool wtm, int fromSq, int targetPiece, U64 blocked, int maxCapt,
                    const ShortestPathData& toSqPath, SqPathData promPath[8]);

    /** Compute all cutsets for all pawns in the assignment problem. */
    bool computeAllCutSets(const Assignment<int>& as, int rowToSq[16], int colToSq[16],
                           bool wtm, U64 blocked, int maxCapt,
                           U64 cutSets[16], int& nConstraints);

    /** Assume a pawn has to move from one of the fromSqMask squares to the toSq square.
     * A cut set is a set of squares that if they were blocked would prevent the pawn
     * from reaching toSq regardless of which square in fromSqMask it started from.
     * Compute zero or more such cut sets and add them to cutSets.
     * Return false if nCutSets becomes > 15. */
    bool computeCutSets(bool wtm, U64 fromSqMask, int toSq, U64 blocked, int maxCapt,
                        U64 cutSets[16], int& nCutSets);

    /** Compute the union of all possible pawn paths from a source square
     * to a target square. */
    U64 allPawnPaths(bool wtm, int fromSq, int toSq, U64 blocked, int maxCapt);

    /** Compute shortest distance from one square to one of several possible target squares. */
    int minDistToSquares(int piece, int fromSq, U64 blocked, int maxCapt,
                         SqPathData promPath[8], U64 targetSquares, bool canPromote);

    /** Compute shortest path for a pawn on fromSq to a target square after
     * any valid promotion. */
    int promPathLen(bool wtm, int fromSq, U64 blocked, int maxCapt,
                    int toSq, SqPathData promPath[8], int pLen);

    /** Compute and return the smallest cost of a solution to the given assignment
     * problem. Also use heuristics to set weights to bigCost in cases where it can
     * be proved that the weight can not be part of *any* solution. Weights that can
     * be part of a non-optimal solution are not modified. */
    static int solveAssignment(Assignment<int>& as);

    /** Compute blocked pieces caused by a king on the first rank trapped by
     *  two enemy pawns on the second and third ranks.
     *  If false is returned, it is impossible to reach goalPos from pos. */
    bool computeKingPawnsTrapBlocked(const Position& pos, U64& blocked) const;

    /** Compute shortest path for a piece p to toSq from all possible start squares,
     *  taking blocked squares into account. For squares that can not reach toSq,
     *  the shortest path is set to -1. For pawns the maximum number of available
     *  captures is taken into account. */
    std::shared_ptr<ShortestPathData> shortestPaths(Piece::Type p, int toSq, U64 blocked,
                                                    int maxCapt);

    /** Compute all squares that can reach toSquares in one move while
     *  taking blocked squares into account. */
    static U64 computeNeighbors(Piece::Type p, U64 toSquares, U64 blocked);

    static const int bigCost = 1000;

    const std::string initialFen;
    Position goalPos;
    int goalPieceCnt[Piece::nPieceTypes];
    std::vector<Move> lastMoves; // Forced moves after reaching goalPos to reach original goalPos

    const int weightA; // Weight for length of current partial solution
    const int weightB; // Weight for heuristic lower bound for length to goalPos
    const bool dynamic; // If true, use dynamic weighting algorithm

    struct TreeNode {
        Position::SerializeData psd; // Position data
        U32 parent;                  // Parent index, not used for root position
        U16 ply;                     // Number of moves already made, 0 for root node
        U16 bound;                   // Lower bound on number of moves to a solution

        int sortWeight(int a, int b, int N) const {
            if (N == 0) {
                return a * ply + b * bound;
            } else {
                int p = std::min((int)ply, N);
                return a*N * ply + (a*N + (b-a)*(N-p)) * bound;
            }
        }
    };

    // All nodes encountered so far
    std::vector<TreeNode> nodes;

    // Hash table of already seen nodes, to avoid duplicate work after transpositions
    std::unordered_map<U64,int> nodeHash;

    class TreeNodeCompare {
    public:
        TreeNodeCompare(const Position& goalPos0, const std::vector<TreeNode>& nodes0,
                        int a0, int b0, int N0)
            : goalPos(goalPos0), nodes(nodes0), k0(a0), k1(b0), N(N0) {}
        bool operator()(int a, int b) const {
            return higherPrio(b, a);
        }
    private:
        /** Return true if node "a" has higher priority than node "b". */
        bool higherPrio(int a, int b) const;

        const Position& goalPos;
        const std::vector<TreeNode>& nodes;
        int k0, k1;
        int N;
    };

    // Nodes ordered by "ply+bound". Elements are indices in the nodes vector.
    using Queue = std::priority_queue<int, std::vector<int>, TreeNodeCompare>;
    std::unique_ptr<Queue> queue;

    // Cache of recently used ShortestPathData objects
    int pathCacheSize = 1024*1024;
    struct PathCacheEntry {
        PathCacheEntry() : piece(-1), toSq(-1), maxCapt(-1), blocked(0) {}
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

    // Squares reachable by a pawn on an empty board, starting at a given square,
    // depending on number of available captures.
    static const int maxPawnCapt = 5;
    static U64 wPawnReachable[64][maxPawnCapt+1]; // [sq][nCaptures]
    static U64 bPawnReachable[64][maxPawnCapt+1];
    static bool staticInitDone;

    std::ostream& log;
};

inline const Position&
ProofGame::getGoalPos() const {
    return goalPos;
}


inline bool
ProofGame::isSolution(const Position& pos) const {
    if (pos.zobristHash() != goalPos.zobristHash())
        return false;
    return pos.drawRuleEquals(goalPos);
}

#endif /* PROOFGAME_HPP_ */
