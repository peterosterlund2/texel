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
#include "threadpool.hpp"
#include "randperm.hpp"
#include "featureperm.hpp"

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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

extern "C" {
#include "tb/gtb/compression/lzma/Lzma86Enc.h"
}

// ------------------------------------------------------------------------------

const int nKIdx = 32;
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
    int k1 = r.wKing < 64 ? r.wKing : E1;
    int k2 = Square(r.bKing < 64 ? r.bKing : E8).mirrorY().asInt();

    auto addIndex = [](std::vector<int>& idxVec, int k, int pieceType, int sq) {
        int idx1, idx2, idx3;
        int kIdx;
        if (k < 64) {
            int x = Square(k).getX();
            int y = Square(k).getY();
            if (x >= 4) {
                x = Square(x).mirrorX().asInt();
                sq = Square(sq).mirrorX().asInt();
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
        int mSq = Square(sq).mirrorY().asInt();
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
    std::vector<int> idxVec1W; idxVec1W.reserve(batchSize * 30);
    std::vector<int> idxVec1B; idxVec1B.reserve(batchSize * 30);
    std::vector<int> idxVec2;  idxVec2.reserve(batchSize * 30);
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

template <typename DataSet>
class DataLoader {
public:
    explicit DataLoader(DataSet& ds) : ds(ds), pool(1) {}

    void startGetData(size_t beg, size_t end);
    void getData(torch::Tensor& inW, torch::Tensor& inB, torch::Tensor& out);

private:
    DataSet& ds;
    struct Result {
        torch::Tensor inW;
        torch::Tensor inB;
        torch::Tensor out;
    };
    ThreadPool<Result> pool;
};

template <typename DataSet>
void
DataLoader<DataSet>::startGetData(size_t beg, size_t end) {
    auto func = [this,beg,end](int workerNo) -> Result {
        Result r;
        ::getData(ds, beg, end, r.inW, r.inB, r.out);
        return r;
    };
    pool.addTask(func);
}

template <typename DataSet>
void
DataLoader<DataSet>::getData(torch::Tensor& inW, torch::Tensor& inB, torch::Tensor& out) {
    Result r;
    if (!pool.getResult(r))
        throw ChessError("startGetData() not called");
    inW = r.inW;
    inB = r.inB;
    out = r.out;
}

// ------------------------------------------------------------------------------

/** A data set that reads the data records from a file. */
class DataSet {
public:
    DataSet(const std::string& filename);
    DataSet(const DataSet& other);
    ~DataSet();

    /** Get number of records in the data set. */
    S64 getSize() const;
    /** Get the idx:th record in the data set. */
    void getItem(S64 idx, Record& r);

private:
    void openFile();

    std::string filename;
    const Record* mapping = nullptr;
    S64 size;
};

DataSet::DataSet(const DataSet& other)
    : filename(other.filename), size(other.size) {
    openFile();
}

DataSet::DataSet(const std::string& filename)
    : filename(filename) {
    openFile();
}

DataSet::~DataSet() {
    munmap((void*)mapping, size * sizeof(Record));
}

void
DataSet::openFile() {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
        throw ChessError("Failed to open file");

    struct stat statbuf;
    int ret = fstat(fd, &statbuf);
    if (ret == -1)
        throw ChessError("stat error");
    size = statbuf.st_size / sizeof(Record);

    mapping = (const Record*)mmap(nullptr, statbuf.st_size,
                                  PROT_READ, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED)
        throw ChessError("mmap failed");

    close(fd);
}

inline S64
DataSet::getSize() const {
    return size;
}

void
DataSet::getItem(S64 idx, Record& r) {
    r = mapping[idx];
}

// ------------------------------------------------------------------------------

/** A randomly shuffled version of the base data set. */
template <typename Base>
class ShuffledDataSet {
public:
    ShuffledDataSet(const Base& baseSet, U64 seed);

    S64 getSize() const;
    void getItem(S64 idx, Record& r);

public:
    Base baseSet;
    RandPerm rndPerm;
};

template <typename Base>
ShuffledDataSet<Base>::ShuffledDataSet(const Base& baseSet, U64 seed)
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
    SubDataSet(const Base& baseSet, U64 beg, U64 end);

    S64 getSize() const;
    void getItem(S64 idx, Record& r);

private:
    Base baseSet;
    const U64 beg;
    const U64 end;
};

template <typename Base>
SubDataSet<Base>::SubDataSet(const Base& baseSet, U64 beg, U64 end)
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
    void clamp(bool useQAT);

    /** Write weight/bias matrices to files in current directory. */
    void printWeights(int epoch) const;

    /** Initialize network weights. */
    void initWeights();

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
    x *= 2;
    return x;
}

void
Net::initWeights() {
    c10::NoGradGuard guard;

    // First layer input is sparse, so default initialization
    // gives smaller values than desired.
    lin1->weight *= 30;
    lin1->bias *= 30;
}

void
Net::clamp(bool useQAT) {
    c10::NoGradGuard guard;

    auto clampLayer = [useQAT](torch::nn::Linear lin) {
        float maxVal = 127.0f / 64.0f;
        auto clampRound = [maxVal](torch::Tensor w) {
            w.clamp_(-maxVal, maxVal);
            w *= 64.0f;
            w.round_();
            w *= 1.0f/64.0f;

        };
        if (useQAT) {
            clampRound(lin->weight);
        } else {
            lin->weight.clamp_(-maxVal, maxVal);
        }
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
            int x = Square(sq).getX();
            int y = Square(sq).getY();
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
    int l1Scale = 1 << NetData::l1Shift;
    scaleCopy(data, &qNet.weight1.data[0], l1Scale*127, 0);
    data = toVec(lin1B);
    scaleCopy(data, &qNet.bias1.data[0], l1Scale*127, l1Scale/2);

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
getQLoss(DataSet& ds, const NetData& qNet, int nWorkers) {
    struct ThreadData {
        DataSet ds;
        std::shared_ptr<NNEvaluator> qEval;
        Record r;
        Position pos;
        ThreadData(DataSet& ds, const NetData& net) : ds(ds) {
            qEval = NNEvaluator::create(net);
            qEval->connectPosition(pos);
            pos.connectNNEval(qEval.get());
        }
    };
    std::vector<std::shared_ptr<ThreadData>> tdVec(nWorkers);
    for (int i = 0; i < nWorkers; i++)
        tdVec[i] = std::make_shared<ThreadData>(ds, qNet);

    S64 nPos = ds.getSize();
    double qLoss = 0;
    const int batchSize = 32*1024;
    ThreadPool<double> pool(nWorkers);
    for (S64 i = 0; i < nPos; i += batchSize) {
        S64 beginIdx = i;
        S64 endIdx = std::min(i + batchSize, nPos);
        auto func = [&tdVec,beginIdx,endIdx](int workerNo) {
            ThreadData& td = *tdVec[workerNo];
            double qLoss = 0;
            for (S64 i = beginIdx; i < endIdx; i++) {
                td.ds.getItem(i, td.r);
                int target;
                NNUtil::recordToPos(td.r, td.pos, target);

                int qVal = td.qEval->eval();
                double err = toProb(qVal*0.01) - toProb(target*0.01);
                qLoss += err*err;
            }
            return qLoss;
        };
        pool.addTask(func);
    }
    pool.getAllResults([&qLoss](double batchLoss) {
        qLoss += batchLoss;
    });
    qLoss = sqrt(qLoss / nPos);
    return qLoss;
}

/** Train a network using training data from "inFile". After each training epoch,
 *  the current net is saved in PyTorch format in the file modelNN.pt. */
static void
train(const std::string& inFile, int nEpochs, bool useQAT, double initialLR, U64 seed,
      const std::string& initialModel, int nWorkers) {
    const auto dev = torch::kCUDA;
    const double t0 = currentTime();

    auto netP = std::make_shared<Net>();
    Net& net = *netP;
    if (initialModel.empty()) {
        net.initWeights();
    } else {
        torch::load(netP, initialModel.c_str());
    }
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

    double lr = initialLR;
    auto opts = torch::optim::AdamOptions(lr).weight_decay(1e-6);
    torch::optim::Adam optimizer(net.parameters(), opts);
    net.printWeights(0);
    for (int epoch = 1; epoch <= nEpochs; epoch++) {
        ShuffledDataSet<SubSet> epochTrainData(trainData, hashU64(seed) + epoch);
        std::cout << "Epoch: " << epoch << " lr: " << lr << std::endl;

        DataLoader<decltype(epochTrainData)> loader(epochTrainData);
        torch::Tensor lossSum = torch::zeros({}, torch::kF32).to(dev);
        int lossNum = 0;
        for (size_t batch = 0, beg = 0; beg < nTrain; batch++, beg += batchSize) {
            size_t end = std::min(beg + batchSize, nTrain);

            torch::Tensor inputW, inputB, target;
            if (beg == 0)
                loader.startGetData(beg, end);
            loader.getData(inputW, inputB, target);
            if (beg + batchSize < nTrain) {
                size_t nextBeg = beg + batchSize;
                size_t nextEnd = std::min(nextBeg + batchSize, nTrain);
                loader.startGetData(nextBeg, nextEnd);
            }

            inputW = inputW.to(dev);
            inputB = inputB.to(dev);
            target = target.to(dev);

            optimizer.zero_grad();
            torch::Tensor output = net.forward(inputW, inputB);

            torch::Tensor loss = mse_loss(toProb(output), toProb(target));
            loss.backward();
            optimizer.step();
            net.clamp(useQAT);

            lossSum += loss;
            lossNum += 1;

            if (batch % 1000 == 0) {
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
                net.to(dev);
                qNet.prepareMatMul();
                qLoss = getQLoss(validateData, qNet, nWorkers);
            }

            std::cout << "Validation error: "
                      << std::sqrt(lossSum.item<float>() / lossNum)
                      << " Quantized error: " << qLoss
                      << std::endl;
        }

        {
            std::stringstream ss;
            ss << "model" << std::setfill('0') << std::setw(2) << epoch << ".pt";
            torch::save(netP, ss.str().c_str());
        }

        lr *= 0.85;
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

/** Permute first layer features to make the first layer output sparse. */
template <typename DataSet>
static void
permuteFeatures(NetData& net, DataSet& ds, bool useLocalSearch, U64 rndSeed,
                int nWorkers) {
    using BitSet = FeaturePerm::BitSet;
    std::vector<BitSet> featureActivations(NetData::n1);
    const int nPos = std::min(BitSet::numBits/2, (int)ds.getSize());
    {
        std::cout << "Evaluating..." << std::endl;
        std::shared_ptr<NetData> copyNetP = NetData::create();
        NetData& copyNet = *copyNetP;
        copyNet = net;
        copyNet.prepareMatMul();

        struct ThreadData {
            DataSet ds;
            std::shared_ptr<NNEvaluator> qEval;
            Record r;
            Position pos;
            ThreadData(DataSet& ds, const NetData& net) : ds(ds) {
                qEval = NNEvaluator::create(net);
                qEval->connectPosition(pos);
                pos.connectNNEval(qEval.get());
            }
        };
        std::vector<std::shared_ptr<ThreadData>> tdVec(nWorkers);
        for (int i = 0; i < nWorkers; i++)
            tdVec[i] = std::make_shared<ThreadData>(ds, copyNet);

        const int batchSize = 32*1024;
        ThreadPool<int> pool(nWorkers);
        for (int i = 0; i < nPos; i += batchSize) {
            int beginIdx = i;
            int endIdx = std::min(i + batchSize, nPos);
            auto func = [&featureActivations,&tdVec,beginIdx,endIdx](int workerNo) {
                ThreadData& td = *tdVec[workerNo];
                for (int i = beginIdx; i < endIdx; i++) {
                    td.ds.getItem(i, td.r);
                    int target;
                    NNUtil::recordToPos(td.r, td.pos, target);
                    td.qEval->eval();
                    for (int f = 0; f < NetData::n1; f++) {
                        if (td.qEval->getL1OutClipped(f) != 0)
                            featureActivations[f].setBit(2*i+0);
                        if (td.qEval->getL1OutClipped(f+NetData::n1) != 0)
                            featureActivations[f].setBit(2*i+1);
                    }
                }
                return 0;
            };
            pool.addTask(func);
        }
        pool.getAllResults([](int){});
    }

    FeaturePerm fp(net);
    fp.permute(featureActivations, nPos, useLocalSearch, rndSeed);
}

/** Convert a serialized floating point network of type Net in "inFile" to a
 *  quantized integer network of type NetData written to "outFile". */
static void
quantize(const std::string& inFile, const std::string& outFile,
         const std::string& validationFile,
         bool compress, bool permute, bool useLocalSearch, U64 rndSeed,
         bool qLoss, int nWorkers) {
    auto netP = std::make_shared<Net>();
    Net& net = *netP;
    torch::load(netP, inFile.c_str());
    net.to(torch::kCPU);

    std::shared_ptr<NetData> qNetP = NetData::create();
    NetData& qNet = *qNetP;
    net.quantize(qNet);

    if (permute) {
        DataSet ds(validationFile);
        permuteFeatures(qNet, ds, useLocalSearch, rndSeed, nWorkers);
    }

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

    if (qLoss) {
        DataSet ds(validationFile);
        qNet.prepareMatMul();
        double qLoss = getQLoss(ds, qNet, nWorkers);
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
            if ((pt == 4 || pt == 9) && (Square(sq).getY() == 0 || Square(sq).getY() == 7))
                continue;
            int kSq = Square(kx, ky).asInt();
            if (sq == kSq)
                continue;
            if (ky < 8) {
                sq = Square(sq).mirrorX().asInt();
                kSq = Square(kSq).mirrorX().asInt();
                ss << 'K' << TextIO::squareToString(Square(kSq)) << ',';
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
            ss << "QRBNPqrbnp"[pt] << TextIO::squareToString(Square(sq));
        } else if (i < inFeats1 + inFeats2) {
            int tmp = i - inFeats1;
            int sq = tmp % 64; tmp /= 64;
            int pt = tmp;
            if ((pt == 4 || pt == 9) && (Square(sq).getY() == 0 || Square(sq).getY() == 7))
                continue;
            sq = Square(sq).mirrorX().asInt();
            ss << "QRBNPqrbnp"[pt] << TextIO::squareToString(Square(sq));
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
    std::cerr << "Usage: torchutil [-j n] cmd params\n";
    std::cerr << " -j n : Use n worker threads\n";
    std::cerr << "cmd is one of:\n";
    std::cerr << " train [-i modelfile] [-lr rate] [-epochs n] [-qat] infile\n";
    std::cerr << "   Train network from data in infile\n";
    std::cerr << " quant [-c] [-p|-pl] [-ql] infile outfile [validationFile]\n";
    std::cerr << "   Quantize infile, write result to outfile\n";
    std::cerr << "   -c       : Also create compressed network\n";
    std::cerr << "   -p       : Permute features for sparsity using validationFile\n";
    std::cerr << "   -pl seed : Permute features using local search. seed=0 uses current time\n";
    std::cerr << "   -ql      : Compute RMS loss for quantized net using validationFile\n";
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

static void
checkFileExists(const std::string& filename) {
    std::ifstream f(filename.c_str());
    if (!f.good())
        throw ChessError("File not found: " + filename);
}

static void
doTrain(int argc, char* argv[], int nWorkers) {
    int nEpochs = 40;
    double initialLR = 1e-3;
    std::string modelFile;
    bool useQAT = false; // Quantization aware training
    argc -= 2;
    argv += 2;
    while (argc > 0) {
        std::string arg = argv[0];
        if (argc >= 2 && arg == "-epochs") {
            if (!str2Num(argv[1], nEpochs) || nEpochs < 1)
                usage();
            argc -= 2;
            argv += 2;
        } else if (argc >= 2 && arg == "-lr") {
            if (!str2Num(argv[1], initialLR) || initialLR <= 0)
                usage();
            argc -= 2;
            argv += 2;
        } else if (argc >= 2 && arg == "-i") {
            modelFile = argv[1];
            argc -= 2;
            argv += 2;
        } else if (arg == "-qat") {
            useQAT = true;
            argc--;
            argv++;
        } else
            break;
    }
    if (argc != 1)
        usage();

    std::string inFile = argv[0];
    checkFileExists(inFile);
    if (!modelFile.empty())
        checkFileExists(modelFile);
    U64 seed = (U64)(currentTime() * 1000);
    train(inFile, nEpochs, useQAT, initialLR, seed, modelFile, nWorkers);
}

static void
doQuantize(int argc, char* argv[], int nWorkers) {
    bool compress = false;
    bool permute = false;
    bool useLocalSearch = false;
    U64 rndSeed = 0;
    bool qLoss = false;

    argc -= 2;
    argv += 2;
    while (argc > 0) {
        std::string arg = argv[0];
        if (arg == "-c") {
            compress = true;
            argc--;
            argv++;
        } else if (arg == "-p") {
            permute = true;
            argc--;
            argv++;
        } else if (argc >= 2 && arg == "-pl") {
            if (!str2Num(argv[1], rndSeed))
                usage();
            permute = true;
            useLocalSearch = true;
            argc -= 2;
            argv += 2;
        } else if (arg == "-ql") {
            qLoss = true;
            argc--;
            argv++;
        } else
            break;
    }
    if (argc < 2 || argc > 3)
        usage();
    std::string inFile = argv[0];
    std::string outFile = argv[1];
    std::string validationFile = (argc == 3) ? argv[2] : "";
    if ((permute || qLoss) && validationFile.empty())
        usage();
    checkFileExists(inFile);
    if (!validationFile.empty())
        checkFileExists(validationFile);
    quantize(inFile, outFile, validationFile, compress, permute,
             useLocalSearch, rndSeed, qLoss, nWorkers);
}

int
main(int argc, char* argv[]) {
    try {
        int nWorkers = std::thread::hardware_concurrency();
        while (true) {
            if ((argc >= 3) && (std::string(argv[1]) == "-j")) {
                if (!str2Num(argv[2], nWorkers) || nWorkers <= 0)
                    usage();
                argc -= 2;
                argv += 2;
            } else
                break;
        }
        if (argc < 2)
            usage();

        std::string cmd = argv[1];
        if (cmd == "train") {
            doTrain(argc, argv, nWorkers);
        } else if (cmd == "quant") {
            doQuantize(argc, argv, nWorkers);
        } else if (cmd == "eval") {
            if (argc != 4)
                usage();
            std::string modelFile = argv[2];
            std::string fen = argv[3];
            checkFileExists(modelFile);
            eval(modelFile, fen);
        } else if (cmd == "subset") {
            if (argc != 5)
                usage();
            std::string inFile = argv[2];
            S64 nPos;
            if (!str2Num(argv[3], nPos) || nPos <= 0)
                usage();
            std::string outFile = argv[4];
            checkFileExists(inFile);
            extractSubset(inFile, nPos, outFile);
        } else if (cmd == "bin2fen") {
            if (argc != 3)
                usage();
            std::string inFile = argv[2];
            checkFileExists(inFile);
            bin2Fen(inFile, std::cout);
        } else if (cmd == "getvalidation") {
            if (argc != 4)
                usage();
            std::string inFile = argv[2];
            std::string outFile = argv[3];
            checkFileExists(inFile);
            extractValidationData(inFile, outFile);
        } else if (cmd == "featstat") {
            if (argc != 3)
                usage();
            std::string inFile = argv[2];
            checkFileExists(inFile);
            featureStats(inFile);
        } else {
            usage();
        }
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}
