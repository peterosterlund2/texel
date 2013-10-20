/*
    Texel - A UCI chess engine.
    Copyright (C) 2013  Peter Österlund, peterosterlund2@gmail.com

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
 * histogram.hpp
 *
 *  Created on: Oct 20, 2013
 *      Author: petero
 */

#ifndef HISTOGRAM_HPP_
#define HISTOGRAM_HPP_

#include "util.hpp"

/** A histogram over integer range [minV,maxV[. Out of range values are ignored. */
template <int minV, int maxV>
class Histogram {
public:
    enum { minValue = minV, maxValue = maxV };

    /** Create an empty histogram. */
    Histogram();

    /** Set all counts to 0. */
    void clear();

    /** Add a data point. */
    void add(int value, int count = 1);

    /** Get count corresponding to value. */
    int get(int value) const;

private:
    int counts[maxV - minV];
};

template <int minV, int maxV>
inline
Histogram<minV,maxV>::Histogram() {
    static_assert(maxV >= minV, "Negative size not allowed");
    clear();
}

template <int minV, int maxV>
inline void
Histogram<minV,maxV>::clear() {
    for (int i = 0; i < (int)COUNT_OF(counts); i++)
        counts[i] = 0;
}

template <int minV, int maxV>
inline void
Histogram<minV,maxV>::add(int value, int count) {
    if ((value >= minV) && (value < maxV))
        counts[value - minV] += count;
}

template <int minV, int maxV>
inline int
Histogram<minV,maxV>::get(int value) const {
    if ((value >= minV) && (value < maxV))
        return counts[value - minV];
    else
        return 0;
}

#endif /* HISTOGRAM_HPP_ */
