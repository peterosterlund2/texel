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
 * featureperm.hpp
 *
 *  Created on: Aug 26, 2023
 *      Author: petero
 */

#ifndef FEATUREPERM_HPP_
#define FEATUREPERM_HPP_

#include "nntypes.hpp"
#include "bitSet.hpp"

#include <vector>


/** Permute features in the first layer of a neural network to make the output
 *  from the second layer sparse on average. */
class FeaturePerm {
public:
    /** Constructor. */
    FeaturePerm(NetData& net);

    constexpr static int maxN = 1024 * 1024 * 8;
    using BitSet = ::BitSet<maxN>;

    /** Permute net to minimize the number of non-zero groups for the given
     *  feature activations. */
    void permute(const std::vector<BitSet>& featureActivations, int nPos);

private:
    /** Compute initial feature permutation using a greedy algorithm. */
    void computeGreedyPerm(const std::vector<BitSet>& featureActivations,
                           int nPos, std::vector<int>& permutation);

    /** Permutes the first layer features according to permutation vector.
     *  Overwrites "permutation". */
    void permuteNet(std::vector<int>& permutation);

    NetData& net;
};

#endif /* FEATUREPERM_HPP_ */
