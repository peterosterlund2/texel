/*
 * random.hpp
 *
 *  Created on: Mar 3, 2012
 *      Author: petero
 */

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include "util.hpp"

#include <random>

/**
 * Pseudo-random number generator.
 */
class Random {
public:
    Random();

    Random(U64 seed);

    void setSeed(U64 seed);

    int nextInt(int modulo);

    U64 nextU64();

private:
    std::mt19937_64 gen;
};


#endif /* RANDOM_HPP_ */
