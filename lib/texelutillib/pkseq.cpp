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
 * pkseq.cpp
 *
 *  Created on: Feb 5, 2022
 *      Author: petero
 */

#include "pkseq.hpp"
#include "proofgame.hpp"
#include "stloutput.hpp"
#include <cassert>
#include <limits.h>

using PieceColor = ProofKernel::PieceColor;
using PieceType = ProofKernel::PieceType;

PkSequence::PkSequence(const std::vector<ExtPkMove>& extKernel,
                       const Position& initPos, const Position& goalPos,
                       std::ostream& log)
    : extKernel(extKernel), initPos(initPos), goalPos(goalPos), log(log) {
}

void
PkSequence::improve() {
    if (extKernel.empty())
        return;

    splitPawnMoves();


    std::vector<MoveData> moves;
    for (const ExtPkMove& m : extKernel) {
        MoveData md(m);
        md.squares = 0;
        if (m.movingPiece == PieceType::PAWN)
            md.squares = (1ULL << m.fromSquare) | (1ULL << m.toSquare);
        moves.push_back(md);
    }

    expandPieceMoves(moves);

    // After each move, test if the king is in check.
    // - If so, generate king evasion move.
    // - If king has nowhere to go, generate move to make space for the king
    //   before the checking move.

    // At end of proof kernel position, if some piece cannot reach its target
    // position, add an ExtPkMove for this piece movement. Then:
    // - Try to add a pawn move before the piece move to make the piece move
    //   possible.
    //   - If this does not work, try to move the piece move earlier in the move
    //      sequence where whatever was blocking its path is not yet in a
    //      blocking position.

    // If a rook cannot reach its target square, check if the other rook can
    // reach it instead.
    // - If so, swap which rook is moved.
    // - If there later is a move of the other rook, that move must be swapped
    //   too.

    extKernel.clear();
    for (const MoveData& md : moves)
        extKernel.push_back(md.move);

    combinePawnMoves();
}

static bool
isNonCapturePawnMove(const ProofKernel::ExtPkMove& m) {
    if (m.movingPiece == PieceType::PAWN &&
        Square::getX(m.fromSquare) == Square::getX(m.toSquare)) {
        assert(!m.capture);
        return true;
    }
    return false;
}

void
PkSequence::splitPawnMoves() {
    std::vector<ExtPkMove> seq;
    for (const ExtPkMove& m : extKernel) {
        if (isNonCapturePawnMove(m)) {
            int x = Square::getX(m.fromSquare);
            int y1 = Square::getY(m.fromSquare);
            int y2 = Square::getY(m.toSquare);
            int d = y1 < y2 ? 1 : -1;
            for (int y = y1 + d; y != y2 + d; y += d) {
                ExtPkMove tmpM(m);
                tmpM.fromSquare = Square::getSquare(x, y1);
                tmpM.toSquare = Square::getSquare(x, y);
                if (y != y2)
                    tmpM.promotedPiece = PieceType::EMPTY;
                seq.push_back(tmpM);
                y1 = y;
            }
        } else {
            seq.push_back(m);
        }
    }
    extKernel = seq;
}

void
PkSequence::combinePawnMoves() {
    std::vector<ExtPkMove> seq;
    for (const ExtPkMove& m : extKernel) {
        bool merged = false;
        if (!seq.empty() && isNonCapturePawnMove(m) && isNonCapturePawnMove(seq.back())) {
            const ExtPkMove& m0 = seq.back();
            int x = Square::getX(m.fromSquare);
            if (x == Square::getX(m0.fromSquare) &&
                Square::getY(m0.toSquare) == Square::getY(m.fromSquare)) {
                int y0 = Square::getY(m0.fromSquare);
                int y1 = Square::getY(m.toSquare);
                bool white = (m.color == PieceColor::WHITE);
                if (y0 == (white ? 1 : 6) && y1 == (white ? 3 : 4)) {
                    seq.back() = m;
                    seq.back().fromSquare = Square::getSquare(x, y0);
                    merged = true;
                }
            }
        }
        if (!merged)
            seq.push_back(m);
    }
    extKernel = seq;
}

void
PkSequence::expandPieceMoves(std::vector<MoveData>& moves) {
    Position currPos(initPos);
    int nPlayed = 0;

    auto makeMove = [&moves,&currPos,&nPlayed](const Move& m) {
        MoveData& md = moves[nPlayed];
        md.m = m;
        UndoInfo ui;
        currPos.makeMove(m, ui);
        currPos.setWhiteMove(true);
        nPlayed++;
    };

    while (nPlayed < (int)moves.size()) {
        MoveData& md = moves[nPlayed];
        md.pos = currPos;
        bool white = md.move.color == PieceColor::WHITE;

        if (md.move.movingPiece == PieceType::EMPTY)
            assignPiece(currPos, moves, nPlayed);

        if (md.move.movingPiece == PieceType::EMPTY) {
            assert(md.move.capture);
            nPlayed++;
        } else if (md.move.movingPiece == PieceType::PAWN) {
            int promoteTo = Piece::EMPTY;
            if (md.move.promotedPiece != PieceType::EMPTY)
                promoteTo = ProofKernel::toPieceType(white, md.move.promotedPiece, false);
            makeMove(Move(md.move.fromSquare, md.move.toSquare, promoteTo));
        } else {
            std::vector<ExtPkMove> expanded;
            expandPieceMove(md.pos, md.move, expanded);
            moves.erase(moves.begin() + nPlayed);
            moves.insert(moves.begin() + nPlayed, expanded.begin(), expanded.end());
            for (int i = 0; i < (int)expanded.size(); i++) {
                MoveData& md = moves[nPlayed];
                makeMove(Move(md.move.fromSquare, md.move.toSquare, Piece::EMPTY));
            }
        }
    }
}

void
PkSequence::assignPiece(const Position& pos, std::vector<MoveData>& moves,
                        int moveIdx) const {
    ExtPkMove& move = moves[moveIdx].move;
    bool whiteMoving = !Piece::isWhite(pos.getPiece(move.toSquare));
    U64 candidates = whiteMoving ? pos.whiteBB() : pos.blackBB();
    candidates &= ~pos.pieceTypeBB(whiteMoving ? Piece::WPAWN : Piece::BPAWN);
    candidates &= ~pos.pieceTypeBB(whiteMoving ? Piece::WKING : Piece::BKING);
    if (pos.a1Castle()) candidates &= ~(1ULL << A1);
    if (pos.h1Castle()) candidates &= ~(1ULL << H1);
    if (pos.a8Castle()) candidates &= ~(1ULL << A8);
    if (pos.h8Castle()) candidates &= ~(1ULL << H8);

    int bestDist = INT_MAX;
    ProofGame::ShortestPathData spd;
    while (candidates != 0) {
        int sq = BitBoard::extractSquare(candidates);
        Piece::Type p = (Piece::Type)pos.getPiece(sq);

        U64 blocked = pos.occupiedBB() & ~(1ULL << sq) & ~(1ULL << move.toSquare);
        ProofGame::shortestPaths(p, move.toSquare, blocked, nullptr, spd);
        int dist = spd.pathLen[sq];
        if (dist > 0 && dist < bestDist) {
            PieceType pt = PieceType::EMPTY;
            switch (p) {
            case Piece::WQUEEN: case Piece::BQUEEN:
                pt = PieceType::QUEEN;
                break;
            case Piece::WROOK: case Piece::BROOK:
                pt = PieceType::ROOK;
                break;
            case Piece::WBISHOP: case Piece::BBISHOP:
                pt = Square::darkSquare(sq) ? PieceType::DARK_BISHOP : PieceType::LIGHT_BISHOP;
                break;
            case Piece::WKNIGHT: case Piece::BKNIGHT:
                pt = PieceType::KNIGHT;
                break;
            default:
                assert(false);
            }
            move.movingPiece = pt;
            move.fromSquare = sq;
            bestDist = dist;
        }
    }

    if (bestDist < INT_MAX) { // Update next move of the same piece
        for (int i = moveIdx + 1; i < (int)moves.size(); i++) {
            ExtPkMove& m = moves[i].move;
            if (m.color == move.color &&
                m.movingPiece == move.movingPiece &&
                m.fromSquare == move.fromSquare) {
                m.fromSquare = move.toSquare;
                break;
            }
        }
    }
}

void
PkSequence::expandPieceMove(const Position& pos, const ExtPkMove& move,
                            std::vector<ExtPkMove>& moves) const {
    moves.clear();

    bool white = move.color == PieceColor::WHITE;
    Piece::Type p = ProofKernel::toPieceType(white, move.movingPiece, false);

    ProofGame::ShortestPathData spd;
    U64 blocked = pos.occupiedBB() & ~(1ULL << move.toSquare) & ~(1ULL << move.fromSquare);
    ProofGame::shortestPaths(p, move.toSquare, blocked, nullptr, spd);
    if (spd.pathLen[move.fromSquare] >= 0) {
        int fromSq = move.fromSquare;
        int toSq = move.toSquare;
        while (fromSq != toSq) {
            U64 nextMask = spd.getNextSquares(p, fromSq, blocked);
            assert(nextMask != 0);
            int nextSq = BitBoard::firstSquare(nextMask);

            ExtPkMove m(move);
            m.fromSquare = fromSq;
            m.toSquare = nextSq;
            if (nextSq != toSq)
                m.capture = false;
            moves.push_back(m);

            fromSq = nextSq;
        }
        return;
    }

    moves.push_back(move);

    // For a non-pawn move, decide if it is possible to make the move without
    // moving any other pieces.
    // - If other non-pawn piece needs to move, insert ExtPkMove to move it.
    //   - If target square conflicts with later move, also generate ExtPkMove
    //     to move the piece back to where it was.
    // - If other pawn needs to move, check if a later move of that pawn can be
    //   moved earlier in the sequence. (add constraint + topological sort)
    //   - If not, check if extra pawn move can be added without conflicting with
    //     existing pawn moves and without making it impossible to reach pawn
    //     goal square.
}
