/*
    Texel - A UCI chess engine.
    Copyright (C) 2023  Peter Österlund, peterosterlund2@gmail.com

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
 * nnTest.hpp
 *
 *  Created on: Jul 29, 2023
 *      Author: petero
 */

#ifndef NNTEST_HPP_
#define NNTEST_HPP_

class NNTest {
public:
    /** Test Matrix/Vector classes. */
    static void testMatMul();

    /** Test incremental NN evaluation. */
    static void testIncremental();
};

#endif /* NNTEST_HPP_ */
