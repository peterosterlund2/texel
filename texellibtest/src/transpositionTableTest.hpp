/*
 * transpositionTableTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TRANSPOSITIONTABLETEST_HPP_
#define TRANSPOSITIONTABLETEST_HPP_

#include "suiteBase.hpp"

class TranspositionTableTest : public SuiteBase {
public:
    std::string getName() const { return "TranspositionTableTest"; }

    cute::suite getSuite() const;
};

#endif /* TRANSPOSITIONTABLETEST_HPP_ */
