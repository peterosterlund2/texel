/*
    Texel - A UCI chess engine.
    Copyright (C) 2024  Peter Österlund, peterosterlund2@gmail.com

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
 * randpermTest.cpp
 *
 *  Created on: Jan 6, 2024
 *      Author: petero
 */

#include "randpermTest.hpp"
#include "randperm.hpp"
#include "stloutput.hpp"

#include <vector>
#include <algorithm>

#include "gtest/gtest.h"

TEST(RandPermTest, testUnique) {
    RandPermTest::testUnique();
}

void
RandPermTest::testUnique() {
    for (U64 size : {1, 2, 100, 128, 10000, 16383, 16384, 16385}) {
        RandPerm rp(size, 1234);
        std::vector<U64> v(size);
        for (U64 i = 0; i < size; i++)
            v[i] = rp.perm(i);
        std::sort(v.begin(), v.end());
        for (U64 i = 0; i < size; i++)
            ASSERT_EQ(i, v[i]);
    }
}

TEST(RandPermTest, testLarge) {
    RandPermTest::testLarge();
}

void
RandPermTest::testLarge() {
    U64 size = 1000000000000; // 1e12
    RandPerm rp(size, 1234);
    std::vector<U64> v;
    const int N = 10000;
    for (U64 i = 0; i < N; i++)
        v.push_back(rp.perm(i));
    for (int i = 0; i < N; i++)
        ASSERT_NE(i, v[i]); // This could in principle fail, but the probability is extremely small
    std::sort(v.begin(), v.end());
    for (U64 i = 1; i < N; i++)
        ASSERT_LT(v[i-1], v[i]); // Probability for duplicates is extremely small

    // Test that average value is reasonable
    double sum = 0;
    for (U64 i = 0; i < N; i++)
        sum += v[i];
    double mean = sum / N;
    ASSERT_GT(mean, 490000000000);
    ASSERT_LT(mean, 510000000000);
}
