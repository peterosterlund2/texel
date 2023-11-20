/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * endGameEval.hpp
 *
 *  Created on: Dec 26, 2014
 *      Author: petero
 */

#ifndef ENDGAMEEVAL_HPP_
#define ENDGAMEEVAL_HPP_

#include "util.hpp"
#include "square.hpp"

class Position;

class EndGameEval {
public:
    /** Implements special knowledge for some endgame situations.
     * If doEval is false the position is not evaluated. Instead 1 is returned if
     * this function has special knowledge about the current material balance, and 0
     * is returned otherwise. */
    template <bool doEval> static int endGameEval(const Position& pos,
                                                  int oldScore);

private:
    /** King evaluation when no pawns left. */
    static int mateEval(Square k1, Square k2);

    static const SqTbl<int> winKingTable;

    /** Return true if the side with the bishop can not win because the opponent
     * has a fortress draw. */
    template <bool whiteBishop> static bool isBishopPawnDraw(const Position& pos);

    static int kqkpEval(Square wKing, Square wQueen, Square bKing, Square bPawn, bool whiteMove, int score);
    static int kqkrpEval(Square wKing, Square wQueen, Square bKing, Square bRook, Square bPawn, bool whiteMove, int score);
    static bool kqkrmFortress(bool bishop, Square wQueen, Square bRook, Square bMinor, U64 wPawns, U64 bPawns);

    static int kpkEval(Square wKing, Square bKing, Square wPawn, bool whiteMove);
    static bool kpkpEval(Square wKing, Square bKing, Square wPawn, Square bPawn, int& score);

    static int krkpEval(Square wKing, Square bKing, Square bPawn, bool whiteMove, int score);
    static int krpkrEval(Square wKing, Square bKing, Square wPawn, Square wRook, Square bRook, bool whiteMove);
    static int krpkrpEval(Square wKing, Square bKing, Square wPawn, Square wRook, Square bRook,
                          Square bPawn, bool whiteMove, int score);

    static int kbnkEval(Square wKing, Square bKing, bool darkBishop);

    static int kbpkbEval(Square wKing, Square wBish, Square wPawn, Square bKing, Square bBish, int score);
    static int kbpknEval(Square wKing, Square wBish, Square wPawn, Square bKing, Square bKnight, int score);
    static int knpkbEval(Square wKing, Square wKnight, Square wPawn, Square bKing, Square bBish, int score, bool wtm);
    static int knpkEval(Square wKing, Square wKnight, Square wPawn, Square bKing, int score, bool wtm);

    static const int distToH1A8[8][8];
    static const U8 kpkTable[2*32*64*48/8];
    static const U8 krkpTable[2*32*48*8];
    static const U64 krpkrTable[2*24*64];
};

#endif
