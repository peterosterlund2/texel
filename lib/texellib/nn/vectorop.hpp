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

#if defined(HAS_NEON) || defined(HAS_NEON_DOT)

/** Return the sum of the 4 32-bit values in a NEON 128-bit register. */
inline S32
neon_hadd_32(int32x4_t sum) {
#ifdef __ARM_EABI__
    return sum[0] + sum[1] + sum[2] + sum[3];
#else
    return vaddvq_s32(sum);
#endif
}

#endif

// ------------------------------------------------------------------------------

/** Rearrange data in "weight" to be compatible with assumptions in matMul(). */
template <int nIn, int nOut>
void
prepareMatMul(Matrix<S8,nOut,nIn>& weight) {
#ifdef HAS_AVX2
    if ((nIn % 8 == 0) && (nOut % 32) == 0) {
        Matrix<S8,nOut,nIn> w2;
        auto weight2 = [&w2](int i, int j) -> S8& {
            int idx = j * 8 + i * nIn;
            return w2(0, idx);
        };
        for (int i = 0; i < nOut; i += 8) {
            for (int j = 0; j < nIn; j += 4) {
                S8* start = &weight2(i, j);
                for (int y = 0; y < 8; y++)
                    for (int x = 0; x < 4; x++)
                        *start++ = weight(i+y, j+x);
            }
        }
        weight = w2;
        return;
    }
#endif
#if defined(HAS_SSSE3) || defined(HAS_NEON_DOT)
    if ((nIn % 8 == 0) && (nOut % 16) == 0) {
        Matrix<S8,nOut,nIn> w2;
        auto weight2 = [&w2](int i, int j) -> S8& {
            int idx = j * 4 + i * nIn;
            return w2(0, idx);
        };
        for (int i = 0; i < nOut; i += 4) {
            for (int j = 0; j < nIn; j += 4) {
                S8* start = &weight2(i, j);
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++)
                        *start++ = weight(i+y, j+x);
            }
        }
        weight = w2;
        return;
    }
#endif
}

/** Compute result += weight * in, where "*" is matrix multiplication.
 * Note that the AVX2/SSSE3 implementations assume all elements in "in" are >= 0. */
template <int nIn, int nOut>
inline void
matMul(Vector<S32,nOut>& result, const Matrix<S8,nOut,nIn>& weight, const Vector<S8,nIn>& in) {
#ifdef HAS_AVX2
    if ((nIn % 8 == 0) && (nOut % 32) == 0) {
        __m256i ones16 = _mm256_set_epi16(1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1);
        for (int i = 0; i < nOut; i += 32) {
            __m256i sum1 = _mm256_load_si256((const __m256i*)&result(i+8*0));
            __m256i sum2 = _mm256_load_si256((const __m256i*)&result(i+8*1));
            __m256i sum3 = _mm256_load_si256((const __m256i*)&result(i+8*2));
            __m256i sum4 = _mm256_load_si256((const __m256i*)&result(i+8*3));
            for (int j = 0; j < nIn; j += 8) {
                auto f = [&weight,&ones16](__m256i b, int i, int j, __m256i& sum) {
                    int idx = j * 8 + i * nIn;
                    __m256i a = _mm256_load_si256((const __m256i*)&weight(0, idx));
                    __m256i d = _mm256_maddubs_epi16(b, a); // d[i]=a[2i]*b[2i]+a[2i+1]*b[2i+1], requires b>=0
                    d = _mm256_madd_epi16(d, ones16);       // Pairwise sum of 16-bit values to 32-bit values
                    sum = _mm256_add_epi32(sum, d);         // Accumulate 8 sums
                };
                __m256i b = _mm256_set1_epi32(*(int*)&in(j+4*0));
                f(b, i+8*0, j+4*0, sum1);
                f(b, i+8*1, j+4*0, sum2);
                f(b, i+8*2, j+4*0, sum3);
                f(b, i+8*3, j+4*0, sum4);
                b = _mm256_set1_epi32(*(int*)&in(j+4*1));
                f(b, i+8*0, j+4*1, sum1);
                f(b, i+8*1, j+4*1, sum2);
                f(b, i+8*2, j+4*1, sum3);
                f(b, i+8*3, j+4*1, sum4);
            }
            _mm256_store_si256((__m256i*)&result(i+8*0), sum1);
            _mm256_store_si256((__m256i*)&result(i+8*1), sum2);
            _mm256_store_si256((__m256i*)&result(i+8*2), sum3);
            _mm256_store_si256((__m256i*)&result(i+8*3), sum4);
        }
        return;
    }
    if (nIn % 32 == 0) {
        __m256i ones16 = _mm256_set_epi16(1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1);
        for (int i = 0; i < nOut; i++) {
            __m256i sum = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0);
            for (int j = 0; j < nIn; j += 32) {
                __m256i a = _mm256_load_si256((const __m256i*)&weight(i,j));
                __m256i b = _mm256_load_si256((const __m256i*)&in(j));
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
    if ((nIn % 8 == 0) && (nOut % 16) == 0) {
        __m128i ones16 = _mm_set_epi16(1,1,1,1,1,1,1,1);
        for (int i = 0; i < nOut; i += 16) {
            __m128i sum1 = _mm_load_si128((const __m128i*)&result(i+4*0));
            __m128i sum2 = _mm_load_si128((const __m128i*)&result(i+4*1));
            __m128i sum3 = _mm_load_si128((const __m128i*)&result(i+4*2));
            __m128i sum4 = _mm_load_si128((const __m128i*)&result(i+4*3));
            for (int j = 0; j < nIn; j += 8) {
                auto f = [&weight,&ones16](__m128i b, int i, int j, __m128i& sum) {
                    int idx = j * 4 + i * nIn;
                    __m128i a = _mm_load_si128((const __m128i*)&weight(0, idx));
                    __m128i d = _mm_maddubs_epi16(b, a); // d[i]=a[2i]*b[2i]+a[2i+1]*b[2i+1], requires b>=0
                    d = _mm_madd_epi16(d, ones16);       // Pairwise sum of 16-bit values to 32-bit values
                    sum = _mm_add_epi32(sum, d);         // Accumulate 4 sums
                };
                __m128i b = _mm_set1_epi32(*(int*)&in(j+4*0));
                f(b, i+4*0, j+4*0, sum1);
                f(b, i+4*1, j+4*0, sum2);
                f(b, i+4*2, j+4*0, sum3);
                f(b, i+4*3, j+4*0, sum4);
                b = _mm_set1_epi32(*(int*)&in(j+4*1));
                f(b, i+4*0, j+4*1, sum1);
                f(b, i+4*1, j+4*1, sum2);
                f(b, i+4*2, j+4*1, sum3);
                f(b, i+4*3, j+4*1, sum4);
            }
            _mm_store_si128((__m128i*)&result(i+4*0), sum1);
            _mm_store_si128((__m128i*)&result(i+4*1), sum2);
            _mm_store_si128((__m128i*)&result(i+4*2), sum3);
            _mm_store_si128((__m128i*)&result(i+4*3), sum4);
        }
        return;
    }
    if (nIn % 16 == 0) {
        __m128i ones16 = _mm_set_epi16(1,1,1,1,1,1,1,1);
        for (int i = 0; i < nOut; i++) {
            __m128i sum = _mm_set_epi32(0, 0, 0, 0);
            for (int j = 0; j < nIn; j += 16) {
                __m128i a = _mm_load_si128((const __m128i*)&weight(i,j));
                __m128i b = _mm_load_si128((const __m128i*)&in(j));
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
    if ((nIn % 8 == 0) && (nOut % 16) == 0) {
        for (int i = 0; i < nOut; i += 16) {
            int32x4_t sum1 = vld1q_s32((const int32_t*)&result(i+4*0));
            int32x4_t sum2 = vld1q_s32((const int32_t*)&result(i+4*1));
            int32x4_t sum3 = vld1q_s32((const int32_t*)&result(i+4*2));
            int32x4_t sum4 = vld1q_s32((const int32_t*)&result(i+4*3));
            for (int j = 0; j < nIn; j += 8) {
                auto f = [&weight](int8x16_t b, int i, int j, int32x4_t& sum) {
                    int idx = j * 4 + i * nIn;
                    int8x16_t w = vld1q_s8((const int8_t*)&weight(0, idx));
                    sum = vdotq_s32(sum, w, b);
                };
                int8x16_t b = vreinterpretq_s8_s32(vdupq_n_s32(*(int32_t*)&in(j+4*0)));
                f(b, i+4*0, j+4*0, sum1);
                f(b, i+4*1, j+4*0, sum2);
                f(b, i+4*2, j+4*0, sum3);
                f(b, i+4*3, j+4*0, sum4);
                b = vreinterpretq_s8_s32(vdupq_n_s32(*(int32_t*)&in(j+4*1)));
                f(b, i+4*0, j+4*1, sum1);
                f(b, i+4*1, j+4*1, sum2);
                f(b, i+4*2, j+4*1, sum3);
                f(b, i+4*3, j+4*1, sum4);
            }
            vst1q_s32((int32_t*)&result(i+4*0), sum1);
            vst1q_s32((int32_t*)&result(i+4*1), sum2);
            vst1q_s32((int32_t*)&result(i+4*2), sum3);
            vst1q_s32((int32_t*)&result(i+4*3), sum4);
        }
        return;
    }
    if (nIn % 16 == 0) {
        if (nOut % 2 != 0) {
            for (int i = 0; i < nOut; i++) {
                int32x4_t sum = {0,0,0,0};
                for (int j = 0; j < nIn; j += 16) {
                    int8x16_t w = vld1q_s8((const int8_t*)&weight(i,j));
                    int8x16_t d = vld1q_s8((const int8_t*)&in(j));
                    sum = vdotq_s32(sum, w, d);
                }
                result(i) += neon_hadd_32(sum);
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
                result(i  ) += neon_hadd_32(sum1);
                result(i+1) += neon_hadd_32(sum2);
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
            result(i) += neon_hadd_32(sum);
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
Layer<nIn,nOut>::forward(const Vector<S8,nIn>& in, Output& out) {
    evalLinear(in, out);
    for (int i = 0; i < nOut; i++)
        out.output(i) = static_cast<S8>(clamp(out.linOutput(i) >> 6, 0, 127));
}

template <int nIn, int nOut>
inline void
Layer<nIn,nOut>::evalLinear(const Vector<S8,nIn>& in, Output& out) {
    out.linOutput = data.bias;
    matMul(out.linOutput, data.weight, in);
}

// ------------------------------------------------------------------------------

/** Add/subtract rows of "weight1" to/from "l1Out". */
template <int n1, int inFeatures>
void
addSubWeights(Vector<S16, n1>& l1Out, const Matrix<S16, inFeatures, n1>& weight1,
              const int* toAdd, int toAddLen,
              const int* toSub, int toSubLen) {
#ifdef HAS_AVX2
    if (n1 % 128 == 0) {
        for (int i = 0; i < n1; i += 128) {
            __m256i s1 = _mm256_load_si256((const __m256i*)&l1Out(i+16*0));
            __m256i s2 = _mm256_load_si256((const __m256i*)&l1Out(i+16*1));
            __m256i s3 = _mm256_load_si256((const __m256i*)&l1Out(i+16*2));
            __m256i s4 = _mm256_load_si256((const __m256i*)&l1Out(i+16*3));
            __m256i s5 = _mm256_load_si256((const __m256i*)&l1Out(i+16*4));
            __m256i s6 = _mm256_load_si256((const __m256i*)&l1Out(i+16*5));
            __m256i s7 = _mm256_load_si256((const __m256i*)&l1Out(i+16*6));
            __m256i s8 = _mm256_load_si256((const __m256i*)&l1Out(i+16*7));
            for (int k = 0; k < toAddLen; k++) {
                int idx = toAdd[k];
                s1 = _mm256_add_epi16(s1, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*0)));
                s2 = _mm256_add_epi16(s2, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*1)));
                s3 = _mm256_add_epi16(s3, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*2)));
                s4 = _mm256_add_epi16(s4, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*3)));
                s5 = _mm256_add_epi16(s5, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*4)));
                s6 = _mm256_add_epi16(s6, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*5)));
                s7 = _mm256_add_epi16(s7, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*6)));
                s8 = _mm256_add_epi16(s8, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*7)));
            }
            for (int k = 0; k < toSubLen; k++) {
                int idx = toSub[k];
                s1 = _mm256_sub_epi16(s1, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*0)));
                s2 = _mm256_sub_epi16(s2, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*1)));
                s3 = _mm256_sub_epi16(s3, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*2)));
                s4 = _mm256_sub_epi16(s4, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*3)));
                s5 = _mm256_sub_epi16(s5, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*4)));
                s6 = _mm256_sub_epi16(s6, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*5)));
                s7 = _mm256_sub_epi16(s7, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*6)));
                s8 = _mm256_sub_epi16(s8, _mm256_load_si256((const __m256i*)&weight1(idx, i+16*7)));
            }
            _mm256_store_si256((__m256i*)&l1Out(i+16*0), s1);
            _mm256_store_si256((__m256i*)&l1Out(i+16*1), s2);
            _mm256_store_si256((__m256i*)&l1Out(i+16*2), s3);
            _mm256_store_si256((__m256i*)&l1Out(i+16*3), s4);
            _mm256_store_si256((__m256i*)&l1Out(i+16*4), s5);
            _mm256_store_si256((__m256i*)&l1Out(i+16*5), s6);
            _mm256_store_si256((__m256i*)&l1Out(i+16*6), s7);
            _mm256_store_si256((__m256i*)&l1Out(i+16*7), s8);
        }
        return;
    }
#endif
#ifdef HAS_SSSE3
    if (n1 % 64 == 0) {
        for (int i = 0; i < n1; i += 64) {
            __m128i s1 = _mm_load_si128((const __m128i*)&l1Out(i+8*0));
            __m128i s2 = _mm_load_si128((const __m128i*)&l1Out(i+8*1));
            __m128i s3 = _mm_load_si128((const __m128i*)&l1Out(i+8*2));
            __m128i s4 = _mm_load_si128((const __m128i*)&l1Out(i+8*3));
            __m128i s5 = _mm_load_si128((const __m128i*)&l1Out(i+8*4));
            __m128i s6 = _mm_load_si128((const __m128i*)&l1Out(i+8*5));
            __m128i s7 = _mm_load_si128((const __m128i*)&l1Out(i+8*6));
            __m128i s8 = _mm_load_si128((const __m128i*)&l1Out(i+8*7));
            for (int k = 0; k < toAddLen; k++) {
                int idx = toAdd[k];
                s1 = _mm_add_epi16(s1, _mm_load_si128((const __m128i*)&weight1(idx, i+8*0)));
                s2 = _mm_add_epi16(s2, _mm_load_si128((const __m128i*)&weight1(idx, i+8*1)));
                s3 = _mm_add_epi16(s3, _mm_load_si128((const __m128i*)&weight1(idx, i+8*2)));
                s4 = _mm_add_epi16(s4, _mm_load_si128((const __m128i*)&weight1(idx, i+8*3)));
                s5 = _mm_add_epi16(s5, _mm_load_si128((const __m128i*)&weight1(idx, i+8*4)));
                s6 = _mm_add_epi16(s6, _mm_load_si128((const __m128i*)&weight1(idx, i+8*5)));
                s7 = _mm_add_epi16(s7, _mm_load_si128((const __m128i*)&weight1(idx, i+8*6)));
                s8 = _mm_add_epi16(s8, _mm_load_si128((const __m128i*)&weight1(idx, i+8*7)));
            }
            for (int k = 0; k < toSubLen; k++) {
                int idx = toSub[k];
                s1 = _mm_sub_epi16(s1, _mm_load_si128((const __m128i*)&weight1(idx, i+8*0)));
                s2 = _mm_sub_epi16(s2, _mm_load_si128((const __m128i*)&weight1(idx, i+8*1)));
                s3 = _mm_sub_epi16(s3, _mm_load_si128((const __m128i*)&weight1(idx, i+8*2)));
                s4 = _mm_sub_epi16(s4, _mm_load_si128((const __m128i*)&weight1(idx, i+8*3)));
                s5 = _mm_sub_epi16(s5, _mm_load_si128((const __m128i*)&weight1(idx, i+8*4)));
                s6 = _mm_sub_epi16(s6, _mm_load_si128((const __m128i*)&weight1(idx, i+8*5)));
                s7 = _mm_sub_epi16(s7, _mm_load_si128((const __m128i*)&weight1(idx, i+8*6)));
                s8 = _mm_sub_epi16(s8, _mm_load_si128((const __m128i*)&weight1(idx, i+8*7)));
            }
            _mm_store_si128((__m128i*)&l1Out(i+8*0), s1);
            _mm_store_si128((__m128i*)&l1Out(i+8*1), s2);
            _mm_store_si128((__m128i*)&l1Out(i+8*2), s3);
            _mm_store_si128((__m128i*)&l1Out(i+8*3), s4);
            _mm_store_si128((__m128i*)&l1Out(i+8*4), s5);
            _mm_store_si128((__m128i*)&l1Out(i+8*5), s6);
            _mm_store_si128((__m128i*)&l1Out(i+8*6), s7);
            _mm_store_si128((__m128i*)&l1Out(i+8*7), s8);
        }
        return;
    }
#endif
#if defined(HAS_NEON) || defined(HAS_NEON_DOT)
    if (n1 % 64 == 0) {
        for (int i = 0; i < n1; i += 64) {
            int16x8_t s1 = vld1q_s16((const int16_t*)&l1Out(i+8*0));
            int16x8_t s2 = vld1q_s16((const int16_t*)&l1Out(i+8*1));
            int16x8_t s3 = vld1q_s16((const int16_t*)&l1Out(i+8*2));
            int16x8_t s4 = vld1q_s16((const int16_t*)&l1Out(i+8*3));
            int16x8_t s5 = vld1q_s16((const int16_t*)&l1Out(i+8*4));
            int16x8_t s6 = vld1q_s16((const int16_t*)&l1Out(i+8*5));
            int16x8_t s7 = vld1q_s16((const int16_t*)&l1Out(i+8*6));
            int16x8_t s8 = vld1q_s16((const int16_t*)&l1Out(i+8*7));
            for (int k = 0; k < toAddLen; k++) {
                int idx = toAdd[k];
                s1 = vaddq_s16(s1, vld1q_s16((const int16_t*)&weight1(idx, i+8*0)));
                s2 = vaddq_s16(s2, vld1q_s16((const int16_t*)&weight1(idx, i+8*1)));
                s3 = vaddq_s16(s3, vld1q_s16((const int16_t*)&weight1(idx, i+8*2)));
                s4 = vaddq_s16(s4, vld1q_s16((const int16_t*)&weight1(idx, i+8*3)));
                s5 = vaddq_s16(s5, vld1q_s16((const int16_t*)&weight1(idx, i+8*4)));
                s6 = vaddq_s16(s6, vld1q_s16((const int16_t*)&weight1(idx, i+8*5)));
                s7 = vaddq_s16(s7, vld1q_s16((const int16_t*)&weight1(idx, i+8*6)));
                s8 = vaddq_s16(s8, vld1q_s16((const int16_t*)&weight1(idx, i+8*7)));
            }
            for (int k = 0; k < toSubLen; k++) {
                int idx = toSub[k];
                s1 = vsubq_s16(s1, vld1q_s16((const int16_t*)&weight1(idx, i+8*0)));
                s2 = vsubq_s16(s2, vld1q_s16((const int16_t*)&weight1(idx, i+8*1)));
                s3 = vsubq_s16(s3, vld1q_s16((const int16_t*)&weight1(idx, i+8*2)));
                s4 = vsubq_s16(s4, vld1q_s16((const int16_t*)&weight1(idx, i+8*3)));
                s5 = vsubq_s16(s5, vld1q_s16((const int16_t*)&weight1(idx, i+8*4)));
                s6 = vsubq_s16(s6, vld1q_s16((const int16_t*)&weight1(idx, i+8*5)));
                s7 = vsubq_s16(s7, vld1q_s16((const int16_t*)&weight1(idx, i+8*6)));
                s8 = vsubq_s16(s8, vld1q_s16((const int16_t*)&weight1(idx, i+8*7)));
            }
            vst1q_s16((int16_t*)&l1Out(i+8*0), s1);
            vst1q_s16((int16_t*)&l1Out(i+8*1), s2);
            vst1q_s16((int16_t*)&l1Out(i+8*2), s3);
            vst1q_s16((int16_t*)&l1Out(i+8*3), s4);
            vst1q_s16((int16_t*)&l1Out(i+8*4), s5);
            vst1q_s16((int16_t*)&l1Out(i+8*5), s6);
            vst1q_s16((int16_t*)&l1Out(i+8*6), s7);
            vst1q_s16((int16_t*)&l1Out(i+8*7), s8);
        }
        return;
    }
#endif

    // Generic fallback
    for (int k = 0; k < toAddLen; k++) {
        int idx = toAdd[k];
        for (int i = 0; i < n1; i++)
            l1Out(i) += weight1(idx, i);
    }
    for (int k = 0; k < toSubLen; k++) {
        int idx = toSub[k];
        for (int i = 0; i < n1; i++)
            l1Out(i) -= weight1(idx, i);
    }
}

// ------------------------------------------------------------------------------

template <int n1>
inline void
scaleClipPack(S8* out, const Vector<S16, n1>& l1OutC) {
#ifdef HAS_AVX2
    if (n1 % 128 == 0) {
        __m256i zero = _mm256_set1_epi8(0);
        __m256i idx = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
        for (int i = 0; i < n1; i += 128) {
            auto f = [&](int i) {
                __m256i a = _mm256_load_si256((const __m256i*)&l1OutC(i));
                __m256i b = _mm256_load_si256((const __m256i*)&l1OutC(i+16));
                a = _mm256_srai_epi16(a, 2);
                b = _mm256_srai_epi16(b, 2);
                __m256i r = _mm256_packs_epi16(a, b);   // a0 a1 b0 b1 a2 a3 b2 b3
                r = _mm256_max_epi8(r, zero);
                r = _mm256_permutevar8x32_epi32(r, idx);
                _mm256_store_si256((__m256i*)&out[i], r);
            };
            f(i+32*0);
            f(i+32*1);
            f(i+32*2);
            f(i+32*3);
        }
        return;
    }
#endif
#ifdef HAS_SSSE3
    if (n1 % 64 == 0) {
        __m128i zero = _mm_set1_epi16(0);
        for (int i = 0; i < n1; i += 64) {
            auto f = [&](int i) {
                __m128i a = _mm_load_si128((const __m128i*)&l1OutC(i));
                __m128i b = _mm_load_si128((const __m128i*)&l1OutC(i+8));
                a = _mm_srai_epi16(a, 2);
                b = _mm_srai_epi16(b, 2);
                a = _mm_max_epi16(a, zero);
                b = _mm_max_epi16(b, zero);
                __m128i r = _mm_packs_epi16(a, b);
                _mm_store_si128((__m128i*)&out[i], r);
            };
            f(i+16*0);
            f(i+16*1);
            f(i+16*2);
            f(i+16*3);
        }
        return;
    }
#endif
#if defined(HAS_NEON) || defined(HAS_NEON_DOT)
    if (n1 % 64 == 0) {
        int8x16_t zero = vdupq_n_s8(0);
        for (int i = 0; i < n1; i += 64) {
            auto f = [&](int i) {
                int16x8_t a = vld1q_s16((const int16_t*)&l1OutC(i));
                int16x8_t b = vld1q_s16((const int16_t*)&l1OutC(i+8));
                int8x8_t a2 = vqshrn_n_s16(a, 2);
                int8x8_t b2 = vqshrn_n_s16(b, 2);
                int8x16_t r = vcombine_s8(a2, b2);
                r = vmaxq_s8(r, zero);
                vst1q_s8((int8_t*)&out[i], r);
            };
            f(i+16*0);
            f(i+16*1);
            f(i+16*2);
            f(i+16*3);
        }
        return;
    }
#endif

    // Generic fallback
    for (int i = 0; i < n1; i++)
        out[i] = clamp(l1OutC(i) >> 2, 0, 127);
}

#endif /* VECTOROP_HPP_ */
