/*
 * evaluateTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef EVALUATETEST_HPP_
#define EVALUATETEST_HPP_

#include "suiteBase.hpp"

class EvaluateTest : public SuiteBase {
public:
    std::string getName() const { return "EvaluateTest"; }

    cute::suite getSuite() const;
};

#endif /* EVALUATETEST_HPP_ */
