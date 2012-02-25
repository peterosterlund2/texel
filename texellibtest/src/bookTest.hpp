/*
 * bookTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BOOKTEST_HPP_
#define BOOKTEST_HPP_

#include "suiteBase.hpp"

class BookTest : public SuiteBase {
public:
    std::string getName() const { return "BookTest"; }

    cute::suite getSuite() const;
};

#endif /* BOOKTEST_HPP_ */
