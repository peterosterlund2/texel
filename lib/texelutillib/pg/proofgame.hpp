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

#include "util.hpp"
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
     * @param initialPath  Only search for solutions starting with this path. */
    ProofGame(const std::string& start, const std::string& goal,
              bool analyzeLastMoves,
              const std::vector<Move>& initialPath,
              bool useNonForcedIrreversible = false, std::ostream& log = std::cout);
    ProofGame(const ProofGame&) = delete;
    ProofGame& operator=(const ProofGame&) = delete;

    /** Set random seed used for breaking ties between positions having the same
     *  priority in the A* search. */
    void setRandomSeed(U64 seed);

    struct Options {
        /** Weight for length of current partial solution. */
        int weightA = 1;
        /** Weight for heuristic lower bound for length to goalPos. */
        int weightB = 1;
        /** If true, use dynamic weighting A* algorithm. */
        bool dynamic = false;
        /** If true, use a non-admissible heuristic function that takes
         *  into account that pieces can block each other. */
        bool useNonAdmissible = false;
        /** If true, use a minimal cache to reduce initialization time. */
        bool smallCache = false;

        /** Maximum number of search nodes before giving up,
         *  or -1 to never give up. */
        S64 maxNodes = -1;
        /** If true, print path every time distance to goal decreases. */
        bool verbose = false;
        /** If true, accept first solution found, otherwise continue searching
         *  for a better solution. */
        bool acceptFirst = false;

        Options& setWeightA(int a) { weightA = a; return *this; }
        Options& setWeightB(int b) { weightB = b; return *this; }
        Options& setDynamic(int d) { dynamic = d; return *this; }
        Options& setUseNonAdmissible(int b) { useNonAdmissible = b; return *this; }
        Options& setSmallCache(bool s) { smallCache = s; return *this; }
        Options& setMaxNodes(S64 m) { maxNodes = m; return *this; }
        Options& setVerbose(bool v) { verbose = v; return *this; }
        Options& setAcceptFirst(bool a) { acceptFirst = a; return *this; }
    };

    struct Result {
        // Data always returned
        int numNodes = 0;              // Number of nodes searched
        double computationTime = 0;    // Time spent searching

        // Data returned when a solution was found
        std::vector<Move> proofGame;

        // Data returned when no solution was found
        std::vector<Move> closestPath; // Best found non-solution
        int smallestBound = 0;         // Minimum distance from closestPath to goal
    };

    /** Search for shortest solution. Print solutions to log stream.
     * @param opts    Options controlling search behavior.
     * @param result  Computed result.
     * @return        Length of shortest path found,
     *                or INT_MAX if no solution exists,
     *                or -1 if unknown whether a solution exists. */
    int search(const Options& opts, Result& result);

    /** Compute blocked pieces in a position. A blocked piece is a piece that
     *  can not move without making it impossible to reach the goal position.
     *  If false is returned, it is impossible to reach goalPos from pos. */
    bool computeBlocked(const Position& pos, U64& blocked) const;
    static bool computeBlocked(const Position& pos, const Position& goalPos,
                               U64& blocked, bool findInfeasible = false);

    /** Return true if it can be determined without search that the problem has
     *  no solution. If a piece can be determined that cannot be moved from the
     *  start square to a goal square, set "fromSq" and "toSq" to the
     *  corresponding squares. */
    bool isInfeasible(Square& fromSq, Square& toSq);

    /** Return the goal position. */
    const Position& getGoalPos() const;

    struct ShortestPathData {
        S8 pathLen[64];    // Distance to target square
        U64 fromSquares;   // Bitboard corresponding to pathLen[i] >= 0

        /** Get squares that are closer to the goal than fromSq and can
         *  be reached from fromSq in one move. */
        U64 getNextSquares(int piece, Square fromSq, U64 blocked) const;
    };

    /** Compute shortest path for a piece p to toSq from all possible start squares,
     *  taking blocked squares into account. For squares that can not reach toSq,
     *  the shortest path is set to -1.
     *  @param pawnSub  When "p" is a pawn, solution for the same problem but with
     *                  one less pawn capture available. nullptr if "p" is not a
     *                  pawn or if no pawn capture is available. */
    static void shortestPaths(Piece::Type p, Square toSq, U64 blocked,
                              const ShortestPathData* pawnSub,
                              ShortestPathData& spd);

    /** Initialize static data. */
    static void staticInitialize();

private:
    /** Compute forced moves at the end of the proof game. Update goalPos to be
     *  the position before the first of "lastMoves" is played. */
    static void computeLastMoves(const Position& startPos, Position& goalPos,
                                 bool useNonForcedIrreversible,
                                 std::vector<Move>& lastMoves,
                                 std::ostream& log);

    /** Check that there are not too many pieces present. */
    static void validatePieceCounts(const Position& pos);

    /** Get moves from a PGN file. Only solutions starting with
     *  this move sequence will be considered during search. */
    static void getInitialPath(const std::string& initialPgnFile, Position& startPos,
                               std::vector<Move>& initialPath);

    /** Queue a new position to be searched. Return true if position was queued. */
    bool addPosition(const Position& pos, U32 parent, bool isRoot, bool checkBound, int best);

    /** Return true if pos is equal to the goal position. */
    bool isSolution(const Position& pos) const;

    /** Get move sequence leading to position given by index "idx".
     * Also print move sequence to "logStream". */
    void getMoves(std::ostream& logStream, const Position& startPos, int idx,
                  bool includeLastMoves, std::vector<Move>& movePath) const;

    /** Compute a lower bound for the minimum number of plies from position pos
     * to position goalPos. If INT_MAX is returned, it means that goalPos can not be
     * reached from pos. */
    int distLowerBound(const Position& pos);

    /** Return true if there are enough remaining pieces to reach goalPos. */
    bool enoughRemainingPieces(int pieceCnt[]) const;

    /** Print required piece count changes to log stream. */
    void showPieceStats(const Position& pos) const;

    /** Compute number of required captures to get pawns into correct files.
     * Check that excess material can satisfy both required captures and
     * required pawn promotions. */
    bool capturesFeasible(const Position& pos, int pieceCnt[],
                          int numWhiteExtraPieces, int numBlackExtraPieces,
                          int excessWPawns, int excessBPawns);

    struct SqPathData {
        SqPathData() {}
        SqPathData(Square s, const std::shared_ptr<ShortestPathData>& d)
            : square(s), spd(d) {}
        Square square;
        std::shared_ptr<ShortestPathData> spd;
    };

    /** Compute lower bound of number of needed moves for white/black.
     * Return false if it is discovered that goalPos is not reachable. */
    bool computeNeededMoves(const Position& pos, U64 blocked,
                            int numWhiteExtraPieces, int numBlackExtraPieces,
                            int excessWPawns, int excessBPawns,
                            int neededMoves[]);

    /** Print assignment cost matrix to log. */
    void printAssignment(const Assignment<int>& as, int N,
                         const Square (&rowToSq)[16], const Square (&colToSq)[16]) const;

    /** Find a single piece that makes the assignment problem infeasible.
     *  If such a piece exists, set "infeasibleFrom" and "infeasibleTo" to the
     *  piece square in the current and goal position. */
    void findInfeasibleMove(const Position& pos, const Assignment<int>& as, int N,
                            const Square (&rowToSq)[16], const Square (&colToSq)[16]);

    /** Return obstacles between fromSq and toSq when following a shortest path
     *  from fromSq to toSq. Obstacles are squares containing the same piece in
     *  "pos" and "goalPos". */
    U64 getMovePathObstacles(const Position& pos, U64 blocked,
                             Square fromSq, Square toSq,
                             const ShortestPathData& spd) const;

    /** If pieceType is a king, extend "blocked" to also include attacks from
     *  blocked opponent pawns. */
    U64 getBlocked(U64 blocked, const Position& pos, int pieceType) const;

    /** Compute shortest path data to all non-blocked goal squares. Update
     * blocked if it is discovered that more pieces are blocked.
     * Return false if it is discovered the goalPos is not reachable. */
    bool computeShortestPathData(const Position& pos,
                                 int numWhiteExtraPieces, int numBlackExtraPieces,
                                 SqPathData promPath[2][8],
                                 std::vector<SqPathData>& sqPathData, U64& blocked);

    /** Compute shortest path for a pawn on fromSq to a target square after
     * a suitable promotion. */
    int promPathLen(bool wtm, Square fromSq, int targetPiece, U64 blocked, int maxCapt,
                    const ShortestPathData& toSqPath, SqPathData promPath[8]);

    /** Compute all cutsets for all pawns in the assignment problem. */
    bool computeAllCutSets(const Assignment<int>& as, Square rowToSq[16], Square colToSq[16],
                           bool wtm, U64 blocked, int maxCapt,
                           U64 cutSets[16], int& nConstraints);

    /** Assume a pawn has to move from one of the fromSqMask squares to the toSq square.
     * A cut set is a set of squares that if they were blocked would prevent the pawn
     * from reaching toSq regardless of which square in fromSqMask it started from.
     * Compute zero or more such cut sets and add them to cutSets.
     * Return false if nCutSets becomes > 15. */
    bool computeCutSets(bool wtm, U64 fromSqMask, Square toSq, U64 blocked, int maxCapt,
                        U64 cutSets[16], int& nCutSets);

    /** Compute the union of all possible pawn paths from a source square
     * to a target square. */
    U64 allPawnPaths(bool wtm, Square fromSq, Square toSq, U64 blocked, int maxCapt);

    /** Compute shortest distance from one square to one of several possible target squares. */
    int minDistToSquares(int piece, Square fromSq, U64 blocked, int maxCapt,
                         SqPathData promPath[8], U64 targetSquares, bool canPromote);

    /** Compute shortest path for a pawn on fromSq to a target square after
     * any valid promotion. */
    int promPathLen(bool wtm, Square fromSq, U64 blocked, int maxCapt,
                    Square toSq, SqPathData promPath[8], int pLen);

    /** Compute and return the smallest cost of a solution to the given assignment
     * problem. Also use heuristics to set weights to bigCost in cases where it can
     * be proved that the weight can not be part of *any* solution. Weights that can
     * be part of a non-optimal solution are not modified. */
    static int solveAssignment(Assignment<int>& as);

    /** Compute pieces that cannot move because they block each other.
     *  If false is returned, it is impossible to reach goalPos from pos. */
    static bool computeDeadlockedPieces(const Position& pos, const Position& goalPos,
                                        U64& blocked);

    /** Compute shortest paths for a piece p to toSq from all possible start squares,
     *  taking blocked squares into account. For squares that can not reach toSq,
     *  the shortest path is set to -1. For pawns the maximum number of available
     *  captures is taken into account. */
    std::shared_ptr<ShortestPathData> shortestPaths(Piece::Type p, Square toSq, U64 blocked,
                                                    int maxCapt);

    /** Compute all squares that can reach toSquares in one move while
     *  taking blocked squares into account. */
    static U64 computeNeighbors(Piece::Type p, U64 toSquares, U64 blocked);

    static const int bigCost = 1000;

    const std::string initialFen;
    Position goalPos;
    const std::vector<Move> initialPath;
    int goalPieceCnt[Piece::nPieceTypes];
    std::vector<Move> lastMoves; // Forced moves after reaching goalPos to reach original goalPos

    U64 rndSeed;   // Random seed for breaking ties between equal priority nodes

    struct TreeNode {
        Position::SerializeData psd; // Position data
        U32 parent;                  // Parent index, not used for root position
        U16 ply;                     // Number of moves already made, 0 for root node
        U16 bound;                   // Lower bound on number of moves to a solution
        U32 prio;                    // For otherwise equal nodes, higher prio nodes are searched first

        int sortWeight(int a, int b, int N) const {
            if (N == 0) {
                return a * ply + b * bound;
            } else {
                int p = std::min((int)ply, N);
                return a*N * ply + (a*N + (b-a)*(N-p)) * bound;
            }
        }
        void computePrio(const Position& pos, const Position& goalPos, U64 rnd);
    };

    // All nodes encountered so far
    std::vector<TreeNode> nodes;

    // Hash table of already seen nodes, to avoid duplicate work after transpositions
    std::unordered_map<U64,int> nodeHash;

    class TreeNodeCompare {
    public:
        TreeNodeCompare(const std::vector<TreeNode>& nodes0,
                        int a0, int b0, int N0)
            : nodes(nodes0), k0(a0), k1(b0), N(N0) {}
        bool operator()(int a, int b) const {
            return higherPrio(b, a);
        }
    private:
        /** Return true if node "a" has higher priority than node "b". */
        bool higherPrio(int a, int b) const;

        const std::vector<TreeNode>& nodes;
        int k0, k1;
        int N;
    };

    // Nodes ordered by "ply+bound". Elements are indices in the nodes vector.
    using Queue = std::priority_queue<int, std::vector<int>, TreeNodeCompare>;
    std::unique_ptr<Queue> queue;

    // Cache of recently used ShortestPathData objects
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

    bool useNonAdmissible = false;

    bool findInfeasible = false;
    Square infeasibleFrom;
    Square infeasibleTo;

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
