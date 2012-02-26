/*
 * position.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "position.hpp"
#include "textio.hpp"
#include <iostream>

U64 Position::psHashKeys[Piece::nPieceTypes][64];
U64 Position::whiteHashKey;
U64 Position::castleHashKeys[16];
U64 Position::epHashKeys[9];
U64 Position::moveCntKeys[101];


static StaticInitializer<Position> posInit;

void
Position::staticInitialize() {
    int rndNo = 0;
    for (int p = 0; p < Piece::nPieceTypes; p++)
        for (int sq = 0; sq < 64; sq++)
            psHashKeys[p][sq] = getRandomHashVal(rndNo++);
    whiteHashKey = getRandomHashVal(rndNo++);
    for (size_t cm = 0; cm < COUNT_OF(castleHashKeys); cm++)
        castleHashKeys[cm] = getRandomHashVal(rndNo++);
    for (size_t f = 0; f < COUNT_OF(epHashKeys); f++)
        epHashKeys[f] = getRandomHashVal(rndNo++);
    for (size_t mc = 0; mc < COUNT_OF(moveCntKeys); mc++)
        moveCntKeys[mc] = getRandomHashVal(rndNo++);
}

std::ostream&
operator<<(std::ostream& os, const Position& pos) {
    std::stringstream ss;
    ss << std::hex << pos.zobristHash();
    os << TextIO::asciiBoard(pos) << (pos.whiteMove ? "white\n" : "black\n") << ss.str();
    return os;
}

U64
Position::computeZobristHash() {
    U64 hash = 0;
    for (int sq = 0; sq < 64; sq++) {
        int p = squares[sq];
        hash ^= psHashKeys[p][sq];
        if ((p == Piece::WPAWN) || (p == Piece::BPAWN))
            pHashKey ^= psHashKeys[p][sq];
    }
    if (whiteMove)
        hash ^= whiteHashKey;
    hash ^= castleHashKeys[castleMask];
    hash ^= epHashKeys[(epSquare >= 0) ? getX(epSquare) + 1 : 0];
    return hash;
}

U64
Position::getRandomHashVal(int rndNo) {
    return rndNo * 12345321971238532197ULL;
#if 0
        MessageDigest md = MessageDigest.getInstance("SHA-1");
        byte[] input = new byte[4];
        for (int i = 0; i < 4; i++)
            input[i] = (byte)((rndNo >> (i * 8)) & 0xff);
        byte[] digest = md.digest(input);
        U64 ret = 0;
        for (int i = 0; i < 8; i++) {
            ret ^= ((U64)digest[i]) << (i * 8);
        }
        return ret;
#endif
}
