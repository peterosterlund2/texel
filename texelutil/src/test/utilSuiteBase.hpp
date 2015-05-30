/*
 * utilSuiteBase.hpp
 *
 *  Created on: May 29, 2015
 *      Author: petero
 */

#ifndef UTILSUITEBASE_HPP_
#define UTILSUITEBASE_HPP_

#include "cute_suite.h"

#include <string>

class UtilSuiteBase {
public:

    virtual ~UtilSuiteBase() {}

    /** Get the test suite name. */
    virtual std::string getName() const = 0;

    /** Get the test suite. */
    virtual cute::suite getSuite() const = 0;
};

#endif /* UTILSUITEBASE_HPP_ */
