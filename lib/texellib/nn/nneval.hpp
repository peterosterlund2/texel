/*
    Texel - A UCI chess engine.
    Copyright (C) 2022  Peter Österlund, peterosterlund2@gmail.com

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
 * nneval.hpp
 *
 *  Created on: Jul 22, 2022
 *      Author: petero
 */

#ifndef NNEVAL_HPP_
#define NNEVAL_HPP_

#include "nntypes.hpp"
#include "util.hpp"

class Position;
class Move;
struct UndoInfo;

/** Handles position evaluation using a neural network. */
class NNEvaluator {
public:
    /** Constructor. */
    NNEvaluator();

    /** Set current position. */
    void setPos(const Position& pos);

    /** Modify current position by making a move. */
    void makeMove(const Move& move);

    /** Modify current position by undoing a move. */
    void unMakeMove(const Move& move, const UndoInfo& ui);

    /** Static evaluation of the current position.
     * @return The evaluation score, measured in centipawns.
     *         Positive values are good for the side to make the next move. */
    int eval();

private:
    int squares[64]; // Piece type for each square

    NetData netData;

    static constexpr int n1 = NetData::n1;
    static constexpr int n2 = NetData::n2;
    static constexpr int n3 = NetData::n3;

    Vector<S16, n1> l1OutW; // Linear output corresponding to white king, incrementally updated
    Vector<S16, n1> l1OutB; // Linear output corresponding to black king, incrementally updated
    Vector<S8, 2*n1> l1Out; // l1Out{W,B} after clipped relu and narrowing

    Layer<n1*2, n2> lin2;
    Layer<n2  , n3> lin3;
    Layer<n3  , 1 > lin4;
};

#endif /* NNEVAL_HPP_ */
