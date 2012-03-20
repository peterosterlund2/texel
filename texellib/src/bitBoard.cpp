/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * bitBoard.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "bitBoard.hpp"
#include "position.hpp"
#include <assert.h>
#include <iostream>

U64 BitBoard::kingAttacks[64];
U64 BitBoard::knightAttacks[64];
U64 BitBoard::wPawnAttacks[64];
U64 BitBoard::bPawnAttacks[64];
U64 BitBoard::wPawnBlockerMask[64];
U64 BitBoard::bPawnBlockerMask[64];

const U64 BitBoard::maskFile[8] = {
    0x0101010101010101ULL,
    0x0202020202020202ULL,
    0x0404040404040404ULL,
    0x0808080808080808ULL,
    0x1010101010101010ULL,
    0x2020202020202020ULL,
    0x4040404040404040ULL,
    0x8080808080808080ULL
};

U64 BitBoard::epMaskW[8];
U64 BitBoard::epMaskB[8];

U64* BitBoard::rTables[64];
U64 BitBoard::rMasks[64];
int BitBoard::rBits[64] = { 12, 11, 11, 11, 11, 11, 11, 12,
                            11, 10, 10, 10, 10, 10, 10, 11,
                            11, 10, 10, 10, 10, 10, 10, 11,
                            11, 10, 10, 10, 10, 10, 10, 11,
                            11, 10, 10, 10, 10, 10, 10, 11,
                            11, 10, 10, 10, 10, 10, 10, 11,
                            10,  9,  9,  9,  9,  9, 10, 10,
                            11, 10, 10, 10, 10, 11, 11, 11 };
const U64 BitBoard::rMagics[64] = {
    0x0080011084624000ULL, 0x1440031000200141ULL, 0x2080082004801000ULL, 0x0100040900100020ULL,
    0x0200020010200408ULL, 0x0300010008040002ULL, 0x040024081000a102ULL, 0x0080003100054680ULL,
    0x1100800040008024ULL, 0x8440401000200040ULL, 0x0432001022008044ULL, 0x0402002200100840ULL,
    0x4024808008000400ULL, 0x100a000410820008ULL, 0x8042001144020028ULL, 0x2451000041002082ULL,
    0x1080004000200056ULL, 0xd41010c020004000ULL, 0x0004410020001104ULL, 0x0000818050000800ULL,
    0x0000050008010010ULL, 0x0230808002000400ULL, 0x2000440090022108ULL, 0x0488020000811044ULL,
    0x8000410100208006ULL, 0x2000a00240100140ULL, 0x2088802200401600ULL, 0x0a10100180080082ULL,
    0x0000080100110004ULL, 0x0021002300080400ULL, 0x8400880400010230ULL, 0x2001008200004401ULL,
    0x0000400022800480ULL, 0x00200040e2401000ULL, 0x4004100084802000ULL, 0x0218800800801002ULL,
    0x0420800800800400ULL, 0x002a000402001008ULL, 0x0e0b000401008200ULL, 0x0815908072000401ULL,
    0x1840008002498021ULL, 0x1070122002424000ULL, 0x1040200100410010ULL, 0x0600080010008080ULL,
    0x0215001008010004ULL, 0x0000020004008080ULL, 0x1300021051040018ULL, 0x0004040040820001ULL,
    0x48fffe99fecfaa00ULL, 0x48fffe99fecfaa00ULL, 0x497fffadff9c2e00ULL, 0x613fffddffce9200ULL,
    0xffffffe9ffe7ce00ULL, 0xfffffff5fff3e600ULL, 0x2000080281100400ULL, 0x510ffff5f63c96a0ULL,
    0xebffffb9ff9fc526ULL, 0x61fffeddfeedaeaeULL, 0x53bfffedffdeb1a2ULL, 0x127fffb9ffdfb5f6ULL,
    0x411fffddffdbf4d6ULL, 0x0005000208040001ULL, 0x264038060100d004ULL, 0x7645fffecbfea79eULL,
};

U64* BitBoard::bTables[64];
U64 BitBoard::bMasks[64];
const int BitBoard::bBits[64] = { 5, 4, 5, 5, 5, 5, 4, 5,
                                  4, 4, 5, 5, 5, 5, 4, 4,
                                  4, 4, 7, 7, 7, 7, 4, 4,
                                  5, 5, 7, 9, 9, 7, 5, 5,
                                  5, 5, 7, 9, 9, 7, 5, 5,
                                  4, 4, 7, 7, 7, 7, 4, 4,
                                  4, 4, 5, 5, 5, 5, 4, 4,
                                  5, 4, 5, 5, 5, 5, 4, 5 };
const U64 BitBoard::bMagics[64] = {
    0xffedf9fd7cfcffffULL, 0xfc0962854a77f576ULL, 0x9010210041047000ULL, 0x52242420800c0000ULL,
    0x884404220480004aULL, 0x0002080248000802ULL, 0xfc0a66c64a7ef576ULL, 0x7ffdfdfcbd79ffffULL,
    0xfc0846a64a34fff6ULL, 0xfc087a874a3cf7f6ULL, 0x02000888010a2211ULL, 0x0040044040801808ULL,
    0x0880040420000000ULL, 0x0000084110109000ULL, 0xfc0864ae59b4ff76ULL, 0x3c0860af4b35ff76ULL,
    0x73c01af56cf4cffbULL, 0x41a01cfad64aaffcULL, 0x1010000200841104ULL, 0x802802142a006000ULL,
    0x0a02000412020020ULL, 0x0000800040504030ULL, 0x7c0c028f5b34ff76ULL, 0xfc0a028e5ab4df76ULL,
    0x0020082044905488ULL, 0xa572211102080220ULL, 0x0014020001280300ULL, 0x0220208058008042ULL,
    0x0001010000104016ULL, 0x0005114028080800ULL, 0x0202640000848800ULL, 0x040040900a008421ULL,
    0x400e094000600208ULL, 0x800a100400120890ULL, 0x0041229001480020ULL, 0x0000020080880082ULL,
    0x0040002020060080ULL, 0x1819100100c02400ULL, 0x04112a4082c40400ULL, 0x0001240130210500ULL,
    0xdcefd9b54bfcc09fULL, 0xf95ffa765afd602bULL, 0x008200222800a410ULL, 0x0100020102406400ULL,
    0x80a8040094000200ULL, 0x002002006200a041ULL, 0x43ff9a5cf4ca0c01ULL, 0x4bffcd8e7c587601ULL,
    0xfc0ff2865334f576ULL, 0xfc0bf6ce5924f576ULL, 0x0900420442088104ULL, 0x0062042084040010ULL,
    0x01380810220a0240ULL, 0x0000101002082800ULL, 0xc3ffb7dc36ca8c89ULL, 0xc3ff8a54f4ca2c89ULL,
    0xfffffcfcfd79edffULL, 0xfc0863fccb147576ULL, 0x0050009040441000ULL, 0x00139a0000840400ULL,
    0x9080000412220a00ULL, 0x0000002020010a42ULL, 0xfc087e8e4bb2f736ULL, 0x43ff9e4ef4ca2c89ULL,
};


const byte BitBoard::dirTable[] = {
        -9,   0,   0,   0,   0,   0,   0,  -8,   0,   0,   0,   0,   0,   0,  -7,
    0,   0,  -9,   0,   0,   0,   0,   0,  -8,   0,   0,   0,   0,   0,  -7,   0,
    0,   0,   0,  -9,   0,   0,   0,   0,  -8,   0,   0,   0,   0,  -7,   0,   0,
    0,   0,   0,   0,  -9,   0,   0,   0,  -8,   0,   0,   0,  -7,   0,   0,   0,
    0,   0,   0,   0,   0,  -9,   0,   0,  -8,   0,   0,  -7,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,  -9, -17,  -8, -15,  -7,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0, -10,  -9,  -8,  -7,  -6,   0,   0,   0,   0,   0,
    0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   1,   1,   1,   1,   1,   1,   1,
    0,   0,   0,   0,   0,   0,   6,   7,   8,   9,  10,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   7,  15,   8,  17,   9,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   7,   0,   0,   8,   0,   0,   9,   0,   0,   0,   0,
    0,   0,   0,   0,   7,   0,   0,   0,   8,   0,   0,   0,   9,   0,   0,   0,
    0,   0,   0,   7,   0,   0,   0,   0,   8,   0,   0,   0,   0,   9,   0,   0,
    0,   0,   7,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   9,   0,
    0,   7,   0,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   0,   9
};

const byte BitBoard::distTable[] = {
       7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    0, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
    0, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7,
    0, 7, 6, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 3, 2, 2, 2, 2, 2, 3, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 3, 2, 1, 1, 1, 2, 3, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 3, 2, 1, 1, 1, 2, 3, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 3, 2, 2, 2, 2, 2, 3, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7,
    0, 7, 6, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 6, 7,
    0, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7,
    0, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
    0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

const int BitBoard::trailingZ[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5
};

U64 BitBoard::squaresBetween[64][64];

static U64 createPattern(int i, U64 mask) {
    U64 ret = 0ULL;
    for (int j = 0; ; j++) {
        U64 nextMask = mask & (mask - 1);
        U64 bit = mask ^ nextMask;
        if ((i & (1ULL << j)) != 0)
            ret |= bit;
        mask = nextMask;
        if (mask == 0)
            break;
    }
    return ret;
}

static U64 addRay(U64 mask, int x, int y, int dx, int dy,
                  U64 occupied, bool inner) {
    int lo = inner ? 1 : 0;
    int hi = inner ? 6 : 7;
    while (true) {
        if (dx != 0) {
            x += dx; if ((x < lo) || (x > hi)) break;
        }
        if (dy != 0) {
            y += dy; if ((y < lo) || (y > hi)) break;
        }
        int sq = Position::getSquare(x, y);
        mask |= 1ULL << sq;
        if ((occupied & (1ULL << sq)) != 0)
            break;
    }
    return mask;
}

static U64 addRookRays(int x, int y, U64 occupied, bool inner) {
    U64 mask = 0;
    mask = addRay(mask, x, y,  1,  0, occupied, inner);
    mask = addRay(mask, x, y, -1,  0, occupied, inner);
    mask = addRay(mask, x, y,  0,  1, occupied, inner);
    mask = addRay(mask, x, y,  0, -1, occupied, inner);
    return mask;
}

static U64 addBishopRays(int x, int y, U64 occupied, bool inner) {
    U64 mask = 0;
    mask = addRay(mask, x, y,  1,  1, occupied, inner);
    mask = addRay(mask, x, y, -1, -1, occupied, inner);
    mask = addRay(mask, x, y,  1, -1, occupied, inner);
    mask = addRay(mask, x, y, -1,  1, occupied, inner);
    return mask;
}

static StaticInitializer<BitBoard> bbInit;

void
BitBoard::staticInitialize() {

    for (int f = 0; f < 8; f++) {
        U64 m = 0;
        if (f > 0) m |= 1ULL << Position::getSquare(f-1, 3);
        if (f < 7) m |= 1ULL << Position::getSquare(f+1, 3);
        epMaskW[f] = m;

        m = 0;
        if (f > 0) m |= 1ULL << Position::getSquare(f-1, 4);
        if (f < 7) m |= 1ULL << Position::getSquare(f+1, 4);
        epMaskB[f] = m;
    }

    // Compute king attacks
    for (int sq = 0; sq < 64; sq++) {
        U64 m = 1ULL << sq;
        U64 mask = (((m >> 1) | (m << 7) | (m >> 9)) & maskAToGFiles) |
                   (((m << 1) | (m << 9) | (m >> 7)) & maskBToHFiles) |
                    (m << 8) | (m >> 8);
        kingAttacks[sq] = mask;
    }

    // Compute knight attacks
    for (int sq = 0; sq < 64; sq++) {
        U64 m = 1ULL << sq;
        U64 mask = (((m <<  6) | (m >> 10)) & maskAToFFiles) |
                   (((m << 15) | (m >> 17)) & maskAToGFiles) |
                   (((m << 17) | (m >> 15)) & maskBToHFiles) |
                   (((m << 10) | (m >>  6)) & maskCToHFiles);
        knightAttacks[sq] = mask;
    }

    // Compute pawn attacks
    for (int sq = 0; sq < 64; sq++) {
        U64 m = 1ULL << sq;
        U64 mask = ((m << 7) & maskAToGFiles) | ((m << 9) & maskBToHFiles);
        wPawnAttacks[sq] = mask;
        mask = ((m >> 9) & maskAToGFiles) | ((m >> 7) & maskBToHFiles);
        bPawnAttacks[sq] = mask;

        int x = Position::getX(sq);
        int y = Position::getY(sq);
        m = 0;
        for (int y2 = y+1; y2 < 8; y2++) {
            if (x > 0) m |= 1ULL << Position::getSquare(x-1, y2);
                       m |= 1ULL << Position::getSquare(x  , y2);
            if (x < 7) m |= 1ULL << Position::getSquare(x+1, y2);
        }
        wPawnBlockerMask[sq] = m;
        m = 0;
        for (int y2 = y-1; y2 >= 0; y2--) {
            if (x > 0) m |= 1ULL << Position::getSquare(x-1, y2);
                       m |= 1ULL << Position::getSquare(x  , y2);
            if (x < 7) m |= 1ULL << Position::getSquare(x+1, y2);
        }
        bPawnBlockerMask[sq] = m;
    }

    // Rook magics
    for (int sq = 0; sq < 64; sq++) {
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        rMasks[sq] = addRookRays(x, y, 0ULL, true);
        int tableSize = 1 << rBits[sq];
        U64* table = new U64[tableSize];
        const U64 unInit = 0xffffffffffffffffULL;
        for (int i = 0; i < tableSize; i++) table[i] = unInit;
        int nPatterns = 1 << BitBoard::bitCount(rMasks[sq]);
        for (int i = 0; i < nPatterns; i++) {
            U64 p = createPattern(i, rMasks[sq]);
            int entry = (int)((p * rMagics[sq]) >> (64 - rBits[sq]));
            U64 atks = addRookRays(x, y, p, false);
            if (table[entry] == unInit) {
                table[entry] = atks;
            } else {
                assert(table[entry] == atks);
            }
        }
        rTables[sq] = table;
    }

    // Bishop magics
    for (int sq = 0; sq < 64; sq++) {
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        bMasks[sq] = addBishopRays(x, y, 0ULL, true);
        int tableSize = 1 << bBits[sq];
        U64* table = new U64[tableSize];
        const U64 unInit = 0xffffffffffffffffULL;
        for (int i = 0; i < tableSize; i++) table[i] = unInit;
        int nPatterns = 1 << BitBoard::bitCount(bMasks[sq]);
        for (int i = 0; i < nPatterns; i++) {
            U64 p = createPattern(i, bMasks[sq]);
            int entry = (int)((p * bMagics[sq]) >> (64 - bBits[sq]));
            U64 atks = addBishopRays(x, y, p, false);
            if (table[entry] == unInit) {
                table[entry] = atks;
            } else {
                assert(table[entry] == atks);
            }
        }
        bTables[sq] = table;
    }

    // squaresBetween
    for (int sq1 = 0; sq1 < 64; sq1++) {
        for (int j = 0; j < 64; j++)
            squaresBetween[sq1][j] = 0;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if ((dx == 0) && (dy == 0))
                    continue;
                U64 m = 0;
                int x = Position::getX(sq1);
                int y = Position::getY(sq1);
                while (true) {
                    x += dx; y += dy;
                    if ((x < 0) || (x > 7) || (y < 0) || (y > 7))
                        break;
                    int sq2 = Position::getSquare(x, y);
                    squaresBetween[sq1][sq2] = m;
                    m |= 1ULL << sq2;
                }
            }
        }
    }
}
