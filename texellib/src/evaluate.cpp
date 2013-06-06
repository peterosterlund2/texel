/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
 * evaluate.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "evaluate.hpp"
#include <vector>

int Evaluate::pieceValue[Piece::nPieceTypes] = {
    0,
    kV, qV, rV, bV, nV, pV,
    kV, qV, rV, bV, nV, pV
};

int Evaluate::pieceValueOrder[Piece::nPieceTypes] = {
    0,
    5, 4, 3, 2, 2, 1,
    5, 4, 3, 2, 2, 1
};

/** Piece/square table for king during middle game. */
const int Evaluate::kt1b[64] = { -22,-35,-40,-40,-40,-40,-35,-22,
                                 -22,-35,-40,-40,-40,-40,-35,-22,
                                 -25,-35,-40,-45,-45,-40,-35,-25,
                                 -15,-30,-35,-40,-40,-35,-30,-15,
                                 -10,-15,-20,-25,-25,-20,-15,-10,
                                   4, -2, -5,-15,-15, -5, -2,  4,
                                  16, 14,  7, -3, -3,  7, 14, 16,
                                  24, 24,  9,  0,  0,  9, 24, 24 };

/** Piece/square table for king during end game. */
const int Evaluate::kt2b[64] = {  0,  8, 16, 24, 24, 16,  8,  0,
                                  8, 16, 24, 32, 32, 24, 16,  8,
                                 16, 24, 32, 40, 40, 32, 24, 16,
                                 24, 32, 40, 48, 48, 40, 32, 24,
                                 24, 32, 40, 48, 48, 40, 32, 24,
                                 16, 24, 32, 40, 40, 32, 24, 16,
                                  8, 16, 24, 32, 32, 24, 16,  8,
                                  0,  8, 16, 24, 24, 16,  8,  0 };

/** Piece/square table for pawns during middle game. */
const int Evaluate::pt1b[64] = {  0,  0,  0,  0,  0,  0,  0,  0,
                                  8, 16, 24, 32, 32, 24, 16,  8,
                                  3, 12, 20, 28, 28, 20, 12,  3,
                                 -5,  4, 10, 20, 20, 10,  4, -5,
                                 -6,  4,  5, 16, 16,  5,  4, -6,
                                 -6,  4,  2,  5,  5,  2,  4, -6,
                                 -6,  4,  4,-15,-15,  4,  4, -6,
                                  0,  0,  0,  0,  0,  0,  0,  0 };

/** Piece/square table for pawns during end game. */
const int Evaluate::pt2b[64] = {   0,  0,  0,  0,  0,  0,  0,  0,
                                  25, 40, 45, 45, 45, 45, 40, 25,
                                  17, 32, 35, 35, 35, 35, 32, 17,
                                   5, 24, 24, 24, 24, 24, 24,  5,
                                  -9, 11, 11, 11, 11, 11, 11, -9,
                                 -17,  3,  3,  3,  3,  3,  3,-17,
                                 -20,  0,  0,  0,  0,  0,  0,-20,
                                   0,  0,  0,  0,  0,  0,  0,  0 };

/** Piece/square table for knights during middle game. */
const int Evaluate::nt1b[64] = { -53,-42,-32,-21,-21,-32,-42,-53,
                                 -42,-32,-10,  0,  0,-10,-32,-42,
                                 -21,  5, 10, 16, 16, 10,  5,-21,
                                 -18,  0, 10, 21, 21, 10,  0,-18,
                                 -18,  0,  3, 21, 21,  3,  0,-18,
                                 -21,-10,  0,  0,  0,  0,-10,-21,
                                 -42,-32,-10,  0,  0,-10,-32,-42,
                                 -53,-42,-32,-21,-21,-32,-42,-53 };

/** Piece/square table for knights during end game. */
const int Evaluate::nt2b[64] = { -56,-44,-34,-22,-22,-34,-44,-56,
                                 -44,-34,-10,  0,  0,-10,-34,-44,
                                 -22,  5, 10, 17, 17, 10,  5,-22,
                                 -19,  0, 10, 22, 22, 10,  0,-19,
                                 -19,  0,  3, 22, 22,  3,  0,-19,
                                 -22,-10,  0,  0,  0,  0,-10,-22,
                                 -44,-34,-10,  0,  0,-10,-34,-44,
                                 -56,-44,-34,-22,-22,-34,-44,-56 };

/** Piece/square table for bishops during middle game. */
const int Evaluate::bt1b[64] = {  0,  0,  0,  0,  0,  0,  0,  0,
                                  0,  4,  2,  2,  2,  2,  4,  0,
                                  0,  2,  4,  4,  4,  4,  2,  0,
                                  0,  2,  4,  4,  4,  4,  2,  0,
                                  0,  2,  4,  4,  4,  4,  2,  0,
                                  0,  3,  4,  4,  4,  4,  3,  0,
                                  0,  4,  2,  2,  2,  2,  4,  0,
                                 -5, -5, -7, -5, -5, -7, -5, -5 };

/** Piece/square table for bishops during end game. */
const int Evaluate::bt2b[64] = {  0,  0,  0,  0,  0,  0,  0,  0,
                                  0,  2,  2,  2,  2,  2,  2,  0,
                                  0,  2,  4,  4,  4,  4,  2,  0,
                                  0,  2,  4,  4,  4,  4,  2,  0,
                                  0,  2,  4,  4,  4,  4,  2,  0,
                                  0,  2,  4,  4,  4,  4,  2,  0,
                                  0,  2,  2,  2,  2,  2,  2,  0,
                                  0,  0,  0,  0,  0,  0,  0,  0 };

/** Piece/square table for queens during middle game. */
const int Evaluate::qt1b[64] = { -10, -5,  0,  0,  0,  0, -5,-10,
                                  -5,  0,  5,  5,  5,  5,  0, -5,
                                   0,  5,  5,  6,  6,  5,  5,  0,
                                   0,  5,  6,  6,  6,  6,  5,  0,
                                   0,  5,  6,  6,  6,  6,  5,  0,
                                   0,  5,  5,  6,  6,  5,  5,  0,
                                  -5,  0,  5,  5,  5,  5,  0, -5,
                                 -10, -5,  0,  0,  0,  0, -5,-10 };

/** Piece/square table for rooks during end game. */
const int Evaluate::rt1b[64] = {  8, 11, 13, 13, 13, 13, 11,  8,
                                 22, 27, 27, 27, 27, 27, 27, 22,
                                  0,  0,  0,  0,  0,  0,  0,  0,
                                  0,  0,  0,  0,  0,  0,  0,  0,
                                 -2,  0,  0,  0,  0,  0,  0, -2,
                                 -2,  0,  0,  2,  2,  0,  0, -2,
                                 -3,  2,  5,  5,  5,  5,  2, -3,
                                  0,  3,  5,  5,  5,  5,  3,  0 };

int Evaluate::kt1w[64];
int Evaluate::qt1w[64];
int Evaluate::rt1w[64];
int Evaluate::bt1w[64];
int Evaluate::nt1w[64];
int Evaluate::pt1w[64];
int Evaluate::kt2w[64];
int Evaluate::bt2w[64];
int Evaluate::nt2w[64];
int Evaluate::pt2w[64];

int Evaluate::castleFactor[256];

std::vector<Evaluate::PawnHashData> Evaluate::pawnHash;
vector_aligned<Evaluate::KingSafetyHashData> Evaluate::kingSafetyHash;
std::vector<Evaluate::MaterialHashData> Evaluate::materialHash;


static StaticInitializer<Evaluate> evInit;

void
Evaluate::staticInitialize() {
    for (int i = 0; i < 64; i++) {
        kt1w[i] = kt1b[63-i];
        qt1w[i] = qt1b[63-i];
        rt1w[i] = rt1b[63-i];
        bt1w[i] = bt1b[63-i];
        nt1w[i] = nt1b[63-i];
        pt1w[i] = pt1b[63-i];
        kt2w[i] = kt2b[63-i];
        bt2w[i] = bt2b[63-i];
        nt2w[i] = nt2b[63-i];
        pt2w[i] = pt2b[63-i];
    }

    pawnHash.resize(1<<16);

    for (int i = 0; i < 256; i++) {
        int h1Dist = 100;
        bool h1Castle = (i & (1<<7)) != 0;
        if (h1Castle)
            h1Dist = 2 + BitBoard::bitCount(i & 0x0000000000000060L); // f1,g1
        int a1Dist = 100;
        bool a1Castle = (i & 1) != 0;
        if (a1Castle)
            a1Dist = 2 + BitBoard::bitCount(i & 0x000000000000000EL); // b1,c1,d1
        castleFactor[i] = 1024 / std::min(a1Dist, h1Dist);
    }

    kingSafetyHash.resize(1 << 15);
    materialHash.resize(1 << 14);

    // Knight mobility scores
    for (int sq = 0; sq < 64; sq++) {
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        if (x >= 4) x = 7 - x;
        if (y >= 4) y = 7 - y;
        if (x < y) std::swap(x, y);
        int maxMob = 0;
        switch (y*8+x) {
        case  0: maxMob = 2; break; // a1
        case  1: maxMob = 3; break; // b1
        case  2: maxMob = 4; break; // c1
        case  3: maxMob = 4; break; // d1
        case  9: maxMob = 4; break; // b2
        case 10: maxMob = 6; break; // c2
        case 11: maxMob = 6; break; // d2
        case 18: maxMob = 8; break; // c3
        case 19: maxMob = 8; break; // d3
        case 27: maxMob = 8; break; // d4
        default:
            assert(false);
        }
        for (int m = 0; m <= 8; m++)
            knightMobScore[sq][m] = m * 16 / maxMob - 12;
    }
}

static const int empty[64] = { 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

const int* Evaluate::psTab1[Piece::nPieceTypes] = {
    empty,
    kt1w, qt1w, rt1w, bt1w, nt1w, pt1w,
    kt1b, qt1b, rt1b, bt1b, nt1b, pt1b
};

const int* Evaluate::psTab2[Piece::nPieceTypes] = {
    empty,
    kt2w, qt1w, rt1w, bt2w, nt2w, pt2w,
    kt2b, qt1b, rt1b, bt2b, nt2b, pt2b
};

const int Evaluate::distToH1A8[8][8] = { { 0, 1, 2, 3, 4, 5, 6, 7 },
                                         { 1, 2, 3, 4, 5, 6, 7, 6 },
                                         { 2, 3, 4, 5, 6, 7, 6, 5 },
                                         { 3, 4, 5, 6, 7, 6, 5, 4 },
                                         { 4, 5, 6, 7, 6, 5, 4, 3 },
                                         { 5, 6, 7, 6, 5, 4, 3, 2 },
                                         { 6, 7, 6, 5, 4, 3, 2, 1 },
                                         { 7, 6, 5, 4, 3, 2, 1, 0 } };

const int Evaluate::rookMobScore[] = {-10,-7,-4,-1,2,5,7,9,11,12,13,14,14,14,14};
const int Evaluate::bishMobScore[] = {-15,-10,-6,-2,2,6,10,13,16,18,20,22,23,24};
const int Evaluate::queenMobScore[] = {-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,8,9,9,10,10,10,10,10,10,10,10,10,10,10,10};
int Evaluate::knightMobScore[64][9];

Evaluate::Evaluate()
    : wKingZone(0), bKingZone(0),
      wKingAttacks(0), bKingAttacks(0),
      wAttacksBB(0), bAttacksBB(0),
      wPawnAttacks(0), bPawnAttacks(0) {
}

int
Evaluate::evalPos(const Position& pos) {
    int score = materialScore(pos);

    wKingAttacks = bKingAttacks = 0;
    wKingZone = BitBoard::kingAttacks[pos.getKingSq(true)]; wKingZone |= wKingZone << 8;
    bKingZone = BitBoard::kingAttacks[pos.getKingSq(false)]; bKingZone |= bKingZone >> 8;
    wAttacksBB = bAttacksBB = 0L;

    U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
    wPawnAttacks = ((pawns & BitBoard::maskBToHFiles) << 7) |
                   ((pawns & BitBoard::maskAToGFiles) << 9);
    pawns = pos.pieceTypeBB[Piece::BPAWN];
    bPawnAttacks = ((pawns & BitBoard::maskBToHFiles) >> 9) |
                   ((pawns & BitBoard::maskAToGFiles) >> 7);

    score += pieceSquareEval(pos);
    score += pawnBonus(pos);
    score += castleBonus(pos);

    score += rookBonus(pos);
    score += bishopEval(pos, score);
    score += knightEval(pos);
    score += threatBonus(pos);
    score += kingSafety(pos);
    score = endGameEval(pos, score);

    if (!pos.whiteMove)
        score = -score;
    return score;

    // FIXME! Test penalty if side to move has >1 hanging piece

    // FIXME! Test "tempo value"

    // FIXME! "Kf1" bad: r1bqk2r/pp1nppb1/2p4p/3p2p1/3P4/2NBPNP1/PPP2PP1/R2QK2R w KQkq - 1 10
}

void
Evaluate::computeMaterialScore(const Position& pos, MaterialHashData& mhd) const {
    // Compute material part of score
    int score = pos.wMtrl - pos.bMtrl;
    score += tradeBonus(pos);
    { // Redundancy of major pieces
        int wMajor = BitBoard::bitCount(pos.pieceTypeBB[Piece::WQUEEN] | pos.pieceTypeBB[Piece::WROOK]);
        int bMajor = BitBoard::bitCount(pos.pieceTypeBB[Piece::BQUEEN] | pos.pieceTypeBB[Piece::BROOK]);
        int w = std::min(wMajor, 3);
        int b = std::min(bMajor, 3);
        static const int bonus[4][4] = { {   0, -50,   0,   0 },
                                         {  50,   0,   0,   0 },
                                         {   0,   0,   0,  38 },
                                         {   0,   0, -38,   0 } };
        score += bonus[w][b];
    }
    mhd.id = pos.materialId();
    mhd.score = score;

    // Compute interpolation factors
    const int wMtrl = pos.wMtrl;
    const int bMtrl = pos.bMtrl;
    const int wMtrlPawns = pos.wMtrlPawns;
    const int bMtrlPawns = pos.bMtrlPawns;
    const int wMtrlNoPawns = wMtrl - wMtrlPawns;
    const int bMtrlNoPawns = bMtrl - bMtrlPawns;
    { // Pawn
        const int loMtrl = rV;
        const int hiMtrl = qV + 2 * rV + 2 * bV;
        mhd.wPawnIPF = interpolate(bMtrlNoPawns, loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bPawnIPF = interpolate(wMtrlNoPawns, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Knight/bishop
        const int loMtrl = nV + 8 * pV;
        const int hiMtrl = qV + 2 * rV + 1 * bV + 1 * nV + 6 * pV;
        mhd.wKnightIPF = interpolate(bMtrl, loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bKnightIPF = interpolate(wMtrl, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Castle
        const int loMtrl = 2 * rV;
        const int hiMtrl = 2 * (qV + 2 * rV + 2 * bV);
        const int m = wMtrlNoPawns + bMtrlNoPawns;
        mhd.castleIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Passed pawn
        const int loMtrl = 0;
        const int hiMtrl = qV + rV;
        mhd.wPassedPawnIPF = interpolate(bMtrlNoPawns, loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bPassedPawnIPF = interpolate(wMtrlNoPawns, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // King safety
        const int loMtrl = rV + bV;
        const int hiMtrl = qV + 2 * rV + 2 * bV + 2 * nV;
        const int m = (wMtrlNoPawns + bMtrlNoPawns) / 2;
        mhd.kingSafetyIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Different color bishops
        const int loMtrl = 2 * bV;
        const int hiMtrl = 2 * (qV + rV + bV);
        const int m = wMtrlNoPawns + bMtrlNoPawns;
        mhd.diffColorBishopIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Knight outpost
        const int loMtrl = 3 * pV;
        const int hiMtrl = 6 * pV;
        mhd.wKnightOutPostIPF = interpolate(bMtrlPawns, loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bKnightOutPostIPF = interpolate(wMtrlPawns, loMtrl, 0, hiMtrl, IPOLMAX);
    }
}

int
Evaluate::tradeBonus(const Position& pos) const {
    const int wM = pos.wMtrl;
    const int bM = pos.bMtrl;
    const int wPawn = pos.wMtrlPawns;
    const int bPawn = pos.bMtrlPawns;
    const int deltaScore = wM - bM;

    int pBonus = 0;
    pBonus += interpolate((deltaScore > 0) ? wPawn : bPawn, 0, -30 * deltaScore / 100, 6 * pV, 0);
    pBonus += interpolate((deltaScore > 0) ? bM : wM, 0, 30 * deltaScore / 100, qV + 2 * rV + 2 * bV + 2 * nV, 0);

    return pBonus;
}

int
Evaluate::pieceSquareEval(const Position& pos) {
    int score = 0;
    const int wMtrlPawns = pos.wMtrlPawns;
    const int bMtrlPawns = pos.bMtrlPawns;

    // Kings/pawns
    {
        const int k1 = pos.psScore1[Piece::WKING] + pos.psScore1[Piece::WPAWN];
        const int k2 = pos.psScore2[Piece::WKING] + pos.psScore2[Piece::WPAWN];
        score += interpolate(k2, k1, mhd->wPawnIPF);
    }
    {
        const int k1 = pos.psScore1[Piece::BKING] + pos.psScore1[Piece::BPAWN];
        const int k2 = pos.psScore2[Piece::BKING] + pos.psScore2[Piece::BPAWN];
        score -= interpolate(k2, k1, mhd->bPawnIPF);
    }

    // Knights/bishops
    {
        int n1 = pos.psScore1[Piece::WKNIGHT] + pos.psScore1[Piece::WBISHOP];
        int n2 = pos.psScore2[Piece::WKNIGHT] + pos.psScore2[Piece::WBISHOP];
        score += interpolate(n2, n1, mhd->wKnightIPF);
        n1 = pos.psScore1[Piece::BKNIGHT] + pos.psScore1[Piece::BBISHOP];
        n2 = pos.psScore2[Piece::BKNIGHT] + pos.psScore2[Piece::BBISHOP];
        score -= interpolate(n2, n1, mhd->bKnightIPF);
    }

    // Queens
    {
        const U64 occupied = pos.whiteBB | pos.blackBB;
        score += pos.psScore1[Piece::WQUEEN];
        U64 m = pos.pieceTypeBB[Piece::WQUEEN];
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
            wAttacksBB |= atk;
            score += queenMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB | bPawnAttacks))];
            bKingAttacks += BitBoard::bitCount(atk & bKingZone) * 2;
            m &= m-1;
        }
        score -= pos.psScore1[Piece::BQUEEN];
        m = pos.pieceTypeBB[Piece::BQUEEN];
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
            bAttacksBB |= atk;
            score -= queenMobScore[BitBoard::bitCount(atk & ~(pos.blackBB | wPawnAttacks))];
            wKingAttacks += BitBoard::bitCount(atk & wKingZone) * 2;
            m &= m-1;
        }
    }

    // Rooks
    {
        int r1 = pos.psScore1[Piece::WROOK];
        if (r1 != 0) {
            const int nP = bMtrlPawns / pV;
            const int s = r1 * std::min(nP, 6) / 6;
            score += s;
        }
        r1 = pos.psScore1[Piece::BROOK];
        if (r1 != 0) {
            const int nP = wMtrlPawns / pV;
            const int s = r1 * std::min(nP, 6) / 6;
            score -= s;
        }
    }

    return score;
}

int
Evaluate::castleBonus(const Position& pos) {
    if (pos.getCastleMask() == 0) return 0;

    const int k1 = kt1b[7*8+6] - kt1b[7*8+4];
    const int k2 = kt2b[7*8+6] - kt2b[7*8+4];
    const int ks = interpolate(k2, k1, mhd->castleIPF);

    const int castleValue = ks + rt1b[7*8+5] - rt1b[7*8+7];
    if (castleValue <= 0)
        return 0;

    U64 occupied = pos.whiteBB | pos.blackBB;
    int tmp = (int) (occupied & 0x6E);
    if (pos.a1Castle()) tmp |= 1;
    if (pos.h1Castle()) tmp |= (1 << 7);
    const int wBonus = (castleValue * castleFactor[tmp]) >> 10;

    tmp = (int) ((occupied >> 56) & 0x6E);
    if (pos.a8Castle()) tmp |= 1;
    if (pos.h8Castle()) tmp |= (1 << 7);
    const int bBonus = (castleValue * castleFactor[tmp]) >> 10;

    return wBonus - bBonus;
}

int
Evaluate::pawnBonus(const Position& pos) {
    U64 key = pos.pawnZobristHash();
    PawnHashData& phd = pawnHash[(int)key & (pawnHash.size() - 1)];
    if (phd.key != key)
        computePawnHashData(pos, phd);
    this->phd = &phd;
    U64 m = phd.passedPawnsW;
    int score = phd.score;

    const int hiMtrl = qV + rV;
    score += interpolate(2 * phd.passedBonusW, phd.passedBonusW, mhd->wPassedPawnIPF);
    score -= interpolate(2 * phd.passedBonusB, phd.passedBonusB, mhd->bPassedPawnIPF);

    // Passed pawns are more dangerous if enemy king is far away
    int bestWPawnDist = 8;
    int bestWPromSq = -1;
    if (m != 0) {
        int mtrlNoPawns = pos.bMtrl - pos.bMtrlPawns;
        if (mtrlNoPawns < hiMtrl) {
            int kingPos = pos.getKingSq(false);
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                int x = Position::getX(sq);
                int y = Position::getY(sq);
                int pawnDist = std::min(5, 7 - y);
                int kingDist = BitBoard::getDistance(kingPos, Position::getSquare(x, 7));
                int kScore = kingDist * 4;
                if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                score += interpolate(kScore, 0, mhd->wPassedPawnIPF);
                if (!pos.whiteMove)
                    kingDist--;
                if ((pawnDist < kingDist) && (mtrlNoPawns == 0)) {
                    if (BitBoard::northFill(1ULL<<sq) & (1LL << pos.getKingSq(true)))
                        pawnDist++; // Own king blocking pawn
                    if (pawnDist < bestWPawnDist) {
                        bestWPawnDist = pawnDist;
                        bestWPromSq = Position::getSquare(x, 7);
                    }
                }
                m &= m-1;
            }
        }
    }
    int bestBPawnDist = 8;
    int bestBPromSq = -1;
    m = phd.passedPawnsB;
    if (m != 0) {
        int mtrlNoPawns = pos.wMtrl - pos.wMtrlPawns;
        if (mtrlNoPawns < hiMtrl) {
            int kingPos = pos.getKingSq(true);
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                int x = Position::getX(sq);
                int y = Position::getY(sq);
                int pawnDist = std::min(5, y);
                int kingDist = BitBoard::getDistance(kingPos, Position::getSquare(x, 0));
                int kScore = kingDist * 4;
                if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                score -= interpolate(kScore, 0, mhd->bPassedPawnIPF);
                if (pos.whiteMove)
                    kingDist--;
                if ((pawnDist < kingDist) && (mtrlNoPawns == 0)) {
                    if (BitBoard::southFill(1ULL<<sq) & (1LL << pos.getKingSq(false)))
                        pawnDist++; // Own king blocking pawn
                    if (pawnDist < bestBPawnDist) {
                        bestBPawnDist = pawnDist;
                        bestBPromSq = Position::getSquare(x, 0);
                    }
                }
                m &= m-1;
            }
        }
    }

    // Evaluate pawn races in pawn end games
    if (bestWPromSq >= 0) {
        if (bestBPromSq >= 0) {
            int wPly = bestWPawnDist * 2; if (pos.whiteMove) wPly--;
            int bPly = bestBPawnDist * 2; if (!pos.whiteMove) bPly--;
            if (wPly < bPly - 1) {
                score += 500;
            } else if (wPly == bPly - 1) {
                if (BitBoard::getDirection(bestWPromSq, pos.getKingSq(false)))
                    score += 500;
            } else if (wPly == bPly + 1) {
                if (BitBoard::getDirection(bestBPromSq, pos.getKingSq(true)))
                    score -= 500;
            } else {
                score -= 500;
            }
        } else
            score += 500;
    } else if (bestBPromSq >= 0)
        score -= 500;

    return score;
}

void
Evaluate::computePawnHashData(const Position& pos, PawnHashData& ph) {
    int score = 0;

    // Evaluate double pawns and pawn islands
    const U64 wPawns = pos.pieceTypeBB[Piece::WPAWN];
    const U64 wPawnFiles = BitBoard::southFill(wPawns) & 0xff;
    const int wDouble = BitBoard::bitCount(wPawns) - BitBoard::bitCount(wPawnFiles);
    const int wIslands = BitBoard::bitCount(((~wPawnFiles) >> 1) & wPawnFiles);
    const int wIsolated = BitBoard::bitCount(~(wPawnFiles<<1) & wPawnFiles & ~(wPawnFiles>>1));

    const U64 bPawns = pos.pieceTypeBB[Piece::BPAWN];
    const U64 bPawnFiles = BitBoard::southFill(bPawns) & 0xff;
    const int bDouble = BitBoard::bitCount(bPawns) - BitBoard::bitCount(bPawnFiles);
    const int bIslands = BitBoard::bitCount(((~bPawnFiles) >> 1) & bPawnFiles);
    const int bIsolated = BitBoard::bitCount(~(bPawnFiles<<1) & bPawnFiles & ~(bPawnFiles>>1));

    score -= (wDouble - bDouble) * 19;
    score -= (wIslands - bIslands) * 14;
    score -= (wIsolated - bIsolated) * 9;

    // Evaluate backward pawns, defined as a pawn that guards a friendly pawn,
    // can't be guarded by friendly pawns, can advance, but can't advance without
    // being captured by an enemy pawn.
    const U64 bPawnNoAtks = ~BitBoard::southFill(bPawnAttacks);
    const U64 wPawnNoAtks = ~BitBoard::northFill(wPawnAttacks);
    ph.outPostsW = bPawnNoAtks & wPawnAttacks;
    ph.outPostsB = wPawnNoAtks & bPawnAttacks;

    U64 wBackward = wPawns & ~((wPawns | bPawns) >> 8) & (bPawnAttacks >> 8) & wPawnNoAtks;
    wBackward &= (((wPawns & BitBoard::maskBToHFiles) >> 9) |
                  ((wPawns & BitBoard::maskAToGFiles) >> 7));
    wBackward &= ~BitBoard::northFill(bPawnFiles);
    U64 bBackward = bPawns & ~((wPawns | bPawns) << 8) & (wPawnAttacks << 8) & bPawnNoAtks;
    bBackward &= (((bPawns & BitBoard::maskBToHFiles) << 7) |
                  ((bPawns & BitBoard::maskAToGFiles) << 9));
    bBackward &= ~BitBoard::northFill(wPawnFiles);
    score -= (BitBoard::bitCount(wBackward) - BitBoard::bitCount(bBackward)) * 15;

    // Evaluate passed pawn bonus, white
    U64 passedPawnsW = wPawns & ~BitBoard::southFill(bPawns | bPawnAttacks | (wPawns >> 8));
    static const int ppBonus[] = {-1,24,26,30,36,55,100,-1};
    int passedBonusW = 0;
    if (passedPawnsW != 0) {
        U64 guardedPassedW = passedPawnsW & wPawnAttacks;
        passedBonusW += 15 * BitBoard::bitCount(guardedPassedW);
        U64 m = passedPawnsW;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            int y = Position::getY(sq);
            passedBonusW += ppBonus[y];
            m &= m-1;
        }
    }

    // Evaluate passed pawn bonus, black
    U64 passedPawnsB = bPawns & ~BitBoard::northFill(wPawns | wPawnAttacks | (bPawns << 8));
    int passedBonusB = 0;
    if (passedPawnsB != 0) {
        U64 guardedPassedB = passedPawnsB & bPawnAttacks;
        passedBonusB += 15 * BitBoard::bitCount(guardedPassedB);
        U64 m = passedPawnsB;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            int y = Position::getY(sq);
            passedBonusB += ppBonus[7-y];
            m &= m-1;
        }
    }

    // Connected passed pawn bonus. Seems logical but scored -8 elo in tests
//    if (passedPawnsW != 0) {
//        U64 mask = passedPawnsW;
//        mask = (((mask >> 7) | (mask << 1) | (mask << 9)) & BitBoard::maskBToHFiles) |
//               (((mask >> 9) | (mask >> 1) | (mask << 7)) & BitBoard::maskAToGFiles);
//        passedBonusW += 13 * BitBoard::bitCount(passedPawnsW & mask);
//    }
//    if (passedPawnsB != 0) {
//        U64 mask = passedPawnsB;
//        mask = (((mask >> 7) | (mask << 1) | (mask << 9)) & BitBoard::maskBToHFiles) |
//               (((mask >> 9) | (mask >> 1) | (mask << 7)) & BitBoard::maskAToGFiles);
//        passedBonusB += 13 * BitBoard::bitCount(passedPawnsB & mask);
//    }

    ph.key = pos.pawnZobristHash();
    ph.score = score;
    ph.passedBonusW = (short)passedBonusW;
    ph.passedBonusB = (short)passedBonusB;
    ph.passedPawnsW = passedPawnsW;
    ph.passedPawnsB = passedPawnsB;
}


int
Evaluate::rookBonus(const Position& pos) {
    int score = 0;
    const U64 wPawns = pos.pieceTypeBB[Piece::WPAWN];
    const U64 bPawns = pos.pieceTypeBB[Piece::BPAWN];
    const U64 occupied = pos.whiteBB | pos.blackBB;
    U64 m = pos.pieceTypeBB[Piece::WROOK];
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        const int x = Position::getX(sq);
        if ((wPawns & BitBoard::maskFile[x]) == 0) // At least half-open file
            score += (bPawns & BitBoard::maskFile[x]) == 0 ? 25 : 12;
        U64 atk = BitBoard::rookAttacks(sq, occupied);
        wAttacksBB |= atk;
        score += rookMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB | bPawnAttacks))];
        if ((atk & bKingZone) != 0)
            bKingAttacks += BitBoard::bitCount(atk & bKingZone);
        m &= m-1;
    }
    U64 r7 = pos.pieceTypeBB[Piece::WROOK] & 0x00ff000000000000ULL;
    if (((r7 & (r7 - 1)) != 0) &&
        ((pos.pieceTypeBB[Piece::BKING] & 0xff00000000000000ULL) != 0))
        score += 30; // Two rooks on 7:th row
    m = pos.pieceTypeBB[Piece::BROOK];
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        const int x = Position::getX(sq);
        if ((bPawns & BitBoard::maskFile[x]) == 0)
            score -= (wPawns & BitBoard::maskFile[x]) == 0 ? 25 : 12;
        U64 atk = BitBoard::rookAttacks(sq, occupied);
        bAttacksBB |= atk;
        score -= rookMobScore[BitBoard::bitCount(atk & ~(pos.blackBB | wPawnAttacks))];
        if ((atk & wKingZone) != 0)
            wKingAttacks += BitBoard::bitCount(atk & wKingZone);
        m &= m-1;
    }
    r7 = pos.pieceTypeBB[Piece::BROOK] & 0xff00L;
    if (((r7 & (r7 - 1)) != 0) &&
        ((pos.pieceTypeBB[Piece::WKING] & 0xffL) != 0))
      score -= 30; // Two rooks on 2:nd row
    return score;
}

int
Evaluate::bishopEval(const Position& pos, int oldScore) {
    int score = 0;
    const U64 occupied = pos.whiteBB | pos.blackBB;
    U64 wBishops = pos.pieceTypeBB[Piece::WBISHOP];
    U64 bBishops = pos.pieceTypeBB[Piece::BBISHOP];
    if ((wBishops | bBishops) == 0)
        return 0;
    U64 m = wBishops;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::bishopAttacks(sq, occupied);
        wAttacksBB |= atk;
        score += bishMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB | bPawnAttacks))];
        if ((atk & bKingZone) != 0)
            bKingAttacks += BitBoard::bitCount(atk & bKingZone);
        m &= m-1;
    }
    m = bBishops;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::bishopAttacks(sq, occupied);
        bAttacksBB |= atk;
        score -= bishMobScore[BitBoard::bitCount(atk & ~(pos.blackBB | wPawnAttacks))];
        if ((atk & wKingZone) != 0)
            wKingAttacks += BitBoard::bitCount(atk & wKingZone);
        m &= m-1;
    }

    bool whiteDark  = (pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq ) != 0;
    bool whiteLight = (pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskLightSq) != 0;
    bool blackDark  = (pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq ) != 0;
    bool blackLight = (pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskLightSq) != 0;
    int numWhite = (whiteDark ? 1 : 0) + (whiteLight ? 1 : 0);
    int numBlack = (blackDark ? 1 : 0) + (blackLight ? 1 : 0);

    // Bishop pair bonus
    if (numWhite == 2) {
        const int numPawns = pos.wMtrlPawns / pV;
        score += 28 + (8 - numPawns) * 3;
    }
    if (numBlack == 2) {
        const int numPawns = pos.bMtrlPawns / pV;
        score -= 28 + (8 - numPawns) * 3;
    }

    if ((numWhite == 1) && (numBlack == 1) && (whiteDark != blackDark) &&
        (pos.wMtrl - pos.wMtrlPawns == pos.bMtrl - pos.bMtrlPawns)) {
        const int penalty = (oldScore + score) / 2;
        score -= interpolate(penalty, 0, mhd->diffColorBishopIPF);
    } else {
        if (numWhite == 1) {
            U64 bishColorMask = whiteDark ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
            U64 m = pos.pieceTypeBB[Piece::WPAWN] & bishColorMask;
            m |= (m << 8) & pos.pieceTypeBB[Piece::BPAWN];
            score -= 2 * BitBoard::bitCount(m);
        }
        if (numBlack == 1) {
            U64 bishColorMask = blackDark ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
            U64 m = pos.pieceTypeBB[Piece::BPAWN] & bishColorMask;
            m |= (m >> 8) & pos.pieceTypeBB[Piece::WPAWN];
            score += 2 * BitBoard::bitCount(m);
        }
    }

    // Penalty for bishop trapped behind pawn at a2/h2/a7/h7
    if (((wBishops | bBishops) & 0x0081000000008100L) != 0) {
        if ((pos.getPiece(48) == Piece::WBISHOP) && // a7
            (pos.getPiece(41) == Piece::BPAWN) &&
            (pos.getPiece(50) == Piece::BPAWN))
            score -= pV * 3 / 2;
        if ((pos.getPiece(55) == Piece::WBISHOP) && // h7
            (pos.getPiece(46) == Piece::BPAWN) &&
            (pos.getPiece(53) == Piece::BPAWN))
            score -= (pos.pieceTypeBB[Piece::WQUEEN] != 0) ? pV : pV * 3 / 2;
        if ((pos.getPiece(8)  == Piece::BBISHOP) &&  // a2
            (pos.getPiece(17) == Piece::WPAWN) &&
            (pos.getPiece(10) == Piece::WPAWN))
            score += pV * 3 / 2;
        if ((pos.getPiece(15) == Piece::BBISHOP) && // h2
            (pos.getPiece(22) == Piece::WPAWN) &&
            (pos.getPiece(13) == Piece::WPAWN))
            score += (pos.pieceTypeBB[Piece::BQUEEN] != 0) ? pV : pV * 3 / 2;
    }

    return score;
}

int
Evaluate::knightEval(const Position& pos) {
    int score = 0;
    U64 wKnights = pos.pieceTypeBB[Piece::WKNIGHT];
    U64 bKnights = pos.pieceTypeBB[Piece::BKNIGHT];
    if ((wKnights | bKnights) == 0)
        return 0;

    // Knight mobility
    U64 m = wKnights;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::knightAttacks[sq];
        wAttacksBB |= atk;
        score += knightMobScore[sq][BitBoard::bitCount(atk & ~pos.whiteBB)];
        m &= m-1;
    }
    m = bKnights;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::knightAttacks[sq];
        bAttacksBB |= atk;
        score -= knightMobScore[sq][BitBoard::bitCount(atk & ~pos.blackBB)];
        m &= m-1;
    }

    // Knight outposts
    static const int outPostBonus[64] = {  0,  0,  0,  0,  0,  0,  0,  0,
                                           0,  0,  0,  0,  0,  0,  0,  0,
                                           0, 14, 25, 36, 36, 25, 14,  0,
                                           0, 14, 33, 43, 43, 33, 14,  0,
                                           0,  0, 18, 29, 29, 18,  0,  0,
                                           0,  0,  0,  0,  0,  0,  0,  0,
                                           0,  0,  0,  0,  0,  0,  0,  0,
                                           0,  0,  0,  0,  0,  0,  0,  0 };
    m = wKnights & phd->outPostsW;
    if (m != 0) {
        int outPost = 0;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            outPost += outPostBonus[63-sq];
            m &= m-1;
        }
        score += interpolate(0, outPost, mhd->wKnightOutPostIPF);
    }

    m = bKnights & phd->outPostsB;
    if (m != 0) {
        int outPost = 0;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            outPost += outPostBonus[sq];
            m &= m-1;
        }
        score -= interpolate(0, outPost, mhd->bKnightOutPostIPF);
    }

    return score;
}

int
Evaluate::threatBonus(const Position& pos) {
    // FIXME!! Try higher weight for attacks on more valuable pieces.
    int score = 0;

    // Sum values for all black pieces under attack
    wAttacksBB &= (pos.pieceTypeBB[Piece::BKNIGHT] |
                   pos.pieceTypeBB[Piece::BBISHOP] |
                   pos.pieceTypeBB[Piece::BROOK] |
                   pos.pieceTypeBB[Piece::BQUEEN]);
    wAttacksBB |= wPawnAttacks;
    U64 m = wAttacksBB & pos.blackBB & ~pos.pieceTypeBB[Piece::BKING];
    int tmp = 0;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        tmp += pieceValue[pos.getPiece(sq)];
        m &= m-1;
    }
    score += tmp + tmp * tmp / qV;

    // Sum values for all white pieces under attack
    bAttacksBB &= (pos.pieceTypeBB[Piece::WKNIGHT] |
                   pos.pieceTypeBB[Piece::WBISHOP] |
                   pos.pieceTypeBB[Piece::WROOK] |
                   pos.pieceTypeBB[Piece::WQUEEN]);
    bAttacksBB |= bPawnAttacks;
    m = bAttacksBB & pos.whiteBB & ~pos.pieceTypeBB[Piece::WKING];
    tmp = 0;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        tmp += pieceValue[pos.getPiece(sq)];
        m &= m-1;
    }
    score -= tmp + tmp * tmp / qV;
    return score / 64;
}

/** Compute king safety for both kings. */
int
Evaluate::kingSafety(const Position& pos) {
    const int minM = rV + bV;
    const int m = (pos.wMtrl - pos.wMtrlPawns + pos.bMtrl - pos.bMtrlPawns) / 2;
    if (m <= minM)
        return 0;
    int score = kingSafetyKPPart(pos);
    if (Position::getY(pos.wKingSq) == 0) {
        if (((pos.pieceTypeBB[Piece::WKING] & 0x60L) != 0) && // King on f1 or g1
            ((pos.pieceTypeBB[Piece::WROOK] & 0xC0L) != 0) && // Rook on g1 or h1
            ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[6]) != 0) &&
            ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[7]) != 0)) {
            score -= 6 * 15;
        } else
        if (((pos.pieceTypeBB[Piece::WKING] & 0x6L) != 0) && // King on b1 or c1
            ((pos.pieceTypeBB[Piece::WROOK] & 0x3L) != 0) && // Rook on a1 or b1
            ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[0]) != 0) &&
            ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[1]) != 0)) {
            score -= 6 * 15;
        }
    }
    if (Position::getY(pos.bKingSq) == 7) {
        if (((pos.pieceTypeBB[Piece::BKING] & 0x6000000000000000L) != 0) && // King on f8 or g8
            ((pos.pieceTypeBB[Piece::BROOK] & 0xC000000000000000L) != 0) && // Rook on g8 or h8
            ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[6]) != 0) &&
            ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[7]) != 0)) {
            score += 6 * 15;
        } else
        if (((pos.pieceTypeBB[Piece::BKING] & 0x600000000000000L) != 0) && // King on b8 or c8
            ((pos.pieceTypeBB[Piece::BROOK] & 0x300000000000000L) != 0) && // Rook on a8 or b8
            ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[0]) != 0) &&
            ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[1]) != 0)) {
            score += 6 * 15;
        }
    }
    score += (bKingAttacks - wKingAttacks) * 4;
    const int kSafety = interpolate(0, score, mhd->kingSafetyIPF);
    return kSafety;
}

int
Evaluate::kingSafetyKPPart(const Position& pos) {
    // FIXME!!! Try non-linear king safety
    const U64 key = pos.pawnZobristHash() ^ pos.kingZobristHash();
    KingSafetyHashData& ksh = kingSafetyHash[(int)key & (kingSafetyHash.size() - 1)];
    if (ksh.key != key) {
        int score = 0;
        U64 wPawns = pos.pieceTypeBB[Piece::WPAWN];
        U64 bPawns = pos.pieceTypeBB[Piece::BPAWN];
        {
            int safety = 0;
            int halfOpenFiles = 0;
            if (Position::getY(pos.wKingSq) < 2) {
                U64 shelter = 1ULL << Position::getX(pos.wKingSq);
                shelter |= ((shelter & BitBoard::maskBToHFiles) >> 1) |
                           ((shelter & BitBoard::maskAToGFiles) << 1);
                shelter <<= 8;
                safety += 3 * BitBoard::bitCount(wPawns & shelter);
                safety -= 2 * BitBoard::bitCount(bPawns & (shelter | (shelter << 8)));
                shelter <<= 8;
                safety += 2 * BitBoard::bitCount(wPawns & shelter);
                shelter <<= 8;
                safety -= BitBoard::bitCount(bPawns & shelter);

                U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                if (wOpen != 0) {
                    halfOpenFiles += 25 * BitBoard::bitCount(wOpen & 0xe7);
                    halfOpenFiles += 10 * BitBoard::bitCount(wOpen & 0x18);
                }
                U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                if (bOpen != 0) {
                    halfOpenFiles += 25 * BitBoard::bitCount(bOpen & 0xe7);
                    halfOpenFiles += 10 * BitBoard::bitCount(bOpen & 0x18);
                }
                safety = std::min(safety, 8);
            }
            const int kSafety = (safety - 9) * 15 - halfOpenFiles;
            score += kSafety;
        }
        {
            int safety = 0;
            int halfOpenFiles = 0;
            if (Position::getY(pos.bKingSq) >= 6) {
                U64 shelter = 1ULL << (56 + Position::getX(pos.bKingSq));
                shelter |= ((shelter & BitBoard::maskBToHFiles) >> 1) |
                           ((shelter & BitBoard::maskAToGFiles) << 1);
                shelter >>= 8;
                safety += 3 * BitBoard::bitCount(bPawns & shelter);
                safety -= 2 * BitBoard::bitCount(wPawns & (shelter | (shelter >> 8)));
                shelter >>= 8;
                safety += 2 * BitBoard::bitCount(bPawns & shelter);
                shelter >>= 8;
                safety -= BitBoard::bitCount(wPawns & shelter);

                U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                if (wOpen != 0) {
                    halfOpenFiles += 25 * BitBoard::bitCount(wOpen & 0xe7);
                    halfOpenFiles += 10 * BitBoard::bitCount(wOpen & 0x18);
                }
                U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                if (bOpen != 0) {
                    halfOpenFiles += 25 * BitBoard::bitCount(bOpen & 0xe7);
                    halfOpenFiles += 10 * BitBoard::bitCount(bOpen & 0x18);
                }
                safety = std::min(safety, 8);
            }
            const int kSafety = (safety - 9) * 15 - halfOpenFiles;
            score -= kSafety;
        }
        ksh.key = key;
        ksh.score = score;
    }
    return ksh.score;
}

/** Implements special knowledge for some endgame situations. */
int
Evaluate::endGameEval(const Position& pos, int oldScore) {
    int score = oldScore;
    if (pos.wMtrl + pos.bMtrl > 6 * rV)
        return score;
    const int wMtrlPawns = pos.wMtrlPawns;
    const int bMtrlPawns = pos.bMtrlPawns;
    const int wMtrlNoPawns = pos.wMtrl - wMtrlPawns;
    const int bMtrlNoPawns = pos.bMtrl - bMtrlPawns;

    // Handle special endgames
    typedef MatId MI;
    switch (pos.materialId()) {
    case 0:
    case MI::WN: case MI::BN: case MI::WB: case MI::BB:
    case MI::WN + MI::BN: case MI::WN + MI::BB:
    case MI::WB + MI::BN: case MI::WB + MI::BB:
        return 0; // King + minor piece vs king + minor piece is a draw
    case MI::WQ + MI::BP: {
        int wk = pos.getKingSq(true);
        int wq = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WQUEEN]);
        int bk = pos.getKingSq(false);
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BPAWN]);
        return evalKQKP(wk, wq, bk, bp, pos.whiteMove);
    }
    case MI::BQ + MI::WP: {
        int bk = pos.getKingSq(false);
        int bq = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BQUEEN]);
        int wk = pos.getKingSq(true);
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WPAWN]);
        return -evalKQKP(63-bk, 63-bq, 63-wk, 63-wp, !pos.whiteMove);
    }
    case MI::WR + MI::BP: {
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BPAWN]);
        return krkpEval(pos.getKingSq(true), pos.getKingSq(false),
                        bp, pos.whiteMove);
    }
    case MI::BR + MI::WP: {
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WPAWN]);
        return -krkpEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                         63-wp, !pos.whiteMove);
    }
    case MI::WR + MI::BB: {
        score /= 8;
        const int kSq = pos.getKingSq(false);
        const int x = Position::getX(kSq);
        const int y = Position::getY(kSq);
        if ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq) != 0)
            score += (7 - distToH1A8[7-y][7-x]) * 7;
        else
            score += (7 - distToH1A8[7-y][x]) * 7;
        return score;
    }
    case MI::BR + MI::WB: {
        score /= 8;
        const int kSq = pos.getKingSq(true);
        const int x = Position::getX(kSq);
        const int y = Position::getY(kSq);
        if ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq) != 0)
            score -= (7 - distToH1A8[7-y][7-x]) * 7;
        else
            score -= (7 - distToH1A8[7-y][x]) * 7;
        return score;
    }
    case MI::WR + MI::WP + MI::BR: {
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WPAWN]);
        int wr = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WROOK]);
        int br = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BROOK]);
        return krpkrEval(wk, bk, wp, wr, br, pos.whiteMove);
    }
    case MI::BR + MI::BP + MI::WR: {
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BPAWN]);
        int wr = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WROOK]);
        int br = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BROOK]);
        return -krpkrEval(63-bk, 63-wk, 63-bp, 63-br, 63-wr, !pos.whiteMove);
    }
    case MI::WN * 2:
    case MI::BN * 2:
        return score / 50; // KNNK is a draw
    case MI::WN + MI::WB: {
        score /= 10;
        score += nV + bV + 300;
        const int kSq = pos.getKingSq(false);
        const int x = Position::getX(kSq);
        const int y = Position::getY(kSq);
        if ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq) != 0)
            score += (7 - distToH1A8[7-y][7-x]) * 10;
        else
            score += (7 - distToH1A8[7-y][x]) * 10;
        return score;
    }
    case MI::BN + MI::BB: {
        score /= 10;
        score -= nV + bV + 300;
        const int kSq = pos.getKingSq(true);
        const int x = Position::getX(kSq);
        const int y = Position::getY(kSq);
        if ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq) != 0)
            score -= (7 - distToH1A8[7-y][7-x]) * 10;
        else
            score -= (7 - distToH1A8[7-y][x]) * 10;
        return score;
    }
    case MI::WP: {
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WPAWN]);
        return kpkEval(pos.getKingSq(true), pos.getKingSq(false),
                       wp, pos.whiteMove);
    }
    case MI::BP: {
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BPAWN]);
        return -kpkEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                        63-bp, !pos.whiteMove);
    }
    }

    // Give bonus/penalty if advantage is/isn't large enough to win
    if (score > 0) {
        if ((wMtrlPawns == 0) && (wMtrlNoPawns <= bMtrlNoPawns + bV)) {
            if (wMtrlNoPawns < rV)
                return -pos.bMtrl / 50;
            else
                return score / 8;        // Too little excess material, probably draw
        } else if ((pos.pieceTypeBB[Piece::WROOK] | pos.pieceTypeBB[Piece::WKNIGHT] |
                    pos.pieceTypeBB[Piece::WQUEEN]) == 0) {
            // Check for rook pawn + wrong color bishop
            if (((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskBToHFiles) == 0) &&
                ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskLightSq) == 0) &&
                ((pos.pieceTypeBB[Piece::BKING] & 0x0303000000000000ULL) != 0)) {
                return 0;
            } else
            if (((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskAToGFiles) == 0) &&
                ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq) == 0) &&
                ((pos.pieceTypeBB[Piece::BKING] & 0xC0C0000000000000ULL) != 0)) {
                return 0;
            }
        }
    }
    if ((bMtrlPawns == 0) && (wMtrlNoPawns - bMtrlNoPawns > bV))
        return score + 300;       // Enough excess material, should win
    if (score < 0) {
        if ((bMtrlPawns == 0) && (bMtrlNoPawns <= wMtrlNoPawns + bV)) {
            if (bMtrlNoPawns < rV)
                return pos.wMtrl / 50;
            else
                return score / 8;        // Too little excess material, probably draw
        } else if ((pos.pieceTypeBB[Piece::BROOK] | pos.pieceTypeBB[Piece::BKNIGHT] |
                    pos.pieceTypeBB[Piece::BQUEEN]) == 0) {
            // Check for rook pawn + wrong color bishop
            if (((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskBToHFiles) == 0) &&
                ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq) == 0) &&
                ((pos.pieceTypeBB[Piece::WKING] & 0x0303ULL) != 0)) {
                return 0;
            } else
            if (((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskAToGFiles) == 0) &&
                ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskLightSq) == 0) &&
                ((pos.pieceTypeBB[Piece::WKING] & 0xC0C0ULL) != 0)) {
                return 0;
            }
        }
    }
    if ((wMtrlPawns == 0) && (bMtrlNoPawns - wMtrlNoPawns > bV))
        return score - 300;       // Enough excess material, should win
    return score;

    // FIXME! KRBKR is very hard to draw
}

int
Evaluate::evalKQKP(int wKing, int wQueen, int bKing, int bPawn, bool whiteMove) {
    bool canWin = false;
    if (((1ULL << bKing) & 0xFFFF) == 0) {
        canWin = true; // King doesn't support pawn
    } else if (std::abs(Position::getX(bPawn) - Position::getX(bKing)) > 2) {
        canWin = true; // King doesn't support pawn
    } else {
        switch (bPawn) {
        case 8:  // a2
            canWin = ((1ULL << wKing) & 0x0F1F1F1F1FULL) != 0;
            if (canWin && (bKing == 0) && (Position::getX(wQueen) == 1) && !whiteMove)
                canWin = false; // Stale-mate
            break;
        case 10: // c2
            canWin = ((1ULL << wKing) & 0x071F1F1FULL) != 0;
            break;
        case 13: // f2
            canWin = ((1ULL << wKing) & 0xE0F8F8F8ULL) != 0;
            break;
        case 15: // h2
            canWin = ((1ULL << wKing) & 0xF0F8F8F8F8ULL) != 0;
            if (canWin && (bKing == 7) && (Position::getX(wQueen) == 6) && !whiteMove)
                canWin = false; // Stale-mate
            break;
        default:
            canWin = true;
            break;
        }
    }

    const int dist = BitBoard::getDistance(wKing, bPawn);
    int score = qV - pV - 20 * dist;
    if (!canWin)
        score /= 50;
    return score;
}

int
Evaluate::kpkEval(int wKing, int bKing, int wPawn, bool whiteMove) {
    if (Position::getX(wKing) >= 4) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
        wPawn ^= 7;
    }
    int index = whiteMove ? 0 : 1;
    index = index * 32 + Position::getY(wKing)*4+Position::getX(wKing);
    index = index * 64 + bKing;
    index = index * 48 + wPawn - 8;

    int bytePos = index / 8;
    int bitPos = index % 8;
    bool draw = (((int)kpkTable[bytePos]) & (1 << bitPos)) == 0;
    if (draw)
        return 0;
    return qV - pV / 4 * (7-Position::getY(wPawn));
}

int
Evaluate::krkpEval(int wKing, int bKing, int bPawn, bool whiteMove) {
    if (Position::getX(bKing) >= 4) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
        bPawn ^= 7;
    }
    int index = whiteMove ? 0 : 1;
    index = index * 32 + Position::getY(bKing)*4+Position::getX(bKing);
    index = index * 48 + bPawn - 8;
    index = index * 8 + Position::getY(wKing);
    byte mask = krkpTable[index];
    bool canWin = (mask & (1 << Position::getX(wKing))) != 0;

    int score = rV - pV + Position::getY(bPawn) * pV / 4;
    if (canWin)
        score += 150;
    else
        score /= 50;
    return score;
}

int Evaluate::krpkrEval(int wKing, int bKing, int wPawn, int wRook, int bRook, bool whiteMove) {
    if (Position::getX(wPawn) >= 4) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
        wPawn ^= 7;
        wRook ^= 7;
        bRook ^= 7;
    }
    int index = whiteMove ? 0 : 1;
    index = index * 24 + (Position::getY(wPawn)-1)*4+Position::getX(wPawn);
    index = index * 64 + wKing;
    const U64 kMask = krpkrTable[index];
    const bool canWin = (kMask & (1ULL << bKing)) != 0;
    U64 kingNeighbors = BitBoard::kingAttacks[bKing];
    const U64 occupied = (1ULL<<wKing) | (1ULL<<bKing) | (1ULL<<wPawn) | (1ULL<<bRook);
    const U64 rAtk = BitBoard::rookAttacks(wRook, occupied);
    kingNeighbors &= ~(BitBoard::kingAttacks[wKing] | BitBoard::wPawnAttacks[wPawn] | rAtk);
    bool close;
    if (canWin) {
        close = (kMask & kingNeighbors) != kingNeighbors;
    } else {
        close = (kMask & kingNeighbors) != 0;
    }
    int score = pV + Position::getY(wPawn) * pV / 4;
    if (canWin) {
        if (!close)
            score += pV;
    } else {
        if (close)
            score /= 2;
        else
            score /= 4;
    }
    return score;
}
