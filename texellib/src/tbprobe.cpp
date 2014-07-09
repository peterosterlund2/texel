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
#include "syzygy/rtb-probe.hpp"
#include "bitBoard.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "constants.hpp"
#include <unordered_map>
#include <cassert>


static bool isInitialized = false;
static const char** paths = NULL;
static int gtbMaxPieces = 0;
static std::unordered_map<int,int> longestMate;

void
TBProbe::initializeGTB(const std::string& gtbPath, int cacheMB) {
    gtbInitialize(gtbPath, cacheMB);
    initWDLBounds();
}

void
TBProbe::initializeRTB(const std::string& rtbPath) {
    Syzygy::init(rtbPath);
    initWDLBounds();
}

bool
TBProbe::tbProbe(Position& pos, int ply, int alpha, int beta,
                 TranspositionTable::TTEntry& ent) {
    const int nPieces = BitBoard::bitCount(pos.occupiedBB());
    bool mateSearch = SearchConst::isLoseScore(alpha) || SearchConst::isWinScore(beta);
    if (!mateSearch && pos.getHalfMoveClock() == 0) { // WDL probe is enough
        int wdlScore;
        if ((nPieces <= Syzygy::TBLargest && rtbProbeWDL(pos, ply, wdlScore)) ||
            (nPieces <= gtbMaxPieces && gtbProbeWDL(pos, ply, wdlScore))) {
            ent.setScore(wdlScore, ply);
            if (wdlScore > 0)
                ent.setType(TType::T_GE);
            else if (wdlScore < 0)
                ent.setType(TType::T_LE);
            else
                ent.setType(TType::T_EXACT);
            return true;
        }
        return false;
    } else { // Need DTM or DTZ probe
        int dtmScore;
        bool hasDtm = false;
        if (nPieces <= gtbMaxPieces && gtbProbeDTM(pos, ply, dtmScore)) {
            if (SearchConst::MATE0 - 1 - abs(dtmScore) - ply <= 100 - pos.getHalfMoveClock()) {
                ent.setScore(dtmScore, ply);
                ent.setType(TType::T_EXACT);
                return true;
            }
            hasDtm = true;
        }
        int dtzScore;
        if (nPieces <= Syzygy::TBLargest && rtbProbeDTZ(pos, ply, dtzScore)) {
            ent.setScore(dtzScore, ply);
            if (dtzScore > 0)
                ent.setType(TType::T_GE);
            else if (dtzScore < 0)
                ent.setType(TType::T_LE);
            else
                ent.setType(TType::T_EXACT);
            return true;
        }
        if (hasDtm) {
            ent.setScore(dtmScore, ply);
            ent.setType(TType::T_EXACT);
            return true;
        }
        return false;
    }
}

template <typename ProbeFunc>
static void handleEP(Position& pos, int ply, int& score, bool& ret, ProbeFunc probeFunc) {
    const bool inCheck = MoveGen::inCheck(pos);
    MoveGen::MoveList moveList;
    if (inCheck) MoveGen::checkEvasions(pos, moveList);
    else         MoveGen::pseudoLegalMoves(pos, moveList);
    const int pawn = pos.getWhiteMove() ? Piece::WPAWN : Piece::BPAWN;
    int bestEP = std::numeric_limits<int>::min();
    UndoInfo ui;
    for (int m = 0; m < moveList.size; m++) {
        const Move& move = moveList[m];
        if ((move.to() == pos.getEpSquare()) && (pos.getPiece(move.from()) == pawn)) {
            if (MoveGen::isLegal(pos, move, inCheck)) {
                pos.makeMove(move, ui);
                int score2;
                bool ret2 = probeFunc(pos, ply+1, score2);
                pos.unMakeMove(move, ui);
                if (!ret2) {
                    ret = false;
                    return;
                }
                bestEP = std::max(bestEP, -score2);
            }
        } else if (MoveGen::isLegal(pos, move, inCheck))
            return;
    }
    if (bestEP != std::numeric_limits<int>::min())
        score = bestEP;
}

bool
TBProbe::gtbProbeDTM(Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    GtbProbeData gtbData;
    getGTBProbeData(pos, gtbData);
    bool ret = gtbProbeDTM(gtbData, ply, score);
    if (ret && score == 0 && pos.getEpSquare() != -1)
        handleEP(pos, ply, score, ret, [](Position& pos, int ply, int& score) -> bool {
            return TBProbe::gtbProbeDTM(pos, ply, score);
        });
    return ret;
}

bool
TBProbe::gtbProbeWDL(Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    GtbProbeData gtbData;
    getGTBProbeData(pos, gtbData);
    bool ret = gtbProbeWDL(gtbData, ply, score);
    if (ret && score == 0 && pos.getEpSquare() != -1)
        handleEP(pos, ply, score, ret, [](Position& pos, int ply, int& score) -> bool {
            return TBProbe::gtbProbeWDL(pos, ply, score);
        });
    return ret;
}

static int
maxZeroing(const Position& pos, int nPieces) {
    bool singleMatePiece = pos.pieceTypeBB(Piece::WQUEEN, Piece::WROOK, Piece::WPAWN,
                                           Piece::BQUEEN, Piece::BROOK, Piece::BPAWN);
    int maxCapt = singleMatePiece ? nPieces - 3 : nPieces - 4;
    int maxPawnMoves = 0;
    U64 m = pos.pieceTypeBB(Piece::WPAWN);
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        maxPawnMoves += 7 - Position::getY(sq);
        m &= m-1;
    }
    m = pos.pieceTypeBB(Piece::BPAWN);
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        maxPawnMoves += Position::getY(sq);
        m &= m-1;
    }
    return maxCapt + maxPawnMoves;
}

bool
TBProbe::rtbProbeDTZ(Position& pos, int ply, int& score) {
    const int nPieces = BitBoard::bitCount(pos.occupiedBB());
    if (nPieces > Syzygy::TBLargest)
        return false;
    if (pos.getCastleMask())
        return false;

    int success;
    const int dtz = Syzygy::probe_dtz(pos, &success);
    if (!success)
        return false;
    if (dtz == 0) {
        score = 0;
        return true;
    }
    const int maxHalfMoveClock = std::abs(dtz) + pos.getHalfMoveClock();
    if (abs(dtz) <= 2) {
        if (maxHalfMoveClock > 101) {
            score = 0;
            return true;
        } else if (maxHalfMoveClock == 101)
            return false; // DTZ can be wrong when mate-in-1
    } else {
        if (maxHalfMoveClock > 100) {
            score = 0;
            return true;
        }
        // FIXME!! Are there positions where maxHalfMoveclock==101 needs special handling?
    }
    int maxZero = maxZeroing(pos, nPieces);
    int plyToMate = maxZero * 100 + std::abs(dtz);
    if (dtz > 0) {
        score = SearchConst::MATE0 - ply - plyToMate - 2;
    } else {
        score = -(SearchConst::MATE0 - ply - plyToMate - 2);
    }
    return true;
}

bool
TBProbe::rtbProbeWDL(Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > Syzygy::TBLargest)
        return false;
    if (pos.getCastleMask())
        return false;

    int success;
    int wdl = Syzygy::probe_wdl(pos, &success);
    if (!success)
        return false;
    int nPieces, maxZero, plyToMate;
    switch (wdl) {
    case 0: case 1: case -1:
        score = 0;
        break;
    case 2:
        nPieces = BitBoard::bitCount(pos.occupiedBB());
        maxZero = maxZeroing(pos, nPieces);
        plyToMate = (maxZero + 1) * 100;
        score = SearchConst::MATE0 - ply - plyToMate - 2;
        break;
    case -2:
        nPieces = BitBoard::bitCount(pos.occupiedBB());
        maxZero = maxZeroing(pos, nPieces);
        plyToMate = (maxZero + 1) * 100;
        score = -(SearchConst::MATE0 - ply - plyToMate - 2);
        break;
    default:
        return false;
    }

    return true;
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
    gtbData.materialId = pos.materialId();
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

void
TBProbe::initWDLBounds() {
    typedef MatId MI;
    longestMate[MI::WQ+0] = longestMate[MI::BQ+0] = 31979;
    longestMate[MI::WR+0] = longestMate[MI::BR+0] = 31967;
    longestMate[MI::WP+0] = longestMate[MI::BP+0] = 31943;

    longestMate[MI::WQ*2] = longestMate[MI::BQ*2] = 31979;
    longestMate[MI::WQ+MI::WR] = longestMate[MI::BQ+MI::BR] = 31967;
    longestMate[MI::WQ+MI::WB] = longestMate[MI::BQ+MI::BB] = 31979;
    longestMate[MI::WQ+MI::WN] = longestMate[MI::BQ+MI::BN] = 31979;
    longestMate[MI::WQ+MI::WP] = longestMate[MI::BQ+MI::BP] = 31943;
    longestMate[MI::WR*2] = longestMate[MI::BR*2] = 31967;
    longestMate[MI::WR+MI::WB] = longestMate[MI::BR+MI::BB] = 31967;
    longestMate[MI::WR+MI::WN] = longestMate[MI::BR+MI::BN] = 31967;
    longestMate[MI::WR+MI::WP] = longestMate[MI::BR+MI::BP] = 31943;
    longestMate[MI::WB*2] = longestMate[MI::BB*2] = 31961;
    longestMate[MI::WB+MI::WN] = longestMate[MI::BB+MI::BN] = 31933;
    longestMate[MI::WB+MI::WP] = longestMate[MI::BB+MI::BP] = 31937;
    longestMate[MI::WN*2] = longestMate[MI::BN*2] = 31998;
    longestMate[MI::WN+MI::WP] = longestMate[MI::BN+MI::BP] = 31943;
    longestMate[MI::WP*2] = longestMate[MI::BP*2] = 31935;
    longestMate[MI::WQ+MI::BQ] = 31974;
    longestMate[MI::WR+MI::BQ] = longestMate[MI::WQ+MI::BR] = 31929;
    longestMate[MI::WR+MI::BR] = 31961;
    longestMate[MI::WB+MI::BQ] = longestMate[MI::WQ+MI::BB] = 31965;
    longestMate[MI::WB+MI::BR] = longestMate[MI::WR+MI::BB] = 31941;
    longestMate[MI::WB+MI::BB] = 31998;
    longestMate[MI::WN+MI::BQ] = longestMate[MI::WQ+MI::BN] = 31957;
    longestMate[MI::WN+MI::BR] = longestMate[MI::WR+MI::BN] = 31919;
    longestMate[MI::WN+MI::BB] = longestMate[MI::WB+MI::BN] = 31998;
    longestMate[MI::WN+MI::BN] = 31998;
    longestMate[MI::WP+MI::BQ] = longestMate[MI::WQ+MI::BP] = 31942;
    longestMate[MI::WP+MI::BR] = longestMate[MI::WR+MI::BP] = 31914;
    longestMate[MI::WP+MI::BB] = longestMate[MI::WB+MI::BP] = 31942;
    longestMate[MI::WP+MI::BN] = longestMate[MI::WN+MI::BP] = 31942;
    longestMate[MI::WP+MI::BP] = 31933;

    longestMate[MI::WQ*3] = longestMate[MI::BQ*3] = 31991;
    longestMate[MI::WQ*2+MI::WR] = longestMate[MI::BQ*2+MI::BR] = 31987;
    longestMate[MI::WQ*2+MI::WB] = longestMate[MI::BQ*2+MI::BB] = 31983;
    longestMate[MI::WQ*2+MI::WN] = longestMate[MI::BQ*2+MI::BN] = 31981;
    longestMate[MI::WQ*2+MI::WP] = longestMate[MI::BQ*2+MI::BP] = 31979;
    longestMate[MI::WQ+MI::WR*2] = longestMate[MI::BQ+MI::BR*2] = 31985;
    longestMate[MI::WQ+MI::WR+MI::WB] = longestMate[MI::BQ+MI::BR+MI::BB] = 31967;
    longestMate[MI::WQ+MI::WR+MI::WN] = longestMate[MI::BQ+MI::BR+MI::BN] = 31967;
    longestMate[MI::WQ+MI::WR+MI::WP] = longestMate[MI::BQ+MI::BR+MI::BP] = 31967;
    longestMate[MI::WQ+MI::WB*2] = longestMate[MI::BQ+MI::BB*2] = 31961;
    longestMate[MI::WQ+MI::WB+MI::WN] = longestMate[MI::BQ+MI::BB+MI::BN] = 31933;
    longestMate[MI::WQ+MI::WB+MI::WP] = longestMate[MI::BQ+MI::BB+MI::BP] = 31937;
    longestMate[MI::WQ+MI::WN*2] = longestMate[MI::BQ+MI::BN*2] = 31981;
    longestMate[MI::WQ+MI::WN+MI::WP] = longestMate[MI::BQ+MI::BN+MI::BP] = 31945;
    longestMate[MI::WQ+MI::WP*2] = longestMate[MI::BQ+MI::BP*2] = 31935;
    longestMate[MI::WR*3] = longestMate[MI::BR*3] = 31985;
    longestMate[MI::WR*2+MI::WB] = longestMate[MI::BR*2+MI::BB] = 31967;
    longestMate[MI::WR*2+MI::WN] = longestMate[MI::BR*2+MI::BN] = 31967;
    longestMate[MI::WR*2+MI::WP] = longestMate[MI::BR*2+MI::BP] = 31967;
    longestMate[MI::WR+MI::WB*2] = longestMate[MI::BR+MI::BB*2] = 31961;
    longestMate[MI::WR+MI::WB+MI::WN] = longestMate[MI::BR+MI::BB+MI::BN] = 31933;
    longestMate[MI::WR+MI::WB+MI::WP] = longestMate[MI::BR+MI::BB+MI::BP] = 31937;
    longestMate[MI::WR+MI::WN*2] = longestMate[MI::BR+MI::BN*2] = 31967;
    longestMate[MI::WR+MI::WN+MI::WP] = longestMate[MI::BR+MI::BN+MI::BP] = 31945;
    longestMate[MI::WR+MI::WP*2] = longestMate[MI::BR+MI::BP*2] = 31935;
    longestMate[MI::WB*3] = longestMate[MI::BB*3] = 31961;
    longestMate[MI::WB*2+MI::WN] = longestMate[MI::BB*2+MI::BN] = 31933;
    longestMate[MI::WB*2+MI::WP] = longestMate[MI::BB*2+MI::BP] = 31937;
    longestMate[MI::WB+MI::WN*2] = longestMate[MI::BB+MI::BN*2] = 31931;
    longestMate[MI::WB+MI::WN+MI::WP] = longestMate[MI::BB+MI::BN+MI::BP] = 31933;
    longestMate[MI::WB+MI::WP*2] = longestMate[MI::BB+MI::BP*2] = 31935;
    longestMate[MI::WN*3] = longestMate[MI::BN*3] = 31957;
    longestMate[MI::WN*2+MI::WP] = longestMate[MI::BN*2+MI::BP] = 31943;
    longestMate[MI::WN+MI::WP*2] = longestMate[MI::BN+MI::BP*2] = 31935;
    longestMate[MI::WP*3] = longestMate[MI::BP*3] = 31933;
    longestMate[MI::WQ*2+MI::BQ] = longestMate[MI::WQ+MI::BQ*2] = 31939;
    longestMate[MI::WQ*2+MI::BR] = longestMate[MI::WR+MI::BQ*2] = 31929;
    longestMate[MI::WQ*2+MI::BB] = longestMate[MI::WB+MI::BQ*2] = 31965;
    longestMate[MI::WQ*2+MI::BN] = longestMate[MI::WN+MI::BQ*2] = 31957;
    longestMate[MI::WQ*2+MI::BP] = longestMate[MI::WP+MI::BQ*2] = 31939;
    longestMate[MI::WQ+MI::WR+MI::BQ] = longestMate[MI::WQ+MI::BQ+MI::BR] = 31865;
    longestMate[MI::WQ+MI::WR+MI::BR] = longestMate[MI::WR+MI::BQ+MI::BR] = 31929;
    longestMate[MI::WQ+MI::WR+MI::BB] = longestMate[MI::WB+MI::BQ+MI::BR] = 31941;
    longestMate[MI::WQ+MI::WR+MI::BN] = longestMate[MI::WN+MI::BQ+MI::BR] = 31919;
    longestMate[MI::WQ+MI::WR+MI::BP] = longestMate[MI::WP+MI::BQ+MI::BR] = 31865;
    longestMate[MI::WQ+MI::WB+MI::BQ] = longestMate[MI::WQ+MI::BQ+MI::BB] = 31933;
    longestMate[MI::WQ+MI::WB+MI::BR] = longestMate[MI::WR+MI::BQ+MI::BB] = 31919;
    longestMate[MI::WQ+MI::WB+MI::BB] = longestMate[MI::WB+MI::BQ+MI::BB] = 31965;
    longestMate[MI::WQ+MI::WB+MI::BN] = longestMate[MI::WN+MI::BQ+MI::BB] = 31957;
    longestMate[MI::WQ+MI::WB+MI::BP] = longestMate[MI::WP+MI::BQ+MI::BB] = 31933;
    longestMate[MI::WQ+MI::WN+MI::BQ] = longestMate[MI::WQ+MI::BQ+MI::BN] = 31917;
    longestMate[MI::WQ+MI::WN+MI::BR] = longestMate[MI::WR+MI::BQ+MI::BN] = 31918;
    longestMate[MI::WQ+MI::WN+MI::BB] = longestMate[MI::WB+MI::BQ+MI::BN] = 31965;
    longestMate[MI::WQ+MI::WN+MI::BN] = longestMate[MI::WN+MI::BQ+MI::BN] = 31957;
    longestMate[MI::WQ+MI::WN+MI::BP] = longestMate[MI::WP+MI::BQ+MI::BN] = 31917;
    longestMate[MI::WQ+MI::WP+MI::BQ] = longestMate[MI::WQ+MI::BQ+MI::BP] = 31752;
    longestMate[MI::WQ+MI::WP+MI::BR] = longestMate[MI::WR+MI::BQ+MI::BP] = 31913;
    longestMate[MI::WQ+MI::WP+MI::BB] = longestMate[MI::WB+MI::BQ+MI::BP] = 31941;
    longestMate[MI::WQ+MI::WP+MI::BN] = longestMate[MI::WN+MI::BQ+MI::BP] = 31939;
    longestMate[MI::WQ+MI::WP+MI::BP] = longestMate[MI::WP+MI::BQ+MI::BP] = 31755;
    longestMate[MI::WR*2+MI::BQ] = longestMate[MI::WQ+MI::BR*2] = 31901;
    longestMate[MI::WR*2+MI::BR] = longestMate[MI::WR+MI::BR*2] = 31937;
    longestMate[MI::WR*2+MI::BB] = longestMate[MI::WB+MI::BR*2] = 31941;
    longestMate[MI::WR*2+MI::BN] = longestMate[MI::WN+MI::BR*2] = 31919;
    longestMate[MI::WR*2+MI::BP] = longestMate[MI::WP+MI::BR*2] = 31900;
    longestMate[MI::WR+MI::WB+MI::BQ] = longestMate[MI::WQ+MI::BR+MI::BB] = 31859;
    longestMate[MI::WR+MI::WB+MI::BR] = longestMate[MI::WR+MI::BR+MI::BB] = 31870;
    longestMate[MI::WR+MI::WB+MI::BB] = longestMate[MI::WB+MI::BR+MI::BB] = 31939;
    longestMate[MI::WR+MI::WB+MI::BN] = longestMate[MI::WN+MI::BR+MI::BB] = 31919;
    longestMate[MI::WR+MI::WB+MI::BP] = longestMate[MI::WP+MI::BR+MI::BB] = 31860;
    longestMate[MI::WR+MI::WN+MI::BQ] = longestMate[MI::WQ+MI::BR+MI::BN] = 31861;
    longestMate[MI::WR+MI::WN+MI::BR] = longestMate[MI::WR+MI::BR+MI::BN] = 31918;
    longestMate[MI::WR+MI::WN+MI::BB] = longestMate[MI::WB+MI::BR+MI::BN] = 31937;
    longestMate[MI::WR+MI::WN+MI::BN] = longestMate[MI::WN+MI::BR+MI::BN] = 31919;
    longestMate[MI::WR+MI::WN+MI::BP] = longestMate[MI::WP+MI::BR+MI::BN] = 31864;
    longestMate[MI::WR+MI::WP+MI::BQ] = longestMate[MI::WQ+MI::BR+MI::BP] = 31792;
    longestMate[MI::WR+MI::WP+MI::BR] = longestMate[MI::WR+MI::BR+MI::BP] = 31851;
    longestMate[MI::WR+MI::WP+MI::BB] = longestMate[MI::WB+MI::BR+MI::BP] = 31853;
    longestMate[MI::WR+MI::WP+MI::BN] = longestMate[MI::WN+MI::BR+MI::BP] = 31891;
    longestMate[MI::WR+MI::WP+MI::BP] = longestMate[MI::WP+MI::BR+MI::BP] = 31794;
    longestMate[MI::WB*2+MI::BQ] = longestMate[MI::WQ+MI::BB*2] = 31837;
    longestMate[MI::WB*2+MI::BR] = longestMate[MI::WR+MI::BB*2] = 31938;
    longestMate[MI::WB*2+MI::BB] = longestMate[MI::WB+MI::BB*2] = 31955;
    longestMate[MI::WB*2+MI::BN] = longestMate[MI::WN+MI::BB*2] = 31843;
    longestMate[MI::WB*2+MI::BP] = longestMate[MI::WP+MI::BB*2] = 31834;
    longestMate[MI::WB+MI::WN+MI::BQ] = longestMate[MI::WQ+MI::BB+MI::BN] = 31893;
    longestMate[MI::WB+MI::WN+MI::BR] = longestMate[MI::WR+MI::BB+MI::BN] = 31918;
    longestMate[MI::WB+MI::WN+MI::BB] = longestMate[MI::WB+MI::BB+MI::BN] = 31921;
    longestMate[MI::WB+MI::WN+MI::BN] = longestMate[MI::WN+MI::BB+MI::BN] = 31786;
    longestMate[MI::WB+MI::WN+MI::BP] = longestMate[MI::WP+MI::BB+MI::BN] = 31791;
    longestMate[MI::WB+MI::WP+MI::BQ] = longestMate[MI::WQ+MI::BB+MI::BP] = 31899;
    longestMate[MI::WB+MI::WP+MI::BR] = longestMate[MI::WR+MI::BB+MI::BP] = 31910;
    longestMate[MI::WB+MI::WP+MI::BB] = longestMate[MI::WB+MI::BB+MI::BP] = 31898;
    longestMate[MI::WB+MI::WP+MI::BN] = longestMate[MI::WN+MI::BB+MI::BP] = 31800;
    longestMate[MI::WB+MI::WP+MI::BP] = longestMate[MI::WP+MI::BB+MI::BP] = 31865;
    longestMate[MI::WN*2+MI::BQ] = longestMate[MI::WQ+MI::BN*2] = 31855;
    longestMate[MI::WN*2+MI::BR] = longestMate[MI::WR+MI::BN*2] = 31918;
    longestMate[MI::WN*2+MI::BB] = longestMate[MI::WB+MI::BN*2] = 31992;
    longestMate[MI::WN*2+MI::BN] = longestMate[MI::WN+MI::BN*2] = 31986;
    longestMate[MI::WN*2+MI::BP] = longestMate[MI::WP+MI::BN*2] = 31770;
    longestMate[MI::WN+MI::WP+MI::BQ] = longestMate[MI::WQ+MI::BN+MI::BP] = 31875;
    longestMate[MI::WN+MI::WP+MI::BR] = longestMate[MI::WR+MI::BN+MI::BP] = 31866;
    longestMate[MI::WN+MI::WP+MI::BB] = longestMate[MI::WB+MI::BN+MI::BP] = 31914;
    longestMate[MI::WN+MI::WP+MI::BN] = longestMate[MI::WN+MI::BN+MI::BP] = 31805;
    longestMate[MI::WN+MI::WP+MI::BP] = longestMate[MI::WP+MI::BN+MI::BP] = 31884;
    longestMate[MI::WP*2+MI::BQ] = longestMate[MI::WQ+MI::BP*2] = 31752;
    longestMate[MI::WP*2+MI::BR] = longestMate[MI::WR+MI::BP*2] = 31892;
    longestMate[MI::WP*2+MI::BB] = longestMate[MI::WB+MI::BP*2] = 31913;
    longestMate[MI::WP*2+MI::BN] = longestMate[MI::WN+MI::BP*2] = 31899;
    longestMate[MI::WP*2+MI::BP] = longestMate[MI::WP+MI::BP*2] = 31745;
}

bool
TBProbe::gtbProbeWDL(const GtbProbeData& gtbData, int ply, int& score) {
    unsigned int tbInfo;
    if (!tb_probe_WDL_hard(gtbData.stm, gtbData.epsq, gtbData.castles,
                           gtbData.wSq, gtbData.bSq,
                           gtbData.wP, gtbData.bP,
                           &tbInfo))
        return false;

    switch (tbInfo) {
    case tb_DRAW:
        score = 0;
        break;
    case tb_WMATE:
        score = longestMate[gtbData.materialId] - ply;
        break;
    case tb_BMATE:
        score = -(longestMate[gtbData.materialId] - ply);
        break;
    default:
        return false;
    };

    if (gtbData.stm == tb_BLACK_TO_MOVE)
        score = -score;
    return true;
}
