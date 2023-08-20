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
#include "nneval.hpp"
#include "square.hpp"

#include <torch/torch.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <chrono>
#include <cmath>
#include <climits>

extern "C" {
#include "tb/gtb/compression/lzma/Lzma86Enc.h"
}

// ------------------------------------------------------------------------------

/** Generates a pseudo-random permutation of 0, 1, ..., upperBound-1.
 *  See: https://en.wikipedia.org/wiki/Feistel_cipher */
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

#define USE_CASTLING 0

#if USE_CASTLING
const int nKIdx = 35;
#else
const int nKIdx = 32;
#endif
const int inFeats1 = nKIdx * 10 * 64;
const int inFeats2 =         10 * 64;
const int inFeats3 =         10;
const int inFeatures = inFeats1 + inFeats2 + inFeats3;
using Record = NNUtil::Record;

/** Compute feature indices for a king+piece combination.
 * idx1 is computed from kIdx, pieceType and sq.
 * idx2 is computed from pieceType and sq. Corresponds to piece square tables.
 * idx3 is computed from pieceType. Corresponds to piece values. */
static void
toIndex(int kIdx, int pieceType, int sq,
        int& idx1, int& idx2, int& idx3) {
    idx1 = (kIdx * 10 + pieceType) * 64 + sq;
    idx2 = inFeats1 + pieceType * 64 + sq;
    idx3 = inFeats1 + inFeats2 + pieceType;
}

/** Given a position, compute which features it activates for both white and
 *  black side. */
static void
toSparse(const Record& r, std::vector<int>& idxVecW, std::vector<int>& idxVecB) {
    int pieceType = 0;
#if USE_CASTLING
    int k1 = r.wKing;
    int k2 = r.bKing < 64 ? Square::mirrorY(r.bKing) : r.bKing;
#else
    int k1 = r.wKing < 64 ? r.wKing : E1;
    int k2 = Square::mirrorY(r.bKing < 64 ? r.bKing : E8);
#endif

    auto addIndex = [](std::vector<int>& idxVec, int k, int pieceType, int sq) {
        int idx1, idx2, idx3;
        int kIdx;
        if (k < 64) {
            int x = Square::getX(k);
            int y = Square::getY(k);
            if (x >= 4) {
                x = Square::mirrorX(x);
                sq = Square::mirrorX(sq);
            }
            kIdx = y * 4 + x;
        } else {
            kIdx = k - 32;
        }
        toIndex(kIdx, pieceType, sq, idx1, idx2, idx3);
        idxVec.push_back(idx1);
        idxVec.push_back(idx2);
        idxVec.push_back(idx3);
    };

    for (int i = 0; i < 30; i++) {
        while (pieceType < 9 && i >= r.nPieces[pieceType])
            pieceType++;
        int sq = r.squares[i];
        if (sq == -1)
            continue;
        int oPieceType = (pieceType + 5) % 10;
        int mSq = Square::mirrorY(sq);
        addIndex(idxVecW, k1, pieceType, sq);
        addIndex(idxVecB, k2, oPieceType, mSq);
    }
}

/** Get a batch of training data from ds[beg:end].
 * @param inW, inB: Shape (batchSize, inFeatures). The sparse binary inputs for
 *                  white/black corresponding to the training positions.
 * @param out:      Shape (batchSize, 1). The corresponding desired outputs
 *                  (position evaluation score). */
template <typename DataSet>
static void
getData(DataSet& ds, size_t beg, size_t end,
        torch::Tensor& inW, torch::Tensor& inB, torch::Tensor& out) {
    int batchSize = end - beg;
    Record r;
    std::vector<int> idxVec1W;
    std::vector<int> idxVec1B;
    std::vector<int> idxVec2;
    out = torch::empty({batchSize, 1}, torch::kF32);
    for (int b = 0; b < batchSize; b++) {
        ds.getItem(beg + b, r);
        toSparse(r, idxVec1W, idxVec1B);
        idxVec2.resize(idxVec1W.size(), b);
        out[b][0] = r.searchScore * 1e-2;
    }
    int nnz = idxVec1W.size();

    for (int c = 0; c < 2; c++) {
        torch::Tensor indices = torch::empty({2, nnz}, torch::kI64);
        auto acc = indices.accessor<S64,2>();
        for (int i = 0; i < nnz; i++) {
            acc[0][i] = idxVec2[i];
            acc[1][i] = c == 0 ? idxVec1W[i] : idxVec1B[i];
        }
        torch::Tensor values = torch::ones({nnz}, torch::kF32);
        (c == 0 ? inW : inB) = sparse_coo_tensor(indices, values, {batchSize, inFeatures});
    }
}

// ------------------------------------------------------------------------------

/** A data set that reads the data records from a file. */
class DataSet {
public:
    DataSet(const std::string& filename);

    /** Get number of records in the data set. */
    S64 getSize() const;
    /** Get the idx:th record in the data set. */
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

/** A randomly shuffled version of the base data set. */
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

/** A subset of the base data set. */
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

/** Write the first "nPos" positions from "ds" to "outFile". */
template <typename DataSet>
static void
writeDataSet(DataSet& ds, S64 nPos, const std::string& outFile) {
    std::ofstream os;
    os.open(outFile.c_str(), std::ios_base::out | std::ios_base::binary);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    Record r;
    for (S64 i = 0; i < nPos; i++) {
        ds.getItem(i, r);
        os.write((const char*)&r, sizeof(Record));
    }
}

/** Extract a random sample of nPos records from "inFile", write to "outFile". */
static void
extractSubset(const std::string& inFile, S64 nPos, const std::string& outFile) {
    DataSet allData(inFile);
    ShuffledDataSet<DataSet> shuffled(allData, 17);
    writeDataSet(shuffled, nPos, outFile);
}

// ------------------------------------------------------------------------------

/** A PyTorch net that will be used as evaluation function after quantization. */
class Net : public torch::nn::Module {
public:
    Net();

    torch::Tensor forward(torch::Tensor xW, torch::Tensor xB);

    /** Clamp weights to remain within a range that is compatible
     *  with later quantization. */
    void clamp();

    /** Write weight/bias matrices to files in current directory. */
    void printWeights(int epoch) const;

    /** Create quantized net. */
    void quantize(NetData& qNet) const;

private:
    torch::nn::Linear lin1 = nullptr; const int n1 = 256;
    torch::nn::Linear lin2 = nullptr; const int n2 = 32;
    torch::nn::Linear lin3 = nullptr; const int n3 = 32;
    torch::nn::Linear lin4 = nullptr;
};

Net::Net() {
    lin1 = register_module("lin1", torch::nn::Linear(inFeatures, n1));
    lin2 = register_module("lin2", torch::nn::Linear(n1*2, n2));
    lin3 = register_module("lin3", torch::nn::Linear(n2, n3));
    lin4 = register_module("lin4", torch::nn::Linear(n3, 1));
}

torch::Tensor
Net::forward(torch::Tensor xW, torch::Tensor xB) {
    xW = torch::clamp(lin1->forward(xW), 0.0f, 1.0f);
    xB = torch::clamp(lin1->forward(xB), 0.0f, 1.0f);
    torch::Tensor x = torch::hstack({xW, xB});

    x = torch::clamp(lin2->forward(x), 0.0f, 1.0f);
    x = torch::clamp(lin3->forward(x), 0.0f, 1.0f);
    x = lin4->forward(x);
    return x;
}

void
Net::clamp() {
    c10::NoGradGuard guard;

    auto clampLayer = [](torch::nn::Linear lin) {
        float maxVal = 127.0f / 64.0f;
        lin->weight.clamp_(-maxVal, maxVal);
        lin->bias.clamp_(-maxVal, maxVal);
    };
    clampLayer(lin2);
    clampLayer(lin3);
    clampLayer(lin4);
}

void
Net::printWeights(int epoch) const {
    auto getName = [](const std::string& baseName, int epoch) {
        std::stringstream ss;
        ss << baseName << std::setfill('0') << std::setw(2) << epoch << ".txt";
        return ss.str();
    };

    auto printTensor = [](const std::string& filename, torch::Tensor x) {
        x = x.to(torch::kCPU);
        if (x.dim() == 1) {
            x = x.reshape({x.size(0), 1});
        } else if (x.dim() == 2) {
            x = x.t();
        }

        std::ofstream os;
        os.open(filename.c_str(), std::ios_base::out | std::ios_base::binary);
        os.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        size_t rows = x.size(0);
        size_t cols = x.size(1);
        auto acc = x.accessor<float,2>();
        for (size_t r = 0; r < rows; r++) {
            for (size_t c = 0; c < cols; c++) {
                if (c > 0)
                    os << ' ';
                os << acc[r][c];
            }
            os << '\n';
        }
    };

    auto printLin = [&](torch::nn::Linear lin, const std::string& name) {
        printTensor(getName(name + "w", epoch), lin->weight);
        printTensor(getName(name + "b", epoch), lin->bias);
    };

    printLin(lin1, "lin1");
    printLin(lin2, "lin2");
    printLin(lin3, "lin3");
    printLin(lin4, "lin4");
}

/** Convert a 1D or 2D tensor to a 1D vector, using row-major order for a 2D tensor. */
static std::vector<float>
toVec(torch::Tensor x) {
    if (x.dim() == 1)
        x = x.reshape({x.size(0), 1});
    const int M = x.size(0);
    const int N = x.size(1);
    auto acc = x.accessor<float,2>();
    std::vector<float> ret(M * N);
    for (int r = 0; r < M; r++)
        for (int c = 0; c < N; c++)
            ret[r*N+c] = acc[r][c];
    return ret;
}

/** Compute out[i] = in[i] * scaleFactor + offs, clamped to the range of T. */
template <typename T>
static void
scaleCopy(const std::vector<float>& in, T* out, int scaleFactor, int offs) {
    int nElem = in.size();
    long minVal = std::numeric_limits<T>::min();
    long maxVal = std::numeric_limits<T>::max();
    for (int i = 0; i < nElem; i++) {
        long val = std::lround(in[i] * scaleFactor) + offs;
        val = clamp(val, minVal, maxVal);
        out[i] = val;
    }
}

void
Net::quantize(NetData& qNet) const {
    // Apply factorized weights to non-factorized weights
    torch::Tensor lin1W = lin1->weight.t().clone();
    torch::Tensor lin1B = lin1->bias;
    {
        auto sameSquare = [](int kIdx, int sq) -> bool {
            int x = Square::getX(sq);
            int y = Square::getY(sq);
            if (x >= 4)
                return false;
            return kIdx == y * 4 + x;
        };
        auto acc = lin1W.accessor<float,2>();
        for (int kIdx = 0; kIdx < nKIdx; kIdx++)
            for (int pieceType = 0; pieceType < 10; pieceType++)
                for (int sq = 0; sq < 64; sq++) {
                    bool illegalPawn = (sq < 8 || sq >= 56) && (pieceType == 4 || pieceType == 9);
                    bool illegal = illegalPawn || sameSquare(kIdx, sq);
                    int idx1, idx2, idx3;
                    toIndex(kIdx, pieceType, sq, idx1, idx2, idx3);
                    for (int i = 0; i < n1; i++) {
                        if (illegal)
                            acc[idx1][i] = 0;
                        else
                            acc[idx1][i] += acc[idx2][i] + acc[idx3][i];
                    }
                }
        lin1W = lin1W.narrow(0, 0, inFeats1);
    }

    std::vector<float> data = toVec(lin1W);
    scaleCopy(data, &qNet.weight1.data[0], 4*127, 0);
    data = toVec(lin1B);
    scaleCopy(data, &qNet.bias1.data[0], 4*127, 2);

    data = toVec(lin2->weight);
    scaleCopy(data, &qNet.lin2.weight.data[0], 64, 0);
    data = toVec(lin2->bias);
    scaleCopy(data, &qNet.lin2.bias.data[0], 64*127, 32);

    data = toVec(lin3->weight);
    scaleCopy(data, &qNet.lin3.weight.data[0], 64, 0);
    data = toVec(lin3->bias);
    scaleCopy(data, &qNet.lin3.bias.data[0], 64*127, 32);

    data = toVec(lin4->weight);
    scaleCopy(data, &qNet.lin4.weight.data[0], 64, 0);
    data = toVec(lin4->bias);
    scaleCopy(data, &qNet.lin4.bias.data[0], 64*127, 32);
}

// ------------------------------------------------------------------------------

/** Splits a data set in a training part and a validation part. */
class SplitData {
public:
    using ShuffledSet = ShuffledDataSet<DataSet>;
    using SubSet = SubDataSet<ShuffledSet>;

    /** Constructor. */
    SplitData(DataSet& allData)
        : nData(allData.getSize()),
          nTrain(nData * 9 / 10),
          seed0(0),
          allShuffled(allData, seed0),
          trainData(allShuffled, 0, nTrain),
          validateData(allShuffled, nTrain, nData) {
    }

    /** Get training data. */
    SubSet& getTrainData() { return trainData; }
    /** Get validation data. */
    SubSet& getValidateData() { return validateData; }

private:
    const size_t nData;
    const size_t nTrain;
    const U64 seed0;
    ShuffledSet allShuffled;
    SubSet trainData;
    SubSet validateData;
};

/** Set learning rate for an optimizer. */
static void
setLR(torch::optim::Optimizer& opt, double lr) {
    for (auto& group : opt.param_groups()) {
        if (group.has_options()) {
            auto& options = static_cast<torch::optim::OptimizerOptions&>(group.options());
            options.set_lr(lr);
        }
    }
}

/** Convert a decimal score (1.0 = 1 pawn) to an expected score in the [0,1] range. */
template <typename T>
static T
toProb(T score) {
    double k = -113.0 / 400 * log(10);
    return 1 / (1 + exp(score * k));
}

/** Compute RMS loss for a quantized network over a data set. */
template <typename DataSet>
static double
getQLoss(DataSet& ds, const NetData& qNet) {
    double qLoss = 0;
    std::shared_ptr<NNEvaluator> qEval = NNEvaluator::create(qNet);
    Record r;
    Position pos;
    S64 nPos = ds.getSize();
    for (S64 i = 0; i < nPos; i++) {
        ds.getItem(i, r);
        int target;
        NNUtil::recordToPos(r, pos, target);

        qEval->connectPosition(pos);
        pos.connectNNEval(qEval.get());
        int qVal = qEval->eval();

        double err = toProb(qVal*0.01) - toProb(target*0.01);
        qLoss += err*err;
    }
    qLoss = sqrt(qLoss / nPos);
    return qLoss;
}

/** Train a network using training data from "inFile". After each training epoch,
 *  the current net is saved in PyTorch format in the file modelNN.pt. */
static void
train(const std::string& inFile, U64 seed) {
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
    SplitData split(allData);
    auto& trainData = split.getTrainData();
    auto& validateData = split.getValidateData();
    using SubSet = SplitData::SubSet;

    const size_t nData = allData.getSize();
    const size_t nTrain = trainData.getSize();
    const size_t nValidate = validateData.getSize();
    const int batchSize = 16*1024;
    std::cout << "nData    : " << nData << std::endl;
    std::cout << "nTrain   : " << nTrain << " (" << (nTrain / batchSize) << " batches)" << std::endl;
    std::cout << "nValidate: " << nValidate << " (" << (nValidate / batchSize) << " batches)" << std::endl;

    double lr = 1e-3;
    auto opts = torch::optim::AdamOptions(lr).weight_decay(1e-6);
    torch::optim::Adam optimizer(net.parameters(), opts);
    net.printWeights(0);
    for (int epoch = 1; epoch <= 30; epoch++) {
        ShuffledDataSet<SubSet> epochTrainData(trainData, hashU64(seed) + epoch);
        std::cout << "Epoch: " << epoch << " lr: " << lr << std::endl;

        torch::Tensor lossSum = torch::zeros({}, torch::kF32).to(dev);
        int lossNum = 0;
        for (size_t batch = 0, beg = 0; beg < nTrain; batch++, beg += batchSize) {
            size_t end = std::min(beg + batchSize, nTrain);

            torch::Tensor inputW, inputB;
            torch::Tensor target;
            getData(epochTrainData, beg, end, inputW, inputB, target);
            inputW = inputW.to(dev);
            inputB = inputB.to(dev);
            target = target.to(dev);

            optimizer.zero_grad();
            torch::Tensor output = net.forward(inputW, inputB);

            torch::Tensor loss = mse_loss(toProb(output), toProb(target));
            loss.backward();
            optimizer.step();
            net.clamp();

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
            {
                c10::InferenceMode guard;
                lossSum *= 0;
                lossNum = 0;
                for (size_t batch = 0, beg = 0; beg < nValidate; batch++, beg += batchSize) {
                    size_t end = std::min(beg + batchSize, nValidate);

                    torch::Tensor inputW, inputB;
                    torch::Tensor target;
                    getData(validateData, beg, end, inputW, inputB, target);
                    inputW = inputW.to(dev);
                    inputB = inputB.to(dev);
                    target = target.to(dev);

                    torch::Tensor output = net.forward(inputW, inputB);
                    torch::Tensor loss = mse_loss(toProb(output), toProb(target));
                    lossSum += loss;
                    lossNum += 1;
                }
            }
            double qLoss;
            {
                std::shared_ptr<NetData> qNetP = NetData::create();
                NetData& qNet = *qNetP;
                net.to(torch::kCPU);
                net.quantize(qNet);
                net.to(torch::kCUDA);
                qNet.prepareMatMul();
                qLoss = getQLoss(validateData, qNet);
            }

            std::cout << "Validation error: "
                      << std::sqrt(lossSum.item<float>() / lossNum)
                      << " Quantized eror: " << qLoss
                      << std::endl;
        }

        {
            std::stringstream ss;
            ss << "model" << std::setfill('0') << std::setw(2) << epoch << ".pt";
            torch::save(netP, ss.str().c_str());
        }

        lr *= 0.8;
        setLR(optimizer, lr);
        net.printWeights(epoch);
    }
}

// ------------------------------------------------------------------------------

template <typename T, int M, int N>
static void
matMinMax(const Matrix<T,M,N>& m, int& minVal, int& maxVal) {
    minVal = INT_MAX;
    maxVal = INT_MIN;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            minVal = std::min(minVal, (int)m(i, j));
            maxVal = std::max(maxVal, (int)m(i, j));
        }
    }
}

template <typename T, int N>
static void
vecMinMax(const Vector<T,N>& m, int& minVal, int& maxVal) {
    minVal = INT_MAX;
    maxVal = INT_MIN;
    for (int i = 0; i < N; i++) {
        minVal = std::min(minVal, (int)m(i));
        maxVal = std::max(maxVal, (int)m(i));
    }
}

/** Print network statistics to stdout. */
static void
printStats(const NetData& net) {
    int minOut = INT_MAX;
    int maxOut = -INT_MAX;
    for (int f = 0; f < NetData::n1; f++) {
        for (int kIdx = 0; kIdx < nKIdx; kIdx++) {
            int minOutF = net.bias1(f);
            int maxOutF = net.bias1(f);
            for (int c = 0; c < 2; c++) {
                std::vector<int> wMin, wMax;
                for (int sq = 0; sq < 64; sq++) {
                    int minSqV = INT_MAX;
                    int maxSqV = INT_MIN;
                    for (int pt = 0; pt < 5; pt++) {
                        int idx1, idx2, idx3;
                        toIndex(kIdx, c*5+pt, sq, idx1, idx2, idx3);
                        int val = net.weight1(idx1, f);
                        minSqV = std::min(minSqV, val);
                        maxSqV = std::max(maxSqV, val);
                    }
                    wMin.push_back(minSqV);
                    wMax.push_back(maxSqV);
                }
                std::sort(wMin.begin(), wMin.end());
                std::sort(wMax.begin(), wMax.end());
                int maxPieces = 15;
                for (int i = 0; i < maxPieces; i++) {
                    minOutF += std::min(0, (int)wMin[i]);
                    maxOutF += std::max(0, (int)wMax[(int)wMax.size() - 1 - i]);
                }
            }
            minOut = std::min(minOut, minOutF);
            maxOut = std::max(maxOut, maxOutF);
        }
    }
    std::cout << "Layer           min   max" << std::endl;
    std::cout << "L1 out     : " << std::setw(6) << minOut << std::setw(6) << maxOut << std::endl;

    int minV, maxV;

    matMinMax(net.lin2.weight, minV, maxV);
    std::cout << "L2 weights : " << std::setw(6) << minV << std::setw(6) << maxV << std::endl;
    vecMinMax(net.lin2.bias, minV, maxV);
    std::cout << "L2 bias    : " << std::setw(6) << minV << std::setw(6) << maxV << std::endl;

    matMinMax(net.lin3.weight, minV, maxV);
    std::cout << "L3 weights : " << std::setw(6) << minV << std::setw(6) << maxV << std::endl;
    vecMinMax(net.lin3.bias, minV, maxV);
    std::cout << "L3 bias    : " << std::setw(6) << minV << std::setw(6) << maxV << std::endl;

    matMinMax(net.lin4.weight, minV, maxV);
    std::cout << "L4 weights : " << std::setw(6) << minV << std::setw(6) << maxV << std::endl;
    vecMinMax(net.lin4.bias, minV, maxV);
    std::cout << "L4 bias    : " << std::setw(6) << minV << std::setw(6) << maxV << std::endl;
}

/** Convert a serialized floating point network of type Net in "inFile" to a
 *  quantized integer network of type NetData written to "outFile". */
static void
quantize(const std::string& inFile, const std::string& outFile,
         bool compress, const std::string& validationFile) {
    auto netP = std::make_shared<Net>();
    Net& net = *netP;
    torch::load(netP, inFile.c_str());
    net.to(torch::kCPU);

    std::shared_ptr<NetData> qNetP = NetData::create();
    NetData& qNet = *qNetP;
    net.quantize(qNet);

    {
        std::ofstream os;
        os.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        os.open(outFile.c_str(), std::ios_base::out | std::ios_base::binary);
        qNet.save(os);
    }

    if (compress) { // Save compressed network using lzma from Gaviota tablebase code
        std::stringstream ss;
        qNet.save(ss);
        std::string data = ss.str();

        size_t outSize = data.size();
        std::vector<unsigned char> comprData(outSize);

        unsigned char* unCompressedData = (unsigned char*)&data[0];
        size_t unCompressedSize = data.size();

        int level = 9;
        unsigned int memory = 64*1024*1024;
        int filter = SZ_FILTER_NO;

        int res = Lzma86_Encode(comprData.data(), &outSize,
                                unCompressedData, unCompressedSize,
                                level, memory, filter);
        if (res != SZ_OK)
            throw ChessError("Failed to compress data");

        std::cout << "Compressed size: " << outSize << std::endl;
        std::ofstream os;
        os.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::string comprName = outFile + ".compr";
        os.open(comprName.c_str(), std::ios_base::out | std::ios_base::binary);
        BinaryFileWriter writer(os);
        writer.writeArray(comprData.data(), outSize);
    }

    printStats(qNet);

    if (!validationFile.empty()) {
        DataSet ds(validationFile);
        qNet.prepareMatMul();
        double qLoss = getQLoss(ds, qNet);
        std::cout << "Quantized error: " << qLoss << std::endl;
    }
}

// ------------------------------------------------------------------------------

static void writeFEN(std::ostream& os, const std::string& fen,
                     double result, int searchScore, int qScore, int gameNo,
                     const std::string& extra = "") {
    os << fen << " : "
       << result << " : "
       << searchScore << " : "
       << qScore << " : "
       << gameNo;
    if (!extra.empty())
        os << " : " << extra;
    os << '\n';
}

/** Write data set to "os" in FEN format. */
template <typename DataSet>
void
writeDataSetFen(DataSet& ds, std::ostream& os) {
    const size_t n = ds.getSize();
    Record r;
    Position pos;
    for (size_t i = 0; i < n; i++) {
        ds.getItem(i, r);
        int score;
        NNUtil::recordToPos(r, pos, score);
        writeFEN(os, TextIO::toFEN(pos), -1, score, -1, -1);
    }
}

/** If "inFile" were to be used to train a network, some of the training data
 *  would be set aside for validation. This function writes that validation data
 *  to "os". */
static void
extractValidationData(const std::string& inFile, const std::string& outFile) {
    DataSet allData(inFile);
    SplitData split(allData);
    auto& validateData = split.getValidateData();
    writeDataSet(validateData, validateData.getSize(), outFile);
}

/** Convert a data set binary file "inFile" to FEN format written to "os". */
static void
bin2Fen(const std::string& inFile, std::ostream& os) {
    DataSet allData(inFile);
    writeDataSetFen(allData, os);
}

// ------------------------------------------------------------------------------

/** Evaluate one or more chess positions using both a floating point network
 *  (read from "modelFile") and a corresponding quantized network.
 *  If "fen" is "-", read a sequence of positions in fen format from standard
 *  input, otherwise evaluate the position given by "fen". */
static void
eval(const std::string& modelFile, const std::string& fen) {
    auto netP = std::make_shared<Net>();
    Net& net = *netP;
    torch::load(netP, modelFile.c_str());
    net.to(torch::kCPU);

    std::shared_ptr<NetData> qNetP = NetData::create();
    NetData& qNet = *qNetP;
    net.quantize(qNet);
    qNet.prepareMatMul();
    std::shared_ptr<NNEvaluator> qEval = NNEvaluator::create(qNet);

    bool fromStdIn = fen == "-";
    std::istream& is = std::cin;

    c10::InferenceMode guard;
    while (true) {
        Position pos;
        if (fromStdIn) {
            std::string line;
            std::getline(is, line);
            if (!is || is.eof())
                break;
            pos = TextIO::readFEN(line);
        } else {
            pos = TextIO::readFEN(fen);
        }

        Record r;
        NNUtil::posToRecord(pos, 0, r);

        std::vector<int> idxVecW, idxVecB;
        toSparse(r, idxVecW, idxVecB);
        const int nnz = idxVecW.size();

        torch::Tensor inW, inB;

        for (int c = 0; c < 2; c++) {
            torch::Tensor indices = torch::empty({2, nnz}, torch::kI64);
            auto acc = indices.accessor<S64,2>();
            for (int i = 0; i < nnz; i++) {
                acc[0][i] = 0;
                acc[1][i] = c == 0 ? idxVecW[i] : idxVecB[i];
            }
            torch::Tensor values = torch::ones({nnz}, torch::kF32);
            (c == 0 ? inW : inB) = sparse_coo_tensor(indices, values, {1, inFeatures});
        }

        torch::Tensor out = net.forward(inW, inB);
        double val = out.item<double>();

        qEval->connectPosition(pos);
        pos.connectNNEval(qEval.get());
        int qVal = qEval->eval();

        std::cout << "val: " << (val*100.0f) << " prob: " << toProb(val)
                  << " qVal: " << qVal << " qProb: " << toProb(qVal * 0.01)
                  << std::endl;

        if (!fromStdIn)
            break;
    }
}

// ------------------------------------------------------------------------------

/** Given training data in "inFile", for each input feature, calculate how many
 *  data points activates it. Print result to standard output. */
static void
featureStats(const std::string& inFile) {
    DataSet allData(inFile);
    const size_t nData = allData.getSize();
    std::vector<S64> stats(inFeatures);
    std::vector<int> idxVecW, idxVecB;
    Record r;
    for (size_t i = 0; i < nData; i++) {
        allData.getItem(i, r);
        idxVecW.clear();
        idxVecB.clear();
        toSparse(r, idxVecW, idxVecB);
        for (int idx : idxVecW)
            stats[idx]++;
        for (int idx : idxVecB)
            stats[idx]++;
    }
    for (int i = 0; i < inFeatures; i++) {
        std::stringstream ss;
        if (i < inFeats1) {
            int tmp = i;
            int sq = tmp % 64; tmp /= 64;
            int pt = tmp % 10; tmp /= 10;
            int kx = tmp % 4; tmp /= 4;
            int ky = tmp;
            if ((pt == 4 || pt == 9) && (Square::getY(sq) == 0 || Square::getY(sq) == 7))
                continue;
            int kSq = Square::getSquare(kx, ky);
            if (sq == kSq)
                continue;
            if (ky < 8) {
                sq = Square::mirrorX(sq);
                kSq = Square::mirrorX(kSq);
                ss << 'K' << TextIO::squareToString(kSq) << ',';
            } else {
                if (sq == E1)
                    continue;
                ss << 'K';
                if ((kx + 1) & 1) {
                    if (sq == A1 && pt != 1)
                        continue; // Must be white rook on A1
                    ss << 'A';
                }
                if ((kx + 1) & 2) {
                    if (sq == H1 && pt != 1)
                        continue; // Must be white rook on H1
                    ss << 'H';
                }
                ss << ',';
            }
            ss << "QRBNPqrbnp"[pt] << TextIO::squareToString(sq);
        } else if (i < inFeats1 + inFeats2) {
            int tmp = i - inFeats1;
            int sq = tmp % 64; tmp /= 64;
            int pt = tmp;
            if ((pt == 4 || pt == 9) && (Square::getY(sq) == 0 || Square::getY(sq) == 7))
                continue;
            sq = Square::mirrorX(sq);
            ss << "QRBNPqrbnp"[pt] << TextIO::squareToString(sq);
        } else {
            int pt = i - inFeats1 - inFeats2;
            ss << "QRBNPqrbnp"[pt];
        }
        std::cout << i << ' ' << stats[i] << ' ' << ss.str() << std::endl;
    }
}

// ------------------------------------------------------------------------------

/** Print usage information to standard error and exit program. */
static void
usage() {
    std::cerr << "Usage: torchutil cmd params\n";
    std::cerr << "cmd is one of:\n";
    std::cerr << " train infile\n";
    std::cerr << "   Train network from data in infile\n";
    std::cerr << " quant [-c] infile outfile [validationFile]\n";
    std::cerr << "   Quantize infile, write result to outfile\n";
    std::cerr << "   -c : Also create compressed network\n";
    std::cerr << " eval modelfile fen\n";
    std::cerr << "   Evaluate position using a saved network\n";
    std::cerr << " subset infile nPos outfile\n";
    std::cerr << "   Extract positions from infile, write to outfile\n";
    std::cerr << " bin2fen infile\n";
    std::cerr << "   Read binary data, write in FEN format to stdout\n";
    std::cerr << " getvalidation infile outfile\n";
    std::cerr << "   Extract validation data in binary format\n";
    std::cerr << " featstat infile\n";
    std::cerr << "   Print feature activation stats from training data\n";

    std::cerr << std::flush;
    ::exit(2);
}

int
main(int argc, const char* argv[]) {
    try {
        if (argc < 2)
            usage();

        std::string cmd = argv[1];
        if (cmd == "train") {
            if (argc != 3)
                usage();
            std::string inFile = argv[2];
            U64 seed = (U64)(currentTime() * 1000);
            train(inFile, seed);
        } else if (cmd == "quant") {
            if (argc < 4)
                usage();
            bool compr = false;
            if (argv[2] == std::string("-c"))
                compr = true;
            if (argc > 5 + compr)
                usage();
            std::string inFile = argv[2+compr];
            std::string outFile = argv[3+compr];
            std::string validationFile = (argc == 5+compr) ? argv[4+compr] : "";
            quantize(inFile, outFile, compr, validationFile);
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
        } else if (cmd == "bin2fen") {
            if (argc != 3)
                usage();
            std::string inFile = argv[2];
            bin2Fen(inFile, std::cout);
        } else if (cmd == "getvalidation") {
            if (argc != 4)
                usage();
            std::string inFile = argv[2];
            std::string outFile = argv[3];
            extractValidationData(inFile, outFile);
        } else if (cmd == "featstat") {
            if (argc != 3)
                usage();
            std::string inFile = argv[2];
            featureStats(inFile);
        } else {
            usage();
        }
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}
