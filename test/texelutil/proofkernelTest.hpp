/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * proofkernelTest.hpp
 *
 *  Created on: Oct 23, 2021
 *      Author: petero
 */

#ifndef PROOFKERNELTEST_HPP_
#define PROOFKERNELTEST_HPP_

#include "proofkernel.hpp"

class ProofKernelTest {
public:
    static void testPawnColumn();
    static void testPawnColPromotion();
    static void testGoal();
    static void testGoalPossible();
    static void testMoveToString();
    static void testMoveGen();
    static void testMakeMove();
    static void testSearch();
    static void testExtMoveToString();
    static void testExtKernel();

private:
    static void testMove(const Position& pos, const ProofKernel& pk,
                         const ProofKernel::PkMove& m);
};

#endif /* PROOFKERNELTEST_HPP_ */
