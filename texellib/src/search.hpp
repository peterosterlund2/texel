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
 * search.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef SEARCH_HPP_
#define SEARCH_HPP_

#include "constants.hpp"
#include "position.hpp"
#include "killerTable.hpp"
#include "history.hpp"
#include "transpositionTable.hpp"
#include "evaluate.hpp"
#include "treeLogger.hpp"
#include "moveGen.hpp"

#include <limits>
#include <memory>

class SearchTest;

/**
 * Implements the nega-scout search algorithm.
 */
class Search {
    friend class SearchTest;
private:
    Position pos;
    Evaluate eval;
    KillerTable kt;
    History ht;
    std::vector<U64> posHashList; // List of hashes for previous positions up to the last "zeroing" move.
    int posHashListSize;          // Number of used entries in posHashList
    int posHashFirstNew;          // First entry in posHashList that has not been played OTB.
    TranspositionTable& tt;
    TreeLoggerWriter log;

    struct SearchTreeInfo {
        bool allowNullMove;    // Don't allow two null-moves in a row
        Move bestMove;         // Copy of the best found move at this ply
        Move currentMove;      // Move currently being searched
        int lmr;               // LMR reduction amount
        S64 nodeIdx;           // For tree logging
        SearchTreeInfo() {
            allowNullMove = true;
            lmr = 0;
            nodeIdx = 0;
        }
    };
    SearchTreeInfo searchTreeInfo[200];

    // Time management
    S64 tStart;                // Time when search started
    S64 minTimeMillis;         // Minimum recommended thinking time
    S64 maxTimeMillis;         // Maximum allowed thinking time
    bool searchNeedMoreTime;   // True if negaScout should use up to maxTimeMillis time.
    S64 maxNodes;              // Maximum number of nodes to search (approximately)
    int nodesToGo;             // Number of nodes until next time check
    int nodesBetweenTimeCheck; // How often to check remaining time

    // Reduced strength variables
    int strength;              // Strength (0-1000)
    bool weak;                 // Set to strength < 1000
    U64 randomSeed;

    // Search statistics stuff
    U64 nodes;
    U64 qNodes;
    int nodesPlyVec[20];
    int nodesDepthVec[20];
    S64 totalNodes;
    S64 tLastStats;        // Time when notifyStats was last called
    bool verbose;

    int q0Eval; // Static eval score at first level of quiescence search

public:
    Search(const Position& pos, const std::vector<U64>& posHashList,
           int posHashListSize, TranspositionTable& tt);

    void init(const Position& pos0, const std::vector<U64>& posHashList0,
              int posHashListSize0);

    class StopSearch : public std::exception {
    public:
        StopSearch() { }
    };

    /**
     * Used to get various search information during search
     */
    class Listener {
    public:
        virtual void notifyDepth(int depth) = 0;
        virtual void notifyCurrMove(const Move& m, int moveNr) = 0;
        virtual void notifyPV(int depth, int score, int time, U64 nodes, int nps,
                              bool isMate, bool upperBound, bool lowerBound,
                              const std::vector<Move>& pv) = 0;
        virtual void notifyStats(U64 nodes, int nps, int time) = 0;
    };

    void setListener(const std::shared_ptr<Listener>& listener) {
        this->listener = listener;
    }

    void timeLimit(int minTimeLimit, int maxTimeLimit);

    void setStrength(int strength, U64 randomSeed);

    Move iterativeDeepening(const MoveGen::MoveList& scMovesIn,
                            int maxDepth, U64 initialMaxNodes, bool verbose);

    /**
     * Main recursive search algorithm.
     * @return Score for the side to make a move, in position given by "pos".
     */
    int negaScout(int alpha, int beta, int ply, int depth, int recaptureSquare,
                  const bool inCheck);

    /** Return >0, 0, <0, depending on the sign of SEE(m). */
    int signSEE(const Move& m) {
        int p0 = Evaluate::pieceValue[pos.getPiece(m.from())];
        int p1 = Evaluate::pieceValue[pos.getPiece(m.to())];
        if (p0 < p1)
            return 1;
        return SEE(m);
    }

    /** Return true if SEE(m) < 0. */
    bool negSEE(const Move& m) {
        int p0 = Evaluate::pieceValue[pos.getPiece(m.from())];
        int p1 = Evaluate::pieceValue[pos.getPiece(m.to())];
        if (p1 >= p0)
            return false;
        return SEE(m) < 0;
    }

    static bool canClaimDraw50(const Position& pos) {
        return (pos.halfMoveClock >= 100);
    }

    static bool canClaimDrawRep(const Position& pos, const std::vector<U64>& posHashList,
                                int posHashListSize, int posHashFirstNew);

    /**
     * Compute scores for each move in a move list, using SEE, killer and history information.
     * @param moves  List of moves to score.
     */
    void scoreMoveList(MoveGen::MoveList& moves, int ply, int startIdx = 0);

private:
    std::shared_ptr<Listener> listener;

    struct MoveInfo {
        Move move;
        U64 nodes;
        MoveInfo(const Move& m, int n) { move = m;  nodes = n; }

        struct SortByScore {
            bool operator()(const MoveInfo& mi1, const MoveInfo& mi2) const {
                return mi1.move.score() > mi2.move.score();
            }
        };
        struct SortByNodes {
            bool operator()(const MoveInfo& mi1, const MoveInfo& mi2) {
                return mi1.nodes > mi2.nodes;
            }
        };
    };

    void notifyPV(int depth, int score, bool uBound, bool lBound, const Move& m);

    void notifyStats();

    Move emptyMove;

    /** Return true if move m2 was made possible by move m1. */
    static bool relatedMoves(const Move& m1, const Move& m2) {
        if ((m1.from() == m1.to()) || (m2.from() == m2.to()))
            return false;
        if ((m1.to() == m2.from()) || (m1.from() == m2.to()) ||
            ((BitBoard::squaresBetween[m2.from()][m2.to()] & (1ULL << m1.from())) != 0))
            return true;
        return false;
    }

    /** Return true if move should be skipped in order to make engine play weaker. */
    bool weakPlaySkipMove(const Position& pos, const Move& m, int ply) const;

    static bool passedPawnPush(const Position& pos, const Move& m) {
        int p = pos.getPiece(m.from());
        if (pos.whiteMove) {
            if (p != Piece::WPAWN)
                return false;
            if ((BitBoard::wPawnBlockerMask[m.to()] & pos.pieceTypeBB[Piece::BPAWN]) != 0)
                return false;
            return m.to() >= 40;
        } else {
            if (p != Piece::BPAWN)
                return false;
            if ((BitBoard::bPawnBlockerMask[m.to()] & pos.pieceTypeBB[Piece::WPAWN]) != 0)
                return false;
            return m.to() <= 23;
        }
    }

    /**
     * Quiescence search. Only non-losing captures are searched.
     */
    int quiesce(int alpha, int beta, int ply, int depth, const bool inCheck);

    int captures[64];   // Value of captured pieces

    /**
     * Static exchange evaluation function.
     * @return SEE score for m. Positive value is good for the side that makes the first move.
     */
    int SEE(const Move& m);

    void scoreMoveListMvvLva(MoveGen::MoveList& moves) {
        for (int i = 0; i < moves.size; i++) {
            Move& m = moves[i];
            int v = pos.getPiece(m.to());
            int a = pos.getPiece(m.from());
            m.setScore(Evaluate::pieceValueOrder[v] * 8 - Evaluate::pieceValueOrder[a]);
        }
    }

    /**
     * Find move with highest score and move it to the front of the list.
     */
    static void selectBest(MoveGen::MoveList& moves, int startIdx);

    /** If hashMove exists in the move list, move the hash move to the front of the list. */
    static bool selectHashMove(MoveGen::MoveList& moves, const Move& hashMove);

    void initNodeStats();

    // Not implemented
    Search(const Search& other);
    Search& operator=(const Search& other);
};

#endif /* SEARCH_HPP_ */
