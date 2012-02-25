/*
 * pieceTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef MOVETEST_HPP_
#define MOVETEST_HPP_

#include "suiteBase.hpp"

class MoveTest : public SuiteBase {
public:
    std::string getName() const { return "MoveTest"; }

    cute::suite getSuite() const;
};

#endif /* MOVETEST_HPP_ */
