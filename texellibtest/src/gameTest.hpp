/*
 * gameTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef GAMETEST_HPP_
#define GAMETEST_HPP_

#include "suiteBase.hpp"

class GameTest : public SuiteBase {
public:
    std::string getName() const { return "GameTest"; }

    cute::suite getSuite() const;
};

#endif /* GAMETEST_HPP_ */
