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
 * cspsolverTest.cpp
 *
 *  Created on: Nov 21, 2021
 *      Author: petero
 */

#include "cspsolverTest.hpp"
#include "cspsolver.hpp"
#include "stloutput.hpp"

#include "gtest/gtest.h"

const CspSolver::Oper LE = CspSolver::LE;
const CspSolver::Oper GE = CspSolver::GE;

const auto SMALL = CspSolver::PrefVal::SMALL;
const auto LARGE = CspSolver::PrefVal::LARGE;
const auto MIDDLE_SMALL = CspSolver::PrefVal::MIDDLE_SMALL;
const auto MIDDLE_LARGE = CspSolver::PrefVal::MIDDLE_LARGE;


TEST(CspSolverTest, testBitSet) {
    CspSolverTest::testBitSet();
}

template <int N, int offs>
static void testBits() {
    BitSet<N,offs> bs;
    ASSERT_EQ(0, bs.bitCount());
    bs.setRange(offs, offs+N-1);
    ASSERT_EQ(N, bs.bitCount());
    bs.removeOdd();
    ASSERT_EQ(N/2, bs.bitCount());
    bs.removeEven();
    ASSERT_EQ(0, bs.bitCount());

    for (int i = 0; i < N; i++) {
        bs.setBit(offs+i);
        ASSERT_TRUE(bs.getBit(offs+i));
        if (i + 1 < N) {
            ASSERT_FALSE(bs.getBit(offs+i+1));
        }
        ASSERT_EQ(i+1, bs.bitCount());
        ASSERT_EQ(offs+0, bs.getMinBit());
        ASSERT_EQ(offs+i, bs.getMaxBit());

    }
    for (int i = 0; i < N; i++) {
        bs.clearBit(offs+i);
        ASSERT_EQ(N-1-i, bs.bitCount());
        if (bs.bitCount() > 0) {
            ASSERT_EQ(offs+i+1, bs.getMinBit());
            ASSERT_EQ(offs+N-1, bs.getMaxBit());
            ASSERT_FALSE(bs.empty());
        } else {
            ASSERT_TRUE(bs.empty());
        }
    }
}

void
CspSolverTest::testBitSet() {
    testBits<64, 0>();
    testBits<64*2, 0>();
    testBits<64*3, 0>();
    testBits<64*40, 0>();
    testBits<64, 17>();
    testBits<64, 100>();
    testBits<64, 101>();
    testBits<64, -100>();
    testBits<64, -101>();
    testBits<64*2, -37>();
    testBits<64*3, 56>();
    testBits<64*40, -3990>();
    {
        BitSet<64> bs1;
        bs1.setRange(3,10);
        BitSet<64> bs2;
        bs2.setRange(7,14);
        {
            BitSet<64> bs(bs1);
            bs |= bs2;
            ASSERT_EQ(12, bs.bitCount());
        }
        {
            BitSet<64> bs;
            bs = bs1;
            ASSERT_TRUE(bs == bs1);
            ASSERT_FALSE(bs != bs1);
            bs &= bs2;
            ASSERT_FALSE(bs == bs1);
            ASSERT_TRUE(bs != bs1);
            ASSERT_EQ(4, bs.bitCount());
        }
        bs2.removeLarger(12);
        ASSERT_EQ(6, bs2.bitCount());
        bs2.removeLarger(63);
        ASSERT_EQ(6, bs2.bitCount());
        bs2.removeSmaller(9);
        ASSERT_EQ(4, bs2.bitCount());
        bs2.removeSmaller(0);
        ASSERT_EQ(4, bs2.bitCount());
    }
    {
        constexpr int N = 1024;
        BitSet<N> primes;
        primes.setRange(2, N-1);
        for (int i = 2; i * i < N; i++)
            if (primes.getBit(i))
                for (int j = i * i; j < N; j += i)
                    primes.clearBit(j);
        ASSERT_EQ(172, primes.bitCount());
    }
    {
        BitSet<64,-15> bs;
        bs.setRange(-1,1);
        ASSERT_EQ(3, bs.bitCount());
        bs.removeEven();
        ASSERT_EQ(2, bs.bitCount());
        bs.setRange(-1,1);
        bs.removeOdd();
        ASSERT_EQ(1, bs.bitCount());
    }
}

TEST(CspSolverTest, basicTests) {
    CspSolverTest::basicTests();
}

void
CspSolverTest::basicTests() {
    std::vector<int> values, expected;
    {
        CspSolver csp;
        ASSERT_EQ(true, csp.solve(values));
        expected = { };
        ASSERT_EQ(expected, values);
    }
    {
        CspSolver csp;
        csp.addVariable(SMALL, 1, 0); // Empty domain
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 0, 9);
        int v2 = csp.addVariable(SMALL, 9, 15);
        csp.addIneq(v2, LE, v1, 0);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 9, 9 };
        ASSERT_EQ(expected, values);

        csp.addMaxVal(0, 8);
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 0, 3);
        int v2 = csp.addVariable(SMALL, 0, 3);
        csp.addIneq(v1, LE, v2, -3);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 0, 3 };
        ASSERT_EQ(expected, values);

        csp.addMinVal(0, 1);
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 8, 8);
        int v2 = csp.addVariable(SMALL, 7, 10);
        csp.addIneq(v1, GE, v2, 1);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 8, 7 };
        ASSERT_EQ(expected, values);

        csp.addMinVal(1, 8);
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 8, 8);
        int v2 = csp.addVariable(SMALL, 7, 10);
        csp.addIneq(v2, GE, v1, 2);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 8, 10 };
        ASSERT_EQ(expected, values);

        csp.addMaxVal(1, 9);
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 0, 40);
        int v2 = csp.addVariable(SMALL, 0, 40);
        csp.addIneq(v2, GE, v1, 2);
        csp.addMinVal(0, 8);
        csp.addMaxVal(0, 8);
        csp.addMinVal(1, 7);
        csp.addMaxVal(1, 10);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 8, 10 };
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 1, 1);
        int v2 = csp.addVariable(SMALL, 4, 6);
        csp.addIneq(v2, GE, v1, 5);
        csp.addMaxVal(1, 4);
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 1, 1);
        int v2 = csp.addVariable(SMALL, 2, 5);
        csp.addIneq(v2, GE, v1, 4);
        csp.addMinVal(1, 4);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 1, 5 };
        ASSERT_EQ(expected, values);
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 0, 3);
        int v2 = csp.addVariable(SMALL, 0, 3);
        int v3 = csp.addVariable(SMALL, 0, 3);
        int v4 = csp.addVariable(SMALL, 0, 3);
        csp.addIneq(v2, GE, v1, 1);
        csp.addIneq(v3, GE, v2, 1);
        csp.addIneq(v4, GE, v3, 1);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 0, 1, 2, 3 };
        ASSERT_EQ(expected, values);
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 0, 3);
        int v2 = csp.addVariable(SMALL, 0, 3);
        int v3 = csp.addVariable(SMALL, 0, 3);
        int v4 = csp.addVariable(SMALL, 0, 3);
        int v5 = csp.addVariable(SMALL, 0, 3);
        csp.addIneq(v2, GE, v1, 1);
        csp.addIneq(v3, GE, v2, 1);
        csp.addIneq(v4, GE, v3, 1);
        csp.addIneq(v5, GE, v4, 1);
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 0, 3);
        int v2 = csp.addVariable(SMALL, 0, 3);
        int v3 = csp.addVariable(LARGE, 0, 3);
        csp.addEq(v1, v2, 3);
        csp.addEq(v3, v2, 0);
        ASSERT_EQ(true, csp.solve(values));
        expected = { 3, 0, 0 };
        ASSERT_EQ(expected, values);
    }
    {
        CspSolver csp;
        for (int i = 0; i < 1000; i++)
            csp.addVariable(SMALL, 1, 2);
        ASSERT_EQ(true, csp.solve(values));
        for (int i = 0; i < 1000; i++) {
            ASSERT_EQ(1, values[i]);
        }
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, -5, 5);
        int v2 = csp.addVariable(SMALL, -5, 5);
        csp.addIneq(v1, LE, v2, -10);
        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(-5, values[0]);
        ASSERT_EQ(5, values[1]);
    }
    {
        int offs = CspSolver::minAllowedValue;
        CspSolver csp;
        for (int i = 0; i < 64; i++) {
            csp.addVariable(SMALL, offs+0, offs+63);
            if (i > 0)
                csp.addIneq(i, GE, i-1, 1);
        }
        ASSERT_EQ(true, csp.solve(values));
        for (int i = 0; i < 64; i++) {
            ASSERT_EQ(offs+i, values[i]);
        }
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, 0, 2);
        csp.makeOdd(v1);
        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(1, values[0]);
    }
}

TEST(CspSolverTest, testPrefVal) {
    CspSolverTest::testPrefVal();
}

void
CspSolverTest::testPrefVal() {
    std::vector<int> values;
    {
        CspSolver csp;
        int v1 = csp.addVariable(MIDDLE_SMALL);
        int v2 = csp.addVariable(LARGE);
        csp.addIneq(v2, GE, v1, 1);
        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(3, values[v1]);
        ASSERT_EQ(6, values[v2]);
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(MIDDLE_SMALL);
        int v2 = csp.addVariable(MIDDLE_LARGE);
        csp.addIneq(v2, GE, v1, 1);
        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(3, values[v1]);
        ASSERT_EQ(4, values[v2]);
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(MIDDLE_SMALL);
        int v2 = csp.addVariable(LARGE);
        csp.addIneq(v2, GE, v1, 1);
        csp.addMaxVal(v1, 2);
        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(2, values[v1]);
        ASSERT_EQ(6, values[v2]);
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(MIDDLE_SMALL);
        int v2 = csp.addVariable(LARGE);
        csp.addIneq(v2, GE, v1, 1);
        csp.addMaxVal(v2, 2);
        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(1, values[v1]);
        ASSERT_EQ(2, values[v2]);
    }
    {
        CspSolver csp;
        int v1 = csp.addVariable(MIDDLE_SMALL);
        int v2 = csp.addVariable(LARGE);
        csp.addIneq(v2, GE, v1, 1);
        csp.addMaxVal(v1, 1);
        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(1, values[v1]);
        ASSERT_EQ(6, values[v2]);
    }
}

TEST(CspSolverTest, testEvenOdd) {
    CspSolverTest::testEvenOdd();
}

void
CspSolverTest::testEvenOdd() {
    std::vector<int> values, expected;

    for (int i = 0; i < 64; i++) {
        int offs = CspSolver::minAllowedValue;
        CspSolver csp;
        int v1 = csp.addVariable(SMALL, offs+0, offs+63);
        if ((offs+i) % 2 == 0) {
            csp.makeEven(v1);
        } else {
            csp.makeOdd(v1);
        }
        if (i > 0)
            csp.addMinVal(v1, offs + i - 1);
        if (i + 1 < 62)
            csp.addMaxVal(v1, offs + i + 1);
        ASSERT_EQ(true, csp.solve(values));
        expected = { offs+i };
        ASSERT_EQ(expected, values) << "i:" << i;
    }
}

TEST(CspSolverTest, testProofKernel) {
    CspSolverTest::testProofKernel();
}

void
CspSolverTest::testProofKernel() {
    std::vector<int> values, expected;
    {
        CspSolver csp;
        int r1 = csp.addVariable(SMALL, 3, 7);
        int r2 = csp.addVariable(SMALL, 4, 7);
        csp.addIneq(r2, LE, r1, -1);
        int r3 = csp.addVariable(SMALL, 3, 7); (void)r3;
        int r4 = csp.addVariable(SMALL, 3, 7);
        int r5 = csp.addVariable(SMALL, 2, 5);
        csp.addIneq(r5, GE, r1, 1);
        int r6 = csp.addVariable(SMALL, 3, 7);
        csp.addIneq(r6, LE, r4, -1);
        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp; // q2R3N/k2nK2p/bPR2Q1q/2r4N/rr2B2P/3BbN1p/r3q1q1/2Q2n1b b - - 0 1
        // bPb1xPa0
        int b7_0 = csp.addVariable();
        int a7_0 = csp.addVariable();
        int b2_0 = csp.addVariable();
        csp.addIneq(b7_0, LE, a7_0, -1);
        csp.addIneq(b2_0, LE, b7_0, 0);

        // bPc1xPd0
        int c7_1 = csp.addVariable();
        int d7_1 = csp.addVariable();
        int c2_1 = csp.addVariable();
        csp.addIneq(c7_1, LE, d7_1, -1);
        csp.addIneq(c2_1, LE, c7_1, 0);

        // bPe1xPf0
        int e7_2 = csp.addVariable();
        int f7_2 = csp.addVariable();
        int e2_2 = csp.addVariable();
        csp.addIneq(e7_2, LE, f7_2, -1);
        csp.addIneq(e2_2, LE, e7_2, 0);

        // bPg1xDBh0
        int g7_3 = csp.addVariable();
        int g2_3 = csp.addVariable();
        int h7_3 = csp.addVariable();
        int h2_3 = csp.addVariable();
        csp.makeOdd(g7_3);
        csp.addIneq(g2_3, LE, g7_3, 0);
        csp.addIneq(g7_3, LE, h2_3, -1);
        csp.addIneq(h2_3, LE, h7_3, -1);

        csp.addMaxVal(b2_0, 5);
        csp.addMinVal(g7_3, 2);
        csp.addMaxVal(h2_3, 3);
        csp.addMinVal(h7_3, 6);

        ASSERT_EQ(false, csp.solve(values));
    }
    {
        CspSolver csp; // nn3K2/6p1/2pqkNn1/qBP1q1Q1/NbrRp3/P1B1r3/2Pb2R1/Q4N1R w - - 0 1
        // bPa1xPb0
        int a7_0 = csp.addVariable();
        int b7_0 = csp.addVariable();
        int a2_0 = csp.addVariable();
        csp.addIneq(a7_0, LE, b7_0, -1);
        csp.addIneq(a2_0, LE, b7_0, 0);

        // wPe0xPf1
        int e2_1 = csp.addVariable();
        int f2_1 = csp.addVariable();
        int e7_1 = csp.addVariable();
        csp.addIneq(f2_1, LE, e2_1, -1);
        csp.addIneq(e2_1, LE, e7_1, 0);

        // bPh1xPg0
        int h7_2 = csp.addVariable();
        int g7_2 = csp.addVariable();
        int h2_2 = csp.addVariable();
        csp.addIneq(h7_2, LE, g7_2, -1);
        csp.addIneq(h2_2, LE, g7_2, 0);

        // wPd0xLBc1
        int c2_3 = csp.addVariable();
        int d2_3 = csp.addVariable();
        int c7_3 = csp.addVariable();
        int d7_3 = csp.addVariable();
        csp.makeOdd(d2_3);
        csp.addIneq(c2_3, LE, d2_3, -1);
        csp.addIneq(d2_3, LE, c7_3, -1);
        csp.addIneq(d2_3, LE, d7_3, 0);

        csp.addMaxVal(a2_0, 2);
        csp.addMaxVal(c2_3, 1);
        csp.addMaxVal(d2_3, 4);
        csp.addMinVal(c7_3, 5);
        csp.addMinVal(e7_1, 3);
        csp.addMinVal(g7_2, 6);

        ASSERT_EQ(true, csp.solve(values));
        ASSERT_EQ(3, values[d2_3]);
        ASSERT_EQ(1, values[c2_3]);
        ASSERT_EQ(6, values[g7_2]);
    }
    {
        CspSolver csp; // 2QB1rr1/1R4Q1/r2q1N2/1R1K1B2/5rB1/r6b/Rrr3r1/RN1bk1nq w - - 0 1
        // bPa1xPb0
        int a2_0 = csp.addVariable(SMALL);
        int a7_0 = csp.addVariable(LARGE);
        int b7_0 = csp.addVariable(LARGE);
        csp.addIneq(a7_0, LE, b7_0, -1);
        csp.addIneq(a2_0, LE, b7_0, 0);

        // bPb0xPa0
        int a7_1 = csp.addVariable(LARGE);
        int b7_1 = csp.addVariable(LARGE);
        csp.addIneq(a7_1, LE, a7_0, -1);
        csp.addIneq(b7_1, LE, b7_0, 0);
        csp.addIneq(a2_0, LE, a7_1, 0);

        // bPc1xPd0
        int c2_2 = csp.addVariable(SMALL);
        int c7_2 = csp.addVariable(LARGE);
        int d7_2 = csp.addVariable(LARGE);
        csp.addIneq(c7_2, LE, d7_2, -1);
        csp.addIneq(c2_2, LE, d7_2, 0);

        // bPe1xPf0
        int e2_3 = csp.addVariable(SMALL);
        int e7_3 = csp.addVariable(LARGE);
        int f7_3 = csp.addVariable(LARGE);
        csp.addIneq(e7_3, LE, f7_3, -1);
        csp.addIneq(e2_3, LE, f7_3, 0);

        // wPg0xDBf2
        int e7_4 = csp.addVariable(LARGE);
        int f7_4 = csp.addVariable(LARGE);
        int g2_4 = csp.addVariable(SMALL);
        int g7_4 = csp.addVariable(LARGE);
        csp.addIneq(e7_4, LE, e7_3, 0);
        csp.addIneq(f7_4, LE, f7_3, 0);
        csp.addIneq(e7_4, LE, f7_4, -1);
        csp.addIneq(f7_4, LE, g2_4, -1);
        csp.addIneq(g2_4, LE, g7_4, 0);
        csp.makeOdd(g2_4);

        // wPh0xNg1
        int g7_5 = csp.addVariable(LARGE);
        int h2_5 = csp.addVariable(SMALL);
        int h7_5 = csp.addVariable(LARGE);
        csp.addIneq(g7_5, LE, g7_4, 0);
        csp.addIneq(g7_5, LE, h2_5, -1);
        csp.addIneq(h2_5, LE, h7_5, 0);

        ASSERT_EQ(true, csp.solve(values));
    }
}
