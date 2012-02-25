/*
 * positionTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef POSITIONTEST_HPP_
#define POSITIONTEST_HPP_

#include "suiteBase.hpp"

class PositionTest : public SuiteBase {
public:
    std::string getName() const { return "PositionTest"; }

    cute::suite getSuite() const;
};

#endif /* POSITIONTEST_HPP_ */
