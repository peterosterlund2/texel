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
 * searchTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef SEARCHTEST_HPP_
#define SEARCHTEST_HPP_

#include "suiteBase.hpp"

class Move;
class Search;

class SearchTest : public SuiteBase {
public:
    std::string getName() const { return "SearchTest"; }

    cute::suite getSuite() const;

private:
    static Move idSearch(Search& sc, int maxDepth);
    static void testNegaScout();
    static void testDraw50();
    static void testDrawRep();
    static void testHashing();
    static void testLMP();
    static void testCheckEvasion();
    static void testStalemateTrap();
    static void testKQKRNullMove();
    static int getSEE(Search& sc, const Move& m);
    static void testSEE();
    static void testScoreMoveList();
};

#endif /* SEARCHTEST_HPP_ */
