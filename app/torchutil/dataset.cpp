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
 * dataset.cpp
 *
 *  Created on: Dec 24, 2023
 *      Author: petero
 */

#include "dataset.hpp"
#include "chessError.hpp"

#include <iomanip>


FileDataSet::FileDataSet(const std::string& filename)
    : filename(filename), size(-1) {
    fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fs.open(filename.c_str(), std::ios_base::in |
                              std::ios_base::binary);

    fs.seekg(0, std::ios_base::end);
    size = fs.tellg() / sizeof(Record);
}

// ------------------------------------------------------------------------------

MemDataSet::MemDataSet() {
}

// ------------------------------------------------------------------------------

const S64 maxValidateData = 32 * 1024 * 1024;
const S64 trainDataMaxChunkSize = 128 * 1024 * 1024;

SplitData::SplitData(FileDataSet& fileDs, int batchSize)
    : fileDs(fileDs), batchSize(batchSize),
      fSize(fileDs.getSize()),
      nValidate(std::min(fSize / 10, maxValidateData)),
      splitPerm(fSize, 0) {

    S64 nTrain = fSize - nValidate;
    if (nTrain <= 0)
        throw ChessError("No training data");
    nTrainParts = (nTrain + trainDataMaxChunkSize - 1) / trainDataMaxChunkSize;

    if (nTrainParts == 2)
        nTrainParts = 1; // Does not use more memory and prevents reading from file more than once

    trainDataChunkSize = (fSize + nTrainParts - 1) / nTrainParts;
    trainDataChunkSize = (trainDataChunkSize + batchSize - 1) / batchSize * batchSize;
}

void
SplitData::getData(U64 seed,
                   int part1, MemDataSet* trainData1,
                   int part2, MemDataSet* trainData2,
                   MemDataSet* validateData) {
    if (part1 >= nTrainParts)
        part1 = -1;
    if (part1 != -1)
        trainData1->reserve(trainDataChunkSize);

    if (part2 >= nTrainParts)
        part2 = -1;
    if (part2 != -1)
        trainData2->reserve(trainDataChunkSize);

    if (validateData)
        validateData->reserve(nValidate);

    S64 nTrain = fSize - nValidate;
    RandPerm shuffle(nTrain, seed);
    S64 fileIdx = 0;
    S64 trainIdx = 0;
    fileDs.forEach([&](Record& r) {
        if (splitPerm.perm(fileIdx++) < (U64)nValidate) {
            if (validateData)
                validateData->addData(r);
        } else {
            S64 p = shuffle.perm(trainIdx++);
            if (part1 != -1)
                if (p >= part1 * trainDataChunkSize && p < (part1 + 1) * trainDataChunkSize)
                    trainData1->addData(r);
            if (part2 != -1)
                if (p >= part2 * trainDataChunkSize && p < (part2 + 1) * trainDataChunkSize)
                    trainData2->addData(r);
        }
    });
}
