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

DEFINE_PARAM(pawnDoubledPenalty);
DEFINE_PARAM(pawnIslandPenalty);
DEFINE_PARAM(pawnIsolatedPenalty);
DEFINE_PARAM(pawnBackwardPenalty);
DEFINE_PARAM(pawnGuardedPassedBonus);
DEFINE_PARAM(pawnRaceBonus);

DEFINE_PARAM(QvsRMBonus1);
DEFINE_PARAM(QvsRMBonus2);
DEFINE_PARAM(knightVsQueenBonus1);
DEFINE_PARAM(knightVsQueenBonus2);
DEFINE_PARAM(knightVsQueenBonus3);

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

DEFINE_PARAM(kingAttackWeight);
DEFINE_PARAM(kingSafetyHalfOpenBCDEFG);
DEFINE_PARAM(kingSafetyHalfOpenAH);
DEFINE_PARAM(kingSafetyWeight);
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
    {   8, 199, 191, 200, 200, 191, 199,   8,
      199, 167, 189, 138, 138, 189, 167, 199,
      118, 188, 171,  95,  95, 171, 188, 118,
      128, 130,  83,  33,  33,  83, 130, 128,
       25,  46,  13,  21,  21,  13,  46,  25,
        9,  -1,   4,  -3,  -3,   4,  -1,   9,
       21,   9, -20,  -6,  -6, -20,   9,  21,
        0,  19, -16,  -1,  -1, -16,  19,   0 },
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
    {   0,  10,  20,  17,  17,  20,  10,   0,
       10,  35,  44,  43,  43,  44,  35,  10,
       20,  44,  53,  58,  58,  53,  44,  20,
       17,  43,  58,  66,  66,  58,  43,  17,
       17,  43,  58,  66,  66,  58,  43,  17,
       20,  44,  53,  58,  58,  53,  44,  20,
       10,  35,  44,  43,  43,  44,  35,  10,
        0,  10,  20,  17,  17,  20,  10,   0 },
    {   0,   1,   2,   3,   3,   2,   1,   0,
        1,   4,   5,   6,   6,   5,   4,   1,
        2,   5,   7,   8,   8,   7,   5,   2,
        3,   6,   8,   9,   9,   8,   6,   3,
        3,   6,   8,   9,   9,   8,   6,   3,
        2,   5,   7,   8,   8,   7,   5,   2,
        1,   4,   5,   6,   6,   5,   4,   1,
        0,   1,   2,   3,   3,   2,   1,   0 }
};
ParamTableMirrored<64> kt2w(kt2b);

/** Piece/square table for pawns during middle game. */
ParamTable<64> pt1b { -200, 300, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      173, 102,  90, 122, 122,  90, 102, 173,
       33,  32,  40,  43,  43,  40,  32,  33,
       12,  -7,   6,  21,  21,   6,  -7,  12,
       -4,  -5,   8,  12,  12,   8,  -5,  -4,
       -2, -10,  -2,   9,   9,  -2, -10,  -2,
       -5, -11, -11, -14, -14, -11, -11,  -5,
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
      -34, -17, -19, -34, -34, -19, -17, -34,
       31,  29,  31,   9,   9,  31,  29,  31,
       27,  22,  20,   7,   7,  20,  22,  27,
       12,  16,  11,   5,   5,  11,  16,  12,
        6,   8,  12,  15,  15,  12,   8,   6,
        6,   9,  18,  26,  26,  18,   9,   6,
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
    {-220, -13, -31, -17, -17, -31, -13,-220,
      -16, -42,  19,  51,  51,  19, -42, -16,
      -14,  11,  45,  52,  52,  45,  11, -14,
       10,   5,  32,  20,  20,  32,   5,  10,
      -19,   5,  22,   9,   9,  22,   5, -19,
      -43, -13,   0,  21,  21,   0, -13, -43,
      -34, -33, -20,   1,   1, -20, -33, -34,
      -70, -41, -42, -30, -30, -42, -41, -70 },
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
    { -57,  -9,   7,   7,   7,   7,  -9, -57,
      -17,  -1,  14,  31,  31,  14,  -1, -17,
       -2,  12,  25,  26,  26,  25,  12,  -2,
       -3,  18,  34,  36,  36,  34,  18,  -3,
      -11,  17,  31,  37,  37,  31,  17, -11,
      -19,   2,   8,  29,  29,   8,   2, -19,
      -39, -17,   6,  -7,  -7,   6, -17, -39,
      -59, -54, -19, -24, -24, -19, -54, -59 },
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
    { -36,   8, -11, -13, -13, -11,   8, -36,
      -31, -41, -15, -33, -33, -15, -41, -31,
        6,  42,  22,  18,  18,  22,  42,   6,
      -18, -17,  17,  29,  29,  17, -17, -18,
       -5,  -2,  -9,  17,  17,  -9,  -2,  -5,
       -3,   7,   1,   3,   3,   1,   7,  -3,
       -7,  14,   6,  -1,  -1,   6,  14,  -7,
      -28,   7,  -9, -11, -11,  -9,   7, -28 },
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
    {   2,   6,   4,  11,  11,   4,   6,   2,
        6,   5,  16,  19,  19,  16,   5,   6,
        4,  16,  25,  29,  29,  25,  16,   4,
       11,  19,  29,  37,  37,  29,  19,  11,
       11,  19,  29,  37,  37,  29,  19,  11,
        4,  16,  25,  29,  29,  25,  16,   4,
        6,   5,  16,  19,  19,  16,   5,   6,
        2,   6,   4,  11,  11,   4,   6,   2 },
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
    {  14,  13,  29,   2,   2,  29,  13,  14,
      -26,-126, -49,-100,-100, -49,-126, -26,
      -25, -14, -55, -54, -54, -55, -14, -25,
      -30, -41, -54, -72, -72, -54, -41, -30,
      -31, -17, -33, -43, -43, -33, -17, -31,
      -36, -21, -23, -24, -24, -23, -21, -36,
      -27, -17, -11,  -6,  -6, -11, -17, -27,
      -26, -24, -12,  -5,  -5, -12, -24, -26 },
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
    { -23, -21, -23, -13, -13, -23, -21, -23,
      -21, -28, -14,  -7,  -7, -14, -28, -21,
      -23, -14,  -1,  -7,  -7,  -1, -14, -23,
      -13,  -7,  -7,   6,   6,  -7,  -7, -13,
      -13,  -7,  -7,   6,   6,  -7,  -7, -13,
      -23, -14,  -1,  -7,  -7,  -1, -14, -23,
      -21, -28, -14,  -7,  -7, -14, -28, -21,
      -23, -21, -23, -13, -13, -23, -21, -23 },
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
    {  48,  39,  39,  39,  39,  39,  39,  48,
       46,  51,  53,  53,  53,  53,  51,  46,
       30,  51,  45,  47,  47,  45,  51,  30,
       12,  24,  30,  31,  31,  30,  24,  12,
       -2,   6,  15,   6,   6,  15,   6,  -2,
      -16,  -3,  -2,  -3,  -3,  -2,  -3, -16,
      -24, -12, -11,  -2,  -2, -11, -12, -24,
        0,   1,   8,   7,   7,   8,   1,   0 },
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
        0,  19,  48,  48,  48,  48,  19,   0,
        0,  14,  40,  40,  40,  40,  14,   0,
        0,   0,  11,  30,  30,  11,   0,   0,
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
    {-13,-13, -5, -3, -3,  5,  7, 12, 16, 22, 24, 27, 26, 24, 32 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-17,-13,  0,  5, 12, 17, 21, 25, 27, 31, 28, 35, 33, 29 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    { -2,  0, -9, -6, -3, -1,  2,  2,  6,  8, 11, 14, 17, 18, 24, 27, 31, 33, 38, 35, 45, 41, 41, 46, 40, 30, 31, 49 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -68,   0,   0,
       68,   0,   0,   0,
        0,   0,   0,  79,
        0,   0, -79,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<8> passedPawnBonus { -200, 200, useUciParam,
    { -1,  6,  4, 14, 29, 54, 95, -1 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1, -2, -1, 20, 38, 65, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    {  4, -5, 38, 81, 81 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    {-10, 17, 28, 17, 29, 37,-87 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-98,-98,-51, -1,-14,  7,  4 },
    {  1,  1,  2,  3,  4,  5,  6 }
};

ParamTable<4> bishopPairValue { 0, 100, useUciParam,
    { 65, 68, 59, 54 },
    {  1,  2,  3,  4 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 26, 25,-33, 10, 15,-18, 12,  1, -7 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -300, 100, useUciParam,
    {-98,-178,-222, 44,  7,-67, 17,-18,-41 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
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

    REGISTER_PARAM(pawnDoubledPenalty, "PawnDoubledPenalty");
    REGISTER_PARAM(pawnIslandPenalty, "PawnIslandPenalty");
    REGISTER_PARAM(pawnIsolatedPenalty, "PawnIsolatedPenalty");
    REGISTER_PARAM(pawnBackwardPenalty, "PawnBackwardPenalty");
    REGISTER_PARAM(pawnGuardedPassedBonus, "PawnGuardedPassedBonus");
    REGISTER_PARAM(pawnRaceBonus, "PawnRaceBonus");

    REGISTER_PARAM(QvsRMBonus1, "QueenVsRookMinorBonus1");
    REGISTER_PARAM(QvsRMBonus2, "QueenVsRookMinorBonus2");
    REGISTER_PARAM(knightVsQueenBonus1, "KnightVsQueenBonus1");
    REGISTER_PARAM(knightVsQueenBonus2, "KnightVsQueenBonus2");
    REGISTER_PARAM(knightVsQueenBonus3, "KnightVsQueenBonus3");

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

    REGISTER_PARAM(kingAttackWeight, "KingAttackWeight");
    REGISTER_PARAM(kingSafetyHalfOpenBCDEFG, "KingSafetyHalfOpenBCDEFG");
    REGISTER_PARAM(kingSafetyHalfOpenAH, "KingSafetyHalfOpenAH");
    REGISTER_PARAM(kingSafetyWeight, "KingSafetyWeight");
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
    queenMobScore.registerParams("QueenMobility", *this);
    majorPieceRedundancy.registerParams("MajorPieceRedundancy", *this);
    passedPawnBonus.registerParams("PassedPawnBonus", *this);
    candidatePassedBonus.registerParams("CandidatePassedPawnBonus", *this);
    QvsRRBonus.registerParams("QueenVs2RookBonus", *this);
    RvsMBonus.registerParams("RookVsMinorBonus", *this);
    RvsMMBonus.registerParams("RookVs2MinorBonus", *this);
    bishopPairValue.registerParams("BishopPairValue", *this);
    pawnShelterTable.registerParams("PawnShelterTable", *this);
    pawnStormTable.registerParams("PawnStormTable", *this);

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

