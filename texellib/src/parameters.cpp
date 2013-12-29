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

DEFINE_PARAM(knightVsQueenBonus1);
DEFINE_PARAM(knightVsQueenBonus2);
DEFINE_PARAM(knightVsQueenBonus3);

DEFINE_PARAM(pawnTradePenalty);
DEFINE_PARAM(pieceTradeBonus);

DEFINE_PARAM(rookHalfOpenBonus);
DEFINE_PARAM(rookOpenBonus);
DEFINE_PARAM(rookDouble7thRowBonus);
DEFINE_PARAM(trappedRookPenalty);

DEFINE_PARAM(bishopPairValue);
DEFINE_PARAM(bishopPairPawnPenalty);

DEFINE_PARAM(kingAttackWeight);
DEFINE_PARAM(kingSafetyHalfOpenBCDEFG);
DEFINE_PARAM(kingSafetyHalfOpenAH);
DEFINE_PARAM(kingSafetyWeight);
DEFINE_PARAM(pawnStormBonus);

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
    {-22,-35,-40,-40,-40,-40,-35,-22,
     -22,-35,-40,-40,-40,-40,-35,-22,
     -25,-35,-40,-45,-45,-40,-35,-25,
     -15,-30,-35,-40,-40,-35,-30,-15,
     -10,-15,-20,-25,-25,-20,-15,-10,
       4, -2, -5,-15,-15, -5, -2,  4,
      16, 14,  7, -3, -3,  7, 14, 16,
      24, 24,  9,  0,  0,  9, 24, 24 },
    {  1,  2,  3,  4,  4,  3,  2,  1,
       5,  6,  7,  8,  8,  7,  6,  5,
       9, 10, 11, 12, 12, 11, 10,  9,
      13, 14, 15, 16, 16, 15, 14, 13,
      17, 18, 19, 20, 20, 19, 18, 17,
      21, 22, 23, 24, 24, 23, 22, 21,
      25, 26, 27, 28, 28, 27, 26, 25,
      29, 30, 31,  0,  0, 31, 30, 29 }
};
ParamTableMirrored<64> kt1w(kt1b);

/** Piece/square table for king during end game. */
ParamTable<64> kt2b { -200, 200, useUciParam,
    {  0,  8, 16, 24, 24, 16,  8,  0,
       8, 16, 24, 32, 32, 24, 16,  8,
      16, 24, 32, 40, 40, 32, 24, 16,
      24, 32, 40, 48, 48, 40, 32, 24,
      24, 32, 40, 48, 48, 40, 32, 24,
      16, 24, 32, 40, 40, 32, 24, 16,
       8, 16, 24, 32, 32, 24, 16,  8,
       0,  8, 16, 24, 24, 16,  8,  0 },
    { 0, 1, 2, 3, 3, 2, 1, 0,
      1, 4, 5, 6, 6, 5, 4, 1,
      2, 5, 7, 8, 8, 7, 5, 2,
      3, 6, 8, 9, 9, 8, 6, 3,
      3, 6, 8, 9, 9, 8, 6, 3,
      2, 5, 7, 8, 8, 7, 5, 2,
      1, 4, 5, 6, 6, 5, 4, 1,
      0, 1, 2, 3, 3, 2, 1, 0 }
};
ParamTableMirrored<64> kt2w(kt2b);

/** Piece/square table for pawns during middle game. */
ParamTable<64> pt1b { -200, 200, useUciParam,
    {  0,  0,  0,  0,  0,  0,  0,  0,
       8, 16, 24, 32, 32, 24, 16,  8,
       3, 12, 20, 28, 28, 20, 12,  3,
      -5,  4, 10, 20, 20, 10,  4, -5,
      -6,  4,  5, 16, 16,  5,  4, -6,
      -6,  4,  2,  5,  5,  2,  4, -6,
      -6,  4,  4,-15,-15,  4,  4, -6,
       0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,
       1,  2,  3,  4,  4,  3,  2,  1,
       5,  6,  7,  8,  8,  7,  6,  5,
       9, 10, 11, 12, 12, 11, 10,  9,
      13, 14, 15, 16, 16, 15, 14, 13,
      17, 18, 19, 20, 20, 19, 18, 17,
      21, 22, 23, 24, 24, 23, 22, 21,
       0,  0,  0,  0,  0,  0,  0,  0 }
};
ParamTableMirrored<64> pt1w(pt1b);

/** Piece/square table for pawns during end game. */
ParamTable<64> pt2b { -200, 200, useUciParam,
    {  0,  0,  0,  0,  0,  0,  0,  0,
      25, 40, 45, 45, 45, 45, 40, 25,
      17, 32, 35, 35, 35, 35, 32, 17,
       5, 24, 24, 24, 24, 24, 24,  5,
      -9, 11, 11, 11, 11, 11, 11, -9,
     -17,  3,  3,  3,  3,  3,  3,-17,
     -20,  0,  0,  0,  0,  0,  0,-20,
       0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,
       1,  2,  3,  4,  4,  3,  2,  1,
       5,  6,  7,  8,  8,  7,  6,  5,
       9, 10, 11, 12, 12, 11, 10,  9,
      13, 14, 15, 16, 16, 15, 14, 13,
      17, 18, 19, 20, 20, 19, 18, 17,
      21, 22, 23, 24, 24, 23, 22, 21,
       0,  0,  0,  0,  0,  0,  0,  0 }
};
ParamTableMirrored<64> pt2w(pt2b);

/** Piece/square table for knights during middle game. */
ParamTable<64> nt1b { -200, 200, useUciParam,
    {-53,-42,-32,-21,-21,-32,-42,-53,
     -42,-32,-10,  0,  0,-10,-32,-42,
     -21,  5, 10, 16, 16, 10,  5,-21,
     -18,  0, 10, 21, 21, 10,  0,-18,
     -18,  0,  3, 21, 21,  3,  0,-18,
     -21,-10,  0,  0,  0,  0,-10,-21,
     -42,-32,-10,  0,  0,-10,-32,-42,
     -53,-42,-32,-21,-21,-32,-42,-53 },
    {  1,  2,  3,  4,  4,  3,  2,  1,
       5,  6,  7,  8,  8,  7,  6,  5,
       9, 10, 11, 12, 12, 11, 10,  9,
      13, 14, 15, 16, 16, 15, 14, 13,
      17, 18, 19, 20, 20, 19, 18, 17,
      21, 22, 23, 24, 24, 23, 22, 21,
      25, 26, 27, 28, 28, 27, 26, 25,
      29, 30, 31, 32, 32, 31, 30, 29 }
};
ParamTableMirrored<64> nt1w(nt1b);

/** Piece/square table for knights during end game. */
ParamTable<64> nt2b { -200, 200, useUciParam,
    {-56,-44,-34,-22,-22,-34,-44,-56,
     -44,-34,-10,  0,  0,-10,-34,-44,
     -22,  5, 10, 17, 17, 10,  5,-22,
     -19,  0, 10, 22, 22, 10,  0,-19,
     -19,  0,  3, 22, 22,  3,  0,-19,
     -22,-10,  0,  0,  0,  0,-10,-22,
     -44,-34,-10,  0,  0,-10,-34,-44,
     -56,-44,-34,-22,-22,-34,-44,-56 },
    {  1,  2,  3,  4,  4,  3,  2,  1,
       5,  6,  7,  8,  8,  7,  6,  5,
       9, 10, 11, 12, 12, 11, 10,  9,
      13, 14, 15, 16, 16, 15, 14, 13,
      17, 18, 19, 20, 20, 19, 18, 17,
      21, 22, 23, 24, 24, 23, 22, 21,
      25, 26, 27, 28, 28, 27, 26, 25,
      29, 30, 31, 32, 32, 31, 30, 29 }
};
ParamTableMirrored<64> nt2w(nt2b);

/** Piece/square table for bishops during middle game. */
ParamTable<64> bt1b { -200, 200, useUciParam,
    { 0,  0,  0,  0,  0,  0,  0,  0,
      0,  4,  2,  2,  2,  2,  4,  0,
      0,  2,  4,  4,  4,  4,  2,  0,
      0,  2,  4,  4,  4,  4,  2,  0,
      0,  2,  4,  4,  4,  4,  2,  0,
      0,  3,  4,  4,  4,  4,  3,  0,
      0,  4,  2,  2,  2,  2,  4,  0,
     -5, -5, -7, -5, -5, -7, -5, -5 },
    { 1,  2,  3,  4,  4,  3,  2,  1,
      5,  6,  7,  8,  8,  7,  6,  5,
      9, 10, 11, 12, 12, 11, 10,  9,
     13, 14, 15, 16, 16, 15, 14, 13,
     17, 18, 19, 20, 20, 19, 18, 17,
     21, 22, 23, 24, 24, 23, 22, 21,
     25, 26, 27, 28, 28, 27, 26, 25,
     29, 30, 31, 32, 32, 31, 30, 29 }
};
ParamTableMirrored<64> bt1w(bt1b);

/** Piece/square table for bishops during end game. */
ParamTable<64> bt2b { -200, 200, useUciParam,
    { 0,  0,  0,  0,  0,  0,  0,  0,
      0,  2,  2,  2,  2,  2,  2,  0,
      0,  2,  4,  4,  4,  4,  2,  0,
      0,  2,  4,  4,  4,  4,  2,  0,
      0,  2,  4,  4,  4,  4,  2,  0,
      0,  2,  4,  4,  4,  4,  2,  0,
      0,  2,  2,  2,  2,  2,  2,  0,
      0,  0,  0,  0,  0,  0,  0,  0 },
    {10, 1, 2, 3, 3, 2, 1,10,
      1, 4, 5, 6, 6, 5, 4, 1,
      2, 5, 7, 8, 8, 7, 5, 2,
      3, 6, 8, 9, 9, 8, 6, 3,
      3, 6, 8, 9, 9, 8, 6, 3,
      2, 5, 7, 8, 8, 7, 5, 2,
      1, 4, 5, 6, 6, 5, 4, 1,
     10, 1, 2, 3, 3, 2, 1,10 }
};
ParamTableMirrored<64> bt2w(bt2b);

/** Piece/square table for queens during middle game. */
ParamTable<64> qt1b { -200, 200, useUciParam,
    {-10, -5,  0,  0,  0,  0, -5,-10,
      -5,  0,  5,  5,  5,  5,  0, -5,
       0,  5,  5,  6,  6,  5,  5,  0,
       0,  5,  6,  6,  6,  6,  5,  0,
       0,  5,  6,  6,  6,  6,  5,  0,
       0,  5,  5,  6,  6,  5,  5,  0,
      -5,  0,  5,  5,  5,  5,  0, -5,
     -10, -5,  0,  0,  0,  0, -5,-10 },
     {10, 1, 2, 3, 3, 2, 1,10,
       1, 4, 5, 6, 6, 5, 4, 1,
       2, 5, 7, 8, 8, 7, 5, 2,
       3, 6, 8, 9, 9, 8, 6, 3,
       3, 6, 8, 9, 9, 8, 6, 3,
       2, 5, 7, 8, 8, 7, 5, 2,
       1, 4, 5, 6, 6, 5, 4, 1,
      10, 1, 2, 3, 3, 2, 1,10 }
};
ParamTableMirrored<64> qt1w(qt1b);

/** Piece/square table for rooks during end game. */
ParamTable<64> rt1b { -200, 200, useUciParam,
    {  8, 11, 13, 13, 13, 13, 11,  8,
      22, 27, 27, 27, 27, 27, 27, 22,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
      -2,  0,  0,  0,  0,  0,  0, -2,
      -2,  0,  0,  2,  2,  0,  0, -2,
      -3,  2,  5,  5,  5,  5,  2, -3,
       0,  3,  5,  5,  5,  5,  3,  0 },
    {  1,  2,  3,  4,  4,  3,  2,  1,
       5,  6,  7,  8,  8,  7,  6,  5,
       9, 10, 11, 12, 12, 11, 10,  9,
      13, 14, 15, 16, 16, 15, 14, 13,
      17, 18, 19, 20, 20, 19, 18, 17,
      21, 22, 23, 24, 24, 23, 22, 21,
      25, 26, 27, 28, 28, 27, 26, 25,
       0, 29, 30, 31, 31, 30, 29,  0 }
};
ParamTableMirrored<64> rt1w(rt1b);

ParamTable<64> knightOutpostBonus { 0, 150, useUciParam,
    {  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0, 14, 25, 36, 36, 25, 14,  0,
       0, 14, 33, 43, 43, 33, 14,  0,
       0,  0, 18, 29, 29, 18,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  1,  2,  3,  3,  2,  1,  0,
       0,  4,  5,  6,  6,  5,  4,  0,
       0,  0,  7,  8,  8,  7,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0 }
};

ParamTable<15> rookMobScore { -50, 50, useUciParam,
    { -10,-7,-4,-1,2,5,7,9,11,12,13,14,14,14,14},
    {   1, 2, 3, 4,5,6,7,8, 9,10,11,12,13,14,15}
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    { -15,-10,-6,-2,2,6,10,13,16,18,20,22,23,24},
    {   1,  2, 3, 4,5,6, 7, 8, 9,10,11,12,13,14}
};
ParamTable<28> queenMobScore { -50, 50, useUciParam,
    {-5,-4,-3,-2,-1,0,1,2,3, 4, 5, 6, 7, 8, 9, 9,10,10,10,10,10,10,10,10,10,10,10,10},
    { 1, 2, 3, 4, 5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28}
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
    addPar(std::make_shared<SpinParam>("Threads", 1, 64, 1));
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

    REGISTER_PARAM(knightVsQueenBonus1, "KnightVsQueenBonus1");
    REGISTER_PARAM(knightVsQueenBonus2, "KnightVsQueenBonus2");
    REGISTER_PARAM(knightVsQueenBonus3, "KnightVsQueenBonus3");

    REGISTER_PARAM(pawnTradePenalty, "PawnTradePenalty");
    REGISTER_PARAM(pieceTradeBonus, "PieceTradeBonus");

    REGISTER_PARAM(rookHalfOpenBonus, "RookHalfOpenBonus");
    REGISTER_PARAM(rookOpenBonus, "RookOpenBonus");
    REGISTER_PARAM(rookDouble7thRowBonus, "RookDouble7thRowBonus");
    REGISTER_PARAM(trappedRookPenalty, "TrappedRookPenalty");

    REGISTER_PARAM(bishopPairValue, "BishopPairValue");
    REGISTER_PARAM(bishopPairPawnPenalty, "BishopPairPawnPenalty");

    REGISTER_PARAM(kingAttackWeight, "KingAttackWeight");
    REGISTER_PARAM(kingSafetyHalfOpenBCDEFG, "KingSafetyHalfOpenBCDEFG");
    REGISTER_PARAM(kingSafetyHalfOpenAH, "KingSafetyHalfOpenAH");
    REGISTER_PARAM(kingSafetyWeight, "KingSafetyWeight");
    REGISTER_PARAM(pawnStormBonus, "PawnStormBonus");


    // Evaluation tables
    kt1b.registerParams("KingTableMG", *this);
    kt2b.registerParams("KingTableEG", *this);
    pt1b.registerParams("PawnTableMG", *this);
    pt2b.registerParams("PawnTableEG", *this);
    nt1b.registerParams("KnightTableMG", *this);
    nt2b.registerParams("KnightTableEG", *this);
    bt1b.registerParams("BishopTableMG", *this);
    bt2b.registerParams("BishopTableEG", *this);
    qt1b.registerParams("QueenTable", *this);
    rt1b.registerParams("RookTable", *this);

    knightOutpostBonus.registerParams("KnightOutpostBonus", *this);
    rookMobScore.registerParams("RookMobility", *this);
    bishMobScore.registerParams("BishopMobility", *this);
    queenMobScore.registerParams("QueenMobility", *this);

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
