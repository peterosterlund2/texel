/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * evaluate.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "evaluate.hpp"
#include "endGameEval.hpp"
#include "constants.hpp"
#include "parameters.hpp"
#include "chessError.hpp"
#include "incbin.h"
#include <vector>

extern "C" {
#include "Lzma86Dec.h"
}

int Evaluate::pieceValueOrder[Piece::nPieceTypes] = {
    0,
    5, 4, 3, 2, 2, 1,
    5, 4, 3, 2, 2, 1
};

INCBIN_EXTERN(NNData);
// const unsigned char* gNNDataData;
// const unsigned int gNNDataSize;

Evaluate::Evaluate(EvalHashTables& et)
    : materialHash(et.materialHash),
      mhd(nullptr),
      evalHash(et.evalHash),
      nnEval(*et.nnEval),
      whiteContempt(0) {
}

void
Evaluate::connectPosition(const Position& pos) {
    posP = &pos;
    nnEval.connectPosition(&pos);
}

int
Evaluate::evalPos() {
    return evalPos<false>();
}

int
Evaluate::evalPosPrint() {
    return evalPos<true>();
}

template <bool print>
inline int
Evaluate::evalPos() {
    const bool useHashTable = !print;
    EvalHashData* ehd = nullptr;
    U64 key = posP->historyHash();
    if (useHashTable) {
        ehd = &getEvalHashEntry(key);
        if ((ehd->data ^ key) < (1 << 16))
            return (ehd->data & 0xffff) - (1 << 15);
    }

    int score = nnEval.eval();
    if (!posP->isWhiteMove())
        score = -score;
    if (print) std::cout << "info string eval nn      :" << score << std::endl;

    score += materialScore(print);
    if (print) std::cout << "info string eval mtrl    :" << score << std::endl;

    if (mhd->endGame)
        score = EndGameEval::endGameEval<true>(*posP, score);
    if (print) std::cout << "info string eval endgame :" << score << std::endl;

    if ((whiteContempt != 0) && !mhd->endGame) {
        int mtrlPawns = posP->wMtrlPawns() + posP->bMtrlPawns();
        int mtrl = posP->wMtrl() + posP->bMtrl();
        int hiMtrl = (rV + bV*2 + nV*2) * 2;
        int piecePlay = interpolate(mtrl - mtrlPawns, 0, 64, hiMtrl, 128);
        score += whiteContempt * piecePlay / 128;
        if (print) std::cout << "info string eval contempt:" << score << ' ' << piecePlay << std::endl;
    }

    if (posP->pieceTypeBB(Piece::WPAWN, Piece::BPAWN)) {
        int hmc = clamp(posP->getHalfMoveClock() / 10, 0, 9);
        score = score * halfMoveFactor[hmc] / 128;
    }
    if (print) std::cout << "info string eval halfmove:" << score << std::endl;

    if (!posP->isWhiteMove())
        score = -score;

    if (useHashTable)
        ehd->data = (key & 0xffffffffffff0000ULL) + (score + (1 << 15));

    return score;
}

/** Compensate for the fact that many knights are stronger compared to queens
 * than what the default material scores would predict. */
static inline int correctionNvsQ(int n, int q) {
    if (n <= q+1)
        return 0;
    int knightBonus = 0;
    if (q == 1)
        knightBonus = knightVsQueenBonus1;
    else if (q == 2)
        knightBonus = knightVsQueenBonus2;
    else if (q >= 3)
        knightBonus = knightVsQueenBonus3;
    int corr = knightBonus * (n - q - 1);
    return corr;
}

void
Evaluate::computeMaterialScore(MaterialHashData& mhd, bool print) const {
    int score = 0;

    const int nWQ = BitBoard::bitCount(posP->pieceTypeBB(Piece::WQUEEN));
    const int nBQ = BitBoard::bitCount(posP->pieceTypeBB(Piece::BQUEEN));
    const int nWN = BitBoard::bitCount(posP->pieceTypeBB(Piece::WKNIGHT));
    const int nBN = BitBoard::bitCount(posP->pieceTypeBB(Piece::BKNIGHT));
    int wCorr = correctionNvsQ(nWN, nBQ);
    int bCorr = correctionNvsQ(nBN, nWQ);
    score += wCorr - bCorr;

    mhd.id = posP->materialId();
    mhd.score = score;
    mhd.endGame = EndGameEval::endGameEval<false>(*posP, 0);
}

std::unique_ptr<Evaluate::EvalHashTables>
Evaluate::getEvalHashTables() {
    return std::make_unique<EvalHashTables>();
}

Evaluate::EvalHashTables::EvalHashTables() {
    materialHash.resize(1 << 14);
    nnEval = NNEvaluator::create(initNetData());
}

const NetData&
Evaluate::EvalHashTables::initNetData() {
    static std::shared_ptr<NetData> staticNetData = []() {
        std::shared_ptr<NetData> netData = NetData::create();
        size_t unCompressedSize = netData->computeSize();
        std::vector<unsigned char> unComprData(unCompressedSize);
        const unsigned char* compressedData = gNNDataData;
        size_t compressedSize = gNNDataSize;
        int res = Lzma86_Decode(unComprData.data(), &unCompressedSize, compressedData, &compressedSize);
        if (res != SZ_OK)
            throw ChessError("Failed to decompress network data");

        std::string nnData((char*)unComprData.data(), unCompressedSize);
        std::stringstream is(nnData);
        netData->load(is);
        return netData;
    }();
    return *staticNetData;
}

int
Evaluate::swindleScore(int evalScore, int distToWin) {
    using namespace SearchConst;
    if (distToWin == 0) {
        int sgn = evalScore >= 0 ? 1 : -1;
        int score = std::abs(evalScore) + 4;
        int lg = BitUtil::lastBit(score);
        score = (lg - 3) * 4 + (score >> (lg - 2));
        score = std::min(score, minFrustrated - 1);
        return sgn * score;
    } else {
        int sgn = distToWin > 0 ? 1 : -1;
        return sgn * std::max(maxFrustrated + 1 - std::abs(distToWin), minFrustrated);
    }
}
