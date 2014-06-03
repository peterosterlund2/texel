/*
    Texel - A UCI chess engine.
    Copyright (C) 2014  Peter Österlund, peterosterlund2@gmail.com

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
 * tbprobe.cpp
 *
 *  Created on: Jun 2, 2014
 *      Author: petero
 */

#include "tbprobe.hpp"
#include "gtb/gtb-probe.h"
#include "bitBoard.hpp"
#include "position.hpp"
#include "constants.hpp"
#include <cassert>


static bool isInitialized = false;
static const char** paths = NULL;
static int gtbMaxPieces = 0;

void
TBProbe::initialize(const std::string& path) {
    static_assert((int)tb_A1 == (int)A1, "Incompatible square numbering");
    static_assert((int)tb_A8 == (int)A8, "Incompatible square numbering");
    static_assert((int)tb_H1 == (int)H1, "Incompatible square numbering");
    static_assert((int)tb_H8 == (int)H8, "Incompatible square numbering");

    if (isInitialized && paths)
        tbpaths_done(paths);

    isInitialized = false;
    paths = tbpaths_init();
    if (paths == NULL)
        return;

    paths = tbpaths_add(paths, path.c_str());
    if (paths == NULL)
        return;

    TB_compression_scheme scheme = tb_CP4;
    int verbose = 0;
    int cacheSize = 4*1024*1024;
    int wdlFraction = 0;
    if (isInitialized) {
        tb_restart(verbose, scheme, paths);
        tbcache_restart(cacheSize, wdlFraction);
    } else {
        tb_init(verbose, scheme, paths);
        tbcache_init(cacheSize, wdlFraction);
    }
    isInitialized = true;

    unsigned int av = tb_availability();
    gtbMaxPieces = 0;
    if (av & 3)
        gtbMaxPieces = 3;
    if (av & 12)
        gtbMaxPieces = 4;
    if (av & 48)
        gtbMaxPieces = 5;
}

bool
TBProbe::gtbProbe(const Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    unsigned int stm = pos.getWhiteMove() ? tb_WHITE_TO_MOVE : tb_BLACK_TO_MOVE;
    unsigned int epsq = pos.getEpSquare() >= 0 ? pos.getEpSquare() : tb_NOSQUARE;

    unsigned int castles = 0;
    if (pos.a1Castle()) castles |= tb_WOOO;
    if (pos.h1Castle()) castles |= tb_WOO;
    if (pos.a8Castle()) castles |= tb_BOOO;
    if (pos.h8Castle()) castles |= tb_BOO;

    const int MAXLEN = 17;
    unsigned int  wSq[MAXLEN];
    unsigned int  bSq[MAXLEN];
    unsigned char wP[MAXLEN];
    unsigned char bP[MAXLEN];

    int cnt = 0;
    U64 m = pos.whiteBB();
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        wSq[cnt] = sq;
        switch (pos.getPiece(sq)) {
        case Piece::WKING:   wP[cnt] = tb_KING;   break;
        case Piece::WQUEEN:  wP[cnt] = tb_QUEEN;  break;
        case Piece::WROOK:   wP[cnt] = tb_ROOK;   break;
        case Piece::WBISHOP: wP[cnt] = tb_BISHOP; break;
        case Piece::WKNIGHT: wP[cnt] = tb_KNIGHT; break;
        case Piece::WPAWN:   wP[cnt] = tb_PAWN;   break;
        default:
            assert(false);
        }
        cnt++;
        m &= m-1;
    }
    wSq[cnt] = tb_NOSQUARE;
    wP[cnt] = tb_NOPIECE;

    cnt = 0;
    m = pos.blackBB();
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        bSq[cnt] = sq;
        switch (pos.getPiece(sq)) {
        case Piece::BKING:   bP[cnt] = tb_KING;   break;
        case Piece::BQUEEN:  bP[cnt] = tb_QUEEN;  break;
        case Piece::BROOK:   bP[cnt] = tb_ROOK;   break;
        case Piece::BBISHOP: bP[cnt] = tb_BISHOP; break;
        case Piece::BKNIGHT: bP[cnt] = tb_KNIGHT; break;
        case Piece::BPAWN:   bP[cnt] = tb_PAWN;   break;
        default:
            assert(false);
        }
        cnt++;
        m &= m-1;
    }
    bSq[cnt] = tb_NOSQUARE;
    bP[cnt] = tb_NOPIECE;

    unsigned int tbInfo;
    unsigned int plies;
    if (!tb_probe_hard(stm, epsq, castles, wSq, bSq, wP, bP, &tbInfo, &plies))
        return false;

    switch (tbInfo) {
    case tb_DRAW:
        score = 0;
        break;
    case tb_WMATE:
        score = SearchConst::MATE0 - ply - plies - 1;
        break;
    case tb_BMATE:
        score = -(SearchConst::MATE0 - ply - plies - 1);
        break;
    default:
        return false;
    };

    if (!pos.getWhiteMove())
        score = -score;

    return true;
}
