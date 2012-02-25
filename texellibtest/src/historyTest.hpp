/*
 * historyTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef HISTORYTEST_HPP_
#define HISTORYTEST_HPP_

#include "suiteBase.hpp"

class HistoryTest : public SuiteBase {
public:
    std::string getName() const { return "HistoryTest"; }

    cute::suite getSuite() const;
};

#endif /* HISTORYTEST_HPP_ */
