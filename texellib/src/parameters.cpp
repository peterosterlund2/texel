/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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

int pieceValue[Piece::nPieceTypes];

DEFINE_PARAM_2REF(pV, pieceValue[Piece::WPAWN]  , pieceValue[Piece::BPAWN]);
DEFINE_PARAM_2REF(nV, pieceValue[Piece::WKNIGHT], pieceValue[Piece::BKNIGHT]);
DEFINE_PARAM_2REF(bV, pieceValue[Piece::WBISHOP], pieceValue[Piece::BBISHOP]);
DEFINE_PARAM_2REF(rV, pieceValue[Piece::WROOK]  , pieceValue[Piece::BROOK]);
DEFINE_PARAM_2REF(qV, pieceValue[Piece::WQUEEN] , pieceValue[Piece::BQUEEN]);
DEFINE_PARAM_2REF(kV, pieceValue[Piece::WKING]  , pieceValue[Piece::BKING]);

DEFINE_PARAM(pawnIslandPenalty);
DEFINE_PARAM(pawnBackwardPenalty);
DEFINE_PARAM(pawnGuardedPassedBonus);
DEFINE_PARAM(pawnRaceBonus);
DEFINE_PARAM(passedPawnEGFactor);

DEFINE_PARAM(QvsRMBonus1);
DEFINE_PARAM(QvsRMBonus2);
DEFINE_PARAM(knightVsQueenBonus1);
DEFINE_PARAM(knightVsQueenBonus2);
DEFINE_PARAM(knightVsQueenBonus3);
DEFINE_PARAM(krkpBonus);

DEFINE_PARAM(pawnTradePenalty);
DEFINE_PARAM(pieceTradeBonus);
DEFINE_PARAM(pawnTradeThreshold);
DEFINE_PARAM(pieceTradeThreshold);

DEFINE_PARAM(threatBonus1);
DEFINE_PARAM(threatBonus2);

DEFINE_PARAM(rookHalfOpenBonus);
DEFINE_PARAM(rookOpenBonus);
DEFINE_PARAM(rookDouble7thRowBonus);
DEFINE_PARAM(trappedRookPenalty);

DEFINE_PARAM(bishopPairPawnPenalty);
DEFINE_PARAM(trappedBishopPenalty1);
DEFINE_PARAM(trappedBishopPenalty2);
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
    {  72, 135, 122, 107, 107, 122, 135,  72,
      165, -47,  22,  82,  82,  22, -47, 165,
      -76,  -9,  28,  17,  17,  28,  -9, -76,
      -18,  31,  18,  -1,  -1,  18,  31, -18,
      -15,  27,   5,  22,  22,   5,  27, -15,
      -21,   7,  16,   4,   4,  16,   7, -21,
       27,  22,  -3,  10,  10,  -3,  22,  27,
        8,  32,   2,  17,  17,   2,  32,   8 },
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
    { -31,  51,  73,  70,  70,  73,  51, -31,
       47,  93,  94,  75,  75,  94,  93,  47,
       83,  99, 104,  91,  91, 104,  99,  83,
       67,  84,  90,  92,  92,  90,  84,  67,
       53,  66,  78,  82,  82,  78,  66,  53,
       52,  60,  66,  71,  71,  66,  60,  52,
       32,  51,  56,  53,  53,  56,  51,  32,
        0,  23,  27,  17,  17,  27,  23,   0 },
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
      188, 109,  89, 138, 138,  89, 109, 188,
       36,  35,  43,  52,  52,  43,  35,  36,
       13,  -8,   3,  18,  18,   3,  -8,  13,
        0,  -5,   5,  11,  11,   5,  -5,   0,
        0,  -8,  -7,   9,   9,  -7,  -8,   0,
        0,  -9, -13, -11, -11, -13,  -9,   0,
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
      -49, -32, -24, -29, -29, -24, -32, -49,
       25,  26,  35,  28,  28,  35,  26,  25,
       21,  20,  23,  17,  17,  23,  20,  21,
        6,  16,  15,  14,  14,  15,  16,   6,
        1,  11,  17,  26,  26,  17,  11,   1,
        0,  12,  23,  38,  38,  23,  12,   0,
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
    {-208, -14, -31, -18, -18, -31, -14,-208,
      -12, -44,  16,  49,  49,  16, -44, -12,
      -20,   0,  40,  48,  48,  40,   0, -20,
        3,  -1,  32,  21,  21,  32,  -1,   3,
      -14,   6,  24,  12,  12,  24,   6, -14,
      -44, -14,   0,  21,  21,   0, -14, -44,
      -31, -33, -18,   4,   4, -18, -33, -31,
      -56, -39, -46, -26, -26, -46, -39, -56 },
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
    { -56,  -5,   2,   1,   1,   2,  -5, -56,
      -12,  -4,  15,  29,  29,  15,  -4, -12,
      -10,  10,  27,  28,  28,  27,  10, -10,
       -6,  14,  34,  37,  37,  34,  14,  -6,
      -10,  14,  33,  38,  38,  33,  14, -10,
      -20,  -2,   8,  29,  29,   8,  -2, -20,
      -38, -17,   6,  -9,  -9,   6, -17, -38,
      -68, -46, -23, -29, -29, -23, -46, -68 },
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
    { -34,   7, -16, -17, -17, -16,   7, -34,
      -32, -39, -14, -33, -33, -14, -39, -32,
        4,  36,  22,  18,  18,  22,  36,   4,
      -23, -16,  16,  27,  27,  16, -16, -23,
       -4,  -5, -10,  17,  17, -10,  -5,  -4,
       -6,   5,   0,   0,   0,   0,   5,  -6,
       -7,  11,   4,  -3,  -3,   4,  11,  -7,
      -31,   6, -12, -15, -15, -12,   6, -31 },
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
    {   2,   5,   7,  12,  12,   7,   5,   2,
        5,   8,  17,  20,  20,  17,   8,   5,
        7,  17,  25,  31,  31,  25,  17,   7,
       12,  20,  31,  39,  39,  31,  20,  12,
       12,  20,  31,  39,  39,  31,  20,  12,
        7,  17,  25,  31,  31,  25,  17,   7,
        5,   8,  17,  20,  20,  17,   8,   5,
        2,   5,   7,  12,  12,   7,   5,   2 },
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
    {  11,  14,  27, -17, -17,  27,  14,  11,
      -32,-117, -53,-101,-101, -53,-117, -32,
      -50, -39, -69, -67, -67, -69, -39, -50,
      -45, -50, -59, -74, -74, -59, -50, -45,
      -31, -15, -30, -37, -37, -30, -15, -31,
      -34, -19, -18, -20, -20, -18, -19, -34,
      -24, -17, -11,  -5,  -5, -11, -17, -24,
      -27, -22, -10,  -7,  -7, -10, -22, -27 },
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
    { -24, -20, -25, -15, -15, -25, -20, -24,
      -20, -25, -14,  -8,  -8, -14, -25, -20,
      -25, -14,  -1,  -7,  -7,  -1, -14, -25,
      -15,  -8,  -7,   7,   7,  -7,  -8, -15,
      -15,  -8,  -7,   7,   7,  -7,  -8, -15,
      -25, -14,  -1,  -7,  -7,  -1, -14, -25,
      -20, -25, -14,  -8,  -8, -14, -25, -20,
      -24, -20, -25, -15, -15, -25, -20, -24 },
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
    {  48,  42,  42,  45,  45,  42,  42,  48,
       46,  54,  56,  54,  54,  56,  54,  46,
       30,  52,  47,  51,  51,  47,  52,  30,
       18,  24,  30,  33,  33,  30,  24,  18,
       -1,   6,  18,   6,   6,  18,   6,  -1,
      -14,  -3,   2,   1,   1,   2,  -3, -14,
      -21, -12,  -9,   1,   1,  -9, -12, -21,
        0,   2,   9,   9,   9,   9,   2,   0 },
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
        0,  22,  48,  48,  48,  48,  22,   0,
        0,  17,  40,  40,  40,  40,  17,   0,
        0,   0,  10,  29,  29,  10,   0,   0,
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

ParamTable<15> rookMobScore { -50, 50, useUciParam,
    {-14,-12, -5, -3, -2,  6,  8, 13, 17, 23, 25, 28, 27, 26, 31 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-18,-14,  0,  6, 13, 17, 22, 26, 27, 32, 29, 34, 36, 30 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTableEv<28> knightMobScore { -200, 200, useUciParam,
    {-41,-18, 13,-45,-13, -2,  2,-45,-21, -6,  8, 13,-23,-23,-18, -7,  3,  9, 11,-28,-28,-20,-13, -5,  1,  5,  7,  3 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    {  1,  2, -7, -5, -2, -1,  2,  3,  7,  9, 13, 16, 19, 19, 25, 29, 33, 35, 39, 37, 44, 42, 41, 44, 37, 29, 33, 41 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -71,   0,   0,
       71,   0,   0,   0,
        0,   0,   0,  80,
        0,   0, -80,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<8> passedPawnBonusX { -200, 200, useUciParam,
    {  0,  3, -4, -7, -7, -4,  3,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

ParamTable<8> passedPawnBonusY { -200, 200, useUciParam,
    {  0,  6,  4, 15, 30, 53, 99,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<5> ppBlockerBonus { -50, 50, useUciParam,
    { 12, 11, -1, -4, 14 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1, -4, -2, 20, 37, 63, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    {  2, -5, 38, 77, 81 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    {-13, 18, 29, 19, 30, 47,-81 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-108,-108,-48,  1,-12, 10, 12 },
    {   1,   1,  2,  3,  4,  5,  6 }
};

ParamTable<4> bishopPairValue { 0, 100, useUciParam,
    { 60, 68, 59, 54 },
    {  1,  2,  3,  4 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 29, 30,-34, 15, 20,-19,  6,  0,-15 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -300, 100, useUciParam,
    {-95,-174,-249, 49, 14,-57, 14,-20,-46 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<10> kingAttackWeight { 0, 200, useUciParam,
    {  0,  2,  1,  4,  8, 21, 31, 54, 65,150 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<5> kingPPSupportK { 0, 200, useUciParam,
    { 45, 69, 61, 56, 97 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> kingPPSupportP { 1, 64, useUciParam,
    {  0,  4,  6, 11, 17, 20, 32,  0 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<8> pawnDoubledPenalty { 0, 50, useUciParam,
    { 33, 18, 21, 20, 20, 21, 18, 33 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<8> pawnIsolatedPenalty { 0, 50, useUciParam,
    {  5, 13, 12, 18, 18, 12, 13,  5 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};


Parameters::Parameters() {
    addPar(std::make_shared<SpinParam>("Hash", 1, 524288, 16));
    addPar(std::make_shared<CheckParam>("OwnBook", false));
    addPar(std::make_shared<CheckParam>("Ponder", true));
    addPar(std::make_shared<CheckParam>("UCI_AnalyseMode", false));
    std::string about = ComputerPlayer::engineName +
                        " by Peter Osterlund, see http://web.comhem.se/petero2home/javachess/index.html#texel";
    addPar(std::make_shared<StringParam>("UCI_EngineAbout", about));
    addPar(std::make_shared<SpinParam>("Strength", 0, 1000, 1000));
#ifdef __arm__
    addPar(std::make_shared<SpinParam>("Threads", 1, 1, 1));
#else
    addPar(std::make_shared<SpinParam>("Threads", 1, 64, 1));
#endif
    addPar(std::make_shared<SpinParam>("MultiPV", 1, 256, 1));

    // Evaluation parameters
    REGISTER_PARAM(pV, "PawnValue");
    REGISTER_PARAM(nV, "KnightValue");
    REGISTER_PARAM(bV, "BishopValue");
    REGISTER_PARAM(rV, "RookValue");
    REGISTER_PARAM(qV, "QueenValue");
    REGISTER_PARAM(kV, "KingValue");

    REGISTER_PARAM(pawnIslandPenalty, "PawnIslandPenalty");
    REGISTER_PARAM(pawnBackwardPenalty, "PawnBackwardPenalty");
    REGISTER_PARAM(pawnGuardedPassedBonus, "PawnGuardedPassedBonus");
    REGISTER_PARAM(pawnRaceBonus, "PawnRaceBonus");
    REGISTER_PARAM(passedPawnEGFactor, "PassedPawnEGFactor");

    REGISTER_PARAM(QvsRMBonus1, "QueenVsRookMinorBonus1");
    REGISTER_PARAM(QvsRMBonus2, "QueenVsRookMinorBonus2");
    REGISTER_PARAM(knightVsQueenBonus1, "KnightVsQueenBonus1");
    REGISTER_PARAM(knightVsQueenBonus2, "KnightVsQueenBonus2");
    REGISTER_PARAM(knightVsQueenBonus3, "KnightVsQueenBonus3");
    REGISTER_PARAM(krkpBonus, "RookVsPawnBonus");

    REGISTER_PARAM(pawnTradePenalty, "PawnTradePenalty");
    REGISTER_PARAM(pieceTradeBonus, "PieceTradeBonus");
    REGISTER_PARAM(pawnTradeThreshold, "PawnTradeThreshold");
    REGISTER_PARAM(pieceTradeThreshold, "PieceTradeThreshold");

    REGISTER_PARAM(threatBonus1, "ThreatBonus1");
    REGISTER_PARAM(threatBonus2, "ThreatBonus2");

    REGISTER_PARAM(rookHalfOpenBonus, "RookHalfOpenBonus");
    REGISTER_PARAM(rookOpenBonus, "RookOpenBonus");
    REGISTER_PARAM(rookDouble7thRowBonus, "RookDouble7thRowBonus");
    REGISTER_PARAM(trappedRookPenalty, "TrappedRookPenalty");

    REGISTER_PARAM(bishopPairPawnPenalty, "BishopPairPawnPenalty");
    REGISTER_PARAM(trappedBishopPenalty1, "TrappedBishopPenalty1");
    REGISTER_PARAM(trappedBishopPenalty2, "TrappedBishopPenalty2");
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
    rookMobScore.registerParams("RookMobility", *this);
    bishMobScore.registerParams("BishopMobility", *this);
    knightMobScore.registerParams("KnightMobility", *this);
    queenMobScore.registerParams("QueenMobility", *this);
    majorPieceRedundancy.registerParams("MajorPieceRedundancy", *this);
    passedPawnBonusX.registerParams("PassedPawnBonusX", *this);
    passedPawnBonusY.registerParams("PassedPawnBonusY", *this);
    ppBlockerBonus.registerParams("PassedPawnBlockerBonus", *this);
    candidatePassedBonus.registerParams("CandidatePassedPawnBonus", *this);
    QvsRRBonus.registerParams("QueenVs2RookBonus", *this);
    RvsMBonus.registerParams("RookVsMinorBonus", *this);
    RvsMMBonus.registerParams("RookVs2MinorBonus", *this);
    bishopPairValue.registerParams("BishopPairValue", *this);
    pawnShelterTable.registerParams("PawnShelterTable", *this);
    pawnStormTable.registerParams("PawnStormTable", *this);
    kingAttackWeight.registerParams("KingAttackWeight", *this);
    kingPPSupportK.registerParams("KingPassedPawnSupportK", *this);
    kingPPSupportP.registerParams("KingPassedPawnSupportP", *this);
    pawnDoubledPenalty.registerParams("PawnDoubledPenalty", *this);
    pawnIsolatedPenalty.registerParams("PawnIsolatedPenalty", *this);

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
        params[p.first] = std::make_shared<TableSpinParam>(pName, *this, p.second);
        pars.addPar(params[p.first]);
    }
}

void
ParamTableBase::modifiedN(int* table, int* parNo, int N) {
    for (int i = 0; i < N; i++)
        if (parNo[i] > 0)
            table[i] = params[parNo[i]]->getIntPar();
        else if (parNo[i] < 0)
            table[i] = -params[-parNo[i]]->getIntPar();
    for (auto d : dependent)
        d->modified();
}
