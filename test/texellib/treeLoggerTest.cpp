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
 * treeLoggerTest.cpp
 *
 *  Created on: Sep 7, 2013
 *      Author: petero
 */

#include "treeLoggerTest.hpp"
#include "treeLogger.hpp"
#include "position.hpp"
#include "textio.hpp"
#include <iostream>
#include <cstring>

#include "gtest/gtest.h"


TEST(TreeLoggerTest, testSerialize) {
    TreeLoggerTest::testSerialize();
}

void
TreeLoggerTest::testSerialize() {
    const U32 a = 123453428;
    const S32 b = -321234355;
    const U16 c = 40000;
    const S16 d = -20000;
    const U8  e = 180;
    const S8  f = -10;

    U8 buffer[14];
    U8* ptr = Serializer::serialize<sizeof(buffer)>(&buffer[0], a, b, c, d, e, f);
    EXPECT_EQ(sizeof(buffer), ptr - buffer);

    U32 a1;
    S32 b1;
    U16 c1;
    S16 d1;
    U8  e1;
    S8  f1;

    const U8* ptr2 = Serializer::deSerialize<sizeof(buffer)>(&buffer[0], a1, b1, c1, d1, e1, f1);
    EXPECT_EQ(sizeof(buffer), ptr2 - buffer);
    EXPECT_EQ(a, a1);
    EXPECT_EQ(b, b1);
    EXPECT_EQ(c, c1);
    EXPECT_EQ(d, d1);
    EXPECT_EQ(e, e1);
    EXPECT_EQ(f, f1);
}

TEST(TreeLoggerTest, testLoggerData) {
    TreeLoggerTest::testLoggerData();
}

void
TreeLoggerTest::testLoggerData() {
    using TB = TreeLoggerBase;
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_INCOMPLETE;
        e.p0.nextIndex = 17;
        e.p0.word0 = 0x3214876587651234ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        EXPECT_EQ(e.type, e2.type);
        EXPECT_EQ(e.p0.nextIndex, e2.p0.nextIndex);
        EXPECT_EQ(e.p0.word0,     e2.p0.word0);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_PART0;
        e.p0.nextIndex = 123987654;
        e.p0.word0 = 0x3876587651234ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        EXPECT_EQ(e.type, e2.type);
        EXPECT_EQ(e.p0.nextIndex, e2.p0.nextIndex);
        EXPECT_EQ(e.p0.word0,     e2.p0.word0);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_PART1;
        e.p1.word1 = 0xfedc1234435432ffULL;
        e.p1.word2 = 0x1324123434534ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        EXPECT_EQ(e.type, e2.type);
        EXPECT_EQ(e.p1.word1,   e2.p1.word1);
        EXPECT_EQ(e.p1.word2,   e2.p1.word2);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_PART2;
        e.p2.word3 = 0x834927342342134ULL;
        e.p2.word4 = 0x1234454656345123ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        EXPECT_EQ(e.type, e2.type);
        EXPECT_EQ(e.p2.word3,        e2.p2.word3);
        EXPECT_EQ(e.p2.word4,        e2.p2.word4);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::NODE_START;
        e.se.endIndex = 134;
        e.se.parentIndex = 2342134;
        e.se.move = 0x1234;
        e.se.alpha = -20000;
        e.se.beta = 30000;
        e.se.ply = 17;
        e.se.depth = 23 * 8;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        EXPECT_EQ(e.type, e2.type);
        EXPECT_EQ(e.se.endIndex,    e2.se.endIndex);
        EXPECT_EQ(e.se.parentIndex, e2.se.parentIndex);
        EXPECT_EQ(e.se.move,        e2.se.move);
        EXPECT_EQ(e.se.alpha,       e2.se.alpha);
        EXPECT_EQ(e.se.beta,        e2.se.beta);
        EXPECT_EQ(e.se.ply,         e2.se.ply);
        EXPECT_EQ(e.se.depth,       e2.se.depth);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::NODE_END;
        e.ee.startIndex = 1000000000;
        e.ee.score = 17389;
        e.ee.scoreType = 2;
        e.ee.evalScore = 389;
        e.ee.hashKey = 0xf23456789abcde10ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        EXPECT_EQ(e.type, e2.type);
        EXPECT_EQ(e.ee.startIndex, e2.ee.startIndex);
        EXPECT_EQ(e.ee.score,      e2.ee.score);
        EXPECT_EQ(e.ee.scoreType,  e2.ee.scoreType);
        EXPECT_EQ(e.ee.evalScore,  e2.ee.evalScore);
        EXPECT_EQ(e.ee.hashKey,    e2.ee.hashKey);
    }
}
