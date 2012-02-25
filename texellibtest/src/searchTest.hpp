/*
 * searchTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef SEARCHTEST_HPP_
#define SEARCHTEST_HPP_

#include "suiteBase.hpp"

class SearchTest : public SuiteBase {
public:
    std::string getName() const { return "SearchTest"; }

    cute::suite getSuite() const;
};

#endif /* SEARCHTEST_HPP_ */
