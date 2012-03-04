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
