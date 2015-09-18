/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * pathsearchTest.hpp
 *
 *  Created on: Aug 22, 2015
 *      Author: petero
 */

#ifndef PATHSEARCHTEST_HPP_
#define PATHSEARCHTEST_HPP_

#include "utilSuiteBase.hpp"

class Position;
class PathSearch;

class PathSearchTest : public UtilSuiteBase {
public:
    std::string getName() const override { return "PathSearchTest"; }

    cute::suite getSuite() const override;
private:
    static void checkBlockedConsistency(PathSearch& ps, Position& pos);
    static int hScore(PathSearch& ps, const std::string& fen);

    static void testMaterial();
    static void testNeighbors();
    static void testShortestPath();
    static void testValidPieceCount();
    static void testPawnReachable();
    static void testBlocked();
    static void testCastling();
    static void testReachable();
    static void testRemainingMoves();
};

#endif /* PATHSEARCHTEST_HPP_ */
