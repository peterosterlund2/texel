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
#include "constants.hpp"

class Position;
class NNTest;

/** Handles position evaluation using a neural network. */
class NNEvaluator {
    friend class NNTest;
public:
    /** Create an instance. This object needs special alignment,
     *  so allocating it on the stack is not supported. */
    static std::shared_ptr<NNEvaluator> create(const NetData& netData);

    /** Set position object used for non-incremental evaluation. */
    void connectPosition(const Position& pos);

    /** Push the evaluation state. Called before making a move.*/
    void pushState();
    /** Pop the evaluation state. Called after undoing a move. */
    void popState();

    /** Set a square to a piece value. Use Piece::EMPTY to clear a square. */
    void setPiece(int square, int oldPiece, int newPiece);

    /** Clear incrementally updated state. Needed if position has changed
     *  in an unknown way. */
    void forceFullEval(bool clearStack = true);

    /** Static evaluation of the current position.
     * @return The evaluation score, measured in centipawns.
     *         Positive values are good for the side to make the next move. */
    int eval();

    /** Get the first layer output for feature f. 0 <= f < 2*n1. */
    int getL1OutClipped(int f) const;

    /** Initialize static data. */
    static void staticInitialize();

private:
    /** Constructor. */
    explicit NNEvaluator(const NetData& netData);

    void computeL1WB();
    void computeL1Out();

    struct FirstLayerState;
    /** Get first layer linear state. */
    FirstLayerState& getLinState(int c);

    static constexpr int n1 = NetData::n1;
    static constexpr int n2 = NetData::n2;
    static constexpr int n3 = NetData::n3;

    static constexpr int maxIncr = 4;
    static constexpr int maxStackSize = SearchConst::MAX_SEARCH_DEPTH * 2;

    struct FirstLayerState {
        Vector<S16, n1> l1Out;   // Linear output corresponding to one side, incrementally updated
        int toAdd[maxIncr];      // Input features to add to l1Out to make it up to date
        int toSub[maxIncr];      // Input features to subtract from l1Out to make it up to date
        int toAddLen = 0;        // Number of entries in toAdd
        int toSubLen = 0;        // Number of entries in toSub
        int kingSqComputed = -1; // King square corresponding to l1Out, or -1 if l1Out not valid
        int pad[5];              // To make size a multiple of 32 bytes
        void clear();
    };
    struct FirstLayerStack {
        FirstLayerState flState[maxStackSize][2];
        int stackTop = 0;   // Current stack entry
        int pad[7];         // To make size a multiple of 32 bytes
    };
    FirstLayerStack stack;

    Vector<S8, 2*n1> l1OutClipped; // l1Out after scaling, clipped ReLU and narrowing, reordered by wtm

    using Layer2 = Layer<n1*2, n2, true >;
    using Layer3 = Layer<n2  , n3, false>;
    using Layer4 = Layer<n3  , 1 , false>;

    Layer2::Output layer2Out;
    Layer3::Output layer3Out;
    Layer4::Output layer4Out;

    Layer2 layer2;
    Layer3 layer3;
    Layer4 layer4;

    const Position* posP = nullptr; // Connected Position object
    const NetData& netData;         // Network weight/bias

    static int ptValue[Piece::nPieceTypes]; // Conversion from Piece to piece values used by NN
};

inline int
NNEvaluator::getL1OutClipped(int f) const {
    return l1OutClipped(f);
}

inline NNEvaluator::FirstLayerState&
NNEvaluator::getLinState(int c) {
    return stack.flState[stack.stackTop][c];
}

inline void
NNEvaluator::FirstLayerState::clear() {
    toAddLen = 0;
    toSubLen = 0;
    kingSqComputed = -1;
}

#endif /* NNEVAL_HPP_ */
