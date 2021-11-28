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
 * cspsolverTest.hpp
 *
 *  Created on: Nov 21, 2021
 *      Author: petero
 */

#ifndef CSPSOLVERTEST_HPP_
#define CSPSOLVERTEST_HPP_

#include "cspsolver.hpp"

class CspSolverTest {
public:
    static void testBitSet();
    static void basicTests();
    static void testEvenOdd();
    static void testProofKernel();
};

#endif /* CSPSOLVERTEST_HPP_ */
