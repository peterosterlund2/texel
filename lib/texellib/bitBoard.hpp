/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * bitBoard.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BITBOARD_HPP_
#define BITBOARD_HPP_

#include "util.hpp"
#include "alignedAlloc.hpp"
#include "square.hpp"

#include <cstdlib>

#if _MSC_VER
#ifdef USE_CTZ
#include <intrin.h>
#endif
#ifdef USE_POPCNT
#include <nmmintrin.h>
#endif
#endif


#ifdef USE_BMI2
#include <immintrin.h>
inline U64 pext(U64 value, U64 mask) {
    return _pext_u64(value, mask);
}
#endif

class BitUtil {
public:
    /** Get the lowest 1 bit from mask. mask must be non-zero. */
    static int firstBit(U64 mask);

    /** Get the lowest 1 bit from mask and remove the corresponding bit in mask. */
    static int extractBit(U64& mask);

    /** Get the highest 1 bit from mask. mask must be non-zero. */
    static int lastBit(U64 mask);

    /** Return number of 1 bits in mask. */
    static int bitCount(U64 mask);

private:
    static const int trailingZ[64], lastBitTable[64];
};

class BitBoard {
public:
    static const U64 maskFileA = 0x0101010101010101ULL;
    static const U64 maskFileB = 0x0202020202020202ULL;
    static const U64 maskFileC = 0x0404040404040404ULL;
    static const U64 maskFileD = 0x0808080808080808ULL;
    static const U64 maskFileE = 0x1010101010101010ULL;
    static const U64 maskFileF = 0x2020202020202020ULL;
    static const U64 maskFileG = 0x4040404040404040ULL;
    static const U64 maskFileH = 0x8080808080808080ULL;

    static const U64 maskAToGFiles = 0x7F7F7F7F7F7F7F7FULL;
    static const U64 maskBToHFiles = 0xFEFEFEFEFEFEFEFEULL;
    static const U64 maskAToFFiles = 0x3F3F3F3F3F3F3F3FULL;
    static const U64 maskCToHFiles = 0xFCFCFCFCFCFCFCFCULL;

    static const U64 maskAToDFiles = 0x0F0F0F0F0F0F0F0FULL;
    static const U64 maskEToHFiles = 0xF0F0F0F0F0F0F0F0ULL;

    static const U64 maskFile[8];

    // Masks for squares where enemy pawn can capture en passant, indexed by file
    static U64 epMaskW[8], epMaskB[8];

    static const U64 maskRow1      = 0x00000000000000FFULL;
    static const U64 maskRow2      = 0x000000000000FF00ULL;
    static const U64 maskRow3      = 0x0000000000FF0000ULL;
    static const U64 maskRow4      = 0x00000000FF000000ULL;
    static const U64 maskRow5      = 0x000000FF00000000ULL;
    static const U64 maskRow6      = 0x0000FF0000000000ULL;
    static const U64 maskRow7      = 0x00FF000000000000ULL;
    static const U64 maskRow8      = 0xFF00000000000000ULL;
    static const U64 maskRow1Row8  = 0xFF000000000000FFULL;

    static const U64 maskDarkSq    = 0xAA55AA55AA55AA55ULL;
    static const U64 maskLightSq   = 0x55AA55AA55AA55AAULL;

    static const U64 maskCorners   = 0x8100000000000081ULL;

    /** Convert one or more squares to a bitmask. */
    static U64 sqMask(SquareName sq) { return 1ULL << sq; }
    template <typename Sq0, typename... Squares> static U64 sqMask(Sq0 sq0, Squares... squares) {
        return sqMask(sq0) | sqMask(squares...);
    }

    /** Mirror a bitmask in the X or Y direction. */
    static U64 mirrorX(U64 mask);
    static U64 mirrorY(U64 mask);


    static U64 bishopAttacks(Square sq, U64 occupied);
    static U64 rookAttacks(Square sq, U64 occupied);

    /** Shift mask in the NW and NE directions. */
    static U64 wPawnAttacksMask(U64 mask);
    /** Shift mask in the SW and SE directions. */
    static U64 bPawnAttacksMask(U64 mask);

    static U64 kingAttacks(Square sq) { return kingAttacksTable[sq]; }
    static U64 knightAttacks(Square sq) { return knightAttacksTable[sq]; }
    static U64 wPawnAttacks(Square sq) { return wPawnAttacksTable[sq]; }
    static U64 bPawnAttacks(Square sq) { return bPawnAttacksTable[sq]; }
    static U64 wPawnBlockerMask(Square sq) { return wPawnBlockerMaskTable[sq]; }
    static U64 bPawnBlockerMask(Square sq) { return bPawnBlockerMaskTable[sq]; }
    static U64 squaresBetween(Square s1, Square s2) { return squaresBetweenTable[s1][s2]; }

    /** Get direction between two squares, 8*sign(dy) + sign(dx) */
    static int getDirection(Square from, Square to);

    /** Get the max norm distance between two squares. */
    static int getKingDistance(Square from, Square to);

    /** Get the L1 norm distance between two squares. */
    static int getTaxiDistance(Square from, Square to);

    static U64 southFill(U64 mask);

    static U64 northFill(U64 mask);

    /** Get the lowest square from mask. mask must be non-zero. */
    static Square firstSquare(U64 mask);

    /** Get the highest square from mask. mask must be non-zero. */
    static Square lastSquare(U64 mask);

    /** Get the lowest square from mask and remove the corresponding bit in mask. */
    static Square extractSquare(U64& mask);

    /** Return number of 1 bits in mask. */
    static int bitCount(U64 mask);

    /** Initialize static data. */
    static void staticInitialize();

private:
    /** Squares attacked by a king on a given square. */
    static SqTbl<U64> kingAttacksTable;
    static SqTbl<U64> knightAttacksTable;
    static SqTbl<U64> wPawnAttacksTable;
    static SqTbl<U64> bPawnAttacksTable;

    // Squares preventing a pawn from being a passed pawn, if occupied by enemy pawn
    static SqTbl<U64> wPawnBlockerMaskTable;
    static SqTbl<U64> bPawnBlockerMaskTable;

    static SqTbl<SqTbl<U64>> squaresBetweenTable;

    static SqTbl<U64*> rTables;
    static SqTbl<U64> rMasks;
    static const SqTbl<int> rBits;
    static const SqTbl<U64> rMagics;

    static SqTbl<U64*> bTables;
    static SqTbl<U64> bMasks;
    static const SqTbl<int> bBits;
    static const SqTbl<U64> bMagics;

    static vector_aligned<U64> tableData;

    static const S8 dirTable[];
};

inline U64 operator<<(U64 b, Square s) {
    return b << s.asInt();
}


inline int
BitUtil::firstBit(U64 mask) {
#ifdef USE_CTZ
#if _MSC_VER
    unsigned long ret;
    _BitScanForward64(&ret, mask);
    return (int)ret;
#else
    if (sizeof(U64) == sizeof(long))
        return __builtin_ctzl(mask);
    else if (sizeof(U64) == sizeof(long long))
        return __builtin_ctzll(mask);
    else
        std::abort();
#endif
#else
    return trailingZ[(int)(((mask & -mask) * 0x07EDD5E59A4E28C2ULL) >> 58)];
#endif
}

inline int
BitUtil::lastBit(U64 mask) {
#ifdef USE_CTZ
#if _MSC_VER
    unsigned long ret;
    _BitScanReverse64(&ret, mask);
    return (int)ret;
#else
    if (sizeof(U64) == sizeof(long))
        return 63 - __builtin_clzl(mask);
    else if (sizeof(U64) == sizeof(long long))
        return 63 - __builtin_clzll(mask);
    else
        std::abort();
#endif
#else
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;
    mask |= mask >> 16;
    mask |= mask >> 32;
    return lastBitTable[(mask * 0x03F79D71B4CB0A89ULL) >> 58];
#endif
}

inline int
BitUtil::extractBit(U64& mask) {
    int ret = firstBit(mask);
    mask &= mask - 1;
    return ret;
}

inline int
BitUtil::bitCount(U64 mask) {
#ifdef USE_POPCNT
#if _MSC_VER
    return _mm_popcnt_u64(mask);
#else
    if (sizeof(U64) == sizeof(long))
        return __builtin_popcountl(mask);
    else if (sizeof(U64) == 2*sizeof(long))
        return __builtin_popcountl(mask >> 32) +
               __builtin_popcountl(mask & 0xffffffffULL);
    else
        std::abort();
#endif
#else
    const U64 k1 = 0x5555555555555555ULL;
    const U64 k2 = 0x3333333333333333ULL;
    const U64 k4 = 0x0f0f0f0f0f0f0f0fULL;
    const U64 kf = 0x0101010101010101ULL;
    U64 t = mask;
    t -= (t >> 1) & k1;
    t = (t & k2) + ((t >> 2) & k2);
    t = (t + (t >> 4)) & k4;
    t = (t * kf) >> 56;
    return (int)t;
#endif
}

inline U64
BitBoard::mirrorX(U64 mask) {
    U64 k1 = 0x5555555555555555ULL;
    U64 k2 = 0x3333333333333333ULL;
    U64 k3 = 0x0f0f0f0f0f0f0f0fULL;
    U64 t = mask;
    t = ((t >> 1) & k1) | ((t & k1) << 1);
    t = ((t >> 2) & k2) | ((t & k2) << 2);
    t = ((t >> 4) & k3) | ((t & k3) << 4);
    return t;
}

inline U64
BitBoard::mirrorY(U64 mask) {
    U64 k1 = 0x00ff00ff00ff00ffULL;
    U64 k2 = 0x0000ffff0000ffffULL;
    U64 t = mask;
    t = ((t >>  8) & k1) | ((t & k1) <<  8);
    t = ((t >> 16) & k2) | ((t & k2) << 16);
    t = ((t >> 32)     ) | ((t     ) << 32);
    return t;
}

inline U64
BitBoard::bishopAttacks(Square sq, U64 occupied) {
#ifdef USE_BMI2
    return bTables[sq][pext(occupied, bMasks[sq])];
#else
    return bTables[sq][(int)(((occupied & bMasks[sq]) * bMagics[sq]) >> bBits[sq])];
#endif
}

inline U64
BitBoard::rookAttacks(Square sq, U64 occupied) {
#ifdef USE_BMI2
    return rTables[sq][pext(occupied, rMasks[sq])];
#else
    return rTables[sq][(int)(((occupied & rMasks[sq]) * rMagics[sq]) >> rBits[sq])];
#endif
}

inline U64
BitBoard::wPawnAttacksMask(U64 mask) {
    return ((mask & maskBToHFiles) << 7) |
           ((mask & maskAToGFiles) << 9);
}

inline U64
BitBoard::bPawnAttacksMask(U64 mask) {
    return ((mask & maskBToHFiles) >> 9) |
           ((mask & maskAToGFiles) >> 7);
}

inline int
BitBoard::getDirection(Square fromS, Square toS) {
    int from = fromS.asInt();
    int to = toS.asInt();
    int offs = to + (to|7) - from - (from|7) + 0x77;
    return dirTable[offs];
}

inline int
BitBoard::getKingDistance(Square from, Square to) {
    int dx = to.getX() - from.getX();
    int dy = to.getY() - from.getY();
    return std::max(std::abs(dx), std::abs(dy));
}

inline int
BitBoard::getTaxiDistance(Square from, Square to) {
    int dx = to.getX() - from.getX();
    int dy = to.getY() - from.getY();
    return std::abs(dx) + std::abs(dy);
}

inline U64
BitBoard::southFill(U64 mask) {
    mask |= (mask >> 8);
    mask |= (mask >> 16);
    mask |= (mask >> 32);
    return mask;
}

inline U64
BitBoard::northFill(U64 mask) {
    mask |= (mask << 8);
    mask |= (mask << 16);
    mask |= (mask << 32);
    return mask;
}

inline Square
BitBoard::firstSquare(U64 mask) {
    return Square(BitUtil::firstBit(mask));
}

inline Square
BitBoard::lastSquare(U64 mask) {
    return Square(BitUtil::lastBit(mask));
}

inline Square
BitBoard::extractSquare(U64& mask) {
    return Square(BitUtil::extractBit(mask));
}

inline int
BitBoard::bitCount(U64 mask) {
    return BitUtil::bitCount(mask);
}

#endif /* BITBOARD_HPP_ */
