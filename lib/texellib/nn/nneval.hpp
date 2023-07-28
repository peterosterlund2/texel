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
#include "piece.hpp"

class Position;
class NNTest;

/** Handles position evaluation using a neural network. */
class NNEvaluator {
    friend class NNTest;
public:
    /** Constructor. */
    NNEvaluator(const NetData& netData);

    /** Set position object used for non-incremental evaluation. */
    void connectPosition(const Position& pos);

    /** Set a square to a piece value. Use Piece::EMPTY to clear a square. */
    void setPiece(int square, int oldPiece, int newPiece);

    /** Clear incrementally updated state. Needed if position has changed
     *  in an unknown way. */
    void forceFullEval();

    /** Static evaluation of the current position.
     * @return The evaluation score, measured in centipawns.
     *         Positive values are good for the side to make the next move. */
    int eval();

    /** Initialize static data. */
    static void staticInitialize();

private:
    void computeL1WB();
    void computeL1Out();

    static constexpr int n1 = NetData::n1;
    static constexpr int n2 = NetData::n2;
    static constexpr int n3 = NetData::n3;

    Vector<S16, n1> l1Out[2];      // Linear output corresponding to white/black king, incrementally updated
    Vector<S8, 2*n1> l1OutClipped; // l1Out after scaling, clipped ReLU and narrowing, reordered by wtm

    Layer<n1*2, n2> lin2;
    Layer<n2  , n3> lin3;
    Layer<n3  , 1 > lin4;

    // White/black king square corresponding to l1Out[i], or -1 if l1Out[i] not valid
    int kingSqComputed[2] = {-1, -1};

    std::vector<int> toAdd[2]; // Input features to add to l1Out to make it up to date
    std::vector<int> toSub[2]; // Input features to subtract from l1Out to make it up to date

    const Position* posP = nullptr; // Connected Position object

    const NetData& netData;

    static int ptValue[Piece::nPieceTypes];
};

#endif /* NNEVAL_HPP_ */
