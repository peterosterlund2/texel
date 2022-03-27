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


/** Pseudo-random number generator. */
class Random {
public:
    /** Constructor using a seed based on the current time. */
    Random();

    /** Constructor using a specified random number seed. */
    explicit Random(U64 seed1, U64 seed2 = 0);

    /** Re-initialize the object using the specified seed. */
    void setSeed(U64 seed1, U64 seed2 = 0);

    /** Return a number >= 0 and < modulo. */
    int nextInt(int modulo);

    /** Faster version of nextInt(int) when the modulo is known at compile time .*/
    template<short modulo> int nextInt();

    /** Return a pseudo-random 64-bit number. */
    U64 nextU64();

private:
    U64 s[4];

    static U64 rotl(U64 x, int k);
};

inline U64
Random::rotl(U64 x, int k) {
    return (x << k) | (x >> (64 - k));
}

inline U64
Random::nextU64() {
    U64 result = rotl(s[0] + s[3], 23) + s[0];
    U64 t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl(s[3], 45);
    return result;
}

template<short modulo>
inline int
Random::nextInt() {
    constexpr int N = 1 << 30;
    constexpr int maxVal = (N / modulo) * modulo;
    while (true) {
        int r = nextU64() & (N - 1);
        if (r < maxVal)
            return r % modulo;
    }
}


/** "Scrambles" a 64 bit number. The sequence hashU64(i) for i=1,2,3,...
 *   passes "dieharder -a -Y 1". */
inline U64 hashU64(U64 v) {
    v *= 0x7CF9ADC6FE4A7653ULL;
    v ^= v >> 37;
    v *= 0xC25D3F49433E7607ULL;
    v ^= v >> 43;
    return v;
}

#endif /* RANDOM_HPP_ */
