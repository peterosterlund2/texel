/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * searchTreeSampler.hpp
 *
 *  Created on: Feb 7, 2021
 *      Author: petero
 */

#ifndef SEARCHTREESAMPLER_HPP_
#define SEARCHTREESAMPLER_HPP_

#include "position.hpp"
#include "random.hpp"

class Evaluate;
class SearchTreeSamplerReal;
class SearchTreeSamplerDummy;

/** Change to SearchTreeSamplerReal to enable search tree sampling. */
using SearchTreeSampler = SearchTreeSamplerDummy;


class SearchTreeSamplerReal {
public:
    SearchTreeSamplerReal() {}
    /** With probability 2^-16, store "pos" along with the corresponding
     *  q-search score, for later writing to a file. */
    void sample(const Position& pos, Evaluate& eval, int q0Eval, U64 seed);
    /** Write positions previously logged by calls to sample() to the file
     *  "positions.txt". */
    void writeToFile(int searchScore);

private:
    void doSample(const Position& pos, Evaluate& eval, int q0Eval);

    struct Data {
        Position::SerializeData pos;
    };
    std::vector<Data> samples;
};

class SearchTreeSamplerDummy {
public:
    SearchTreeSamplerDummy() {}
    void sample(const Position& pos, Evaluate& eval, int q0Eval, U64 seed) {}
    void writeToFile(int searchScore) {}
};

inline void
SearchTreeSamplerReal::sample(const Position& pos, Evaluate& eval, int q0Eval, U64 seed) {
    if ((hashU64(seed) & ((1<<16)-1)) == 0)
        doSample(pos, eval, q0Eval);
}

#endif /* SEARCHTREESAMPLER_HPP_ */
