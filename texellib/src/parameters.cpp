/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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

namespace UciParams {
    std::shared_ptr<Parameters::SpinParam> hash(std::make_shared<Parameters::SpinParam>("Hash", 1, 524288, 16));
    std::shared_ptr<Parameters::CheckParam> ownBook(std::make_shared<Parameters::CheckParam>("OwnBook", false));
    std::shared_ptr<Parameters::StringParam> bookFile(std::make_shared<Parameters::StringParam>("BookFile", ""));
    std::shared_ptr<Parameters::CheckParam> ponder(std::make_shared<Parameters::CheckParam>("Ponder", true));
    std::shared_ptr<Parameters::CheckParam> analyseMode(std::make_shared<Parameters::CheckParam>("UCI_AnalyseMode", false));
    std::shared_ptr<Parameters::StringParam> opponent(std::make_shared<Parameters::StringParam>("UCI_Opponent", ""));
    std::shared_ptr<Parameters::SpinParam> strength(std::make_shared<Parameters::SpinParam>("Strength", 0, 1000, 1000));
    std::shared_ptr<Parameters::SpinParam> threads(std::make_shared<Parameters::SpinParam>("Threads", 1, 64, 1));
    std::shared_ptr<Parameters::SpinParam> multiPV(std::make_shared<Parameters::SpinParam>("MultiPV", 1, 256, 1));

    std::shared_ptr<Parameters::CheckParam> useNullMove(std::make_shared<Parameters::CheckParam>("UseNullMove", true));

    std::shared_ptr<Parameters::StringParam> gtbPath(std::make_shared<Parameters::StringParam>("GaviotaTbPath", ""));
    std::shared_ptr<Parameters::SpinParam> gtbCache(std::make_shared<Parameters::SpinParam>("GaviotaTbCache", 1, 2047, 1));
    std::shared_ptr<Parameters::StringParam> rtbPath(std::make_shared<Parameters::StringParam>("SyzygyPath", ""));
    std::shared_ptr<Parameters::SpinParam> minProbeDepth(std::make_shared<Parameters::SpinParam>("MinProbeDepth", 0, 100, 1));

    std::shared_ptr<Parameters::ButtonParam> clearHash(std::make_shared<Parameters::ButtonParam>("Clear Hash"));
}

int pieceValue[Piece::nPieceTypes];

DEFINE_PARAM(pV);
DEFINE_PARAM(nV);
DEFINE_PARAM(bV);
DEFINE_PARAM(rV);
DEFINE_PARAM(qV);
DEFINE_PARAM(kV);

DEFINE_PARAM(pawnIslandPenalty);
DEFINE_PARAM(pawnBackwardPenalty);
DEFINE_PARAM(pawnSemiBackwardPenalty1);
DEFINE_PARAM(pawnSemiBackwardPenalty2);
DEFINE_PARAM(pawnRaceBonus);
DEFINE_PARAM(passedPawnEGFactor);
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
DEFINE_PARAM(pieceTradeBonus);
DEFINE_PARAM(pawnTradeThreshold);
DEFINE_PARAM(pieceTradeThreshold);

DEFINE_PARAM(threatBonus1);
DEFINE_PARAM(threatBonus2);
DEFINE_PARAM(latentAttackBonus);

DEFINE_PARAM(rookHalfOpenBonus);
DEFINE_PARAM(rookOpenBonus);
DEFINE_PARAM(rookDouble7thRowBonus);
DEFINE_PARAM(trappedRookPenalty1);
DEFINE_PARAM(trappedRookPenalty2);

DEFINE_PARAM(bishopPairPawnPenalty);
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

DEFINE_PARAM(pawnLoMtrl);
DEFINE_PARAM(pawnHiMtrl);
DEFINE_PARAM(minorLoMtrl);
DEFINE_PARAM(minorHiMtrl);
DEFINE_PARAM(castleLoMtrl);
DEFINE_PARAM(castleHiMtrl);
DEFINE_PARAM(queenLoMtrl);
DEFINE_PARAM(queenHiMtrl);
DEFINE_PARAM(passedPawnLoMtrl);
DEFINE_PARAM(passedPawnHiMtrl);
DEFINE_PARAM(kingSafetyLoMtrl);
DEFINE_PARAM(kingSafetyHiMtrl);
DEFINE_PARAM(oppoBishopLoMtrl);
DEFINE_PARAM(oppoBishopHiMtrl);
DEFINE_PARAM(knightOutpostLoMtrl);
DEFINE_PARAM(knightOutpostHiMtrl);


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

/** Piece/square table for king during middle game. */
ParamTable<64> kt1b { -200, 200, useUciParam,
    {  67,  71,  72,  65,  37,  86,  76,-190,
       40, -23,  17,  31,  65,  43,  26,  58,
      -46, -44, -30,   6,  45,  77,  17, -20,
      -63, -38, -42, -31, -23,  -5, -26, -74,
      -31,  -6, -11, -30, -29, -28, -33, -82,
      -37,  23, -10, -30, -16,  -6,   6, -22,
       27,  28,  18,  -3,   6,  -4,  40,  36,
      -18,  37,  19, -20,  14, -16,  33,  10 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> kt1w(kt1b);

/** Piece/square table for king during end game. */
ParamTable<64> kt2b { -200, 200, useUciParam,
    {   5,  56,  84,  86,  86,  84,  56,   5,
       45, 104, 108, 103, 103, 108, 104,  45,
       85, 113, 115, 111, 111, 115, 113,  85,
       75, 103, 110, 112, 112, 110, 103,  75,
       54,  83,  93, 100, 100,  93,  83,  54,
       46,  65,  76,  85,  85,  76,  65,  46,
       31,  51,  65,  64,  64,  65,  51,  31,
        0,  20,  35,  21,  21,  35,  20,   0 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
        0,  29,  30,  31,  31,  30,  29,   0 }
};
ParamTableMirrored<64> kt2w(kt2b);

/** Piece/square table for pawns during middle game. */
ParamTable<64> pt1b { -200, 300, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      150, 110, 108, 106,  76,  89, -35, 114,
       12,  24,  17,  27,  27,  54,  23,  21,
       -4,  -7, -16,  -4,   3,  -5,   4,  -2,
      -10,  -9, -10,  -7,  -2,  12,  -1, -20,
      -19, -14, -25, -19, -10,  -9,   1, -14,
      -14, -17, -25, -21, -18,   7,   8,  -6,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
        0,   0,   0,   0,   0,   0,   0,   0 }
};
ParamTableMirrored<64> pt1w(pt1b);

/** Piece/square table for pawns during end game. */
ParamTable<64> pt2b { -200, 200, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
        3,  15,  15,   1,   1,  15,  15,   3,
       41,  34,  27,   7,   7,  27,  34,  41,
       24,  22,  11,   6,   6,  11,  22,  24,
       15,  24,  15,  16,  16,  15,  24,  15,
        7,  19,  17,  25,  25,  17,  19,   7,
        7,  19,  28,  36,  36,  28,  19,   7,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
        0,   0,   0,   0,   0,   0,   0,   0 }
};
ParamTableMirrored<64> pt2w(pt2b);

/** Piece/square table for knights during middle game. */
ParamTable<64> nt1b { -300, 200, useUciParam,
    {-240, -97, -80, -27,  21,-119, -49,-248,
      -46, -43,  23,  52,  14,  67, -30,  -7,
      -45,  -5,  22,  37,  73, 121,  33, -15,
      -23,  -4,  12,  36,  14,  49,  14,  11,
      -21,  -6,  14,   5,  19,  23,  31, -13,
      -53, -18,  -7,   5,   8, -11, -14, -46,
      -59, -53, -25,  -3, -11, -23, -47, -42,
      -89, -45, -61, -43, -39, -51, -43, -88 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> nt1w(nt1b);

/** Piece/square table for knights during end game. */
ParamTable<64> nt2b { -200, 200, useUciParam,
    { -52,  10,  19,  11,  11,  19,  10, -52,
       -1,  12,  25,  45,  45,  25,  12,  -1,
       -2,  34,  45,  48,  48,  45,  34,  -2,
        7,  37,  48,  58,  58,  48,  37,   7,
        3,  36,  48,  52,  52,  48,  36,   3,
      -20,  16,  26,  44,  44,  26,  16, -20,
      -26,  -3,  13,  15,  15,  13,  -3, -26,
      -71, -45, -14,  -6,  -6, -14, -45, -71 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
       29,  30,  31,  32,  32,  31,  30,  29 }
};
ParamTableMirrored<64> nt2w(nt2b);

/** Piece/square table for bishops during middle game. */
ParamTable<64> bt1b { -200, 200, useUciParam,
    { -56, -31, -37, -50, -70,-100, -20, -63,
      -40, -25, -20, -23, -28, -11, -69, -42,
      -13,  -1,  -1,  10,  26,  74,  48,   9,
      -30,  -7,  -3,  37,   3,  14, -12, -16,
      -12, -15, -10,  10,  13, -16, -14,   2,
      -20,   0,  -4,  -7, -12,  -7, -12, -10,
       -7,  -4,  -2, -13,  -6,  -5,  13, -12,
      -22,   0, -20, -26, -17, -20, -23, -37 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> bt1w(bt1b);

/** Piece/square table for bishops during end game. */
ParamTable<64> bt2b { -200, 200, useUciParam,
    {  20,  22,  18,  25,  25,  18,  22,  20,
       22,  21,  31,  33,  33,  31,  21,  22,
       18,  31,  37,  44,  44,  37,  31,  18,
       25,  33,  44,  48,  48,  44,  33,  25,
       25,  33,  44,  48,  48,  44,  33,  25,
       18,  31,  37,  44,  44,  37,  31,  18,
       22,  21,  31,  33,  33,  31,  21,  22,
       20,  22,  18,  25,  25,  18,  22,  20 },
    {  10,   1,   2,   3,   3,   2,   1,  10,
        1,   4,   5,   6,   6,   5,   4,   1,
        2,   5,   7,   8,   8,   7,   5,   2,
        3,   6,   8,   9,   9,   8,   6,   3,
        3,   6,   8,   9,   9,   8,   6,   3,
        2,   5,   7,   8,   8,   7,   5,   2,
        1,   4,   5,   6,   6,   5,   4,   1,
       10,   1,   2,   3,   3,   2,   1,  10 }
};
ParamTableMirrored<64> bt2w(bt2b);

/** Piece/square table for queens during middle game. */
ParamTable<64> qt1b { -200, 200, useUciParam,
    { -73,  -7, -20, -49, -21, -28,  49,  19,
      -54, -88, -50, -42,-110, -40, -82,  30,
      -45, -39, -64, -58, -45, -27, -35, -59,
      -36, -28, -37, -48, -56, -45, -48, -35,
      -26, -24, -13, -31, -25, -32,  -2, -36,
      -27, -13, -11, -20, -20, -20,  -8, -28,
      -22, -14, -12,  -4, -13,  -5, -22, -49,
      -25, -12,  -8,  -6, -15, -41, -81, -60 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> qt1w(qt1b);

ParamTable<64> qt2b { -200, 200, useUciParam,
    { -16, -18, -20, -16, -16, -20, -18, -16,
      -18, -21, -12,  -9,  -9, -12, -21, -18,
      -20, -12,  -1,  -1,  -1,  -1, -12, -20,
      -16,  -9,  -1,  10,  10,  -1,  -9, -16,
      -16,  -9,  -1,  10,  10,  -1,  -9, -16,
      -20, -12,  -1,  -1,  -1,  -1, -12, -20,
      -18, -21, -12,  -9,  -9, -12, -21, -18,
      -16, -18, -20, -16, -16, -20, -18, -16 },
     { 10,   1,   2,   3,   3,   2,   1,  10,
        1,   4,   5,   6,   6,   5,   4,   1,
        2,   5,   7,   8,   8,   7,   5,   2,
        3,   6,   8,   9,   9,   8,   6,   3,
        3,   6,   8,   9,   9,   8,   6,   3,
        2,   5,   7,   8,   8,   7,   5,   2,
        1,   4,   5,   6,   6,   5,   4,   1,
       10,   1,   2,   3,   3,   2,   1,  10 }
};
ParamTableMirrored<64> qt2w(qt2b);

/** Piece/square table for rooks during middle game. */
ParamTable<64> rt1b { -200, 200, useUciParam,
    {  35,  45,  45,  39,  40,  61,  66,  60,
       38,  36,  51,  53,  45,  59,  63,  47,
       28,  36,  40,  41,  51,  71,  67,  49,
       15,  18,  27,  27,  18,  39,  39,  18,
       -5,  -1,   9,   6,   3,   9,  24, -11,
      -22, -10,  -7,  -6, -12,  -7,   1, -18,
      -27, -16,  -3,   0, -10,  -6, -12, -46,
       -2,  -3,   6,   9,   6,   9,  -3,   4 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> rt1w(rt1b);

ParamTable<64> knightOutpostBonus { 0, 150, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  27,  40,  48,  48,  40,  27,   0,
        0,  24,  40,  38,  38,  40,  24,   0,
        0,   0,  28,  40,  40,  28,   0,   0,
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
      118, 122, 130, 121, 121, 130, 122, 118,
       30,  45,  53,  61,  61,  53,  45,  30,
        6,  12,  23,  21,  21,  23,  12,   6,
        5,  11,   9,  13,  13,   9,  11,   5,
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
      -18, -45,  23, -15, -15,  23, -45, -18,
       10,  -2,  17,  15,  15,  17,  -2,  10,
      -16,  -9,  -7,   9,   9,  -7,  -9, -16,
      -35,   5, -17,  10,  10, -17,   5, -35,
      -79, -25, -64, -55, -55, -64, -25, -79,
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
    { -1, 11,  9,  1 },
    {  1,  2,  3,  4 }
};

ParamTable<15> rookMobScore { -50, 50, useUciParam,
    {-23,-12, -7, -1,  0,  5, 10, 12, 17, 21, 24, 26, 27, 24, 28 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-18,-10, -1,  6, 13, 19, 24, 28, 31, 35, 35, 38, 40, 38 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTable<28> knightMobScore { -200, 200, useUciParam,
    {-26,-48, 26,-48,-13,  0,  5,-52,-23, -4,  9, 14,-24,-24,-18,-10, -2,  5,  6,-28,-28,-18,-11, -4,  1,  5,  8,  6 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    {  6,  2, -3, -2,  0,  1,  3,  5,  8, 11, 14, 16, 19, 23, 27, 30, 34, 38, 41, 45, 49, 48, 49, 47, 46, 45, 39, 52 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<36> connectedPPBonus { -200, 400, useUciParam,
    {  -6,  -9,   0,   5,   7,   7,
       -9,  -6,  -1,  10,  12,  15,
        0,  -1,  14,   8,  25,  21,
        5,  10,   8,  40,   2,  50,
        7,  12,  25,   2, 114, -28,
        7,  15,  21,  50, -28, 369 },
    {   1,   2,   4,   7,  11,  16,
        2,   3,   5,   8,  12,  17,
        4,   5,   6,   9,  13,  18,
        7,   8,   9,  10,  14,  19,
       11,  12,  13,  14,  15,  20,
       16,  17,  18,  19,  20,  21 }
};

ParamTable<8> passedPawnBonusX { -200, 200, useUciParam,
    {  0,  5, -1, -5, -5, -1,  5,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

ParamTable<8> passedPawnBonusY { -200, 200, useUciParam,
    {  0,  4,  4, 14, 35, 64,103,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<10> ppBlockerBonus { -50, 50, useUciParam,
    { 25, 30, 11, -7, 50,  1,  5,  3, -2, 10 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1,  1,  6, 14, 41, 28, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -91,   0,   0,
       91,   0,   0,   0,
        0,   0,   0,  91,
        0,   0, -91,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    {-18,  4, 32, 78, 39 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    { 31, 47, 57, 51, 46, -3,-55 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-89,-89,  0, 17, 16, 27, 46 },
    {   1,   1,  2,  3,  4,  5,  6 }
};

ParamTable<4> bishopPairValue { 0, 200, useUciParam,
    { 73, 73, 60, 54 },
    {  1,  2,  3,  4 }
};

ParamTable<7> rookEGDrawFactor { 0, 255, useUciParam,
    { 70, 72,111,138,135,153,155 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsBPDrawFactor { 0, 255, useUciParam,
    {128, 90,103,133,119,235,175 },
    {  0,  1,  2,  3,  4,  5,  6 }
};
ParamTable<4> castleFactor { 0, 128, useUciParam,
    { 64, 43, 27, 10 },
    {  1,  2,  3,  4 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 17, 30,-17,  5, 19,  1, -2, 12, 10 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -400, 100, useUciParam,
    {-110,-41,-224, 34, 50,  4, 14,-11,-18 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<14> kingAttackWeight { 0, 400, useUciParam,
    {  0,  2,  0,  6,  6, 12, 22, 47, 58, 92,103,140,200,321 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 }
};

ParamTable<5> qContactCheckBonus { -1000, 1000, useUciParam,
    {-519,-184,  0,184,519 },
    {  -2,  -1,  0,   1,   2 }
};

ParamTable<7> pieceKingAttackBonus { -1000, 1000, useUciParam,
    {-42,-21, -8,  0,  8, 21, 42 },
    { -3, -2, -1,  0,  1,  2,  3 }
};

ParamTable<5> kingPPSupportK { 0, 200, useUciParam,
    { 46, 71, 66, 62, 97 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> kingPPSupportP { 1, 64, useUciParam,
    {  0,  3,  4,  9, 15, 22, 32,  0 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<8> pawnDoubledPenalty { 0, 50, useUciParam,
    { 40, 22, 21, 16, 16, 21, 22, 40 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<8> pawnIsolatedPenalty { 0, 50, useUciParam,
    {  1, 10,  7, 11, 11,  7, 10,  1 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<10> halfMoveFactor { 0, 192, useUciParam,
    {128,128,128,128, 38, 24, 16, 11,  6,  3 },
    {  0,  0,  0,  0,  1,  2,  3,  4,  5,  6 }
};

ParamTable<9> stalePawnFactor { 0, 192, useUciParam,
    {114,122,129,129,129,131,105, 65, 22 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

Parameters::Parameters() {
    std::string about = ComputerPlayer::engineName +
                        " by Peter Osterlund, see http://web.comhem.se/petero2home/javachess/index.html#texel";
    addPar(std::make_shared<StringParam>("UCI_EngineAbout", about));

    addPar(UciParams::hash);
    addPar(UciParams::ownBook);
    addPar(UciParams::bookFile);
    addPar(UciParams::ponder);
    addPar(UciParams::analyseMode);
    addPar(UciParams::opponent);
    addPar(UciParams::strength);
    addPar(UciParams::threads);
    addPar(UciParams::multiPV);

    addPar(UciParams::useNullMove);

    addPar(UciParams::gtbPath);
    addPar(UciParams::gtbCache);
    addPar(UciParams::rtbPath);
    addPar(UciParams::minProbeDepth);
    addPar(UciParams::clearHash);

    // Evaluation parameters
    REGISTER_PARAM(pV, "PawnValue");
    REGISTER_PARAM(nV, "KnightValue");
    REGISTER_PARAM(bV, "BishopValue");
    REGISTER_PARAM(rV, "RookValue");
    REGISTER_PARAM(qV, "QueenValue");
    REGISTER_PARAM(kV, "KingValue");

    REGISTER_PARAM(pawnIslandPenalty, "PawnIslandPenalty");
    REGISTER_PARAM(pawnBackwardPenalty, "PawnBackwardPenalty");
    REGISTER_PARAM(pawnSemiBackwardPenalty1, "PawnSemiBackwardPenalty1");
    REGISTER_PARAM(pawnSemiBackwardPenalty2, "PawnSemiBackwardPenalty2");
    REGISTER_PARAM(pawnRaceBonus, "PawnRaceBonus");
    REGISTER_PARAM(passedPawnEGFactor, "PassedPawnEGFactor");
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
    REGISTER_PARAM(pieceTradeBonus, "PieceTradeBonus");
    REGISTER_PARAM(pawnTradeThreshold, "PawnTradeThreshold");
    REGISTER_PARAM(pieceTradeThreshold, "PieceTradeThreshold");

    REGISTER_PARAM(threatBonus1, "ThreatBonus1");
    REGISTER_PARAM(threatBonus2, "ThreatBonus2");
    REGISTER_PARAM(latentAttackBonus, "LatentAttackBonus");

    REGISTER_PARAM(rookHalfOpenBonus, "RookHalfOpenBonus");
    REGISTER_PARAM(rookOpenBonus, "RookOpenBonus");
    REGISTER_PARAM(rookDouble7thRowBonus, "RookDouble7thRowBonus");
    REGISTER_PARAM(trappedRookPenalty1, "TrappedRookPenalty1");
    REGISTER_PARAM(trappedRookPenalty2, "TrappedRookPenalty2");

    REGISTER_PARAM(bishopPairPawnPenalty, "BishopPairPawnPenalty");
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

    REGISTER_PARAM(pawnLoMtrl, "PawnLoMtrl");
    REGISTER_PARAM(pawnHiMtrl, "PawnHiMtrl");
    REGISTER_PARAM(minorLoMtrl, "MinorLoMtrl");
    REGISTER_PARAM(minorHiMtrl, "MinorHiMtrl");
    REGISTER_PARAM(castleLoMtrl, "CastleLoMtrl");
    REGISTER_PARAM(castleHiMtrl, "CastleHiMtrl");
    REGISTER_PARAM(queenLoMtrl, "QueenLoMtrl");
    REGISTER_PARAM(queenHiMtrl, "QueenHiMtrl");
    REGISTER_PARAM(passedPawnLoMtrl, "PassedPawnLoMtrl");
    REGISTER_PARAM(passedPawnHiMtrl, "PassedPawnHiMtrl");
    REGISTER_PARAM(kingSafetyLoMtrl, "KingSafetyLoMtrl");
    REGISTER_PARAM(kingSafetyHiMtrl, "KingSafetyHiMtrl");
    REGISTER_PARAM(oppoBishopLoMtrl, "OppositeBishopLoMtrl");
    REGISTER_PARAM(oppoBishopHiMtrl, "OppositeBishopHiMtrl");
    REGISTER_PARAM(knightOutpostLoMtrl, "KnightOutpostLoMtrl");
    REGISTER_PARAM(knightOutpostHiMtrl, "KnightOutpostHiMtrl");

    // Evaluation tables
    kt1b.registerParams("KingTableMG", *this);
    kt2b.registerParams("KingTableEG", *this);
    pt1b.registerParams("PawnTableMG", *this);
    pt2b.registerParams("PawnTableEG", *this);
    nt1b.registerParams("KnightTableMG", *this);
    nt2b.registerParams("KnightTableEG", *this);
    bt1b.registerParams("BishopTableMG", *this);
    bt2b.registerParams("BishopTableEG", *this);
    qt1b.registerParams("QueenTableMG", *this);
    qt2b.registerParams("QueenTableEG", *this);
    rt1b.registerParams("RookTable", *this);

    knightOutpostBonus.registerParams("KnightOutpostBonus", *this);
    protectedPawnBonus.registerParams("ProtectedPawnBonus", *this);
    attackedPawnBonus.registerParams("AttackedPawnBonus", *this);
    protectBonus.registerParams("ProtectBonus", *this);
    rookMobScore.registerParams("RookMobility", *this);
    bishMobScore.registerParams("BishopMobility", *this);
    knightMobScore.registerParams("KnightMobility", *this);
    queenMobScore.registerParams("QueenMobility", *this);
    majorPieceRedundancy.registerParams("MajorPieceRedundancy", *this);
    connectedPPBonus.registerParams("ConnectedPPBonus", *this);
    passedPawnBonusX.registerParams("PassedPawnBonusX", *this);
    passedPawnBonusY.registerParams("PassedPawnBonusY", *this);
    ppBlockerBonus.registerParams("PassedPawnBlockerBonus", *this);
    candidatePassedBonus.registerParams("CandidatePassedPawnBonus", *this);
    QvsRRBonus.registerParams("QueenVs2RookBonus", *this);
    RvsMBonus.registerParams("RookVsMinorBonus", *this);
    RvsMMBonus.registerParams("RookVs2MinorBonus", *this);
    bishopPairValue.registerParams("BishopPairValue", *this);
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
    std::string name = toLowerCase(p->name);
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
