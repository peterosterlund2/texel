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
 * dataset.hpp
 *
 *  Created on: Dec 24, 2023
 *      Author: petero
 */

#ifndef DATASET_HPP_
#define DATASET_HPP_

#include "nnutil.hpp"
#include "randperm.hpp"

#include <string>
#include <fstream>
#include <cassert>

using Record = NNUtil::Record;


/** A data set where the data is stored in a file.
 * Does not support random access for performance reasons. */
class FileDataSet {
public:
    /** Constructor. */
    explicit FileDataSet(const std::string& filename);
    FileDataSet(const FileDataSet& other) = delete;

    /** Get number of records in the data set. */
    S64 getSize() const;

    /** Call action(Record& r) for all elements in the data set, in file order. */
    template <typename Consumer> void forEach(Consumer action);

private:
    std::string filename;
    std::fstream fs;
    S64 size;
};


/** A data set stored in memory. */
class MemDataSet {
public:
    /** Constructor. Create an empty data set. */
    MemDataSet();
    /** Create a data set from all entries in fileDs that satisfy pred(i) == true.
     *  pred() is called only once for idx=0,1,...,(fileDs.getSize()-1), in that order. */
    template <typename Pred>
    MemDataSet(FileDataSet& fileDs, Pred pred, S64 expectedSize = 0);
    MemDataSet(const MemDataSet& other) = delete;

    /** Clear all data and reserve space for "expectedSize" new entries. */
    void reserve(S64 expectedSize);
    /** Add a data entry to the set. */
    void addData(const Record& r);

    /** Swaps the content of this object and "other". */
    void swap(MemDataSet& other);

    /** Get number of records in the data set. */
    S64 getSize() const;
    /** Get the idx:th record in the data set. */
    void getItem(S64 idx, Record& r) const;

private:
    std::vector<Record> data;
};


/** A randomly shuffled version of the base data set. */
template <typename Base>
class ShuffledDataSet {
public:
    ShuffledDataSet(Base& baseSet, U64 seed);

    S64 getSize() const;
    void getItem(S64 idx, Record& r) const;

public:
    Base& baseSet;
    RandPerm rndPerm;
};


/** Splits a data set in a training part and a validation part.
 *  The training part is further split into chunks that fit in memory. */
class SplitData {
public:
    /** Constructor. */
    SplitData(FileDataSet& fileDs, int batchSize);

    /** Get number of training data samples. */
    S64 numTrainData() const;
    /** Get number of training data parts. */
    int numTrainParts() const;
    /** Get batch size. */
    int getBatchSize() const;

    /** Get training and/or validation data. All data is gathered using
     *  only one iteration over the data file.
     * @param seed          The random seed used to partition training data.
     * @param part1         Part of first training data to get, or -1.
     * @param trainData1    First part of training data. Used if part1 != -1.
     * @param part2         Part of second training data to get, or -1.
     * @param trainData2    Second part of training data. Used if part2 != -1.
     * @param validateData  The validation data to get, or nullptr.
     */
    void getData(U64 seed,
                 int part1, MemDataSet* trainData1,
                 int part2, MemDataSet* trainData2,
                 MemDataSet* validateData);

private:
    FileDataSet& fileDs;
    const int batchSize;
    const S64 fSize;
    const S64 nValidate;
    int nTrainParts;
    S64 trainDataChunkSize;
    const RandPerm splitPerm;
};

// ------------------------------------------------------------------------------

inline S64
FileDataSet::getSize() const {
    return size;
}

template <typename Consumer>
inline void
FileDataSet::forEach(Consumer action) {
    fs.seekg(0, std::ios_base::beg);
    Record r;
    for (S64 i = 0; i < size; i++) {
        fs.read((char*)&r, sizeof(Record));
        action(r);
    }
}

// ------------------------------------------------------------------------------

template <typename Pred>
MemDataSet::MemDataSet(FileDataSet& fileDs, Pred pred, S64 expectedSize) {
    reserve(expectedSize);
    S64 fIdx = 0;
    fileDs.forEach([&pred,this,&fIdx](Record& r) {
        if (pred(fIdx++))
            data.push_back(r);
    });
}

inline void
MemDataSet::reserve(S64 expectedSize) {
    data.clear();
    data.reserve(expectedSize);
}

inline void
MemDataSet::addData(const Record& r) {
    data.push_back(r);
}

inline void
MemDataSet::swap(MemDataSet& other) {
    data.swap(other.data);
}

inline S64
MemDataSet::getSize() const {
    return data.size();
}

inline void
MemDataSet::getItem(S64 idx, Record& r) const {
    r = data[idx];
}

// ------------------------------------------------------------------------------

template <typename Base>
ShuffledDataSet<Base>::ShuffledDataSet(Base& baseSet, U64 seed)
    : baseSet(baseSet), rndPerm(baseSet.getSize(), seed) {
}

template <typename Base>
inline S64
ShuffledDataSet<Base>::getSize() const {
    return baseSet.getSize();
}

template <typename Base>
inline void
ShuffledDataSet<Base>::getItem(S64 idx, Record& r) const {
    baseSet.getItem(rndPerm.perm(idx), r);
}

// ------------------------------------------------------------------------------

inline S64
SplitData::numTrainData() const {
    return fSize - nValidate;
}

inline int
SplitData::numTrainParts() const {
    return nTrainParts;
}

inline int
SplitData::getBatchSize() const {
    return batchSize;
}

#endif /* DATASET_HPP_ */
