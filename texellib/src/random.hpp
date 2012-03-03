/*
 * random.hpp
 *
 *  Created on: Mar 3, 2012
 *      Author: petero
 */

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include "util.hpp"

class Random {
public:
    Random(U64 seed = 0);

    void setSeed(U64 seed);

    int nextInt(int modulo);
};


#endif /* RANDOM_HPP_ */
