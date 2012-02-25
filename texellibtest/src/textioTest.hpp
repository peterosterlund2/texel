/*
 * textioTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TEXTIOTEST_HPP_
#define TEXTIOTEST_HPP_

#include "suiteBase.hpp"

class TextIOTest : public SuiteBase {
public:
    std::string getName() const { return "TextIOTest"; }

    cute::suite getSuite() const;
};

#endif /* TEXTIOTEST_HPP_ */
