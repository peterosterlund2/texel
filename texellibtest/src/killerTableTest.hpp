/*
 * killerTableTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef KILLERTABLETEST_HPP_
#define KILLERTABLETEST_HPP_

#include "suiteBase.hpp"

class KillerTableTest : public SuiteBase {
public:
    std::string getName() const { return "KillerTableTest"; }

    cute::suite getSuite() const;
};

#endif /* KILLERTABLETEST_HPP_ */
