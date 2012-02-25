/*
 * computerPlayerTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef COMPUTERPLAYERTEST_HPP_
#define COMPUTERPLAYERTEST_HPP_

#include "suiteBase.hpp"

class ComputerPlayerTest : public SuiteBase {
public:
    std::string getName() const { return "ComputerPlayerTest"; }

    cute::suite getSuite() const;
};

#endif /* COMPUTERPLAYERTEST_HPP_ */
