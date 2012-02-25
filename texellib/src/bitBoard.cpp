/*
 * bitBoard.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "bitBoard.hpp"
#include "position.hpp"


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

#if 0
    static { // Rook magics
        rTables = new U64[64][];
        for (int sq = 0; sq < 64; sq++) {
            int x = Position::getX(sq);
            int y = Position::getY(sq);
            rMasks[sq] = addRookRays(x, y, 0ULL, true);
            int tableSize = 1 << rBits[sq];
            U64[] table = new U64[tableSize];
            for (int i = 0; i < tableSize; i++) table[i] = -1;
            int nPatterns = 1 << Long.bitCount(rMasks[sq]);
            for (int i = 0; i < nPatterns; i++) {
                U64 p = createPattern(i, rMasks[sq]);
                int entry = (int)((p * rMagics[sq]) >> (64 - rBits[sq]));
                U64 atks = addRookRays(x, y, p, false);
                if (table[entry] == -1) {
                    table[entry] = atks;
                } else if (table[entry] != atks) {
                    throw new RuntimeException();
                }
            }
            rTables[sq] = table;
        }
    }

    static { // Bishop magics
        bTables = new U64[64][];
        bMasks = new U64[64];
        for (int sq = 0; sq < 64; sq++) {
            int x = Position::getX(sq);
            int y = Position::getY(sq);
            bMasks[sq] = addBishopRays(x, y, 0ULL, true);
            int tableSize = 1 << bBits[sq];
            U64[] table = new U64[tableSize];
            for (int i = 0; i < tableSize; i++) table[i] = -1;
            int nPatterns = 1 << Long.bitCount(bMasks[sq]);
            for (int i = 0; i < nPatterns; i++) {
                U64 p = createPattern(i, bMasks[sq]);
                int entry = (int)((p * bMagics[sq]) >> (64 - bBits[sq]));
                U64 atks = addBishopRays(x, y, p, false);
                if (table[entry] == -1) {
                    table[entry] = atks;
                } else if (table[entry] != atks) {
                    throw new RuntimeException();
                }
            }
            bTables[sq] = table;
        }
    }
#endif
