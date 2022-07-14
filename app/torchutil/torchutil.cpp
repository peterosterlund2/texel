/*
    Texel - A UCI chess engine.
    Copyright (C) 2022  Peter Österlund, peterosterlund2@gmail.com

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
 * torchutil.cpp
 *
 *  Created on: Jul 11, 2022
 *      Author: petero
 */

#include "util.hpp"
#include "random.hpp"
#include "timeUtil.hpp"
#include "textio.hpp"
#include "nnutil.hpp"

#include <torch/torch.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <chrono>

// ------------------------------------------------------------------------------

/** Generates a pseudo-random permutation of 0, 1, ..., upperBound-1. */
class RandPerm {
public:
    RandPerm(U64 upperBound, U64 seed);

    /** Return the i:th element in the permutation. */
    U64 perm(U64 i) const;

private:
    static int getNumBits(U64 upperBound);
    U64 permRaw(U64 i) const;

    constexpr static int rounds = 3;
    const U64 upperBound;
    int loBits;
    int hiBits;
    U64 lowMask;
    U64 fullMask;
    U64 keys[rounds];
};

RandPerm::RandPerm(U64 upperBound, U64 seed)
    : upperBound(upperBound) {
    int bits = getNumBits(upperBound);
    loBits = bits / 2;
    hiBits = bits - loBits;
    lowMask = (1ULL << loBits) - 1;
    fullMask = (1ULL << bits) - 1;
    for (int k = 0; k < rounds; k++)
        keys[k] = hashU64(hashU64(seed) + k + 1);
}

U64
RandPerm::perm(U64 i) const {
    while (true) {
        i = permRaw(i);
        if (i < upperBound)
            return i;
    }
}

inline U64
RandPerm::permRaw(U64 i) const {
    for (int r = 0; r < rounds; r++) {
        U64 x = i & lowMask;
        x = hashU64(x + keys[r]);
        i ^= x << loBits;
        i &= fullMask;
        i = (i >> loBits) + ((i << hiBits) & fullMask);
    }
    return i;
}

inline int
RandPerm::getNumBits(U64 upperBound) {
    int ret = 1;
    while (upperBound > 1) {
        upperBound >>= 1;
        ret++;
    }
    return ret;
}

// ------------------------------------------------------------------------------

const int inFeatures = 2 * 64 * 10 * 64;
using Record = NNUtil::Record;

void
toSparse(const Record& r, std::vector<int>& idxVec) {
    int pieceType = 0;
    int k1 = r.wKing;
    int k2 = r.bKing;
    for (int i = 0; i < 30; i++) {
        while (pieceType < 9 && i >= r.nPieces[pieceType])
            pieceType++;
        int sq = r.squares[i];
        if (sq == -1)
            continue;
        idxVec.push_back(((     k1) * 10 + pieceType) * 64 + sq);
        idxVec.push_back(((64 + k2) * 10 + pieceType) * 64 + sq);
    }
}

template <typename DataSet>
void
getData(DataSet& ds, size_t beg, size_t end,
        torch::Tensor& in, torch::Tensor& out) {
    int batchSize = end - beg;
    Record r;
    std::vector<int> idxVec1;
    std::vector<int> idxVec2;
    out = torch::empty({batchSize, 1}, torch::kF32);
    for (int b = 0; b < batchSize; b++) {
        ds.getItem(beg + b, r);
        toSparse(r, idxVec1);
        idxVec2.resize(idxVec1.size(), b);
        out[b][0] = r.searchScore * 1e-2;
    }
    int nnz = idxVec1.size();

    torch::Tensor indices = torch::empty({2, nnz}, torch::kI64);
    auto acc = indices.accessor<S64,2>();
    for (int i = 0; i < nnz; i++) {
        acc[0][i] = idxVec2[i];
        acc[1][i] = idxVec1[i];
    }
    torch::Tensor values = torch::ones({nnz}, torch::kF32);
    in = sparse_coo_tensor(indices, values, {batchSize, inFeatures});
}

// ------------------------------------------------------------------------------

class DataSet {
public:
    DataSet(const std::string& filename);

    S64 getSize() const;
    void getItem(S64 idx, Record& r);

private:
    std::fstream fs;
    S64 size;
};

DataSet::DataSet(const std::string& filename) {
    fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fs.open(filename.c_str(), std::ios_base::in |
                              std::ios_base::binary);
    fs.seekg(0, std::ios_base::end);
    size = fs.tellg() / sizeof(Record);
}

inline S64
DataSet::getSize() const {
    return size;
}

void
DataSet::getItem(S64 idx, Record& r) {
    S64 offs = idx * sizeof(Record);
    fs.seekg(offs, std::ios_base::beg);
    fs.read((char*)&r, sizeof(Record));
}

// ------------------------------------------------------------------------------

template <typename Base>
class ShuffledDataSet {
public:
    ShuffledDataSet(Base& baseSet, U64 seed);

    S64 getSize() const;
    void getItem(S64 idx, Record& r);

public:
    Base& baseSet;
    RandPerm rndPerm;
};

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
void
ShuffledDataSet<Base>::getItem(S64 idx, Record& r) {
    baseSet.getItem(rndPerm.perm(idx), r);
}

// ------------------------------------------------------------------------------

template <typename Base>
class SubDataSet {
public:
    SubDataSet(Base& baseSet, U64 beg, U64 end);

    S64 getSize() const;
    void getItem(S64 idx, Record& r);

private:
    Base& baseSet;
    const U64 beg;
    const U64 end;
};

template <typename Base>
SubDataSet<Base>::SubDataSet(Base& baseSet, U64 beg, U64 end)
    : baseSet(baseSet), beg(beg), end(end) {
}

template <typename Base>
inline S64
SubDataSet<Base>::getSize() const {
    return end - beg;
}

template <typename Base>
inline void
SubDataSet<Base>::getItem(S64 idx, Record& r) {
    baseSet.getItem(beg + idx, r);
}

// ------------------------------------------------------------------------------

void
extractSubset(const std::string& inFile, S64 nPos, const std::string& outFile) {
    DataSet allData(inFile);
    ShuffledDataSet<DataSet> shuffled(allData, 17);
    Record r;

    std::ofstream os;
    os.open(outFile.c_str(), std::ios_base::out | std::ios_base::binary);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    for (S64 i = 0; i < nPos; i++) {
        shuffled.getItem(i, r);
        os.write((const char*)&r, sizeof(Record));
    }
}

// ------------------------------------------------------------------------------

class Net : public torch::nn::Module {
public:
    Net();

    torch::Tensor forward(torch::Tensor x);

private:
    torch::nn::Linear lin1 = nullptr; const int n1 = 256;
    torch::nn::Linear lin2 = nullptr; const int n2 = 32;
    torch::nn::Linear lin3 = nullptr; const int n3 = 32;
    torch::nn::Linear lin4 = nullptr;
};

Net::Net() {
    lin1 = register_module("lin1", torch::nn::Linear(inFeatures, n1));
    lin2 = register_module("lin2", torch::nn::Linear(n1, n2));
    lin3 = register_module("lin3", torch::nn::Linear(n2, n3));
    lin4 = register_module("lin4", torch::nn::Linear(n3, 1));
}

torch::Tensor
Net::forward(torch::Tensor x) {
    x = torch::relu(lin1->forward(x));
    x = torch::relu(lin2->forward(x));
    x = torch::relu(lin3->forward(x));
    x = lin4->forward(x);
    return x;
}

// ------------------------------------------------------------------------------

void
setLR(torch::optim::Optimizer& opt, double lr) {
    for (auto& group : opt.param_groups()) {
        if (group.has_options()) {
            auto& options = static_cast<torch::optim::OptimizerOptions&>(group.options());
            options.set_lr(lr);
        }
    }
}

template <typename T>
T
toProb(T score) {
    return 1 / (1 + pow(10, score * (-113.0 / 400)));
}

void
train(const std::string& inFile) {
    auto dev = torch::kCUDA;
    const double t0 = currentTime();

    auto netP = std::make_shared<Net>();
    Net& net = *netP;
    net.to(dev);

    size_t nPars = 0;
    for (auto& p : net.parameters())
        if (p.requires_grad())
            nPars += p.numel();
    std::cout << "Number of parameters: " << nPars << std::endl;

    DataSet allData(inFile);
    const size_t nData = allData.getSize();
    const size_t nTrain = nData * 9 / 10;
    const size_t nValidate = nData - nTrain;
    const int batchSize = 8192;
    std::cout << "nData    : " << nData << std::endl;
    std::cout << "nTrain   : " << nTrain << " (" << (nTrain / batchSize) << " batches)" << std::endl;
    std::cout << "nValidate: " << nValidate << " (" << (nValidate / batchSize) << " batches)" << std::endl;

    U64 seed0 = 0;
    using ShuffledSet = ShuffledDataSet<DataSet>;
    ShuffledSet allShuffled(allData, seed0);

    using SubSet = SubDataSet<ShuffledSet>;
    SubSet trainData(allShuffled, 0, nTrain);
    SubSet validateData(allShuffled, nTrain, nData);

    double lr = 5e-4;
    torch::optim::Adam optimizer(net.parameters(), lr);
    for (int epoch = 1; epoch <= 30; epoch++) {
        ShuffledDataSet<SubSet> epochTrainData(trainData, epoch);
        std::cout << "Epoch: " << epoch << " lr: " << lr << std::endl;

        torch::Tensor lossSum = torch::zeros({}, torch::kF32).to(dev);
        int lossNum = 0;
        for (size_t batch = 0, beg = 0; beg < nTrain; batch++, beg += batchSize) {
            size_t end = std::min(beg + batchSize, nTrain);

            torch::Tensor input;
            torch::Tensor target;
            getData(epochTrainData, beg, end, input, target);
            input = input.to(dev);
            target = target.to(dev);

            optimizer.zero_grad();
            torch::Tensor output = net.forward(input);

            torch::Tensor loss = mse_loss(toProb(output), toProb(target));
            loss.backward();
            optimizer.step();

            lossSum += loss;
            lossNum += 1;

            if (batch % 200 == 0) {
                printf("t: %8.3f batch: %5ld loss: %g\n",
                       currentTime() - t0,
                       batch,
                       std::sqrt(lossSum.item<float>() / lossNum));
                lossSum *= 0;
                lossNum = 0;
            }
        }

        {
            c10::NoGradGuard noGrad;
            lossSum *= 0;
            lossNum = 0;
            for (size_t batch = 0, beg = 0; beg < nValidate; batch++, beg += batchSize) {
                size_t end = std::min(beg + batchSize, nValidate);

                torch::Tensor input;
                torch::Tensor target;
                getData(validateData, beg, end, input, target);
                input = input.to(dev);
                target = target.to(dev);

                torch::Tensor output = net.forward(input);
                torch::Tensor loss = mse_loss(toProb(output), toProb(target));
                lossSum += loss;
                lossNum += 1;
            }
            std::cout << "Validation error: "
                      << std::sqrt(lossSum.item<float>() / lossNum)
                      << std::endl;
        }

        {
            std::stringstream ss;
            ss << "model" << std::setfill('0') << std::setw(2) << epoch << ".pt";
            torch::save(netP, ss.str().c_str());
        }

        lr *= 0.8;
        setLR(optimizer, lr);
    }
}

// ------------------------------------------------------------------------------

void
eval(const std::string& modelFile, const std::string& fen) {
    Position pos = TextIO::readFEN(fen);

    auto netP = std::make_shared<Net>();
    Net& net = *netP;
    torch::load(netP, modelFile.c_str());
    net.to(torch::kCPU);

    Record r;
    NNUtil::posToRecord(pos, 0, r);

    std::vector<int> idxVec;
    toSparse(r, idxVec);
    const int nnz = idxVec.size();

    torch::Tensor indices = torch::empty({2, nnz}, torch::kI64);
    auto acc = indices.accessor<S64,2>();
    for (int i = 0; i < nnz; i++) {
        acc[0][i] = 0;
        acc[1][i] = idxVec[i];
    }

    c10::NoGradGuard noGrad;
    torch::Tensor values = torch::ones({nnz}, torch::kF32);
    torch::Tensor in = sparse_coo_tensor(indices, values, {1, inFeatures});

    torch::Tensor out = net.forward(in);
    double val = out.item<double>();
    std::cout << "val: " << val << " prob: " << toProb(val) << std::endl;
}

// ------------------------------------------------------------------------------

void
usage() {
    std::cerr << "Usage: torchutil cmd params\n";
    std::cerr << "cmd is one of:\n";
    std::cerr << " train infile       : Train network from data in infile\n";
    std::cerr << " eval modelfile fen : Evaluate position using a saved network\n";
    std::cerr << " subset infile nPos outfile : Extract positions from infile, write to outfile\n";
    std::cerr << std::flush;
    ::exit(2);
}

int main(int argc, const char* argv[]) {
    try {
        if (argc < 2)
            usage();

        std::string cmd = argv[1];
        if (cmd == "train") {
            if (argc != 3)
                usage();
            std::string inFile = argv[2];
            train(inFile);
        } else if (cmd == "eval") {
            if (argc != 4)
                usage();
            std::string modelFile = argv[2];
            std::string fen = argv[3];
            eval(modelFile, fen);
        } else if (cmd == "subset") {
            if (argc != 5)
                usage();
            std::string inFile = argv[2];
            S64 nPos;
            if (!str2Num(argv[3], nPos) || nPos <= 0)
                usage();
            std::string outFile = argv[4];
            extractSubset(inFile, nPos, outFile);
        } else {
            usage();
        }
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}
