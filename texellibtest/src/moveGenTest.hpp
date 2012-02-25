/*
 * moveGenTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef MOVEGENTEST_HPP_
#define MOVEGENTEST_HPP_

#include "suiteBase.hpp"

class MoveGenTest : public SuiteBase {
public:
    std::string getName() const { return "MoveGenTest"; }

    cute::suite getSuite() const;
};

#endif /* MOVEGENTEST_HPP_ */
