/*
 * random.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: petero
 */

#include "random.hpp"


Random::Random()
    : gen(currentTimeMillis()) {
}

Random::Random(U64 seed)
    : gen(seed) {
}

void
Random::setSeed(U64 seed) {
    gen.seed(seed);
}

int
Random::nextInt(int modulo) {
    std::uniform_int_distribution<int> dist(0, modulo-1);
    return dist(gen);
}

U64
Random::nextU64() {
    return gen();
}
