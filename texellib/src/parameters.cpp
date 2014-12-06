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
    std::shared_ptr<Parameters::CheckParam> ponder(std::make_shared<Parameters::CheckParam>("Ponder", true));
    std::shared_ptr<Parameters::CheckParam> analyseMode(std::make_shared<Parameters::CheckParam>("UCI_AnalyseMode", false));
    std::shared_ptr<Parameters::SpinParam> strength(std::make_shared<Parameters::SpinParam>("Strength", 0, 1000, 1000));
#ifdef __arm__
    std::shared_ptr<Parameters::SpinParam> threads(std::make_shared<Parameters::SpinParam>("Threads", 1, 1, 1));
#else
    std::shared_ptr<Parameters::SpinParam> threads(std::make_shared<Parameters::SpinParam>("Threads", 1, 64, 1));
#endif
    std::shared_ptr<Parameters::SpinParam> multiPV(std::make_shared<Parameters::SpinParam>("MultiPV", 1, 256, 1));

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
    {  63, 126, 112, 100, 100, 112, 126,  63,
      119,   1,  35,  56,  56,  35,   1, 119,
      -56,  -4,  19,  16,  16,  19,  -4, -56,
      -10,   0,   1, -21, -21,   1,   0, -10,
      -55, -14, -34, -23, -23, -34, -14, -55,
      -27,   8,  -5, -28, -28,  -5,   8, -27,
       36,  36,  -1,  -3,  -3,  -1,  36,  36,
        8,  31,   1,  15,  15,   1,  31,   8 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
       29,  30,  31,  32,  32,  31,  30,  29 }
};
ParamTableMirrored<64> kt1w(kt1b);

/** Piece/square table for king during end game. */
ParamTable<64> kt2b { -200, 200, useUciParam,
    { -23,  44,  73,  80,  80,  73,  44, -23,
       31,  91, 106,  96,  96, 106,  91,  31,
       79, 103, 109, 106, 106, 109, 103,  79,
       65,  94, 103, 106, 106, 103,  94,  65,
       51,  76,  90,  95,  95,  90,  76,  51,
       44,  60,  72,  80,  80,  72,  60,  44,
       28,  47,  61,  61,  61,  61,  47,  28,
        0,  21,  30,  13,  13,  30,  21,   0 },
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
      151,  88, 114, 118, 118, 114,  88, 151,
       19,  33,  37,  29,  29,  37,  33,  19,
       -4,  -3, -10,   5,   5, -10,  -3,  -4,
       -7,   0,  -1,   3,   3,  -1,   0,  -7,
       -8,  -5, -20,  -5,  -5, -20,  -5,  -8,
       -5,  -7, -16, -11, -11, -16,  -7,  -5,
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
ParamTableMirrored<64> pt1w(pt1b);

/** Piece/square table for pawns during end game. */
ParamTable<64> pt2b { -200, 200, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      -16,   3,   1, -14, -14,   1,   3, -16,
       33,  33,  22,   8,   8,  22,  33,  33,
       20,  25,  15,   6,   6,  15,  25,  20,
       11,  21,  13,  13,  13,  13,  21,  11,
        3,  17,  18,  22,  22,  18,  17,   3,
        3,  17,  29,  32,  32,  29,  17,   3,
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
    {-254, -21, -47,   2,   2, -47, -21,-254,
      -44, -45,  27,  41,  41,  27, -45, -44,
      -26,  10,  44,  49,  49,  44,  10, -26,
        2,  11,  29,  24,  24,  29,  11,   2,
      -15,  18,  17,  17,  17,  17,  18, -15,
      -46, -10,   0,  15,  15,   0, -10, -46,
      -34, -37, -20,   2,   2, -20, -37, -34,
      -84, -37, -37, -27, -27, -37, -37, -84 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
       29,  30,  31,  32,  32,  31,  30,  29 }
};
ParamTableMirrored<64> nt1w(nt1b);

/** Piece/square table for knights during end game. */
ParamTable<64> nt2b { -200, 200, useUciParam,
    { -83,   7,   6,  -2,  -2,   6,   7, -83,
      -11,   6,  19,  34,  34,  19,   6, -11,
       -8,  17,  33,  41,  41,  33,  17,  -8,
       -4,  28,  43,  42,  42,  43,  28,  -4,
       -3,  25,  41,  41,  41,  41,  25,  -3,
      -26,   6,  20,  32,  32,  20,   6, -26,
      -40,  -5,   2,   2,   2,   2,  -5, -40,
      -48, -56, -18, -14, -14, -18, -56, -48 },
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
    { -55,   4, -15,  -8,  -8, -15,   4, -55,
      -43, -37, -21, -12, -12, -21, -37, -43,
       -1,  21,  13,  17,  17,  13,  21,  -1,
      -14, -12,   2,  17,  17,   2, -12, -14,
       -1, -13, -11,  13,  13, -11, -13,  -1,
      -10,   0,  -2,  -1,  -1,  -2,   0, -10,
       10,   3,   3,  -2,  -2,   3,   3,  10,
      -26,   4, -13, -15, -15, -13,   4, -26 },
     {  1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
       29,  30,  31,  32,  32,  31,  30,  29 }
};
ParamTableMirrored<64> bt1w(bt1b);

/** Piece/square table for bishops during end game. */
ParamTable<64> bt2b { -200, 200, useUciParam,
    {  19,  16,  14,  20,  20,  14,  16,  19,
       16,  19,  24,  29,  29,  24,  19,  16,
       14,  24,  34,  37,  37,  34,  24,  14,
       20,  29,  37,  43,  43,  37,  29,  20,
       20,  29,  37,  43,  43,  37,  29,  20,
       14,  24,  34,  37,  37,  34,  24,  14,
       16,  19,  24,  29,  29,  24,  19,  16,
       19,  16,  14,  20,  20,  14,  16,  19 },
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
    {  15,   3, -15, -25, -25, -15,   3,  15,
      -41,-101, -52, -93, -93, -52,-101, -41,
      -48, -41, -70, -63, -63, -70, -41, -48,
      -46, -41, -53, -66, -66, -53, -41, -46,
      -29, -16, -26, -34, -34, -26, -16, -29,
      -24, -15, -16, -19, -19, -16, -15, -24,
      -30, -18, -11,  -5,  -5, -11, -18, -30,
      -14, -19, -12,  -8,  -8, -12, -19, -14 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
       29,  30,  31,  32,  32,  31,  30,  29 }
};
ParamTableMirrored<64> qt1w(qt1b);

ParamTable<64> qt2b { -200, 200, useUciParam,
    { -17, -20, -19, -17, -17, -19, -20, -17,
      -20, -23, -14,  -8,  -8, -14, -23, -20,
      -19, -14,  -3,  -5,  -5,  -3, -14, -19,
      -17,  -8,  -5,   7,   7,  -5,  -8, -17,
      -17,  -8,  -5,   7,   7,  -5,  -8, -17,
      -19, -14,  -3,  -5,  -5,  -3, -14, -19,
      -20, -23, -14,  -8,  -8, -14, -23, -20,
      -17, -20, -19, -17, -17, -19, -20, -17 },
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

/** Piece/square table for rooks during end game. */
ParamTable<64> rt1b { -200, 200, useUciParam,
    {  45,  46,  48,  39,  39,  48,  46,  45,
       38,  40,  52,  50,  50,  52,  40,  38,
       30,  40,  46,  44,  44,  46,  40,  30,
       18,  24,  27,  27,  27,  27,  24,  18,
       -1,   6,  12,   9,   9,  12,   6,  -1,
      -13,  -2,  -1,   0,   0,  -1,  -2, -13,
      -25, -12,  -4,   1,   1,  -4, -12, -25,
        0,   1,   9,   9,   9,   9,   1,   0 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
        0,  29,  30,  31,  31,  30,  29,   0 }
};
ParamTableMirrored<64> rt1w(rt1b);

ParamTable<64> knightOutpostBonus { 0, 150, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  24,  48,  46,  46,  48,  24,   0,
        0,  13,  32,  48,  48,  32,  13,   0,
        0,   0,  20,  40,  40,  20,   0,   0,
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
      109, 112,  75, 139, 139,  75, 112, 109,
       28,  52,  50,  63,  63,  50,  52,  28,
        6,  11,  17,  17,  17,  17,  11,   6,
        3,   8,   5,   8,   8,   5,   8,   3,
       10,  10,  15,  12,  12,  15,  10,  10,
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
      -31, -56,  15, -16, -16,  15, -56, -31,
       11,  -2,  21,  19,  19,  21,  -2,  11,
       -9,  -4,  -5,  11,  11,  -5,  -4,  -9,
      -37,  11, -13,  14,  14, -13,  11, -37,
     -100, -24, -62, -44, -44, -62, -24,-100,
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
    {  1,  8,  7,  3 },
    {  1,  2,  3,  4 }
};

ParamTable<15> rookMobScore { -50, 50, useUciParam,
    {-17,-11, -6, -2, -1,  5,  9, 12, 17, 20, 23, 26, 27, 24, 30 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-14,-10, -1,  5, 12, 18, 23, 27, 30, 33, 31, 37, 39, 31 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTable<28> knightMobScore { -200, 200, useUciParam,
    {-97,-23, 14,-38,-15,  3,  9,-48,-17, -3, 11, 15,-21,-21,-13, -7,  2,  6,  8,-20,-20,-18,-10, -3,  2,  7,  9,  5 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    { -5, -9,-10, -6, -3,  0,  3,  5,  9, 10, 14, 17, 20, 23, 27, 30, 34, 36, 41, 43, 44, 47, 45, 46, 42, 39, 46, 31 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<36> connectedPPBonus { -200, 300, useUciParam,
    {  -2,  -2,   3,  12,   6, -15,
       -2,   3,   0,   8,   6,   8,
        3,   0,  16,  11,  22,  16,
       12,   8,  11,  39,  -2,  37,
        6,   6,  22,  -2, 102, -29,
      -15,   8,  16,  37, -29, 300 },
    {   1,   2,   4,   7,  11,  16,
        2,   3,   5,   8,  12,  17,
        4,   5,   6,   9,  13,  18,
        7,   8,   9,  10,  14,  19,
       11,  12,  13,  14,  15,  20,
       16,  17,  18,  19,  20,  21 }
};

ParamTable<8> passedPawnBonusX { -200, 200, useUciParam,
    {  0,  2, -3, -6, -6, -3,  2,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

ParamTable<8> passedPawnBonusY { -200, 200, useUciParam,
    {  0,  2,  4, 14, 33, 62,103,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<10> ppBlockerBonus { -50, 50, useUciParam,
    { 23, 26, 10,-11, 46, -1,  3,  1, -1,  7 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1, -2,  5, 13, 36, 17, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -83,   0,   0,
       83,   0,   0,   0,
        0,   0,   0,  85,
        0,   0, -85,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    { -8, 32, 57,118, 72 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    { 10, 33, 42, 32, 22, -3,-60 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-102,-102,-22,  3, 10, 14, 49 },
    {   1,   1,  2,  3,  4,  5,  6 }
};

ParamTable<4> bishopPairValue { 0, 100, useUciParam,
    { 98, 79, 63, 57 },
    {  1,  2,  3,  4 }
};

ParamTable<7> rookEGDrawFactor { 0, 255, useUciParam,
    { 65, 71,110,137,133,158,156 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsBPDrawFactor { 0, 255, useUciParam,
    {128, 94,108,123,127,247,164 },
    {  0,  1,  2,  3,  4,  5,  6 }
};
ParamTable<4> castleFactor { 0, 128, useUciParam,
    { 64, 43, 31, 25 },
    {  1,  2,  3,  4 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 25, 35,-27,  9, 16,-19, -2, -2,-13 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -400, 100, useUciParam,
    {-179,-202,-293, 40, 30, -8,  9, -8,-20 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<10> kingAttackWeight { 0, 200, useUciParam,
    {  0,  2,  1,  8,  8, 16, 30, 58, 69,157 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<5> kingPPSupportK { 0, 200, useUciParam,
    { 47, 70, 65, 56, 98 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> kingPPSupportP { 1, 64, useUciParam,
    {  0,  4,  5, 10, 16, 22, 32,  0 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<8> pawnDoubledPenalty { 0, 50, useUciParam,
    { 41, 20, 17, 12, 12, 17, 20, 41 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<8> pawnIsolatedPenalty { 0, 50, useUciParam,
    {  2, 12, 10, 14, 14, 10, 12,  2 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<10> halfMoveFactor { 0, 192, useUciParam,
    {128,128,128,128, 43, 27, 19, 13,  8,  5 },
    {  0,  0,  0,  0,  1,  2,  3,  4,  5,  6 }
};

ParamTable<9> stalePawnFactor { 0, 192, useUciParam,
    {120,127,131,133,137,129,104, 65, 29 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

Parameters::Parameters() {
    std::string about = ComputerPlayer::engineName +
                        " by Peter Osterlund, see http://web.comhem.se/petero2home/javachess/index.html#texel";
    addPar(std::make_shared<StringParam>("UCI_EngineAbout", about));

    addPar(UciParams::hash);
    addPar(UciParams::ownBook);
    addPar(UciParams::ponder);
    addPar(UciParams::analyseMode);
    addPar(UciParams::strength);
    addPar(UciParams::threads);
    addPar(UciParams::multiPV);

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
