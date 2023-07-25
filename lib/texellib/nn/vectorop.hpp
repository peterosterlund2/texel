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
 * vectorop.hpp
 *
 *  Created on: Jul 25, 2023
 *      Author: petero
 */

#ifndef VECTOROP_HPP_
#define VECTOROP_HPP_

#include "nntypes.hpp"

// ------------------------------------------------------------------------------

/** Compute result += weight * in, where "*" is matrix multiplication. */
template <int nIn, int nOut>
void
matMul(Vector<S32,nOut>& result, const Matrix<S8,nOut,nIn>& weight, const Vector<S8,nIn>& in) {
    for (int i = 0; i < nOut; i++) {
        S32 sum = 0;
        for (int j = 0; j < nIn; j++)
            sum += weight(i,j) * in(j);
        result(i) += sum;
    }
}

// ------------------------------------------------------------------------------

template <int nIn, int nOut>
inline void
Layer<nIn,nOut>::forward(const Vector<S8,nIn>& in) {
    evalLinear(in);
    for (int i = 0; i < nOut; i++)
        output(i) = static_cast<S8>(clamp(linOutput(i) >> 6, 0, 127));
}

template <int nIn, int nOut>
inline void
Layer<nIn,nOut>::evalLinear(const Vector<S8,nIn>& in) {
    for (int i = 0; i < nOut; i++)
        linOutput(i) = data.bias(i);
    matMul(linOutput, data.weight, in);
}

#endif /* VECTOROP_HPP_ */
