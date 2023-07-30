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

#ifdef HAS_AVX2
#include <immintrin.h>
#endif
#ifdef HAS_SSSE3
#include <smmintrin.h>
#endif

#if defined(HAS_NEON) || defined(HAS_NEON_DOT)
#include <arm_neon.h>
#endif

// ------------------------------------------------------------------------------

#ifdef HAS_AVX2

/** Return the sum of the 8 32-bit values in an AVX2 256-bit register. */
inline S32
avx2_hadd_32(__m256i v) {
    v = _mm256_hadd_epi32(v, v);
    v = _mm256_hadd_epi32(v, v);
    return _mm256_extract_epi32(v, 0) + _mm256_extract_epi32(v, 4);
}

#endif

#ifdef HAS_SSSE3

/** Return the sum of the 4 32-bit values in an SSSE3 128-bit register. */
inline S32
ssse3_hadd_32(__m128i v) {
    v = _mm_hadd_epi32(v, v);
    v = _mm_hadd_epi32(v, v);
    return _mm_cvtsi128_si32(v);
}

#endif

// ------------------------------------------------------------------------------

/** Compute result += weight * in, where "*" is matrix multiplication. */
template <int nIn, int nOut>
inline void
matMul(Vector<S32,nOut>& result, const Matrix<S8,nOut,nIn>& weight, const Vector<S8,nIn>& in) {
#ifdef HAS_AVX2
    if (nIn % 32 == 0) {
        // Note that this only works because all elements in "in" are non-negative
        __m256i ones16 = _mm256_set_epi16(1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1);
        for (int i = 0; i < nOut; i++) {
            __m256i sum = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0);
            for (int j = 0; j < nIn; j += 32) {
                __m256i a = _mm256_loadu_si256((const __m256i*)&weight(i,j));
                __m256i b = _mm256_loadu_si256((const __m256i*)&in(j));
                __m256i d = _mm256_maddubs_epi16(b, a); // d[i]=a[2i]*b[2i]+a[2i+1]*b[2i+1], requires b>=0
                d = _mm256_madd_epi16(d, ones16);       // Pairwise sum of 16-bit values to 32-bit values
                sum = _mm256_add_epi32(sum, d);         // Accumulate 8 sums
            }
            result(i) += avx2_hadd_32(sum);             // Combine 8 32-bit values to one
        }
        return;
    }
#endif
#ifdef HAS_SSSE3
    if (nIn % 16 == 0) {
        // Note that this only works because all elements in "in" are non-negative
        __m128i ones16 = _mm_set_epi16(1,1,1,1,1,1,1,1);
        for (int i = 0; i < nOut; i++) {
            __m128i sum = _mm_set_epi32(0, 0, 0, 0);
            for (int j = 0; j < nIn; j += 16) {
                __m128i a = _mm_loadu_si128((const __m128i*)&weight(i,j));
                __m128i b = _mm_loadu_si128((const __m128i*)&in(j));
                __m128i d = _mm_maddubs_epi16(b, a); // d[i]=a[2i]*b[2i]+a[2i+1]*b[2i+1], requires b>=0
                d = _mm_madd_epi16(d, ones16);       // Pairwise sum of 16-bit values to 32-bit values
                sum = _mm_add_epi32(sum, d);         // Accumulate 4 sums
            }
            result(i) += ssse3_hadd_32(sum);         // Combine 4 32-bit values to one
        }
        return;
    }
#endif
#ifdef HAS_NEON_DOT
    if (nIn % 16 == 0) {
        if (nOut % 2 != 0) {
            for (int i = 0; i < nOut; i++) {
                int32x4_t sum = {0,0,0,0};
                for (int j = 0; j < nIn; j += 16) {
                    int8x16_t w = vld1q_s8((const int8_t*)&weight(i,j));
                    int8x16_t d = vld1q_s8((const int8_t*)&in(j));
                    sum = vdotq_s32(sum, w, d);
                }
                result(i) += vaddvq_s32(sum);
            }
        } else {
            for (int i = 0; i < nOut; i += 2) {
                int32x4_t sum1 = {0,0,0,0};
                int32x4_t sum2 = {0,0,0,0};
                for (int j = 0; j < nIn; j += 16) {
                    int8x16_t d = vld1q_s8((const int8_t*)&in(j));
                    int8x16_t w1 = vld1q_s8((const int8_t*)&weight(i,j));
                    sum1 = vdotq_s32(sum1, w1, d);
                    int8x16_t w2 = vld1q_s8((const int8_t*)&weight(i+1,j));
                    sum2 = vdotq_s32(sum2, w2, d);
                }
                result(i  ) += vaddvq_s32(sum1);
                result(i+1) += vaddvq_s32(sum2);
            }
        }
        return;
    }
#endif
#ifdef HAS_NEON
    if (nIn % 16 == 0) {
        for (int i = 0; i < nOut; i++) {
            int32x4_t sum = {0,0,0,0};
            for (int j = 0; j < nIn; j += 16) {
                int8x8_t w = vld1_s8((const int8_t*)&weight(i,j));
                int8x8_t d = vld1_s8((const int8_t*)&in(j));
                int16x8_t s = vmull_s8(d, w);
                w = vld1_s8((const int8_t*)&weight(i,j+8));
                d = vld1_s8((const int8_t*)&in(j+8));
                s = vmlal_s8(s, d, w);
                sum = vpadalq_s16(sum, s);
            }
#ifdef __ARM_EABI__
            result(i) += sum[0] + sum[1] + sum[2] + sum[3];
#else
            result(i) += vaddvq_s32(sum);
#endif
        }
        return;
    }
#endif

    // Generic fallback
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
