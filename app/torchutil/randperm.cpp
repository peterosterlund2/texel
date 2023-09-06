/*
    Texel - A UCI chess engine.
    Copyright (C) 2023  Peter Österlund, peterosterlund2@gmail.com

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
 * randperm.cpp
 *
 *  Created on: Sep 6, 2023
 *      Author: petero
 */

#include "randperm.hpp"
#include "random.hpp"


RandPerm::RandPerm(U64 upperBound, U64 seed)
    : upperBound(upperBound) {
    int bits = getNumBits(upperBound);
    loBits = bits / 2;
    hiBits = bits - loBits;
    lowMask = (1ULL << loBits) - 1;
    fullMask = (1ULL << bits) - 1;
    for (int k = 0; k < rounds; k++)
        keys[k] = hashU64(hashU64(seed) + k + 1);
}

U64
RandPerm::perm(U64 i) const {
    while (true) {
        i = permRaw(i);
        if (i < upperBound)
            return i;
    }
}

inline U64
RandPerm::permRaw(U64 i) const {
    for (int r = 0; r < rounds; r++) {
        U64 x = i & lowMask;
        x = hashU64(x + keys[r]);
        i ^= x << loBits;
        i &= fullMask;
        i = (i >> loBits) + ((i << hiBits) & fullMask);
    }
    return i;
}

inline int
RandPerm::getNumBits(U64 upperBound) {
    int ret = 1;
    while (upperBound > 1) {
        upperBound >>= 1;
        ret++;
    }
    return ret;
}
