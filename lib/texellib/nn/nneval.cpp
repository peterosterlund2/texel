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
 * nneval.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: petero
 */

#include "nneval.hpp"
#include "vectorop.hpp"
#include "position.hpp"

int NNEvaluator::ptValue[Piece::nPieceTypes];

static StaticInitializer<NNEvaluator> nnInit;

void
NNEvaluator::staticInitialize() {
    ptValue[Piece::WQUEEN]  = 0;
    ptValue[Piece::WROOK]   = 1;
    ptValue[Piece::WBISHOP] = 2;
    ptValue[Piece::WKNIGHT] = 3;
    ptValue[Piece::WPAWN]   = 4;

    ptValue[Piece::BQUEEN]  = 5;
    ptValue[Piece::BROOK]   = 6;
    ptValue[Piece::BBISHOP] = 7;
    ptValue[Piece::BKNIGHT] = 8;
    ptValue[Piece::BPAWN]   = 9;
}


NNEvaluator::NNEvaluator(const NetData& netData)
    : lin2(netData.lin2),
      lin3(netData.lin3),
      lin4(netData.lin4),
      netData(netData) {
}

void
NNEvaluator::connectPosition(const Position& pos) {
    posP = &pos;
    forceFullEval();
}

void
NNEvaluator::forceFullEval() {
    for (int c = 0; c < 2; c++) {
        kingSqComputed[c] = -1;
        toAddLen[c] = 0;
        toSubLen[c] = 0;
    }
}

/** Return the row in the first layer weight matrix corresponding
 *  to king + piece type + square. */
static inline int
getIndex(int kSq, int pt, int sq, bool white) {
    if (!white) {
        kSq = Square::mirrorY(kSq);
        pt = (pt >= 5) ? (pt - 5) : (pt + 5);
        sq = Square::mirrorY(sq);
    }
    int x = Square::getX(kSq);
    int y = Square::getY(kSq);
    if (x >= 4) {
        x = Square::mirrorX(x);
        sq = Square::mirrorX(sq);
    }
    int kIdx = y * 4 + x;
    return (kIdx * 10 + pt) * 64 + sq;
};

void
NNEvaluator::setPiece(int square, int oldPiece, int newPiece) {
    auto isNonKing = [](int p) {
        return p != Piece::EMPTY && p != Piece::WKING && p != Piece::BKING;
    };

    for (int c = 0; c < 2; c++) {
        int kSq = kingSqComputed[c];
        if (kSq == -1)
            continue;

        if (isNonKing(oldPiece)) {
            int pt = ptValue[oldPiece];
            int idx = getIndex(kSq, pt, square, c == 0);
            if (toAddLen[c] > 0 && toAdd[c][toAddLen[c]-1] == idx)
                toAddLen[c]--;
            else if (toSubLen[c] < maxIncr)
                toSub[c][toSubLen[c]++] = idx;
            else {
                kingSqComputed[c] = -1;
                toAddLen[c] = 0;
                toSubLen[c] = 0;
                continue;
            }
        }

        if (isNonKing(newPiece)) {
            int pt = ptValue[newPiece];
            int idx = getIndex(kSq, pt, square, c == 0);
            if (toSubLen[c] > 0 && toSub[c][toSubLen[c]-1] == idx)
                toSubLen[c]--;
            else if (toAddLen[c] < maxIncr)
                toAdd[c][toAddLen[c]++] = idx;
            else {
                kingSqComputed[c] = -1;
                toAddLen[c] = 0;
                toSubLen[c] = 0;
                continue;
            }
        }
    }
}

void
NNEvaluator::computeL1WB() {
    bool doFull[2];
    int kingSq[2];
    kingSq[0] = posP->getKingSq(true);
    kingSq[1] = posP->getKingSq(false);
    for (int c = 0; c < 2; c++) {
        doFull[c] = kingSqComputed[c] != kingSq[c];
        if (!doFull[c]) {
            for (int k = 0; k < toAddLen[c]; k++) {
                int idx = toAdd[c][k];
                for (int i = 0; i < n1; i++)
                    l1Out[c](i) += netData.weight1(idx, i);
            }
            for (int k = 0; k < toSubLen[c]; k++) {
                int idx = toSub[c][k];
                for (int i = 0; i < n1; i++)
                    l1Out[c](i) -= netData.weight1(idx, i);
            }
        }
        toAddLen[c] = 0;
        toSubLen[c] = 0;
    }

    if (!doFull[0] && !doFull[1])
        return;

    for (int c = 0; c < 2; c++) {
        if (doFull[c]) {
            for (int i = 0; i < n1; i++)
                l1Out[c](i) = netData.bias1(i);
            kingSqComputed[c] = posP->getKingSq(c == 0);
        }
    }

    for (int sq = 0; sq < 64; sq++) {
        const int p = posP->getPiece(sq);
        if (p == Piece::EMPTY || p == Piece::WKING || p == Piece::BKING)
            continue;
        int pt = ptValue[p];
        if (doFull[0]) {
            int idx = getIndex(kingSqComputed[0], pt, sq, true);
            for (int i = 0; i < n1; i++)
                l1Out[0](i) += netData.weight1(idx, i);
        }
        if (doFull[1]) {
            int idx = getIndex(kingSqComputed[1], pt, sq, false);
            for (int i = 0; i < n1; i++)
                l1Out[1](i) += netData.weight1(idx, i);
        }
    }
}

void
NNEvaluator::computeL1Out() {
    bool wtm = posP->isWhiteMove();
    for (int c = 0; c < 2; c++) {
        const auto& l1OutC = l1Out[wtm ? c : (1-c)];
        int i0 = c * n1;
        for (int i = 0; i < n1; i++)
            l1OutClipped(i0 + i) = clamp(l1OutC(i) >> 2, 0, 127);
    }
}

int
NNEvaluator::eval() {
    computeL1WB();
    computeL1Out();

    lin2.forward(l1OutClipped);
    lin3.forward(lin2.output);
    lin4.evalLinear(lin3.output);

    return lin4.linOutput(0) * 100 / (127 * 64);
}
