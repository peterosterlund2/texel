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


std::shared_ptr<NNEvaluator>
NNEvaluator::create(const NetData& netData) {
    NNEvaluator* mem = AlignedAllocator<NNEvaluator>().allocate(1);
    return std::shared_ptr<NNEvaluator>(new (mem) NNEvaluator(netData),
                                        [](NNEvaluator* p) {
        AlignedAllocator<NNEvaluator>().deallocate(p, 1);
    });
}

NNEvaluator::NNEvaluator(const NetData& netData)
    : layer2(netData.lin2),
      layer3(netData.lin3),
      layer4(netData.lin4),
      netData(netData) {
    static_assert(sizeof(FirstLayerState) % 32 == 0, "Bad alignment");
}

void
NNEvaluator::connectPosition(const Position& pos) {
    posP = &pos;
    forceFullEval();
}

void
NNEvaluator::forceFullEval() {
    for (int c = 0; c < 2; c++) {
        FirstLayerState& s = linState[c];
        s.kingSqComputed = -1;
        s.toAddLen = 0;
        s.toSubLen = 0;
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
        FirstLayerState& s = linState[c];
        int kSq = s.kingSqComputed;
        if (kSq == -1)
            continue;

        if (isNonKing(oldPiece)) {
            int pt = ptValue[oldPiece];
            int idx = getIndex(kSq, pt, square, c == 0);
            if (s.toAddLen > 0 && s.toAdd[s.toAddLen-1] == idx)
                s.toAddLen--;
            else if (s.toSubLen < maxIncr)
                s.toSub[s.toSubLen++] = idx;
            else {
                s.kingSqComputed = -1;
                s.toAddLen = 0;
                s.toSubLen = 0;
                continue;
            }
        }

        if (isNonKing(newPiece)) {
            int pt = ptValue[newPiece];
            int idx = getIndex(kSq, pt, square, c == 0);
            if (s.toSubLen > 0 && s.toSub[s.toSubLen-1] == idx)
                s.toSubLen--;
            else if (s.toAddLen < maxIncr)
                s.toAdd[s.toAddLen++] = idx;
            else {
                s.kingSqComputed = -1;
                s.toAddLen = 0;
                s.toSubLen = 0;
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
        FirstLayerState& s = linState[c];
        doFull[c] = s.kingSqComputed != kingSq[c];
        if (!doFull[c])
            addSubWeights(s.l1Out, netData.weight1, s.toAdd, s.toAddLen, s.toSub, s.toSubLen);
        s.toAddLen = 0;
        s.toSubLen = 0;
    }

    if (!doFull[0] && !doFull[1])
        return;

    for (int c = 0; c < 2; c++) {
        if (doFull[c]) {
            FirstLayerState& s = linState[c];
            s.l1Out = netData.bias1;
            s.kingSqComputed = posP->getKingSq(c == 0);
        }
    }

    int add[2][32];
    int len[2] = {0, 0};
    for (int sq = 0; sq < 64; sq++) {
        const int p = posP->getPiece(sq);
        if (p == Piece::EMPTY || p == Piece::WKING || p == Piece::BKING)
            continue;
        int pt = ptValue[p];
        for (int c = 0; c < 2; c++) {
            if (doFull[c]) {
                FirstLayerState& s = linState[c];
                add[c][len[c]++] = getIndex(s.kingSqComputed, pt, sq, c == 0);
            }
        }
    }
    for (int c = 0; c < 2; c++) {
        if (doFull[c]) {
            FirstLayerState& s = linState[c];
            addSubWeights(s.l1Out, netData.weight1, add[c], len[c], add[c], 0);
        }
    }
}

void
NNEvaluator::computeL1Out() {
    bool wtm = posP->isWhiteMove();
    for (int c = 0; c < 2; c++) {
        const Vector<S16, n1>& l1OutC = linState[wtm ? c : (1-c)].l1Out;
        scaleClipPack(&l1OutClipped(c * n1), l1OutC);
    }
}

int
NNEvaluator::eval() {
    computeL1WB();
    computeL1Out();

    layer2.forward(l1OutClipped, layer2Out);
    layer3.forward(layer2Out.output, layer3Out);
    layer4.evalLinear(layer3Out.output, layer4Out);

    return layer4Out.linOutput(0) * 100 / (127 * 64);
}
