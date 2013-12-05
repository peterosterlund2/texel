/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * evaluateTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef EVALUATETEST_HPP_
#define EVALUATETEST_HPP_

#include "suiteBase.hpp"

class EvaluateTest : public SuiteBase {
public:
    std::string getName() const { return "EvaluateTest"; }

    cute::suite getSuite() const;
};

class Position;
class Evaluate;
int swapSquareY(int square);
Position swapColors(const Position& pos);
int evalWhite(const Position& pos);
int evalWhite(Evaluate& eval, const Position& pos);


#endif /* EVALUATETEST_HPP_ */
