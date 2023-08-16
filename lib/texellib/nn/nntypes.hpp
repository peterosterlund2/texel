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
 * nntypes.hpp
 *
 *  Created on: Jul 23, 2022
 *      Author: petero
 */

#ifndef NNTYPES_HPP_
#define NNTYPES_HPP_

#include "util.hpp"
#include "random.hpp"
#include "binfile.hpp"
#include <limits>
#include <memory>

// ------------------------------------------------------------------------------

/** A matrix with size known at compile-time. */
template <typename T, int M, int N>
class Matrix {
public:
    const T& operator()(int i, int j) const { return data[i*N+j]; }
          T& operator()(int i, int j)       { return data[i*N+j]; }

    T data[M*N];

    U64 computeHash() const;
};

template <typename T, int M, int N>
U64
Matrix<T,M,N>::computeHash() const {
    U64 ret = hashU64(std::numeric_limits<T>::max());
    ret = hashU64(ret + M);
    ret = hashU64(ret + N);
    for (size_t i = 0; i < M*N; i++)
        ret = hashU64(ret + data[i]);
    return ret;
}

/** A vector with size known at compile-time. */
template <typename T, int N>
class Vector {
public:
    const T& operator()(int i) const { return data[i]; }
          T& operator()(int i)       { return data[i]; }

    T data[N];

    U64 computeHash() const;
};

template <typename T, int N>
U64
Vector<T,N>::computeHash() const {
    U64 ret = hashU64(std::numeric_limits<T>::max());
    ret = hashU64(ret + N);
    for (size_t i = 0; i < N; i++)
        ret = hashU64(ret + data[i]);
    return ret;
}

// ------------------------------------------------------------------------------

template <int nIn, int nOut>
class LayerData {
public:
    Matrix<S8,nOut,nIn> weight;
    Vector<S32,nOut> bias;

    void save(BinaryFileWriter& writer) const;
    void load(BinaryFileReader& reader);
    U64 computeHash() const;
    int computeSize() const;
};

template <int nIn, int nOut>
void
LayerData<nIn,nOut>::save(BinaryFileWriter& writer) const {
    writer.writeArray(&weight.data[0], COUNT_OF(weight.data));
    writer.writeArray(&bias.data[0], COUNT_OF(bias.data));
}

template <int nIn, int nOut>
void
LayerData<nIn,nOut>::load(BinaryFileReader& reader) {
    reader.readArray(&weight.data[0], COUNT_OF(weight.data));
    reader.readArray(&bias.data[0], COUNT_OF(bias.data));
}

template <int nIn, int nOut>
U64
LayerData<nIn,nOut>::computeHash() const {
    U64 ret = hashU64(1);
    ret = hashU64(ret + weight.computeHash());
    ret = hashU64(ret + bias.computeHash());
    return ret;
}

template <int nIn, int nOut>
int
LayerData<nIn,nOut>::computeSize() const {
    int s = 0;
    s += COUNT_OF(weight.data) * sizeof(weight.data[0]);
    s += COUNT_OF(bias.data) * sizeof(bias.data[0]);
    return s;
}

// ------------------------------------------------------------------------------

template <int nIn, int nOut>
class Layer {
public:
    Layer(const LayerData<nIn,nOut>& data) : data(data) {}

    struct Output {
        Vector<S32,nOut> linOutput;  // Result after applying weight and bias
        Vector<S8,nOut> output;      // Result after scaling, clipped ReLU and narrowing
    };

    /** Compute output from input. */
    void forward(const Vector<S8,nIn>& in, Output& out);
    /** Compute linOutput from input. */
    void evalLinear(const Vector<S8,nIn>& in, Output& out);

    const LayerData<nIn,nOut>& data;
};

// ------------------------------------------------------------------------------

/** Holds all neural network data required for position evaluation. */
class NetData {
public:
    /** Create an instance. This object is very large and needs special alignment,
     *  so allocating it on the stack is not supported. */
    static std::shared_ptr<NetData> create();

    static constexpr int inFeatures = 32 * 10 * 64;
    static constexpr int n1 = 256;
    static constexpr int n2 = 32;
    static constexpr int n3 = 32;

    Matrix<S16, inFeatures, n1> weight1;
    Vector<S16, n1> bias1;

    LayerData<n1*2, n2> lin2;
    LayerData<n2  , n3> lin3;
    LayerData<n3  , 1 > lin4;

    /** Serialize this object to "os". */
    void save(std::ostream& os) const;

    /** Deserialize this object from "is".  */
    void load(std::istream& is);

    /** Return a hash value corresponding to all data in this object. */
    U64 computeHash() const;

    /** Return the size of the serialized network data. */
    int computeSize() const;

private:
    NetData() = default;
    NetData(const NetData& other) = delete;
};


#endif /* NNTYPES_HPP_ */
