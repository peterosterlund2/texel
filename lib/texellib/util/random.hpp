/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

/** Pseudo-random number generator. */
class Random {
public:
    Random();

    explicit Random(U64 seed);

    void setSeed(U64 seed);

    int nextInt(int modulo);

    U64 nextU64();

private:
    std::mt19937_64 gen;
};


/** "Scrambles" a 64 bit number. The sequence hashU64(i) for i=1,2,3,...
 *   passes "dieharder -a -Y 1". */
inline U64 hashU64(U64 v) {
    v *= 0x7CF9ADC6FE4A7653ULL;
    v ^= v >> 37;
    v *= 0xC25D3F49433E7607ULL;
    v ^= v >> 43;
    return v;
}


inline U64
Random::nextU64() {
    return gen();
}

#endif /* RANDOM_HPP_ */
