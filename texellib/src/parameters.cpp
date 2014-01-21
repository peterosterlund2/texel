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

DEFINE_PARAM(bishopPairValue);
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
    {   7, 200, 189, 200, 200, 189, 200,   7,
      200, 151, 164,  90,  90, 164, 151, 200,
       94, 169, 145,  87,  87, 145, 169,  94,
      107, 123,  64,  32,  32,  64, 123, 107,
       16,  43,  12,  27,  27,  12,  43,  16,
        4,  -2,   1, -11, -11,   1,  -2,   4,
       24,   5, -19, -14, -14, -19,   5,  24,
       -2,  19, -10,  -4,  -4, -10,  19,  -2 },
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
    {   0,   8,  18,  14,  14,  18,   8,   0,
        8,  34,  42,  42,  42,  42,  34,   8,
       18,  42,  52,  57,  57,  52,  42,  18,
       14,  42,  57,  64,  64,  57,  42,  14,
       14,  42,  57,  64,  64,  57,  42,  14,
       18,  42,  52,  57,  57,  52,  42,  18,
        8,  34,  42,  42,  42,  42,  34,   8,
        0,   8,  18,  14,  14,  18,   8,   0 },
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
      141, 103,  94, 144, 144,  94, 103, 141,
       13,  33,  51,  46,  46,  51,  33,  13,
        3, -11,   9,  23,  23,   9, -11,   3,
       -3,  -4,  11,  14,  14,  11,  -4,  -3,
       -2,  -7,  -1,   9,   9,  -1,  -7,  -2,
      -13,  -6,  -6, -18, -18,  -6,  -6, -13,
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
      -31, -18, -20, -39, -39, -20, -18, -31,
       32,  29,  30,   8,   8,  30,  29,  32,
       24,  18,  16,   2,   2,  16,  18,  24,
        8,  11,   7,  -2,  -2,   7,  11,   8,
        0,   4,   8,  11,  11,   8,   4,   0,
        1,   4,  13,  22,  22,  13,   4,   1,
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
    {-207, -12, -34, -18, -18, -34, -12,-207,
      -17, -38,  21,  54,  54,  21, -38, -17,
      -16,  14,  44,  54,  54,  44,  14, -16,
       10,   7,  35,  21,  21,  35,   7,  10,
      -16,   7,  25,  10,  10,  25,   7, -16,
      -41, -12,   3,  23,  23,   3, -12, -41,
      -31, -30, -19,   3,   3, -19, -30, -31,
      -72, -42, -42, -28, -28, -42, -42, -72 },
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
    { -65, -13,   5,   4,   4,   5, -13, -65,
      -18,  -4,  13,  27,  27,  13,  -4, -18,
       -5,  11,  23,  24,  24,  23,  11,  -5,
       -6,  15,  32,  31,  31,  32,  15,  -6,
      -12,  15,  28,  34,  34,  28,  15, -12,
      -20,  -2,   5,  25,  25,   5,  -2, -20,
      -40, -22,   5,  -9,  -9,   5, -22, -40,
      -59, -52, -23, -29, -29, -23, -52, -59 },
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
    { -39,  11,   0,  -7,  -7,   0,  11, -39,
      -37, -43,  -9, -30, -30,  -9, -43, -37,
        9,  44,  31,  20,  20,  31,  44,   9,
      -16, -16,  20,  29,  29,  20, -16, -16,
       -4,   0,  -8,  18,  18,  -8,   0,  -4,
        0,   8,   3,   4,   4,   3,   8,   0,
       -7,  14,   7,   0,   0,   7,  14,  -7,
      -26,  10, -12, -11, -11, -12,  10, -26 },
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
    {   0,   4,   3,   9,   9,   3,   4,   0,
        4,   4,  15,  19,  19,  15,   4,   4,
        3,  15,  24,  29,  29,  24,  15,   3,
        9,  19,  29,  37,  37,  29,  19,   9,
        9,  19,  29,  37,  37,  29,  19,   9,
        3,  15,  24,  29,  29,  24,  15,   3,
        4,   4,  15,  19,  19,  15,   4,   4,
        0,   4,   3,   9,   9,   3,   4,   0 },
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
    {  14,   9,  26,  -2,  -2,  26,   9,  14,
      -30,-129, -27, -85, -85, -27,-129, -30,
      -25, -12, -49, -45, -45, -49, -12, -25,
      -28, -38, -54, -73, -73, -54, -38, -28,
      -30, -14, -34, -41, -41, -34, -14, -30,
      -32, -21, -22, -24, -24, -22, -21, -32,
      -27, -17, -11,  -6,  -6, -11, -17, -27,
      -25, -28, -18,  -8,  -8, -18, -28, -25 },
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
    { -23, -21, -23, -14, -14, -23, -21, -23,
      -21, -28, -14,  -9,  -9, -14, -28, -21,
      -23, -14,  -2,  -7,  -7,  -2, -14, -23,
      -14,  -9,  -7,   6,   6,  -7,  -9, -14,
      -14,  -9,  -7,   6,   6,  -7,  -9, -14,
      -23, -14,  -2,  -7,  -7,  -2, -14, -23,
      -21, -28, -14,  -9,  -9, -14, -28, -21,
      -23, -21, -23, -14, -14, -23, -21, -23 },
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
    {  42,  36,  36,  39,  39,  36,  36,  42,
       45,  51,  53,  53,  53,  53,  51,  45,
       30,  49,  44,  47,  47,  44,  49,  30,
       12,  24,  30,  31,  31,  30,  24,  12,
       -4,   6,  15,   6,   6,  15,   6,  -4,
      -16,  -5,  -3,  -3,  -3,  -3,  -5, -16,
      -27, -12, -12,  -3,  -3, -12, -12, -27,
        0,   1,   5,   7,   7,   5,   1,   0 },
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
        0,  23,  51,  49,  49,  51,  23,   0,
        0,  15,  39,  42,  42,  39,  15,   0,
        0,   0,  13,  32,  32,  13,   0,   0,
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
    { -6, -8, -3, -3, -3,  5,  7, 12, 16, 22, 24, 27, 26, 24, 32 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-19,-15, -1,  4, 11, 16, 20, 24, 26, 29, 26, 32, 30, 27 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    { -3,  1, -9, -6, -3,  0,  2,  3,  6,  9, 12, 15, 18, 18, 25, 28, 33, 34, 39, 36, 45, 42, 43, 47, 41, 31, 32, 47 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -70,   0,   0,
       70,   0,   0,   0,
        0,   0,   0,  67,
        0,   0, -67,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<8> passedPawnBonus { -200, 200, useUciParam,
    { -1,  8,  6, 16, 30, 54, 95, -1 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1, -2, -1, 21, 39, 63, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
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

    REGISTER_PARAM(bishopPairValue, "BishopPairValue");
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
