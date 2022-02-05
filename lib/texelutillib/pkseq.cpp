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
#include "stloutput.hpp"
#include <cassert>

using PieceColor = ProofKernel::PieceColor;
using PieceType = ProofKernel::PieceType;

PkSequence::PkSequence(std::vector<ExtPkMove>& extKernel,
                       const Position& initPos, const Position& goalPos,
                       std::ostream& log)
    : extKernel(extKernel), initPos(initPos), goalPos(goalPos), log(log) {
}

void
PkSequence::improve() {
    splitPawnMoves();

    // For each pawn move, compute bitmask of affected squares, ie all squares
    // between fromSq and toSq.

    // For each non-pawn move, decide if it is possible to make the move without
    // moving any other pieces.
    // - If other non-pawn piece needs to move, insert ExtPkMove to move it.
    //   - If target square conflicts with later move, also generate ExtPkMove
    //     to move the piece back to where it was.
    // - If other pawn needs to move, check if a later move of that pawn can be
    //   moved earlier in the sequence. (add constraint + topological sort)
    //   - If not, check if extra pawn move can be added without conflicting with
    //     existing pawn moves and without making it impossible to reach pawn
    //     goal square.
    // - Split the non-pawn move in several parts, using shortest path data.

    // At end of proof kernel position, if some piece cannot reach its target
    // position, add an ExtPkMove for this piece movement. Then:
    // - Try to add a pawn move before the piece move to make the piece move
    //   possible.
    //   - If this does not work, try to move the piece move earlier in the move
    //      sequence where whatever was blocking its path is not yet in a
    //      blocking position.

    // After each move, test if the king is in check.
    // - If so, generate king evasion move.
    // - If king has nowhere to go, generate move to make space for the king
    //   before the checking move.

    // If a rook cannot reach its target square, check if the other rook can
    // reach it instead.
    // - If so, swap which rook is moved.
    // - If there later is a move of the other rook, that move must be swapped
    //   too.

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
