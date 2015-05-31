/*
 * bookBuildTest.hpp
 *
 *  Created on: May 29, 2015
 *      Author: petero
 */

#ifndef BOOKBUILDTEST_HPP_
#define BOOKBUILDTEST_HPP_

#include "utilSuiteBase.hpp"

class BookBuildTest : public UtilSuiteBase {
    std::string getName() const override { return "BookBuildTest"; }

    cute::suite getSuite() const override;
private:
    static void testBookNode();
    static void testShortestDepth();
    static void testBookNodeDAG();
    static void testExtend();
};

#endif /* BOOKBUILDTEST_HPP_ */
