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
TBProbe::initialize(const std::string& path, int cacheMB) {
    gtbInitialize(path, cacheMB);
}

bool
TBProbe::tbProbe(const Position& pos, int ply, int alpha, int beta,
                 TranspositionTable::TTEntry& ent) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    GtbProbeData gtbData;
    getGTBProbeData(pos, gtbData);

    int score;
    if (SearchConst::isLoseScore(alpha) || SearchConst::isWinScore(beta)) {
        if (!gtbProbeDTM(gtbData, ply, score))
            return false;
        ent.setScore(score, ply);
        ent.setType(TType::T_EXACT);
        return true;
    }

    if (!gtbProbeWDL(gtbData, ply, score))
        return false;

    ent.setScore(score, ply);
    if (score > 0)
        ent.setType(TType::T_GE);
    else if (score < 0)
        ent.setType(TType::T_LE);
    else
        ent.setType(TType::T_EXACT);

    return true;
}

bool
TBProbe::gtbProbeDTM(const Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    GtbProbeData gtbData;
    getGTBProbeData(pos, gtbData);
    return gtbProbeDTM(gtbData, ply, score);
}

bool
TBProbe::gtbProbeWDL(const Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    GtbProbeData gtbData;
    getGTBProbeData(pos, gtbData);
    return gtbProbeWDL(gtbData, ply, score);
}


void
TBProbe::gtbInitialize(const std::string& path, int cacheMB) {
    static_assert((int)tb_A1 == (int)A1, "Incompatible square numbering");
    static_assert((int)tb_A8 == (int)A8, "Incompatible square numbering");
    static_assert((int)tb_H1 == (int)H1, "Incompatible square numbering");
    static_assert((int)tb_H8 == (int)H8, "Incompatible square numbering");

    if (isInitialized && paths)
        tbpaths_done(paths);

    isInitialized = false;
    gtbMaxPieces = 0;
    paths = tbpaths_init();
    if (paths == NULL)
        return;

    if (path.empty())
        return;

    paths = tbpaths_add(paths, path.c_str());
    if (paths == NULL)
        return;

    TB_compression_scheme scheme = tb_CP4;
    int verbose = 0;
    int cacheSize = 1024 * 1024 * cacheMB;
    int wdlFraction = 96;
    if (isInitialized) {
        tb_restart(verbose, scheme, paths);
        tbcache_restart(cacheSize, wdlFraction);
    } else {
        tb_init(verbose, scheme, paths);
        tbcache_init(cacheSize, wdlFraction);
    }
    isInitialized = true;

    unsigned int av = tb_availability();
    if (av & 3)
        gtbMaxPieces = 3;
    if (av & 12)
        gtbMaxPieces = 4;
    if (av & 48)
        gtbMaxPieces = 5;
}

void
TBProbe::getGTBProbeData(const Position& pos, GtbProbeData& gtbData) {
    gtbData.stm = pos.getWhiteMove() ? tb_WHITE_TO_MOVE : tb_BLACK_TO_MOVE;
    gtbData.epsq = pos.getEpSquare() >= 0 ? pos.getEpSquare() : tb_NOSQUARE;

    gtbData.castles = 0;
    if (pos.a1Castle()) gtbData.castles |= tb_WOOO;
    if (pos.h1Castle()) gtbData.castles |= tb_WOO;
    if (pos.a8Castle()) gtbData.castles |= tb_BOOO;
    if (pos.h8Castle()) gtbData.castles |= tb_BOO;

    int cnt = 0;
    U64 m = pos.whiteBB();
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        gtbData.wSq[cnt] = sq;
        switch (pos.getPiece(sq)) {
        case Piece::WKING:   gtbData.wP[cnt] = tb_KING;   break;
        case Piece::WQUEEN:  gtbData.wP[cnt] = tb_QUEEN;  break;
        case Piece::WROOK:   gtbData.wP[cnt] = tb_ROOK;   break;
        case Piece::WBISHOP: gtbData.wP[cnt] = tb_BISHOP; break;
        case Piece::WKNIGHT: gtbData.wP[cnt] = tb_KNIGHT; break;
        case Piece::WPAWN:   gtbData.wP[cnt] = tb_PAWN;   break;
        default:
            assert(false);
        }
        cnt++;
        m &= m-1;
    }
    gtbData.wSq[cnt] = tb_NOSQUARE;
    gtbData.wP[cnt] = tb_NOPIECE;

    cnt = 0;
    m = pos.blackBB();
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        gtbData.bSq[cnt] = sq;
        switch (pos.getPiece(sq)) {
        case Piece::BKING:   gtbData.bP[cnt] = tb_KING;   break;
        case Piece::BQUEEN:  gtbData.bP[cnt] = tb_QUEEN;  break;
        case Piece::BROOK:   gtbData.bP[cnt] = tb_ROOK;   break;
        case Piece::BBISHOP: gtbData.bP[cnt] = tb_BISHOP; break;
        case Piece::BKNIGHT: gtbData.bP[cnt] = tb_KNIGHT; break;
        case Piece::BPAWN:   gtbData.bP[cnt] = tb_PAWN;   break;
        default:
            assert(false);
        }
        cnt++;
        m &= m-1;
    }
    gtbData.bSq[cnt] = tb_NOSQUARE;
    gtbData.bP[cnt] = tb_NOPIECE;
}

bool
TBProbe::gtbProbeDTM(const GtbProbeData& gtbData, int ply, int& score) {
    unsigned int tbInfo;
    unsigned int plies;
    if (!tb_probe_hard(gtbData.stm, gtbData.epsq, gtbData.castles,
                       gtbData.wSq, gtbData.bSq,
                       gtbData.wP, gtbData.bP,
                       &tbInfo, &plies))
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

    if (gtbData.stm == tb_BLACK_TO_MOVE)
        score = -score;

    return true;
}

bool
TBProbe::gtbProbeWDL(const GtbProbeData& gtbData, int ply, int& score) {
    unsigned int tbInfo;
    if (!tb_probe_WDL_hard(gtbData.stm, gtbData.epsq, gtbData.castles,
                           gtbData.wSq, gtbData.bSq,
                           gtbData.wP, gtbData.bP,
                           &tbInfo))
        return false;

    const int longestWin = 256;
    switch (tbInfo) {
    case tb_DRAW:
        score = 0;
        break;
    case tb_WMATE:
        score = SearchConst::MATE0 - ply - longestWin - 1;
        break;
    case tb_BMATE:
        score = -(SearchConst::MATE0 - ply - longestWin - 1);
        break;
    default:
        return false;
    };

    if (gtbData.stm == tb_BLACK_TO_MOVE)
        score = -score;
    return true;
}
