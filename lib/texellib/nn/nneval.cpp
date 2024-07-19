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
#include <cassert>

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
        p->~NNEvaluator();
        AlignedAllocator<NNEvaluator>().deallocate(p, 1);
    });
}

NNEvaluator::NNEvaluator(const NetData& netData)
    : head{ netData.head[0], netData.head[1], netData.head[2], netData.head[3] },
      netData(netData) {
    static_assert(NetData::nHeads == 4, "nHeads does not match constructor arguments");
    static_assert(sizeof(FirstLayerState) % 64 == 0, "Bad alignment");
    static_assert(sizeof(FirstLayerStack) % 64 == 0, "Bad alignment");
}

NNEvaluator::~NNEvaluator() {
    connectPosition(nullptr);
}

NNEvaluator::Head::Head(const NetData::Head& headData)
    : layer2(headData.lin2),
      layer3(headData.lin3),
      layer4(headData.lin4) {
}

void
NNEvaluator::connectPosition(const Position* pos) {
    const Position* oldPos = posP;
    if (pos == oldPos)
        return;

    posP = nullptr;
    if (oldPos)
        oldPos->connectNNEval(nullptr);
    posP = pos;
    if (pos)
        pos->connectNNEval(this);

    forceFullEval();
}

void
NNEvaluator::forceFullEval(bool clearStack) {
    if (clearStack)
        stack.stackTop = 0;
    for (int c = 0; c < 2; c++)
        getLinState(c).clear();
}

void
NNEvaluator::pushState() {
    for (int c = 0; c < 2; c++) {
        FirstLayerState& fls = getLinState(c);
        if (fls.toAddLen + fls.toSubLen > 0)
            computeL1WB();
    }

    stack.stackTop++;
    assert(stack.stackTop < maxStackSize);
    for (int c = 0; c < 2; c++)
        stack.flState[stack.stackTop][c] = stack.flState[stack.stackTop-1][c];
}

void
NNEvaluator::popState() {
    if (stack.stackTop > 0) {
        stack.stackTop--;
    } else {
        forceFullEval();
    }
}

/** Return the row in the first layer weight matrix corresponding
 *  to king + piece type + square. */
static inline int
getIndex(Square kSq, int pt, Square sq, bool white) {
    if (!white) {
        kSq = kSq.mirrorY();
        pt = (pt >= 5) ? (pt - 5) : (pt + 5);
        sq = sq.mirrorY();
    }
    int x = kSq.getX();
    int y = kSq.getY();
    if (x >= 4) {
        x ^= 7;
        sq = sq.mirrorX();
    }
    int kIdx = y * 4 + x;
    return (kIdx * 10 + pt) * 64 + sq.asInt();
};

void
NNEvaluator::setPiece(Square square, int oldPiece, int newPiece) {
    auto isNonKing = [](int p) {
        return p != Piece::EMPTY && p != Piece::WKING && p != Piece::BKING;
    };

    for (int c = 0; c < 2; c++) {
        FirstLayerState& s = getLinState(c);
        Square kSq = s.kingSqComputed;
        if (!kSq.isValid())
            continue;

        if (isNonKing(oldPiece)) {
            int pt = ptValue[oldPiece];
            int idx = getIndex(kSq, pt, square, c == 0);
            if (s.toSubLen < maxIncr) {
                s.toSub[s.toSubLen++] = idx;
            } else {
                s.kingSqComputed = Square();
                s.toAddLen = 0;
                s.toSubLen = 0;
                continue;
            }
        }

        if (isNonKing(newPiece)) {
            int pt = ptValue[newPiece];
            int idx = getIndex(kSq, pt, square, c == 0);
            if (s.toAddLen < maxIncr) {
                s.toAdd[s.toAddLen++] = idx;
            } else {
                s.kingSqComputed = Square();
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
    Square kingSq[2];
    kingSq[0] = posP->getKingSq(true);
    kingSq[1] = posP->getKingSq(false);
    for (int c = 0; c < 2; c++) {
        FirstLayerState& s = getLinState(c);
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
            FirstLayerState& s = getLinState(c);
            copyVec(s.l1Out, netData.bias1);
            s.kingSqComputed = posP->getKingSq(c == 0);
        }
    }

    int add[2][32];
    int len[2] = {0, 0};
    U64 squares = posP->occupiedBB() & ~posP->pieceTypeBB(Piece::WKING, Piece::BKING);
    while (squares) {
        Square sq = BitBoard::extractSquare(squares);
        const int p = posP->getPiece(sq);
        int pt = ptValue[p];
        for (int c = 0; c < 2; c++) {
            if (doFull[c]) {
                FirstLayerState& s = getLinState(c);
                add[c][len[c]++] = getIndex(s.kingSqComputed, pt, sq, c == 0);
            }
        }
    }
    for (int c = 0; c < 2; c++) {
        if (doFull[c]) {
            FirstLayerState& s = getLinState(c);
            addSubWeights(s.l1Out, netData.weight1, add[c], len[c], add[c], 0);
        }
    }
}

void
NNEvaluator::computeL1Out() {
    bool wtm = posP->isWhiteMove();
    for (int c = 0; c < 2; c++) {
        const Vector<S16, n1>& l1OutC = getLinState(wtm ? c : (1-c)).l1Out;
        scaleClipPack<NetData::l1Shift>(&l1OutClipped(c * n1), l1OutC);
    }
}

int
NNEvaluator::eval() {
    computeL1WB();
    computeL1Out();

    const int nPieces = posP->nPieces();
    int hi = NetData::getHeadNo(nPieces);

    HeadOut& ho = out[hi];
    Head& h = head[hi];

    h.layer2.forward(l1OutClipped, ho.layer2);
    h.layer3.forward(ho.layer2.output, ho.layer3);
    h.layer4.evalLinear(ho.layer3.output, ho.layer4);

    return ho.layer4.linOutput(0) * (100 * 2) / (127 * 64);
}
