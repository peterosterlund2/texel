/*
 * position.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "position.hpp"
#include "evaluate.hpp"
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


Position::Position() {
    for (int i = 0; i < 64; i++)
        squares[i] = Piece::EMPTY;
    for (int i = 0; i < Piece::nPieceTypes; i++) {
        psScore1[i] = 0;
        psScore2[i] = 0;
        pieceTypeBB[i] = 0;
    }
    whiteBB = blackBB = 0;
    whiteMove = true;
    castleMask = 0;
    epSquare = -1;
    halfMoveClock = 0;
    fullMoveCounter = 1;
    hashKey = computeZobristHash();
    pHashKey = 0;
    wKingSq = bKingSq = -1;
    wMtrl = bMtrl = -Evaluate::kV;
    wMtrlPawns = bMtrlPawns = 0;
}

void
Position::setPiece(int square, int piece) {
    int removedPiece = squares[square];
    squares[square] = piece;

    // Update hash key
    hashKey ^= psHashKeys[removedPiece][square];
    hashKey ^= psHashKeys[piece][square];

    // Update bitboards
    const U64 sqMask = 1ULL << square;
    pieceTypeBB[removedPiece] &= ~sqMask;
    pieceTypeBB[piece] |= sqMask;

    if (removedPiece != Piece::EMPTY) {
        int pVal = Evaluate::pieceValue[removedPiece];
        if (Piece::isWhite(removedPiece)) {
            wMtrl -= pVal;
            whiteBB &= ~sqMask;
            if (removedPiece == Piece::WPAWN) {
                wMtrlPawns -= pVal;
                pHashKey ^= psHashKeys[Piece::WPAWN][square];
            }
        } else {
            bMtrl -= pVal;
            blackBB &= ~sqMask;
            if (removedPiece == Piece::BPAWN) {
                bMtrlPawns -= pVal;
                pHashKey ^= psHashKeys[Piece::BPAWN][square];
            }
        }
    }

    if (piece != Piece::EMPTY) {
        int pVal = Evaluate::pieceValue[piece];
        if (Piece::isWhite(piece)) {
            wMtrl += pVal;
            whiteBB |= sqMask;
            if (piece == Piece::WPAWN) {
                wMtrlPawns += pVal;
                pHashKey ^= psHashKeys[Piece::WPAWN][square];
            }
            if (piece == Piece::WKING)
                wKingSq = square;
        } else {
            bMtrl += pVal;
            blackBB |= sqMask;
            if (piece == Piece::BPAWN) {
                bMtrlPawns += pVal;
                pHashKey ^= psHashKeys[Piece::BPAWN][square];
            }
            if (piece == Piece::BKING)
                bKingSq = square;
        }
    }

    // Update piece/square table scores
    psScore1[removedPiece] -= Evaluate::psTab1[removedPiece][square];
    psScore2[removedPiece] -= Evaluate::psTab2[removedPiece][square];
    psScore1[piece]        += Evaluate::psTab1[piece][square];
    psScore2[piece]        += Evaluate::psTab2[piece][square];
}

void
Position::movePieceNotPawn(int from, int to) {
    const int piece = squares[from];
    hashKey ^= psHashKeys[piece][from];
    hashKey ^= psHashKeys[piece][to];
    hashKey ^= psHashKeys[Piece::EMPTY][from];
    hashKey ^= psHashKeys[Piece::EMPTY][to];

    squares[from] = Piece::EMPTY;
    squares[to] = piece;

    const U64 sqMaskF = 1ULL << from;
    const U64 sqMaskT = 1ULL << to;
    pieceTypeBB[piece] &= ~sqMaskF;
    pieceTypeBB[piece] |= sqMaskT;
    if (Piece::isWhite(piece)) {
        whiteBB &= ~sqMaskF;
        whiteBB |= sqMaskT;
        if (piece == Piece::WKING)
            wKingSq = to;
    } else {
        blackBB &= ~sqMaskF;
        blackBB |= sqMaskT;
        if (piece == Piece::BKING)
            bKingSq = to;
    }

    psScore1[piece] += Evaluate::psTab1[piece][to] - Evaluate::psTab1[piece][from];
    psScore2[piece] += Evaluate::psTab2[piece][to] - Evaluate::psTab2[piece][from];
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
