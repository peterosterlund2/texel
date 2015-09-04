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
 * pathsearch.cpp
 *
 *  Created on: Aug 15, 2015
 *      Author: petero
 */

#include "pathsearch.hpp"
#include "assignment.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "util/timeUtil.hpp"

#include <iostream>
#include <climits>


bool PathSearch::staticInitDone = false;
U64 PathSearch::wPawnReachable[64];
U64 PathSearch::bPawnReachable[64];


void
PathSearch::staticInit() {
    if (staticInitDone)
        return;

    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);
            U64 mask = 1ULL << sq;
            if (y < 7) {
                mask |= wPawnReachable[sq + 8];
                if (x > 0)
                    mask |= wPawnReachable[sq + 7];
                if (x < 7)
                    mask |= wPawnReachable[sq + 9];
            }
            wPawnReachable[sq] = mask;
        }
    }

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);
            U64 mask = 1ULL << sq;
            if (y > 0) {
                mask |= bPawnReachable[sq - 8];
                if (x > 0)
                    mask |= bPawnReachable[sq - 9];
                if (x < 7)
                    mask |= bPawnReachable[sq - 7];
            }
            bPawnReachable[sq] = mask;
        }
    }

    staticInitDone = true;
}

PathSearch::PathSearch(const std::string& goal)
    : queue(TreeNodeCompare(nodes)) {
    goalPos = TextIO::readFEN(goal);
    validatePieceCounts(goalPos);
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        goalPieceCnt[p] = BitBoard::bitCount(goalPos.pieceTypeBB((Piece::Type)p));

    // FIXME! Handle ep square in goalPos by searching for previous position and
    // append the double pawn move after the search.


    staticInit();
}

void
PathSearch::validatePieceCounts(const Position& pos) {
    int pieceCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieceCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    // White must not have too many pieces
    int maxWPawns = 8;
    maxWPawns -= std::max(0, pieceCnt[Piece::WKNIGHT] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WBISHOP] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WROOK  ] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WQUEEN ] - 1);
    if (pieceCnt[Piece::WPAWN] > maxWPawns)
        throw ChessParseError("Too many white pieces");

    // Black must not have too many pieces
    int maxBPawns = 8;
    maxBPawns -= std::max(0, pieceCnt[Piece::BKNIGHT] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BBISHOP] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BROOK  ] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BQUEEN ] - 1);
    if (pieceCnt[Piece::BPAWN] > maxBPawns)
        throw ChessParseError("Too many black pieces");
}

void
PathSearch::search(const std::string& initialFen) {
    Position pos = TextIO::readFEN(initialFen);
    validatePieceCounts(pos);
    addPosition(pos, 0);

    double t0 = currentTime();
    U64 numNodes = 0;
    int minCost = -1;
    int best = INT_MAX;
    UndoInfo ui;
    while (!queue.empty()) {
        const U32 idx = queue.top();
        queue.pop();
        const TreeNode& tn = nodes[idx];
        if (tn.ply >= best)
            continue;
        if (tn.ply + tn.bound > minCost) {
            minCost = tn.ply + tn.bound;
            std::cout << "min cost: " << minCost << " queue: " << queue.size() << std::endl;
        }

        numNodes++;

        pos.deSerialize(tn.psd);
        if (tn.ply < best && isSolution(pos)) {
            printSolution(idx);
            best = tn.ply;
        }

        U64 blocked;
        if (!computeBlocked(pos, blocked))
            continue;

        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        for (int i = 0; i < moves.size; i++) {
            if ((1ULL << moves[i].from()) & blocked)
                continue;
            pos.makeMove(moves[i], ui);
            addPosition(pos, idx);
            pos.unMakeMove(moves[i], ui);
        }
    }
    double t1 = currentTime();
    std::cout << "nodes: " << numNodes
              << " time: " << t1 - t0 <<  std::endl;
}

void
PathSearch::addPosition(const Position& pos, U32 parent) {
    if (nodeHash.find(pos.zobristHash()) != nodeHash.end())
        return;

    TreeNode tn;
    pos.serialize(tn.psd);
    tn.parent = parent;
    tn.ply = (pos.getFullMoveCounter() - 1) * 2 + (pos.isWhiteMove() ? 0 : 1);
    int bound = distLowerBound(pos);
    if (bound < INT_MAX) {
        tn.bound = bound;
        U32 idx = nodes.size();
        nodes.push_back(tn);
        nodeHash.insert(pos.zobristHash());
        queue.push(idx);
    }
}

void
PathSearch::printSolution(int idx) const {
    std::function<void(int)> print = [this,&print](U32 idx) {
        const TreeNode& tn = nodes[idx];
        int ply = tn.ply;
        if (ply > 0)
            print(tn.parent);
        if (ply > 1)
            std::cout << ' ';
        if (ply > 0) {
            Position target;
            target.deSerialize(tn.psd);

            Position pos;
            pos.deSerialize(nodes[tn.parent].psd);
            MoveList moves;
            MoveGen::pseudoLegalMoves(pos, moves);
            MoveGen::removeIllegal(pos, moves);
            UndoInfo ui;
            for (int i = 0; i < moves.size; i++) {
                pos.makeMove(moves[i], ui);
                if (pos.equals(target)) {
                    pos.unMakeMove(moves[i], ui);
                    std::cout << TextIO::moveToString(pos, moves[i], false);
                    break;
                }
                pos.unMakeMove(moves[i], ui);
            }
        }
    };
    print(idx);
    std::cout << std::endl;
}

int
PathSearch::distLowerBound(const Position& pos) const {
    int pieceCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieceCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    // Make sure there are enough remaining pieces
    {
        int wProm = pieceCnt[Piece::WPAWN] - goalPieceCnt[Piece::WPAWN];
        if (wProm < 0)
            return INT_MAX;
        wProm -= std::max(0, goalPieceCnt[Piece::WQUEEN] - pieceCnt[Piece::WQUEEN]);
        if (wProm < 0)
            return INT_MAX;
        wProm -= std::max(0, goalPieceCnt[Piece::WROOK] - pieceCnt[Piece::WROOK]);
        if (wProm < 0)
            return INT_MAX;
        wProm -= std::max(0, goalPieceCnt[Piece::WBISHOP] - pieceCnt[Piece::WBISHOP]);
        if (wProm < 0)
            return INT_MAX;
        wProm -= std::max(0, goalPieceCnt[Piece::WKNIGHT] - pieceCnt[Piece::WKNIGHT]);
        if (wProm < 0)
            return INT_MAX;

        int bProm = pieceCnt[Piece::BPAWN] - goalPieceCnt[Piece::BPAWN];
        if (bProm < 0)
            return INT_MAX;
        bProm -= std::max(0, goalPieceCnt[Piece::BQUEEN] - pieceCnt[Piece::BQUEEN]);
        if (bProm < 0)
            return INT_MAX;
        bProm -= std::max(0, goalPieceCnt[Piece::BROOK] - pieceCnt[Piece::BROOK]);
        if (bProm < 0)
            return INT_MAX;
        bProm -= std::max(0, goalPieceCnt[Piece::BBISHOP] - pieceCnt[Piece::BBISHOP]);
        if (bProm < 0)
            return INT_MAX;
        bProm -= std::max(0, goalPieceCnt[Piece::BKNIGHT] - pieceCnt[Piece::BKNIGHT]);
        if (bProm < 0)
            return INT_MAX;
    }

    U64 blocked;
    if (!computeBlocked(pos, blocked))
        return INT_MAX;

    bool wCanPromote, bCanPromote; // True if promotion possible

    // Compute number of required captures to get pawns into correct files.
    // Check that excess material can satisfy both required captures and
    // required pawn promotions.
    {
        int numWhiteExtraPieces = (BitBoard::bitCount(pos.whiteBB()) -
                                   BitBoard::bitCount(goalPos.whiteBB()));
        int numBlackExtraPieces = (BitBoard::bitCount(pos.blackBB()) -
                                   BitBoard::bitCount(goalPos.blackBB()));
        const int bigCost = 1000;
        for (int c = 0; c < 2; c++) {
            const Piece::Type  p = (c == 0) ? Piece::WPAWN : Piece::BPAWN;
            Matrix<int> m(8, 8);
            U64 from = pos.pieceTypeBB(p);
            int fi = 0;
            while (from) {
                int fromSq = BitBoard::extractSquare(from);
                U64 to = goalPos.pieceTypeBB(p);
                int ti = 0;
                while (to) {
                    int toSq = BitBoard::extractSquare(to);
                    int d = std::abs(Position::getX(fromSq) - Position::getX(toSq));
                    m(fi, ti) = d;
                    ti++;
                }
                for ( ; ti < 8; ti++)
                    m(fi, ti) = 0;    // sq -> captured, no cost
                fi++;
            }
            for (; fi < 8; fi++) {
                int ti;
                for (ti = 0; ti < goalPieceCnt[p]; ti++)
                    m(fi, ti) = bigCost; // captured -> sq, not possible
                for ( ; ti < 8; ti++)
                    m(fi, ti) = 0;       // captured -> captured, no cost
            }

            Assignment<int> as(m);
            std::vector<int> s = as.optWeightMatch();
            int cost = 0;
            for (int i = 0; i < 8; i++)
                cost += m(i, s[i]);
            if (c == 0) {
                if (cost > numBlackExtraPieces)
                    return INT_MAX;

                int neededBCaptured = cost;
                int neededBProm = (std::max(0, goalPieceCnt[Piece::BQUEEN]  - pieceCnt[Piece::BQUEEN]) +
                                   std::max(0, goalPieceCnt[Piece::BROOK]   - pieceCnt[Piece::BROOK]) +
                                   std::max(0, goalPieceCnt[Piece::BBISHOP] - pieceCnt[Piece::BBISHOP]) +
                                   std::max(0, goalPieceCnt[Piece::BKNIGHT] - pieceCnt[Piece::BKNIGHT]));
                int excessBPawns = pieceCnt[Piece::BPAWN] - goalPieceCnt[Piece::BPAWN];
                int excessBPieces = (std::max(0, pieceCnt[Piece::BQUEEN]  - goalPieceCnt[Piece::BQUEEN]) +
                                     std::max(0, pieceCnt[Piece::BROOK]   - goalPieceCnt[Piece::BROOK]) +
                                     std::max(0, pieceCnt[Piece::BBISHOP] - goalPieceCnt[Piece::BBISHOP]) +
                                     std::max(0, pieceCnt[Piece::BKNIGHT] - goalPieceCnt[Piece::BKNIGHT]));
                if (neededBCaptured + neededBProm > excessBPawns + excessBPieces)
                    return INT_MAX;
                bCanPromote = (cost < numBlackExtraPieces) && (excessBPawns > 0);
            } else {
                if (cost > numWhiteExtraPieces)
                    return INT_MAX;

                int neededWCaptured = cost;
                int neededWProm = (std::max(0, goalPieceCnt[Piece::WQUEEN]  - pieceCnt[Piece::WQUEEN]) +
                                   std::max(0, goalPieceCnt[Piece::WROOK]   - pieceCnt[Piece::WROOK]) +
                                   std::max(0, goalPieceCnt[Piece::WBISHOP] - pieceCnt[Piece::WBISHOP]) +
                                   std::max(0, goalPieceCnt[Piece::WKNIGHT] - pieceCnt[Piece::WKNIGHT]));
                int excessWPawns = pieceCnt[Piece::WPAWN] - goalPieceCnt[Piece::WPAWN];
                int excessWPieces = (std::max(0, pieceCnt[Piece::WQUEEN]  - goalPieceCnt[Piece::WQUEEN]) +
                                     std::max(0, pieceCnt[Piece::WROOK]   - goalPieceCnt[Piece::WROOK]) +
                                     std::max(0, pieceCnt[Piece::WBISHOP] - goalPieceCnt[Piece::WBISHOP]) +
                                     std::max(0, pieceCnt[Piece::WKNIGHT] - goalPieceCnt[Piece::WKNIGHT]));

                if (neededWCaptured + neededWProm > excessWPawns + excessWPieces)
                    return INT_MAX;
                wCanPromote = (cost < numWhiteExtraPieces) && (excessWPawns > 0);
            }
        }
    }




    // For each piece type, compute minimum number of moves needed to move all
    // pieces from current to target positions.
    // * Use single source shortest path algorithm to compute distance between
    //   all source and target squares.
    // * Use hungarian algorithm to compute optimal matching.
    // * Set weight = 1000 for unreachable source/target combinations.
    // * If optimal matching has cost > 1000, there is no solution.

    // Note that shortest path can involve promoting an unneeded pawn
    // to the correct piece type and then move the promoted piece to
    // its target square.

    // Note that shortest path can involve castling moves.

    // Note that extra moves can be needed to get rid of unwanted castling flags.

    // B3k2B/1pppppp1/8/8/8/8/PPPP1PPP/RN1QK1NR w KQ - 0 1
    //   Unreachable, 2 promotions required

    // B3k2B/1pppppp1/8/8/8/8/PPP2PPP/RN1QK1NR w KQ - 0 1
    //   Should be reachable, 2 promotions required and 2 available.  6 captures
    //   needed to promote, but 7 available.

    // Bn1qk2B/1pppppp1/8/8/8/8/PPP2PPP/RN1QK1NR w KQ - 0 1
    //   Unreachable, 6 captures needed to promote. 7 captures available, but
    //   the 2 bishop captures can not help white to promote.

    // Nn1qk2B/1pppppp1/8/8/8/8/PPPP1PPP/RN1QK1NR w KQ - 0 1
    //   Unreachable, only 1 pawn promotion available, but need to promote to
    //   both knight (to satisfy piece counts) and bishop (existing bishops can
    //   not reach target square).

    // 4k3/1p6/2P5/3P4/4P1B1/3P4/2P2PP1/4K3 w - - 0 1
    //   If starting from this pos and goal pos has pawn on f3 and bishop on a4,
    //   all other pieces at current position, there is no solution.
    //   The f2 pawn must go to f3, so nothing can replace the e4 pawn if it moves.
    //   Therefore, e4, d5, c6 are all blocked and there is no way for the
    //   bishop to get to the other side of the pawn chain.

    return 0;
}

bool
PathSearch::computeBlocked(const Position& pos, U64& blocked) const {
    const U64 wGoalPawns = goalPos.pieceTypeBB(Piece::WPAWN);
    const U64 bGoalPawns = goalPos.pieceTypeBB(Piece::BPAWN);
    const U64 wCurrPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 bCurrPawns = pos.pieceTypeBB(Piece::BPAWN);

    U64 goalUnMovedPawns = (wGoalPawns | bGoalPawns) & (BitBoard::maskRow2 | BitBoard::maskRow7);
    U64 currUnMovedPawns = (wCurrPawns | bCurrPawns) & (BitBoard::maskRow2 | BitBoard::maskRow7);
    if (goalUnMovedPawns & ~currUnMovedPawns)
        return false;
    blocked = goalUnMovedPawns;

    U64 m = wGoalPawns & ~blocked;
    while (m) {
        int sq = BitBoard::extractSquare(m);
        U64 mask = bPawnReachable[sq];
        int nGoal = BitBoard::bitCount(wGoalPawns & mask);
        int nCurr = BitBoard::bitCount(wCurrPawns & mask);
        if (nCurr < nGoal)
            return false;
        if ((nCurr == nGoal) && (wCurrPawns & (1ULL << sq)))
            blocked |= (1ULL << sq);
    }

    m = bGoalPawns & ~blocked;
    while (m) {
        int sq = BitBoard::extractSquare(m);
        U64 mask = wPawnReachable[sq];
        int nGoal = BitBoard::bitCount(bGoalPawns & mask);
        int nCurr = BitBoard::bitCount(bCurrPawns & mask);
        if (nCurr < nGoal)
            return false;
        if ((nCurr == nGoal) && (bCurrPawns & (1ULL << sq)))
            blocked |= (1ULL << sq);
    }

    // Handle castling rights
    int cMask = goalPos.getCastleMask();
    if (cMask & ~pos.getCastleMask())
        return false;
    if (goalPos.h1Castle())
        blocked |= BitBoard::sqMask(E1,H1);
    if (goalPos.a1Castle())
        blocked |= BitBoard::sqMask(E1,A1);
    if (goalPos.h8Castle())
        blocked |= BitBoard::sqMask(E8,H8);
    if (goalPos.a8Castle())
        blocked |= BitBoard::sqMask(E8,A8);

    return true;
}

void
PathSearch::shortestPaths(Piece::Type p, int toSq, U64 blocked, ShortestPathData& spd) {
    for (int i = 0; i < 64; i++)
        spd.pathLen[i] = -1;
    spd.pathLen[toSq] = 0;
    U64 reached = 1ULL << toSq;
    int dist = 1;
    U64 newSquares = reached;
    while (true) {
        U64 neighbors = computeNeighbors(p, newSquares, blocked);
        newSquares = neighbors & ~reached;
        if (newSquares == 0)
            break;
        U64 m = newSquares;
        while (m) {
            int sq = BitBoard::extractSquare(m);
            spd.pathLen[sq] = dist;
        }
        reached |= newSquares;
        dist++;
    }
    spd.fromSquares = reached;
}

U64
PathSearch::computeNeighbors(Piece::Type p, U64 toSquares, U64 blocked) {
    U64 ret = 0;
    switch (p) {
    case Piece::WKING: case Piece::BKING:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::kingAttacks[sq];
        }
        break;
    case Piece::WQUEEN: case Piece::BQUEEN:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::rookAttacks(sq, blocked);
            ret |= BitBoard::bishopAttacks(sq, blocked);
        }
        break;
    case Piece::WROOK: case Piece::BROOK:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::rookAttacks(sq, blocked);
        }
        break;
    case Piece::WBISHOP: case Piece::BBISHOP:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::bishopAttacks(sq, blocked);
        }
        break;
    case Piece::WKNIGHT: case Piece::BKNIGHT:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::knightAttacks[sq];
        }
        break;
    case Piece::WPAWN: {
        U64 tmp = (toSquares >> 8) & ~blocked;
        ret |= tmp;
        ret |= (tmp & BitBoard::maskRow3) >> 8;
        ret |= BitBoard::bPawnAttacksMask(toSquares);
        break;
    }
    case Piece::BPAWN: {
        U64 tmp = (toSquares << 8) & ~blocked;
        ret |= tmp;
        ret |= (tmp & BitBoard::maskRow6) << 8;
        ret |= BitBoard::wPawnAttacksMask(toSquares);
        break;
    }
    default:
        assert(false);
    }
    return ret & ~blocked;
}
