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
    : wtm(true), wKingSq(-1), bKingSq(-1),
      netData(netData),
      lin2(netData.lin2),
      lin3(netData.lin3),
      lin4(netData.lin4) {
    for (int sq = 0; sq < 64; sq++)
        squares[sq] = Piece::EMPTY;
}

void
NNEvaluator::setPos(const Position& pos) {
    for (int sq = 0; sq < 64; sq++)
        squares[sq] = pos.getPiece(sq);
    wtm = pos.isWhiteMove();
    wKingSq = pos.getKingSq(true);
    bKingSq = pos.getKingSq(false);
}

void
NNEvaluator::makeMove(const Move& move) {

}

void
NNEvaluator::unMakeMove(const Move& move, const UndoInfo& ui) {

}

void
NNEvaluator::computeL1WB() {
    for (int i = 0; i < n1; i++) {
        l1OutW(i) = netData.bias1(i);
        l1OutB(i) = netData.bias1(i);
    }

    auto getIndex = [](int kSq, int pt, int sq) {
        int kIdx;
        int x = Square::getX(kSq);
        int y = Square::getY(kSq);
        if (x >= 4) {
            x = Square::mirrorX(x);
            sq = Square::mirrorX(sq);
        }
        kIdx = y * 4 + x;
        return (kIdx * 10 + pt) * 64 + sq;
    };

    for (int sq = 0; sq < 64; sq++) {
        const int p = squares[sq];
        if (p == Piece::EMPTY || p == Piece::WKING || p == Piece::BKING)
            continue;
        int pt = ptValue[p];
        {
            int idx = getIndex(wKingSq, pt, sq);
            for (int i = 0; i < n1; i++)
                l1OutW(i) += netData.weight1(idx, i);
        }
        {
            int oKSq = Square::mirrorY(bKingSq);
            int oPt = (pt + 5) % 10;
            int oSq = Square::mirrorY(sq);
            int idx = getIndex(oKSq, oPt, oSq);
            for (int i = 0; i < n1; i++)
                l1OutB(i) += netData.weight1(idx, i);
        }
    }
}

void
NNEvaluator::computeL1Out() {
    for (int c = 0; c < 2; c++) {
        const auto& l1OutC = (wtm == (c == 0)) ? l1OutW : l1OutB;
        int i0 = c * n1;
        for (int i = 0; i < n1; i++)
            l1Out(i0 + i) = clamp(l1OutC(i) >> 2, 0, 127);
    }
}

int
NNEvaluator::eval() {
    computeL1WB();
    computeL1Out();

    lin2.forward(l1Out);
    lin3.forward(lin2.output);
    lin4.evalLinear(lin3.output);

    return lin4.linOutput(0) * 100 / (127 * 64);
}
