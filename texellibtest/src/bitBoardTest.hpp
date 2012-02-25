/*
 * bitBoardTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BITBOARDTEST_HPP_
#define BITBOARDTEST_HPP_

#include "suiteBase.hpp"

class BitBoardTest : public SuiteBase {
public:
    std::string getName() const { return "BitBoardTest"; }

    cute::suite getSuite() const;
};

#endif /* BITBOARDTEST_HPP_ */
