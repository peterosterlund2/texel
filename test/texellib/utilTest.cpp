/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * utilTest.cpp
 *
 *  Created on: Sep 21, 2013
 *      Author: petero
 */

#include "util.hpp"
#include "timeUtil.hpp"
#include "histogram.hpp"
#include "nntypes.hpp"

#include <iostream>
#include <memory>

#include "gtest/gtest.h"

TEST(UtilTest, testUtil) {
    int arr1[10];
    EXPECT_EQ(10, COUNT_OF(arr1));

    std::vector<std::string> splitResult;
    splitString(std::string("a b c def"), splitResult);
    auto v = std::vector<std::string>{"a", "b", "c", "def"};
    EXPECT_EQ((std::vector<std::string>{"a", "b", "c", "def"}), splitResult);

    U64 val;
    EXPECT_TRUE(str2Num("123456789012345", val));
    EXPECT_EQ(123456789012345ULL, val);
    EXPECT_TRUE(hexStr2Num("1f2c", val));
    EXPECT_EQ(0x1f2c, val);
    EXPECT_EQ("12345", num2Str(12345));
    EXPECT_EQ("000000001234fdec", num2Hex(0x1234fdec));

    EXPECT_EQ("peter", toLowerCase("Peter"));
    EXPECT_TRUE( startsWith("Peter", "Pe"));
    EXPECT_TRUE( startsWith("Peter", "Peter"));
    EXPECT_TRUE(!startsWith("Peter", "PeterO"));
    EXPECT_TRUE(!startsWith("Peter", "Pex"));
    EXPECT_TRUE(!startsWith("Peter", "eter"));
    EXPECT_TRUE( startsWith("", ""));
    EXPECT_TRUE(!startsWith("", "x"));

    EXPECT_TRUE( endsWith("test.txt", "txt"));
    EXPECT_TRUE( endsWith("test.txt", ".txt"));
    EXPECT_TRUE(!endsWith("test.txt", "ttxt"));
    EXPECT_TRUE(!endsWith("a", "ab"));
    EXPECT_TRUE(!endsWith("", "ab"));
    EXPECT_TRUE( endsWith("", ""));

    EXPECT_TRUE( contains((std::vector<int>{1,2,3,4}), 3));
    EXPECT_TRUE(!contains((std::vector<int>{1,2,3,4}), 5));
    EXPECT_TRUE( contains((std::vector<int>{1,3,2,3,4}), 3));
    EXPECT_TRUE(!contains((std::vector<int>{}), 0));
    EXPECT_TRUE(!contains((std::vector<int>{}), 1));

    EXPECT_EQ(std::string("asdf  adf"), trim(std::string(" asdf  adf  ")));
    EXPECT_EQ(std::string("asdf xyz"), trim(std::string("\t asdf xyz")));
}

TEST(UtilTest, testSampleStat) {
    const double tol = 1e-14;

    SampleStatistics stat;
    EXPECT_EQ(0, stat.numSamples());
    EXPECT_NEAR(0, stat.avg(), 1e-14);
    EXPECT_NEAR(0, stat.std(), 1e-14);

    stat.addSample(3);
    EXPECT_EQ(1, stat.numSamples());
    EXPECT_NEAR(3, stat.avg(), tol);
    EXPECT_NEAR(0, stat.std(), tol);

    stat.addSample(4);
    EXPECT_EQ(2, stat.numSamples());
    EXPECT_NEAR(3.5, stat.avg(), tol);
    EXPECT_NEAR(::sqrt(0.5), stat.std(), tol);

    stat.reset();
    EXPECT_EQ(0, stat.numSamples());
    EXPECT_NEAR(0, stat.avg(), 1e-14);
    EXPECT_NEAR(0, stat.std(), 1e-14);

    for (int i = 0; i < 10; i++)
        stat.addSample(i);
    EXPECT_EQ(10, stat.numSamples());
    EXPECT_NEAR(4.5, stat.avg(), 1e-14);
    EXPECT_NEAR(::sqrt(55.0/6.0), stat.std(), 1e-14);

    SampleStatistics stat2;
    for (int i = 10; i < 20; i++)
        stat2.addSample(i);
    EXPECT_EQ(10, stat2.numSamples());
    EXPECT_NEAR(14.5, stat2.avg(), 1e-14);
    EXPECT_NEAR(::sqrt(55.0/6.0), stat2.std(), 1e-14);

    stat += stat2;
    EXPECT_EQ(20, stat.numSamples());
    EXPECT_NEAR(9.5, stat.avg(), 1e-14);
    EXPECT_NEAR(::sqrt(35.0), stat.std(), 1e-14);
}

TEST(UtilTest, testTime) {
    TimeSampleStatistics stat1;
    TimeSampleStatistics stat2;
    {
        EXPECT_EQ(0, stat1.numSamples());
        EXPECT_EQ(0, stat2.numSamples());
        ScopedTimeSample ts(stat1);
        EXPECT_EQ(0, stat1.numSamples());
        EXPECT_EQ(0, stat2.numSamples());
        ScopedTimeSample ts2(stat2);
        EXPECT_EQ(0, stat1.numSamples());
        EXPECT_EQ(0, stat2.numSamples());
    }
    EXPECT_EQ(1, stat1.numSamples());
    EXPECT_EQ(1, stat2.numSamples());
    {
        ScopedTimeSample ts(stat1);
        EXPECT_EQ(1, stat1.numSamples());
        EXPECT_EQ(1, stat2.numSamples());
    }
    EXPECT_EQ(2, stat1.numSamples());
    EXPECT_EQ(1, stat2.numSamples());
}

TEST(UtilTest, testHistogram) {
    const int maxV = 15;
    Histogram<0,maxV> hist;
    for (int i = 0; i < maxV; i++)
        EXPECT_EQ(0, hist.get(i));
    EXPECT_EQ(0, hist.get(-1));
    EXPECT_EQ(0, hist.get(maxV));

    for (int i = -1; i < maxV+2; i++) {
        for (int j = 0; j <= i; j++)
            hist.add(j);
    }
    for (int i = 0; i < maxV; i++)
        EXPECT_EQ(maxV+2-i, hist.get(i));
    EXPECT_EQ(0, hist.get(-1));
    EXPECT_EQ(0, hist.get(maxV));

    hist.clear();
    for (int i = 0; i < maxV; i++)
        EXPECT_EQ(0, hist.get(i));

    for (int i = 0; i < maxV; i++) {
        HistogramAdder<decltype(hist)> ha(hist);
        for (int j = 0; j <= i; j++)
            ha.inc();
    }
    for (int i = 0; i < maxV; i++)
        EXPECT_EQ(i==0?0:1, hist.get(i));
}

TEST(UtilTest, binFileTest) {
    std::stringstream ss;

    S8 v1 = -18;
    U8 v2 = 150;
    S16 v3 = -10000;
    U16 v4 = 1234;
    S32 v5 = 1000000;
    U32 v6 = 3000000000;
    S64 v7 = -1;
    U64 v8 = 1ULL << 50;

    const int n = 40000;
    S16 buf[n];
    for (int i = 0; i < n; i++)
        buf[i] = i >= 32768 ? (i - 65536) : i;

    {
        BinaryFileWriter writer(ss);
        writer.writeScalar(v1);
        writer.writeScalar(v2);
        writer.writeScalar(v3);
        writer.writeScalar(v4);
        writer.writeScalar(v5);
        writer.writeScalar(v6);
        writer.writeScalar(v7);
        writer.writeScalar(v8);
        writer.writeArray(buf, n);
    }

    S8 v1r;
    U8 v2r;
    S16 v3r;
    U16 v4r;
    S32 v5r;
    U32 v6r;
    S64 v7r;
    U64 v8r;
    S16 bufr[n];

    {
        BinaryFileReader reader(ss);
        reader.readScalar(v1r);
        reader.readScalar(v2r);
        reader.readScalar(v3r);
        reader.readScalar(v4r);
        reader.readScalar(v5r);
        reader.readScalar(v6r);
        reader.readScalar(v7r);
        reader.readScalar(v8r);
        reader.readArray(bufr, n);
    }

    ASSERT_EQ(v1, v1r);
    ASSERT_EQ(v2, v2r);
    ASSERT_EQ(v3, v3r);
    ASSERT_EQ(v4, v4r);
    ASSERT_EQ(v5, v5r);
    ASSERT_EQ(v6, v6r);
    ASSERT_EQ(v7, v7r);
    ASSERT_EQ(v8, v8r);

    for (int i = 0; i < n; i++)
        ASSERT_EQ(buf[i], bufr[i]);
}

template <typename T, int N>
static void fillArray(T (&arr)[N], Random& rnd) {
    static_assert(sizeof(T) <= sizeof(int), "Unsupported array type");
    S64 minVal = std::numeric_limits<T>::min();
    S64 maxVal = std::numeric_limits<T>::max();
    for (int i = 0; i < N; i++)
        arr[i] = rnd.nextU64() % (maxVal - minVal + 1) + minVal;
}

TEST(UtilTest, nnDataTest) {
    auto netP = NetData::create();
    NetData& net = *netP;
    Random rnd;
    fillArray(net.weight1.data, rnd);
    fillArray(net.bias1.data, rnd);
    for (NetData::Head& h : net.head) {
        fillArray(h.lin2.weight.data, rnd);
        fillArray(h.lin2.bias.data, rnd);
        fillArray(h.lin3.weight.data, rnd);
        fillArray(h.lin3.bias.data, rnd);
        fillArray(h.lin4.weight.data, rnd);
        fillArray(h.lin4.bias.data, rnd);
    }

    std::stringstream ss;
    net.save(ss);

    auto net2P = NetData::create();
    NetData& net2 = *net2P;
    net2.load(ss);

    ASSERT_EQ(net.weight1.data[0], net2.weight1.data[0]);
}
