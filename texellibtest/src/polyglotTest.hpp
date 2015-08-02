/*
 * polyglotTest.hpp
 *
 *  Created on: Jul 5, 2015
 *      Author: petero
 */

#ifndef POLYGLOTTEST_HPP_
#define POLYGLOTTEST_HPP_

#include "suiteBase.hpp"

class PolyglotTest : public SuiteBase {
    std::string getName() const override { return "PolyglotTest"; }

    cute::suite getSuite() const override;
private:
    static void testHashKey();
    static void testMove();
    static void testSerialize();
};

#endif /* POLYGLOTTEST_HPP_ */
