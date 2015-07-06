/*
 * polyglotTest.hpp
 *
 *  Created on: Jul 5, 2015
 *      Author: petero
 */

#ifndef POLYGLOTTEST_HPP_
#define POLYGLOTTEST_HPP_

#include "utilSuiteBase.hpp"

class PolyglotTest : public UtilSuiteBase {
    std::string getName() const override { return "PolyglotTest"; }

    cute::suite getSuite() const override;
private:
    static void testHashKey();
    static void testMove();
};

#endif /* POLYGLOTTEST_HPP_ */
