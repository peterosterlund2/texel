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
 * parameters.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "parameters.hpp"
#include "computerPlayer.hpp"
#include <cmath>

namespace UciParams {
    std::shared_ptr<Parameters::SpinParam> hash(std::make_shared<Parameters::SpinParam>("Hash", 1, 1024*1024, 16));
    std::shared_ptr<Parameters::CheckParam> ownBook(std::make_shared<Parameters::CheckParam>("OwnBook", false));
    std::shared_ptr<Parameters::StringParam> bookFile(std::make_shared<Parameters::StringParam>("BookFile", ""));
    std::shared_ptr<Parameters::CheckParam> ponder(std::make_shared<Parameters::CheckParam>("Ponder", false));
    std::shared_ptr<Parameters::CheckParam> analyseMode(std::make_shared<Parameters::CheckParam>("UCI_AnalyseMode", false));
    std::shared_ptr<Parameters::StringParam> opponent(std::make_shared<Parameters::StringParam>("UCI_Opponent", ""));
    std::shared_ptr<Parameters::SpinParam> strength(std::make_shared<Parameters::SpinParam>("Strength", 0, 1000, 1000));
    std::shared_ptr<Parameters::SpinParam> maxNPS(std::make_shared<Parameters::SpinParam>("MaxNPS", 0, 10000000, 0));
#ifdef CLUSTER
    int maxThreads = 64*1024*1024;
#else
    int maxThreads = 512;
#endif
    std::shared_ptr<Parameters::SpinParam> threads(std::make_shared<Parameters::SpinParam>("Threads", 1, maxThreads, 1));
    std::shared_ptr<Parameters::SpinParam> multiPV(std::make_shared<Parameters::SpinParam>("MultiPV", 1, 256, 1));

    std::shared_ptr<Parameters::CheckParam> useNullMove(std::make_shared<Parameters::CheckParam>("UseNullMove", true));

    std::shared_ptr<Parameters::StringParam> gtbPath(std::make_shared<Parameters::StringParam>("GaviotaTbPath", ""));
    std::shared_ptr<Parameters::SpinParam> gtbCache(std::make_shared<Parameters::SpinParam>("GaviotaTbCache", 1, 2047, 1));
    std::shared_ptr<Parameters::StringParam> rtbPath(std::make_shared<Parameters::StringParam>("SyzygyPath", ""));
    std::shared_ptr<Parameters::SpinParam> minProbeDepth(std::make_shared<Parameters::SpinParam>("MinProbeDepth", 0, 100, 1));

    std::shared_ptr<Parameters::CheckParam> analysisAgeHash(std::make_shared<Parameters::CheckParam>("AnalysisAgeHash", true));
    std::shared_ptr<Parameters::ButtonParam> clearHash(std::make_shared<Parameters::ButtonParam>("Clear Hash"));
}

int pieceValue[Piece::nPieceTypes];

DEFINE_PARAM(pV);
DEFINE_PARAM(nV);
DEFINE_PARAM(bV);
DEFINE_PARAM(rV);
DEFINE_PARAM(qV);
DEFINE_PARAM(kV);

#if 0
DEFINE_PARAM(pawnIslandPenalty);
DEFINE_PARAM(pawnBackwardPenalty);
DEFINE_PARAM(pawnSemiBackwardPenalty1);
DEFINE_PARAM(pawnSemiBackwardPenalty2);
DEFINE_PARAM(pawnRaceBonus);
#endif
DEFINE_PARAM(passedPawnEGFactor);
#if 0
DEFINE_PARAM(RBehindPP1);
DEFINE_PARAM(RBehindPP2);
DEFINE_PARAM(activePawnPenalty);

DEFINE_PARAM(QvsRMBonus1);
DEFINE_PARAM(QvsRMBonus2);
DEFINE_PARAM(knightVsQueenBonus1);
DEFINE_PARAM(knightVsQueenBonus2);
DEFINE_PARAM(knightVsQueenBonus3);
DEFINE_PARAM(krkpBonus);
DEFINE_PARAM(krpkbBonus);
DEFINE_PARAM(krpkbPenalty);
DEFINE_PARAM(krpknBonus);
DEFINE_PARAM(RvsBPBonus);

DEFINE_PARAM(pawnTradePenalty);
DEFINE_PARAM(pawnTradeThreshold);

DEFINE_PARAM(threatBonus1);
DEFINE_PARAM(threatBonus2);
DEFINE_PARAM(latentAttackBonus);

DEFINE_PARAM(rookHalfOpenBonus);
DEFINE_PARAM(rookOpenBonus);
DEFINE_PARAM(rookDouble7thRowBonus);
DEFINE_PARAM(trappedRookPenalty1);
DEFINE_PARAM(trappedRookPenalty2);
#endif
DEFINE_PARAM(bishopPairPawnPenalty);
#if 0
DEFINE_PARAM(trappedBishopPenalty);
DEFINE_PARAM(oppoBishopPenalty);

DEFINE_PARAM(kingSafetyHalfOpenBCDEFG1);
DEFINE_PARAM(kingSafetyHalfOpenBCDEFG2);
DEFINE_PARAM(kingSafetyHalfOpenAH1);
DEFINE_PARAM(kingSafetyHalfOpenAH2);
DEFINE_PARAM(kingSafetyWeight1);
DEFINE_PARAM(kingSafetyWeight2);
DEFINE_PARAM(kingSafetyWeight3);
DEFINE_PARAM(kingSafetyWeight4);
DEFINE_PARAM(kingSafetyThreshold);
DEFINE_PARAM(knightKingProtectBonus);
DEFINE_PARAM(bishopKingProtectBonus);
DEFINE_PARAM(pawnStormBonus);
#endif

DEFINE_PARAM(pawnLoMtrl);
DEFINE_PARAM(pawnHiMtrl);
DEFINE_PARAM(minorLoMtrl);
DEFINE_PARAM(minorHiMtrl);
#if 0
DEFINE_PARAM(castleLoMtrl);
DEFINE_PARAM(castleHiMtrl);
#endif
DEFINE_PARAM(queenLoMtrl);
DEFINE_PARAM(queenHiMtrl);
DEFINE_PARAM(passedPawnLoMtrl);
DEFINE_PARAM(passedPawnHiMtrl);
#if 0
DEFINE_PARAM(kingSafetyLoMtrl);
DEFINE_PARAM(kingSafetyHiMtrl);
DEFINE_PARAM(oppoBishopLoMtrl);
DEFINE_PARAM(oppoBishopHiMtrl);
DEFINE_PARAM(knightOutpostLoMtrl);
DEFINE_PARAM(knightOutpostHiMtrl);
#endif


DEFINE_PARAM(aspirationWindow);
DEFINE_PARAM(rootLMRMoveCount);

DEFINE_PARAM(razorMargin1);
DEFINE_PARAM(razorMargin2);

DEFINE_PARAM(reverseFutilityMargin1);
DEFINE_PARAM(reverseFutilityMargin2);
DEFINE_PARAM(reverseFutilityMargin3);
DEFINE_PARAM(reverseFutilityMargin4);

DEFINE_PARAM(futilityMargin1);
DEFINE_PARAM(futilityMargin2);
DEFINE_PARAM(futilityMargin3);
DEFINE_PARAM(futilityMargin4);

DEFINE_PARAM(lmpMoveCountLimit1);
DEFINE_PARAM(lmpMoveCountLimit2);
DEFINE_PARAM(lmpMoveCountLimit3);
DEFINE_PARAM(lmpMoveCountLimit4);

DEFINE_PARAM(lmrMoveCountLimit1);
DEFINE_PARAM(lmrMoveCountLimit2);

DEFINE_PARAM(quiesceMaxSortMoves);
DEFINE_PARAM(deltaPruningMargin);

DEFINE_PARAM(timeMaxRemainingMoves);
DEFINE_PARAM(bufferTime);
DEFINE_PARAM(minTimeUsage);
DEFINE_PARAM(maxTimeUsage);
DEFINE_PARAM(timePonderHitRate);

#if 0
ParamTable<64> knightOutpostBonus { 0, 150, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  26,  41,  43,  43,  41,  26,   0,
        0,  23,  39,  37,  37,  39,  23,   0,
        0,   0,  29,  34,  34,  29,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   1,   2,   3,   3,   2,   1,   0,
        0,   4,   5,   6,   6,   5,   4,   0,
        0,   0,   7,   8,   8,   7,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 }
};

ParamTable<64> protectedPawnBonus { -50, 150, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      127, 123,  95, 124, 124,  95, 123, 127,
       30,  41,  54,  68,  68,  54,  41,  30,
        5,  10,  19,  20,  20,  19,  10,   5,
        7,  10,   9,  12,  12,   9,  10,   7,
       14,  13,  22,  16,  16,  22,  13,  14,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 }
};

ParamTable<64> attackedPawnBonus { -150, 100, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      -20, -43,  22,  -2,  -2,  22, -43, -20,
        7,  -1,  14,  19,  19,  14,  -1,   7,
      -12,  -8,  -8,   7,   7,  -8,  -8, -12,
      -36,   0, -18,  10,  10, -18,   0, -36,
      -83, -29, -63, -49, -49, -63, -29, -83,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 }
};

ParamTable<4> protectBonus { -50, 50, useUciParam,
    {  0, 11, 10,  1 },
    {  1,  2,  3,  4 }
};

ParamTable<36> connectedPPBonus { -200, 400, useUciParam,
    {  -2,  -6,   3,   7,   9,  -5,
       -6,  -4,  -1,   9,  14,   8,
        3,  -1,  16,   9,  23,  21,
        7,   9,   9,  37,   0,  46,
        9,  14,  23,   0, 113, -27,
       -5,   8,  21,  46, -27, 362 },
    {   1,   2,   4,   7,  11,  16,
        2,   3,   5,   8,  12,  17,
        4,   5,   6,   9,  13,  18,
        7,   8,   9,  10,  14,  19,
       11,  12,  13,  14,  15,  20,
       16,  17,  18,  19,  20,  21 }
};
#endif

ParamTable<8> passedPawnBonusX { -200, 200, useUciParam,
    {  0,  2, -3, -3, -3, -3,  2,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

ParamTable<8> passedPawnBonusY { -200, 200, useUciParam,
    {  0,  2,  2, 10, 20, 37,  6,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

#if 0
ParamTable<10> ppBlockerBonus { -50, 50, useUciParam,
    { 25, 29, 13, -9, 50,  1,  4,  3, -1,  9 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1,  0,  5, 14, 42, 30, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -88,   0,   0,
       88,   0,   0,   0,
        0,   0,   0,  96,
        0,   0, -96,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    {-21, -3, 23, 32, 38 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    { 27, 47, 57, 52, 45,-12,-64 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-73,-73, -4, 19, 19, 33, 52 },
    {   1,   1,  2,  3,  4,  5,  6 }
};
#endif
ParamTable<4> bishopPairValue { 0, 200, useUciParam,
    { 69, 48, 54, 51 },
    {  1,  2,  3,  4 }
};
#if 0
ParamTable<7> rookEGDrawFactor { 0, 255, useUciParam,
    { 71, 75,110,139,136,154,158 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsBPDrawFactor { 0, 255, useUciParam,
    {128, 90, 93,133,118,229,189 },
    {  0,  1,  2,  3,  4,  5,  6 }
};
ParamTable<4> castleFactor { 0, 128, useUciParam,
    { 64, 43, 27, 11 },
    {  1,  2,  3,  4 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 17, 31,-16,  3, 19,  4, -1, 14, 10 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -400, 100, useUciParam,
    {-105,-47,-262, 43, 55,  9, 13,-15,-13 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<14> kingAttackWeight { 0, 400, useUciParam,
    {  0,  3,  0,  6,  6, 13, 25, 47, 62, 97,104,147,207,321 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 }
};

ParamTable<5> qContactCheckBonus { -1000, 1000, useUciParam,
    {-512,-188,  0,188,512 },
    {  -2,  -1,  0,   1,   2 }
};

ParamTable<7> pieceKingAttackBonus { -1000, 1000, useUciParam,
    {-51,-26,-10,  0, 10, 26, 51 },
    { -3, -2, -1,  0,  1,  2,  3 }
};

ParamTable<5> kingPPSupportK { 0, 200, useUciParam,
    { 41, 71, 66, 60, 97 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> kingPPSupportP { 1, 64, useUciParam,
    {  0,  3,  4,  9, 15, 21, 32,  0 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<8> pawnDoubledPenalty { 0, 50, useUciParam,
    { 41, 22, 20, 15, 15, 20, 22, 41 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<8> pawnIsolatedPenalty { 0, 50, useUciParam,
    {  1, 11,  6, 11, 11,  6, 11,  1 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<10> halfMoveFactor { 0, 192, useUciParam,
    {128,128,128,128, 31, 19, 12,  9,  5,  3 },
    {  0,  0,  0,  0,  1,  2,  3,  4,  5,  6 }
};

ParamTable<9> stalePawnFactor { 0, 192, useUciParam,
    {114,124,129,129,132,126,106, 73, 41 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};
#endif

// ----------------------------------------------------------------------------

// Piece/square tables for knights
int knightTableWhiteMG[64], knightTableBlackMG[64];
int knightTableWhiteEG[64], knightTableBlackEG[64];
ParamTable<6> knightTableParams { -250, 250, useUciParam,
    {  5, 41,-24, 19, 41,-55 },
    {  1,  2,  3,  4,  5,  6 }
};
static void knightTableUpdate() {
    double k0mg = knightTableParams[0];
    double k1mg = knightTableParams[1];
    double k2mg = knightTableParams[2];
    double k0eg = knightTableParams[3];
    double k1eg = knightTableParams[4];
    double k2eg = knightTableParams[5];
    auto fxy = [](double x, double y, double k0, double k1, double k2) -> int {
        auto sqr = [](double x) { return x * x; };
        return (int)std::round(k0 + k1 * y / 10 + k2 * (sqr(x - 3.5) + sqr(y - 3.5)) / 10);
    };
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);
            int valMG = fxy(x, y, k0mg, k1mg, k2mg);
            knightTableWhiteMG[sq] = valMG;
            knightTableBlackMG[Position::mirrorY(sq)] = valMG;
            int valEG = fxy(x, y, k0eg, k1eg, k2eg);
            knightTableWhiteEG[sq] = valEG;
            knightTableBlackEG[Position::mirrorY(sq)] = valEG;
        }
    }
}

// Piece/square tables for bishops
int bishopTableWhiteMG[64], bishopTableBlackMG[64];
int bishopTableWhiteEG[64], bishopTableBlackEG[64];
ParamTable<8> bishopTableParams { -250, 250, useUciParam,
    { -5, 26,-48, -5, 11,-33, 10,-73 },
    {  1,  2,  3,  4,  5,  6,  7,  8 }
};
static void bishopTableUpdate() {
    int k0mg = bishopTableParams[0];
    int k1mg = bishopTableParams[1];
    int k2mg = bishopTableParams[2];
    int k3mg = bishopTableParams[3];
    int k4mg = bishopTableParams[4];
    int k0eg = bishopTableParams[5];
    int k1eg = bishopTableParams[6];
    int k2eg = bishopTableParams[7];

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);

            int valMG = std::min(k0mg + (k1mg - k0mg) * y / 5,
                                 k1mg + (k2mg - k1mg) * (y - 5) / 2);
            if (x == 0 || x == 7)
                valMG += k3mg;
            if (sq == B2 || sq == G2)
                valMG += k4mg;
            bishopTableWhiteMG[sq] = valMG;
            bishopTableBlackMG[Position::mirrorY(sq)] = valMG;

            auto distFromMiddle = [](int x) { return std::max(3 - x, x - 4); };
            int dist = distFromMiddle(x) + distFromMiddle(y);
            int valEG = std::max(k0eg, k1eg + k2eg * dist / 10);
            bishopTableWhiteEG[sq] = valEG;
            bishopTableBlackEG[Position::mirrorY(sq)] = valEG;
        }
    }
}

// Piece/square tables for pawns
int pawnTableWhiteMG[64], pawnTableBlackMG[64];
int pawnTableWhiteEG[64], pawnTableBlackEG[64];
ParamTable<20> pawnTableParams { -250, 250, useUciParam,
    {-17, -2,-12,-10,-22,-23,-22,-14, 17,118,  3, 11,  5,  1, -7,-13,-12, -6, 10,114 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 }
};
static void pawnTableUpdate() {
    for (int y = 1; y < 7; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);

            int xi = std::min(x, 7-x);
            int valMG = pawnTableParams[xi] + pawnTableParams[4+(y-1)];
            pawnTableWhiteMG[sq] = valMG;
            pawnTableBlackMG[Position::mirrorY(sq)] = valMG;

            int valEG = pawnTableParams[10+xi] + pawnTableParams[10+4+(y-1)];
            pawnTableWhiteEG[sq] = valEG;
            pawnTableBlackEG[Position::mirrorY(sq)] = valEG;
        }
    }
}

// Piece/square tables for kings
int kingTableWhiteMG[64], kingTableBlackMG[64];
int kingTableWhiteEG[64], kingTableBlackEG[64];
ParamTable<5> kingTableParams { -250, 250, useUciParam,
    { 22, 53,-26, 19,-20 },
    {  1,  2,  3,  4,  5 }
};
static void kingTableUpdate() {
    double k0mg = kingTableParams[0];
    double k1mg = kingTableParams[1];
    double k0eg = kingTableParams[2];
    double k1eg = kingTableParams[3];
    double k2eg = kingTableParams[4];
    auto sqr = [](double x) { return x * x; };
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);

            int valMG = (int)std::round(k0mg/10*sqr(x-3.5) + k1mg/10*sqr(y-3.5));
            kingTableWhiteMG[sq] = valMG;
            kingTableBlackMG[Position::mirrorY(sq)] = valMG;

            int valEG = (int)std::round(k0eg/10*(sqr(x-3.5)+sqr(y-3.5)) +
                                        k1eg / (1 + exp(k2eg/10*(y-4))));
            kingTableWhiteEG[sq] = valEG;
            kingTableBlackEG[Position::mirrorY(sq)] = valEG;
        }
    }
}

// Piece/square tables for rooks
int rookTableWhiteMG[64], rookTableBlackMG[64];
ParamTable<12> rookTableParams { -250, 250, useUciParam,
    {-10, -1, 11, 11, -5,-18,-14,  1, 28, 49, 49, 37 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12 }
};
static void rookTableUpdate() {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);
            int xi = std::min(x, 7-x);
            int valMG = rookTableParams[xi] + rookTableParams[4+y];
            rookTableWhiteMG[sq] = valMG;
            rookTableBlackMG[Position::mirrorY(sq)] = valMG;
        }
    }
}

// Piece/square tables for queens
int queenTableWhiteMG[64], queenTableBlackMG[64];
int queenTableWhiteEG[64], queenTableBlackEG[64];
ParamTable<14> queenTableParams { -250, 250, useUciParam,
    {-13,-21,-23,-26,-15, 12,-50,-12,-107,-112,-101,-96,-88,-77 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
static void queenTableUpdate() {
    auto symSquare = [](int sq) -> int {
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        if (x >= 4) x = 7 - x;
        if (y >= 4) y = 7 - y;
        if (x < y)
            std::swap(x, y);
        return Position::getSquare(x, y);
    };
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);

            int valMG = queenTableParams[y];
            queenTableWhiteMG[sq] = valMG;
            queenTableBlackMG[Position::mirrorY(sq)] = valMG;

            int valEG = 0;
            switch (symSquare(sq)) {
            case A1: case B1: case C1: case D1:
                valEG = queenTableParams[8+0];
                break;
            case B2:
                valEG = queenTableParams[8+1];
                break;
            case C2:
                valEG = queenTableParams[8+2];
                break;
            case D2:
                valEG = queenTableParams[8+3];
                break;
            case C3: case D3:
                valEG = queenTableParams[8+4];
                break;
            case D4:
                valEG = queenTableParams[8+5];
                break;
            default:
                break;
            }
            queenTableWhiteEG[sq] = valEG;
            queenTableBlackEG[Position::mirrorY(sq)] = valEG;
        }
    }
}

// ----------------------------------------------------------------------------

int rookMobScore[15];
ParamTable<3> rookMobParams { -250, 250, useUciParam,
    {-83, 74,-16 },
    {  1,  2,   3 }
};
static void rookMobScoreUpdate() {
    int k0 = rookMobParams[0];
    int k1 = rookMobParams[1];
    int k2 = rookMobParams[2];
    for (int i = 0; i < 15; i++)
        rookMobScore[i] = k0 + k1 * i / 10 + k2 * i * i / 100;
}

int bishMobScore[14];
ParamTable<3> bishMobParams { -250, 250, useUciParam,
    {-84, 90,-35 },
    {  1,  2,   3 }
};
static void bishMobScoreUpdate() {
    int k0 = bishMobParams[0];
    int k1 = bishMobParams[1];
    int k2 = bishMobParams[2];
    for (int i = 0; i < 14; i++)
        bishMobScore[i] = k0 + k1 * i / 10 + k2 * i * i / 100;
}

int queenMobScore[28];
ParamTable<2> queenMobParams { -250, 250, useUciParam,
    { 34,-31 },
    { 1,   2 },
};
static void queenMobScoreUpdate() {
    double k0 = queenMobParams[0];
    double k1 = queenMobParams[1];
    for (int i = 0; i < 28; i++)
        queenMobScore[i] = (int)std::round(k0 / (1 + exp(k1 / 100 * (i - 13.5))));
}

int knightMobScore[64][9];
ParamTable<3> knightMobParams { -250, 250, useUciParam,
    {-102, 65,-27 },
    {  1,  2,  3 }
};
static void knightMobScoreUpdate() {
    int k0 = knightMobParams[0];
    int k1 = knightMobParams[1];
    int k2 = knightMobParams[2];
    for (int sq = 0; sq < 64; sq++) {
        int maxMob = BitBoard::bitCount(BitBoard::knightAttacks[sq]);
        for (int m = 0; m <= 8; m++) {
            double x = std::min(m, maxMob) / (double)maxMob;
            knightMobScore[sq][m] = (int)std::round(k0 + k1 * x + k2 * x * x);
        }
    }
}

// ----------------------------------------------------------------------------

Parameters::Parameters() {
    std::string about = ComputerPlayer::engineName +
                        " by Peter Osterlund, see http://hem.bredband.net/petero2b/javachess/index.html#texel";
    addPar(std::make_shared<StringParam>("UCI_EngineAbout", about));

    addPar(UciParams::hash);
    addPar(UciParams::ownBook);
    addPar(UciParams::bookFile);
    addPar(UciParams::ponder);
    addPar(UciParams::analyseMode);
    addPar(UciParams::opponent);
    addPar(UciParams::strength);
    addPar(UciParams::maxNPS);
    addPar(UciParams::threads);
    addPar(UciParams::multiPV);

    addPar(UciParams::useNullMove);

    addPar(UciParams::gtbPath);
    addPar(UciParams::gtbCache);
    addPar(UciParams::rtbPath);
    addPar(UciParams::minProbeDepth);
    addPar(UciParams::analysisAgeHash);
    addPar(UciParams::clearHash);

    // Evaluation parameters
    REGISTER_PARAM(pV, "PawnValue");
    REGISTER_PARAM(nV, "KnightValue");
    REGISTER_PARAM(bV, "BishopValue");
    REGISTER_PARAM(rV, "RookValue");
    REGISTER_PARAM(qV, "QueenValue");
    REGISTER_PARAM(kV, "KingValue");

    rookMobParams.registerParams("RookMobility", *this);
    rookMobParams.addListener(rookMobScoreUpdate);
    bishMobParams.registerParams("BishopMobility", *this);
    bishMobParams.addListener(bishMobScoreUpdate);
    queenMobParams.registerParams("QueenMobility", *this);
    queenMobParams.addListener(queenMobScoreUpdate);
    knightMobParams.registerParams("KnightMobility", *this);
    knightMobParams.addListener(knightMobScoreUpdate);

    knightTableParams.registerParams("KnightTable", *this);
    knightTableParams.addListener(knightTableUpdate);
    bishopTableParams.registerParams("BishopTable", *this);
    bishopTableParams.addListener(bishopTableUpdate);
    pawnTableParams.registerParams("PawnTable", *this);
    pawnTableParams.addListener(pawnTableUpdate);
    kingTableParams.registerParams("KingTable", *this);
    kingTableParams.addListener(kingTableUpdate);
    rookTableParams.registerParams("RookTable", *this);
    rookTableParams.addListener(rookTableUpdate);
    queenTableParams.registerParams("QueenTable", *this);
    queenTableParams.addListener(queenTableUpdate);

#if 0
    REGISTER_PARAM(pawnIslandPenalty, "PawnIslandPenalty");
    REGISTER_PARAM(pawnBackwardPenalty, "PawnBackwardPenalty");
    REGISTER_PARAM(pawnSemiBackwardPenalty1, "PawnSemiBackwardPenalty1");
    REGISTER_PARAM(pawnSemiBackwardPenalty2, "PawnSemiBackwardPenalty2");
    REGISTER_PARAM(pawnRaceBonus, "PawnRaceBonus");
#endif
    REGISTER_PARAM(passedPawnEGFactor, "PassedPawnEGFactor");
#if 0
    REGISTER_PARAM(RBehindPP1, "RookBehindPassedPawn1");
    REGISTER_PARAM(RBehindPP2, "RookBehindPassedPawn2");
    REGISTER_PARAM(activePawnPenalty, "ActivePawnPenalty");

    REGISTER_PARAM(QvsRMBonus1, "QueenVsRookMinorBonus1");
    REGISTER_PARAM(QvsRMBonus2, "QueenVsRookMinorBonus2");
    REGISTER_PARAM(knightVsQueenBonus1, "KnightVsQueenBonus1");
    REGISTER_PARAM(knightVsQueenBonus2, "KnightVsQueenBonus2");
    REGISTER_PARAM(knightVsQueenBonus3, "KnightVsQueenBonus3");
    REGISTER_PARAM(krkpBonus, "RookVsPawnBonus");
    REGISTER_PARAM(krpkbBonus, "RookPawnVsBishopBonus");
    REGISTER_PARAM(krpkbPenalty, "RookPawnVsBishopPenalty");
    REGISTER_PARAM(krpknBonus, "RookPawnVsKnightBonus");
    REGISTER_PARAM(RvsBPBonus, "RookVsBishopPawnBonus");

    REGISTER_PARAM(pawnTradePenalty, "PawnTradePenalty");
    REGISTER_PARAM(pawnTradeThreshold, "PawnTradeThreshold");

    REGISTER_PARAM(threatBonus1, "ThreatBonus1");
    REGISTER_PARAM(threatBonus2, "ThreatBonus2");
    REGISTER_PARAM(latentAttackBonus, "LatentAttackBonus");

    REGISTER_PARAM(rookHalfOpenBonus, "RookHalfOpenBonus");
    REGISTER_PARAM(rookOpenBonus, "RookOpenBonus");
    REGISTER_PARAM(rookDouble7thRowBonus, "RookDouble7thRowBonus");
    REGISTER_PARAM(trappedRookPenalty1, "TrappedRookPenalty1");
    REGISTER_PARAM(trappedRookPenalty2, "TrappedRookPenalty2");
#endif
    REGISTER_PARAM(bishopPairPawnPenalty, "BishopPairPawnPenalty");
#if 0
    REGISTER_PARAM(trappedBishopPenalty, "TrappedBishopPenalty");
    REGISTER_PARAM(oppoBishopPenalty, "OppositeBishopPenalty");

    REGISTER_PARAM(kingSafetyHalfOpenBCDEFG1, "KingSafetyHalfOpenBCDEFG1");
    REGISTER_PARAM(kingSafetyHalfOpenBCDEFG2, "KingSafetyHalfOpenBCDEFG2");
    REGISTER_PARAM(kingSafetyHalfOpenAH1, "KingSafetyHalfOpenAH1");
    REGISTER_PARAM(kingSafetyHalfOpenAH2, "KingSafetyHalfOpenAH2");
    REGISTER_PARAM(kingSafetyWeight1, "KingSafetyWeight1");
    REGISTER_PARAM(kingSafetyWeight2, "KingSafetyWeight2");
    REGISTER_PARAM(kingSafetyWeight3, "KingSafetyWeight3");
    REGISTER_PARAM(kingSafetyWeight4, "KingSafetyWeight4");
    REGISTER_PARAM(kingSafetyThreshold, "KingSafetyThreshold");
    REGISTER_PARAM(knightKingProtectBonus, "KnightKingProtectBonus");
    REGISTER_PARAM(bishopKingProtectBonus, "BishopKingProtectBonus");
    REGISTER_PARAM(pawnStormBonus, "PawnStormBonus");
#endif

    REGISTER_PARAM(pawnLoMtrl, "PawnLoMtrl");
    REGISTER_PARAM(pawnHiMtrl, "PawnHiMtrl");
    REGISTER_PARAM(minorLoMtrl, "MinorLoMtrl");
    REGISTER_PARAM(minorHiMtrl, "MinorHiMtrl");
#if 0
    REGISTER_PARAM(castleLoMtrl, "CastleLoMtrl");
    REGISTER_PARAM(castleHiMtrl, "CastleHiMtrl");
#endif
    REGISTER_PARAM(queenLoMtrl, "QueenLoMtrl");
    REGISTER_PARAM(queenHiMtrl, "QueenHiMtrl");
    REGISTER_PARAM(passedPawnLoMtrl, "PassedPawnLoMtrl");
    REGISTER_PARAM(passedPawnHiMtrl, "PassedPawnHiMtrl");
#if 0
    REGISTER_PARAM(kingSafetyLoMtrl, "KingSafetyLoMtrl");
    REGISTER_PARAM(kingSafetyHiMtrl, "KingSafetyHiMtrl");
    REGISTER_PARAM(oppoBishopLoMtrl, "OppositeBishopLoMtrl");
    REGISTER_PARAM(oppoBishopHiMtrl, "OppositeBishopHiMtrl");
    REGISTER_PARAM(knightOutpostLoMtrl, "KnightOutpostLoMtrl");
    REGISTER_PARAM(knightOutpostHiMtrl, "KnightOutpostHiMtrl");

    // Evaluation tables
    knightOutpostBonus.registerParams("KnightOutpostBonus", *this);
    protectedPawnBonus.registerParams("ProtectedPawnBonus", *this);
    attackedPawnBonus.registerParams("AttackedPawnBonus", *this);
    protectBonus.registerParams("ProtectBonus", *this);
    majorPieceRedundancy.registerParams("MajorPieceRedundancy", *this);
    connectedPPBonus.registerParams("ConnectedPPBonus", *this);
#endif
    passedPawnBonusX.registerParams("PassedPawnBonusX", *this);
    passedPawnBonusY.registerParams("PassedPawnBonusY", *this);
#if 0
    ppBlockerBonus.registerParams("PassedPawnBlockerBonus", *this);
    candidatePassedBonus.registerParams("CandidatePassedPawnBonus", *this);
    QvsRRBonus.registerParams("QueenVs2RookBonus", *this);
    RvsMBonus.registerParams("RookVsMinorBonus", *this);
    RvsMMBonus.registerParams("RookVs2MinorBonus", *this);
#endif
    bishopPairValue.registerParams("BishopPairValue", *this);
#if 0
    rookEGDrawFactor.registerParams("RookEndGameDrawFactor", *this);
    RvsBPDrawFactor.registerParams("RookVsBishopPawnDrawFactor", *this);
    castleFactor.registerParams("CastleFactor", *this);
    pawnShelterTable.registerParams("PawnShelterTable", *this);
    pawnStormTable.registerParams("PawnStormTable", *this);
    kingAttackWeight.registerParams("KingAttackWeight", *this);
    qContactCheckBonus.registerParams("QueenContactCheckBonus", *this);
    pieceKingAttackBonus.registerParams("PieceKingAttackBonus", *this);
    kingPPSupportK.registerParams("KingPassedPawnSupportK", *this);
    kingPPSupportP.registerParams("KingPassedPawnSupportP", *this);
    pawnDoubledPenalty.registerParams("PawnDoubledPenalty", *this);
    pawnIsolatedPenalty.registerParams("PawnIsolatedPenalty", *this);
    halfMoveFactor.registerParams("HalfMoveFactor", *this);
    stalePawnFactor.registerParams("StalePawnFactor", *this);
#endif

    // Search parameters
    REGISTER_PARAM(aspirationWindow, "AspirationWindow");
    REGISTER_PARAM(rootLMRMoveCount, "RootLMRMoveCount");

    REGISTER_PARAM(razorMargin1, "RazorMargin1");
    REGISTER_PARAM(razorMargin2, "RazorMargin2");

    REGISTER_PARAM(reverseFutilityMargin1, "ReverseFutilityMargin1");
    REGISTER_PARAM(reverseFutilityMargin2, "ReverseFutilityMargin2");
    REGISTER_PARAM(reverseFutilityMargin3, "ReverseFutilityMargin3");
    REGISTER_PARAM(reverseFutilityMargin4, "ReverseFutilityMargin4");

    REGISTER_PARAM(futilityMargin1, "FutilityMargin1");
    REGISTER_PARAM(futilityMargin2, "FutilityMargin2");
    REGISTER_PARAM(futilityMargin3, "FutilityMargin3");
    REGISTER_PARAM(futilityMargin4, "FutilityMargin4");

    REGISTER_PARAM(lmpMoveCountLimit1, "LMPMoveCountLimit1");
    REGISTER_PARAM(lmpMoveCountLimit2, "LMPMoveCountLimit2");
    REGISTER_PARAM(lmpMoveCountLimit3, "LMPMoveCountLimit3");
    REGISTER_PARAM(lmpMoveCountLimit4, "LMPMoveCountLimit4");

    REGISTER_PARAM(lmrMoveCountLimit1, "LMRMoveCountLimit1");
    REGISTER_PARAM(lmrMoveCountLimit2, "LMRMoveCountLimit2");

    REGISTER_PARAM(quiesceMaxSortMoves, "QuiesceMaxSortMoves");
    REGISTER_PARAM(deltaPruningMargin, "DeltaPruningMargin");

    // Time management parameters
    REGISTER_PARAM(timeMaxRemainingMoves, "TimeMaxRemainingMoves");
    REGISTER_PARAM(bufferTime, "BufferTime");
    REGISTER_PARAM(minTimeUsage, "MinTimeUsage");
    REGISTER_PARAM(maxTimeUsage, "MaxTimeUsage");
    REGISTER_PARAM(timePonderHitRate, "TimePonderHitRate");
}

Parameters&
Parameters::instance() {
    static Parameters inst;
    return inst;
}

void
Parameters::getParamNames(std::vector<std::string>& parNames) {
    parNames = paramNames;
}

std::shared_ptr<Parameters::ParamBase>
Parameters::getParam(const std::string& name) const {
    auto it = params.find(toLowerCase(name));
    if (it == params.end())
        return nullptr;
    return it->second;
}

void
Parameters::addPar(const std::shared_ptr<ParamBase>& p) {
    std::string name = toLowerCase(p->getName());
    assert(params.find(name) == params.end());
    params[name] = p;
    paramNames.push_back(name);
}

int
Parameters::Listener::addListener(Func f, bool callNow) {
    int id = ++nextId;
    listeners[id] = f;
    if (callNow)
        f();
    return id;
}

void
Parameters::Listener::removeListener(int id) {
    listeners.erase(id);
}

void
Parameters::Listener::notify() {
    for (auto& e : listeners)
        (e.second)();
}

void
ParamTableBase::registerParamsN(const std::string& name, Parameters& pars,
                                int* table, int* parNo, int N) {
    // Check that each parameter has a single value
    std::map<int,int> parNoToVal;
    int maxParIdx = -1;
    for (int i = 0; i < N; i++) {
        if (parNo[i] == 0)
            continue;
        const int pn = std::abs(parNo[i]);
        const int sign = parNo[i] > 0 ? 1 : -1;
        maxParIdx = std::max(maxParIdx, pn);
        auto it = parNoToVal.find(pn);
        if (it == parNoToVal.end())
            parNoToVal.insert(std::make_pair(pn, sign*table[i]));
        else
            assert(it->second == sign*table[i]);
    }
    if (!uci)
        return;
    params.resize(maxParIdx+1);
    for (const auto& p : parNoToVal) {
        std::string pName = name + num2Str(p.first);
        params[p.first] = std::make_shared<Parameters::SpinParam>(pName, minValue, maxValue, p.second);
        pars.addPar(params[p.first]);
        params[p.first]->addListener([=]() { modifiedN(table, parNo, N); }, false);
    }
    modifiedN(table, parNo, N);
}

void
ParamTableBase::modifiedN(int* table, int* parNo, int N) {
    for (int i = 0; i < N; i++)
        if (parNo[i] > 0)
            table[i] = params[parNo[i]]->getIntPar();
        else if (parNo[i] < 0)
            table[i] = -params[-parNo[i]]->getIntPar();
    notify();
}
