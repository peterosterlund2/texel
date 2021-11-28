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
 * bitSet.hpp
 *
 *  Created on: Dec 6, 2021
 *      Author: petero
 */

#ifndef BITSET_HPP_
#define BITSET_HPP_

#include <util/util.hpp>
#include "bitBoard.hpp"


/** A set of N bits where N is a compile time constant.
 *  Valid bit numbers are offs <= b < offs + N. */
template <int N, int offs = 0>
class BitSet {
private:
    static_assert(N % 64 == 0, "N must be a multiple of 64");
    constexpr static int nWords = (N + 63) / 64;

public:
    constexpr static int numBits = N;
    constexpr static int minAllowed = offs;

    BitSet() {}
    BitSet(const BitSet& b) {
        for (int i = 0; i < nWords; i++)
            data[i] = b.data[i];
    }
    BitSet& operator=(const BitSet& b) {
        for (int i = 0; i < nWords; i++)
            data[i] = b.data[i];
        return *this;
    }

    bool operator==(const BitSet& b) const {
        for (int i = 0; i < nWords; i++)
            if (data[i] != b.data[i])
                return false;
        return true;
    }
    bool operator!=(const BitSet& b) const { return !(*this == b); }

    /** Get/set single bit operations. */
    void setBit(int i) {
        i -= offs;
        data[i >> 6] |= 1ULL << (i & 63);
    }
    void clearBit(int i) {
        i -= offs;
        data[i >> 6] &= ~(1ULL << (i & 63));
    }
    bool getBit(int i) const {
        i -= offs;
        return (data[i >> 6] & (1ULL << (i & 63))) != 0;
    }

    /** Return true if all bits are 0. */
    bool empty() const {
        for (int i = 0; i < nWords; i++)
            if (data[i] != 0)
                return false;
        return true;
    }

    /** Set all bits minVal <= b <= maxVal to 1, all other bits to 0. */
    void setRange(int minVal, int maxVal) {
        for (int i = 0; i < nWords; i++)
            data[i] = ~0ULL;
        removeSmaller(minVal);
        removeLarger(maxVal);
    }

    /** Set all odd bits to 0. */
    void removeOdd() {
        U64 ptrn = 0x5555555555555555ULL;
        if (offs % 2)
            ptrn <<= 1;
        for (int i = 0; i < nWords; i++)
            data[i] &= ptrn;
    }
    /** Set all even bits to 0. */
    void removeEven() {
        U64 ptrn = 0x5555555555555555ULL;
        if (offs % 2 == 0)
            ptrn <<= 1;
        for (int i = 0; i < nWords; i++)
            data[i] &= ptrn;
    }
    /** Set all bits < minVal to 0. */
    void removeSmaller(int minVal) {
        minVal -= offs;
        if (minVal > 0) {
            int w = minVal >> 6;
            data[w] &= ~((1ULL << (minVal&63)) - 1);
            while (--w >= 0)
                data[w] = 0;
        }
    }
    /** Set all bits > maxVal to 0. */
    void removeLarger(int maxVal) {
        maxVal -= offs;
        maxVal++;
        if (maxVal < numBits) {
            int w = maxVal >> 6;
            data[w] &= (1ULL << (maxVal&63)) - 1;
            while (++w < nWords)
                data[w] = 0;
        }
    }


    BitSet& operator|=(const BitSet& b) {
        for (int i = 0 ; i < nWords; i++)
            data[i] |= b.data[i];
        return *this;
    }

    BitSet& operator&=(const BitSet& b) {
        for (int i = 0; i < nWords; i++)
            data[i] &= b.data[i];
        return *this;
    }

    int getMinBit() const {
        for (int i = 0; i < nWords; i++)
            if (data[i] != 0)
                return i * 64 + BitBoard::firstSquare(data[i]) + offs;
        return -1;
    }

    int getMaxBit() const {
        for (int i = nWords - 1; i >= 0; i--)
            if (data[i] != 0)
                return i * 64 + BitBoard::lastSquare(data[i]) + offs;
        return -1;
    }

    int bitCount() const {
        int cnt = 0;
        for (int i = 0; i < nWords; i++)
            cnt += BitBoard::bitCount(data[i]);
        return cnt;
    }

private:
    U64 data[nWords] = { 0 };
};


#endif /* BITSET_HPP_ */
