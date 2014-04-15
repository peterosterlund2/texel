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
DEFINE_PARAM(krpkbBonus);
DEFINE_PARAM(krpkbPenalty);
DEFINE_PARAM(krpknBonus);

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
    {  68, 130, 116, 108, 108, 116, 130,  68,
      132, -20,  32,  66,  66,  32, -20, 132,
      -67,  -8,  20,  18,  18,  20,  -8, -67,
      -12,   5,   0,   0,   0,   0,   5, -12,
      -38,  12, -16,  -1,  -1, -16,  12, -38,
      -32,   1,   7, -13, -13,   7,   1, -32,
       29,  24,   1,   7,   7,   1,  24,  29,
        6,  32,   3,  15,  15,   3,  32,   6 },
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
    { -27,  50,  66,  52,  52,  66,  50, -27,
       44,  89,  92,  80,  80,  92,  89,  44,
       82,  97, 102,  96,  96, 102,  97,  82,
       66,  89,  94,  94,  94,  94,  89,  66,
       55,  72,  84,  88,  88,  84,  72,  55,
       51,  61,  68,  75,  75,  68,  61,  51,
       32,  51,  56,  55,  55,  56,  51,  32,
        0,  22,  26,  16,  16,  26,  22,   0 },
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
      161,  92, 110, 134, 134, 110,  92, 161,
       27,  35,  32,  38,  38,  32,  35,  27,
        6,  -2,  -7,  11,  11,  -7,  -2,   6,
       -1,  -4,   1,   6,   6,   1,  -4,  -1,
       -7,  -6, -16,  -2,  -2, -16,  -6,  -7,
        0,  -7, -15,  -9,  -9, -15,  -7,   0,
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
      -54, -36, -34, -36, -36, -34, -36, -54,
       25,  25,  21,  12,  12,  21,  25,  25,
       21,  22,  16,  11,  11,  16,  22,  21,
        9,  21,  15,  14,  14,  15,  21,   9,
        2,  15,  16,  25,  25,  16,  15,   2,
        0,  15,  25,  36,  36,  25,  15,   0,
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
    {-256, -15, -47,  -5,  -5, -47, -15,-256,
      -27, -44,  19,  45,  45,  19, -44, -27,
      -26,   8,  40,  47,  47,  40,   8, -26,
        1,   1,  32,  22,  22,  32,   1,   1,
      -12,  14,  20,  13,  13,  20,  14, -12,
      -47, -13,   0,  16,  16,   0, -13, -47,
      -39, -32, -16,   2,   2, -16, -32, -39,
      -58, -39, -44, -24, -24, -44, -39, -58 },
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
    { -44,  -6,   1,  -4,  -4,   1,  -6, -44,
      -16,  -4,  16,  27,  27,  16,  -4, -16,
      -12,  15,  30,  33,  33,  30,  15, -12,
       -7,  22,  34,  39,  39,  34,  22,  -7,
      -10,  17,  34,  42,  42,  34,  17, -10,
      -21,   2,  14,  27,  27,  14,   2, -21,
      -40, -16,   5,  -4,  -4,   5, -16, -40,
      -71, -46, -22, -25, -25, -22, -46, -71 },
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
    { -12,  -7, -17, -16, -16, -17,  -7, -12,
      -37, -42,  -6, -26, -26,  -6, -42, -37,
        3,  21,  27,  15,  15,  27,  21,   3,
      -22, -13,   8,  27,  27,   8, -13, -22,
        2,  -6, -10,  13,  13, -10,  -6,   2,
       -6,   4,   1,  -1,  -1,   1,   4,  -6,
       -3,  10,   0,  -1,  -1,   0,  10,  -3,
      -30,  -2, -13, -11, -11, -13,  -2, -30 },
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
    {   2,   8,   9,  14,  14,   9,   8,   2,
        8,   7,  22,  24,  24,  22,   7,   8,
        9,  22,  28,  37,  37,  28,  22,   9,
       14,  24,  37,  43,  43,  37,  24,  14,
       14,  24,  37,  43,  43,  37,  24,  14,
        9,  22,  28,  37,  37,  28,  22,   9,
        8,   7,  22,  24,  24,  22,   7,   8,
        2,   8,   9,  14,  14,   9,   8,   2 },
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
    {  13,  11,  18, -27, -27,  18,  11,  13,
      -45,-111, -53, -97, -97, -53,-111, -45,
      -49, -42, -72, -68, -68, -72, -42, -49,
      -54, -50, -62, -69, -69, -62, -50, -54,
      -34, -15, -28, -35, -35, -28, -15, -34,
      -32, -19, -18, -20, -20, -18, -19, -32,
      -24, -13, -11,  -5,  -5, -11, -13, -24,
      -12, -23, -12,  -9,  -9, -12, -23, -12 },
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
    { -21, -20, -25, -16, -16, -25, -20, -21,
      -20, -25, -14,  -8,  -8, -14, -25, -20,
      -25, -14,  -1,  -5,  -5,  -1, -14, -25,
      -16,  -8,  -5,   7,   7,  -5,  -8, -16,
      -16,  -8,  -5,   7,   7,  -5,  -8, -16,
      -25, -14,  -1,  -5,  -5,  -1, -14, -25,
      -20, -25, -14,  -8,  -8, -14, -25, -20,
      -21, -20, -25, -16, -16, -25, -20, -21 },
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
    {  48,  42,  48,  42,  42,  48,  42,  48,
       46,  51,  57,  54,  54,  57,  51,  46,
       30,  49,  50,  51,  51,  50,  49,  30,
       18,  24,  33,  32,  32,  33,  24,  18,
       -2,   6,  15,   6,   6,  15,   6,  -2,
      -16,  -1,   0,   1,   1,   0,  -1, -16,
      -24,  -9,  -9,   0,   0,  -9,  -9, -24,
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
        0,  19,  43,  45,  45,  43,  19,   0,
        0,  17,  39,  45,  45,  39,  17,   0,
        0,   0,  18,  29,  29,  18,   0,   0,
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
      101,  79,  69,  84,  84,  69,  79, 101,
       18,  17,  42,  50,  50,  42,  17,  18,
        9,   8,  19,  16,  16,  19,   8,   9,
        1,   6,   5,   7,   7,   5,   6,   1,
       10,   7,  15,  11,  11,  15,   7,  10,
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
      -14, -44,  13, -17, -17,  13, -44, -14,
        1, -12,   5,  20,  20,   5, -12,   1,
       -7,  -7,  -4,  10,  10,  -4,  -7,  -7,
      -35,  -6, -14,  17,  17, -14,  -6, -35,
     -100, -24, -49, -50, -50, -49, -24,-100,
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

ParamTable<15> rookMobScore { -50, 50, useUciParam,
    {-19,-12, -6, -3, -1,  6,  9, 13, 17, 22, 25, 27, 26, 25, 29 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-16,-11, -1,  5, 13, 18, 22, 26, 28, 32, 29, 32, 37, 30 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTableEv<28> knightMobScore { -200, 200, useUciParam,
    {-30,-28, 17,-35,-11,  0,  6,-49,-18, -5,  9, 15,-22,-22,-16, -8,  1,  7, 10,-31,-31,-20,-12, -5,  1,  6,  9,  3 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    {  5, -1, -6, -4, -2,  0,  2,  3,  7, 10, 14, 17, 19, 22, 25, 29, 32, 33, 36, 37, 43, 41, 42, 40, 40, 34, 36, 36 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<36> connectedPPBonus { -200, 300, useUciParam,
    {   6,  -6,   2,   8,   2,   2,
       -6,  -2,  -3,   9,  13,  10,
        2,  -3,  22,   4,  22,  17,
        8,   9,   4,  41,  -3,  40,
        2,  13,  22,  -3,  87, -13,
        2,  10,  17,  40, -13, 266 },
    {   1,   2,   4,   7,  11,  16,
        2,   3,   5,   8,  12,  17,
        4,   5,   6,   9,  13,  18,
        7,   8,   9,  10,  14,  19,
       11,  12,  13,  14,  15,  20,
       16,  17,  18,  19,  20,  21 }
};

ParamTable<8> passedPawnBonusX { -200, 200, useUciParam,
    {  0,  2, -3, -7, -7, -3,  2,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

ParamTable<8> passedPawnBonusY { -200, 200, useUciParam,
    {  0,  4,  3, 11, 27, 53,101,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<5> ppBlockerBonus { -50, 50, useUciParam,
    { 12, 11, -4, -5, 17 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1, -4,  3, 18, 41, 26, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -77,   0,   0,
       77,   0,   0,   0,
        0,   0,   0,  80,
        0,   0, -80,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    { -9,-20, 40, 74, 80 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    {  2, 20, 34, 24, 30,  1,-76 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-107,-107,-34,  1, -7,  6,  2 },
    {   1,   1,  2,  3,  4,  5,  6 }
};

ParamTable<4> bishopPairValue { 0, 100, useUciParam,
    { 65, 75, 60, 54 },
    {  1,  2,  3,  4 }
};

ParamTable<7> rookEGDrawFactor { 0, 255, useUciParam,
    { 73, 72,110,136,129,157,151 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 32, 29,-30, 12, 15,-16,  0, -4,-22 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -400, 100, useUciParam,
    {-128,-201,-306, 42, 40,-27, 15,-17,-27 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<10> kingAttackWeight { 0, 200, useUciParam,
    {  0,  1,  0,  6,  8, 18, 32, 56, 69,152 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<5> kingPPSupportK { 0, 200, useUciParam,
    { 47, 71, 64, 55, 97 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> kingPPSupportP { 1, 64, useUciParam,
    {  0,  4,  6, 10, 16, 22, 32,  0 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<8> pawnDoubledPenalty { 0, 50, useUciParam,
    { 36, 18, 18, 19, 19, 18, 18, 36 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<8> pawnIsolatedPenalty { 0, 50, useUciParam,
    {  2, 10,  9, 13, 13,  9, 10,  2 },
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
    REGISTER_PARAM(krpkbBonus, "RookPawnVsBishopBonus");
    REGISTER_PARAM(krpkbPenalty, "RookPawnVsBishopPenalty");
    REGISTER_PARAM(krpknBonus, "RookPawnVsKnightBonus");

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
