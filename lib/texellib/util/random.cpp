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
 * random.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: petero
 */

#include "random.hpp"
#include "timeUtil.hpp"

Random::Random()
    : Random(currentTimeMillis(), 0) {
}

Random::Random(U64 seed1, U64 seed2) {
    setSeed(seed1, seed2);
}

void
Random::setSeed(U64 seed1, U64 seed2) {
    for (int i = 0; i < 4; i++)
        s[i] = hashU64(seed1 + hashU64(i + 1));
    for (int i = 2; i < 4; i++)
        s[i] ^= hashU64(seed2 + hashU64(i + 5));
}

int
Random::nextInt(int modulo) {
    int N = 1 << 30;
    int maxVal = (N / modulo) * modulo;
    while (true) {
        int r = nextU64() & (N - 1);
        if (r < maxVal)
            return r % modulo;
    }
}
