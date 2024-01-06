/*
    Texel - A UCI chess engine.
    Copyright (C) 2024  Peter Österlund, peterosterlund2@gmail.com

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
 * datasetTest.hpp
 *
 *  Created on: Jan 6, 2024
 *      Author: petero
 */

#ifndef DATASETTEST_HPP_
#define DATASETTEST_HPP_

class DataSetTest {
public:
    static void testMemDS();
    static void testFileDS();
    static void testShuffledDS();
    static void testSplitData();
};

#endif /* DATASETTEST_HPP_ */
