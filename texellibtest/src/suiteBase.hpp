/*
 * suiteBase.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef SUITEBASE_HPP_
#define SUITEBASE_HPP_

#include "cute_suite.h"

#include <string>

class SuiteBase {
public:

    virtual ~SuiteBase() {}

    /** Get the test suite name. */
    virtual std::string getName() const = 0;

    /** Get the test suite. */
    virtual cute::suite getSuite() const = 0;
};

#endif /* SUITEBASE_HPP_ */
