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
 * datasetTest.cpp
 *
 *  Created on: Jan 6, 2024
 *      Author: petero
 */

#include "datasetTest.hpp"
#include "dataset.hpp"
#include "square.hpp"

#include <cstdio>

#include "gtest/gtest.h"

static void initRecord(Record& r, int score) {
    r.wKing = E1;
    r.bKing = E8;
    for (size_t i = 0; i < COUNT_OF(Record::nPieces); i++)
        r.nPieces[i] = 0;
    r.halfMoveClock = 0;
    r.searchScore = score;
}

static void
writeFileDS(const std::vector<Record>& data, const std::string& outFile) {
    std::ofstream os;
    os.open(outFile.c_str(), std::ios_base::out | std::ios_base::binary);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    for (const Record& r : data)
        os.write((const char*)&r, sizeof(Record));
}



TEST(DataSetTest, testMemDS) {
    DataSetTest::testMemDS();
}

void
DataSetTest::testMemDS() {
    MemDataSet ds;
    Record r;
    for (int i = 0; i < 100; i++) {
        initRecord(r, i + 1);
        ds.addData(r);
    }
    ASSERT_EQ(100, ds.getSize());
    for (int i = 0; i < 100; i++) {
        ds.getItem(i, r);
        ASSERT_EQ(i + 1, r.searchScore);
    }

    MemDataSet ds2;
    ds.swap(ds2);
    ASSERT_EQ(0, ds.getSize());
    ASSERT_EQ(100, ds2.getSize());
    for (int i = 0; i < 100; i++) {
        ds2.getItem(i, r);
        ASSERT_EQ(i + 1, r.searchScore);
    }
}

TEST(DataSetTest, testFileDS) {
    DataSetTest::testFileDS();
}

void
DataSetTest::testFileDS() {
    const std::string fileName(".testFileDS_file");
    {
        std::vector<Record> data;
        Record r;
        for (int i = 0; i < 100; i++) {
            initRecord(r, i);
            data.push_back(r);
        }
        writeFileDS(data, fileName);
    }

    {
        FileDataSet ds(fileName);
        ASSERT_EQ(100, ds.getSize());
        int sum = 0;
        ds.forEach([&](Record& r) {
            sum += r.searchScore;
        });
        ASSERT_EQ(100*99/2, sum);
    }

    {
        FileDataSet fDs(fileName);
        MemDataSet mDs(fDs, [](S64 idx) {
            return (idx % 2) != 0;
        });
        ASSERT_EQ(100, fDs.getSize());
        ASSERT_EQ(50, mDs.getSize());

        std::vector<int> v;
        Record r;
        for (S64 idx = 0; idx < 50; idx++) {
            mDs.getItem(idx, r);
            ASSERT_EQ(idx * 2 + 1, r.searchScore);
        }
    }

    std::remove(fileName.c_str());
}

TEST(DataSetTest, testShuffledDS) {
    DataSetTest::testShuffledDS();
}

void
DataSetTest::testShuffledDS() {
    MemDataSet mDs;
    Record r;
    for (int i = 0; i < 100; i++) {
        initRecord(r, i);
        mDs.addData(r);
    }
    ASSERT_EQ(100, mDs.getSize());

    ShuffledDataSet<MemDataSet> shuffled(mDs, 4711);
    ASSERT_EQ(100, shuffled.getSize());

    std::vector<int> v;
    for (int i = 0; i < 100; i++) {
        shuffled.getItem(i, r);
        v.push_back(r.searchScore);
    }
    std::sort(v.begin(), v.end());
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(i, v[i]);
    }
}

TEST(DataSetTest, testSplitData) {
    DataSetTest::testSplitData();
}

void
DataSetTest::testSplitData() {
    const std::string fileName(".testSplitData_file");
    {
        std::vector<Record> data;
        Record r;
        for (int i = 0; i < 200; i++) {
            initRecord(r, i);
            data.push_back(r);
        }
        writeFileDS(data, fileName);
    }

    {
        FileDataSet ds(fileName);
        ASSERT_EQ(200, ds.getSize());

        SplitData split(ds, 10);
        ASSERT_EQ(180, split.numTrainData());
        ASSERT_EQ(1, split.numTrainParts());
        ASSERT_EQ(10, split.getBatchSize());

        MemDataSet train1;
        MemDataSet train2;
        MemDataSet validate;

        split.getData(17, 0, &train1, 1, &train2, &validate);
        ASSERT_EQ(180, train1.getSize());
        ASSERT_EQ(0, train2.getSize());
        ASSERT_EQ(20, validate.getSize());

        std::vector<int> v;
        Record r;
        for (S64 i = 0; i < train1.getSize(); i++) {
            train1.getItem(i, r);
            v.push_back(r.searchScore);
        }
        for (S64 i = 0; i < validate.getSize(); i++) {
            validate.getItem(i, r);
            v.push_back(r.searchScore);
        }
        std::sort(v.begin(), v.end());
        for (int i = 0; i < 200; i++) {
            ASSERT_EQ(i, v[i]);
        }
    }

    std::remove(fileName.c_str());
}
