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
 * randperm.hpp
 *
 *  Created on: Sep 6, 2023
 *      Author: petero
 */

#ifndef RANDPERM_HPP_
#define RANDPERM_HPP_

#include "util.hpp"

/** Generates a pseudo-random permutation of 0, 1, ..., upperBound-1,
 *  using O(1) memory and O(1) random access time to the permutation.
 *  See: https://en.wikipedia.org/wiki/Feistel_cipher */
class RandPerm {
public:
    RandPerm(U64 upperBound, U64 seed);

    /** Return the i:th element in the permutation. */
    U64 perm(U64 i) const;

private:
    static int getNumBits(U64 upperBound);
    U64 permRaw(U64 i) const;

    constexpr static int rounds = 3;
    const U64 upperBound;
    int loBits;
    int hiBits;
    U64 lowMask;
    U64 fullMask;
    U64 keys[rounds];
};

#endif /* RANDPERM_HPP_ */
