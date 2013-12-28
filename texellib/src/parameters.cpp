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
