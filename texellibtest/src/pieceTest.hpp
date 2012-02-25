/*
 * pieceTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef PIECETEST_HPP_
#define PIECETEST_HPP_

#include "suiteBase.hpp"

class PieceTest : public SuiteBase {
public:
    std::string getName() const { return "PieceTest"; }

    cute::suite getSuite() const;
};

#endif /* PIECETEST_HPP_ */
