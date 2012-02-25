/*
 * bitBoard.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BITBOARD_HPP_
#define BITBOARD_HPP_

#include <inttypes.h>
#include "types.hpp"

class BitBoard {
public:
    /** Squares attacked by a king on a given square. */
    static U64 kingAttacks[64];
    static U64 knightAttacks[64];
    static U64 wPawnAttacks[64], bPawnAttacks[64];

    // Squares preventing a pawn from being a passed pawn, if occupied by enemy pawn
    static U64 wPawnBlockerMask[64], bPawnBlockerMask[64];

    static const U64 maskAToGFiles = 0x7F7F7F7F7F7F7F7FULL;
    static const U64 maskBToHFiles = 0xFEFEFEFEFEFEFEFEULL;
    static const U64 maskAToFFiles = 0x3F3F3F3F3F3F3F3FULL;
    static const U64 maskCToHFiles = 0xFCFCFCFCFCFCFCFCULL;

    static const U64 maskFile[8];

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

#if 0
    static {
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
    }

    static U64 bishopAttacks(int sq, U64 occupied) {
        return bTables[sq][(int)(((occupied & bMasks[sq]) * bMagics[sq]) >> (64 - bBits[sq]))];
    }

    static U64 rookAttacks(int sq, U64 occupied) {
        return rTables[sq][(int)(((occupied & rMasks[sq]) * rMagics[sq]) >> (64 - rBits[sq]))];
    }

    static U64 squaresBetween[64][64];
    static {
        squaresBetween = new U64[64][];
        for (int sq1 = 0; sq1 < 64; sq1++) {
            squaresBetween[sq1] = new U64[64];
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
#endif

    static int getDirection(int from, int to) {
        int offs = to + (to|7) - from - (from|7) + 0x77;
        return dirTable[offs];
    }

    static U64 southFill(U64 mask) {
        mask |= (mask >> 8);
        mask |= (mask >> 16);
        mask |= (mask >> 32);
        return mask;
    }

    static U64 northFill(U64 mask) {
        mask |= (mask << 8);
        mask |= (mask << 16);
        mask |= (mask << 32);
        return mask;
    }

    static int numberOfTrailingZeros(U64 mask) {
        return trailingZ[(int)(((mask & -mask) * 0x07EDD5E59A4E28C2ULL) >> 58)];
    }


private:
#if 0
    static U64[][] rTables;
    static U64 rMasks[64];
    static int rBits[64] = { 12, 11, 11, 11, 11, 11, 11, 12,
                             11, 10, 10, 10, 10, 10, 10, 11,
                             11, 10, 10, 10, 10, 10, 10, 11,
                             11, 10, 10, 10, 10, 10, 10, 11,
                             11, 10, 10, 10, 10, 10, 10, 11,
                             11, 10, 10, 10, 10, 10, 10, 11,
                             10,  9,  9,  9,  9,  9, 10, 10,
                             11, 10, 10, 10, 10, 11, 11, 11 };
    static const U64 rMagics[64] = {
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
    static U64[][] bTables;
    static U64[] bMasks;
    static const int[] bBits = { 5, 4, 5, 5, 5, 5, 4, 5,
                                 4, 4, 5, 5, 5, 5, 4, 4,
                                 4, 4, 7, 7, 7, 7, 4, 4,
                                 5, 5, 7, 9, 9, 7, 5, 5,
                                 5, 5, 7, 9, 9, 7, 5, 5,
                                 4, 4, 7, 7, 7, 7, 4, 4,
                                 4, 4, 5, 5, 5, 5, 4, 4,
                                 5, 4, 5, 5, 5, 5, 4, 5 };
    static const U64 bMagics[64] = {
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
#endif

    static const byte dirTable[];

    static const int trailingZ[64];
};


#endif /* BITBOARD_HPP_ */
