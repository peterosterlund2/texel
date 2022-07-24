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
 * nneval.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: petero
 */

#include "nneval.hpp"


NNEvaluator::NNEvaluator()
    : lin2(netData.lin2),
      lin3(netData.lin3),
      lin4(netData.lin4) {
}

void
NNEvaluator::setPos(const Position& pos) {

}

void
NNEvaluator::makeMove(const Move& move) {

}

void
NNEvaluator::unMakeMove(const Move& move, const UndoInfo& ui) {

}

int
NNEvaluator::eval() {
    // Update l1OutW, l1OutB
    // Compute l1Out

    lin2.forward(l1Out);
    lin3.forward(lin2.output);
    lin4.evalLinear(lin3.output);

    return lin4.linOutput(0) / (127 * 64 / 100);
}
