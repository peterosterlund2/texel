/*
 * evaluate.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "evaluate.hpp"

int Evaluate::pieceValue[Piece::nPieceTypes] = {
    0,
    kV, qV, rV, bV, nV, pV,
    kV, qV, rV, bV, nV, pV
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

/** Piece/square table for bishops during middle game. */
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

/** Piece/square table for rooks during middle game. */
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

#if 0
    static {
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
    }
#endif

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
