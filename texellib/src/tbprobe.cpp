/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2016  Peter Österlund, peterosterlund2@gmail.com

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

#include "util/timeUtil.hpp"

static std::string currentGtbPath;
static int currentGtbCacheMB;
static int currentGtbWdlFraction;
static std::string currentRtbPath;

static const char** gtbPaths = nullptr;
static int gtbMaxPieces = 0;

int TBProbeData::maxPieces = 0;

static std::unordered_map<int,int> maxDTM; // MatId -> Max DTM value in GTB TB
static std::unordered_map<int,int> maxDTZ; // MatId -> Max DTZ value in RTB TB
struct IIPairHash {
    size_t operator()(const std::pair<int,int>& p) const {
        return ((U64)p.first) * 0x714d3559 + (U64)p.second;
    }
};
// (MatId,maxPawnMoves) -> Max DTM in sub TBs
static std::unordered_map<std::pair<int,int>,int,IIPairHash> maxSubDTM;


void
TBProbe::initialize(const std::string& gtbPath, int cacheMB,
                    const std::string& rtbPath) {
    if (rtbPath != currentRtbPath) {
        Syzygy::init(rtbPath);
        currentRtbPath = rtbPath;
    }

    int wdlFraction = Syzygy::TBLargest >= gtbMaxPieces ? 8 : 96;
    if ((gtbPath != currentGtbPath) ||
        (cacheMB != currentGtbCacheMB) ||
        (wdlFraction != currentGtbWdlFraction)) {
        gtbInitialize(gtbPath, cacheMB, wdlFraction);
        currentGtbPath = gtbPath;
        currentGtbCacheMB = cacheMB;
        currentGtbWdlFraction = wdlFraction;
    }

    static bool initialized = false;
    if (!initialized) {
        initWDLBounds();
        initialized = true;
    }

    TBProbeData::maxPieces = std::max({4, gtbMaxPieces, Syzygy::TBLargest});
}

bool
TBProbe::tbEnabled() {
    return Syzygy::TBLargest > 0 || gtbMaxPieces > 0;
}

const int maxFrustratedDist = 1000;

static inline void updateEvScore(TranspositionTable::TTEntry& ent,
                                 int newScore) {
    int oldScore = ent.getEvalScore();
    if ((oldScore == 0) || (std::abs(newScore) < std::abs(oldScore)))
        ent.setEvalScore(newScore);
}

/**
 * Return the margin (in number of plies) for a win to turn into a draw
 * because of the 50 move rule. If the margin is negative the position is
 * a draw, and ent.evalScore is set to indicate how far away from a win
 * the position is.
 */
static inline int rule50Margin(int dtmScore, int ply, int hmc,
                               TranspositionTable::TTEntry& ent) {
    int margin = (100 - hmc) - (SearchConst::MATE0 - 1 - abs(dtmScore) - ply);
    if (margin < 0)
        updateEvScore(ent, dtmScore > 0 ? -margin : margin);
    return margin;
}

bool
TBProbe::tbProbe(Position& pos, int ply, int alpha, int beta,
                 const TranspositionTable& tt, TranspositionTable::TTEntry& ent,
                 const int nPieces) {
    // Probe on-demand TB
    const int hmc = pos.getHalfMoveClock();
    bool hasDtm = false;
    int dtmScore;
    if (nPieces <= 4 && tt.probeDTM(pos, ply, dtmScore)) {
        if ((dtmScore == 0) || (rule50Margin(dtmScore, ply, hmc, ent) >= 0)) {
            ent.setScore(dtmScore, ply);
            ent.setType(TType::T_EXACT);
            return true;
        }
        ent.setScore(0, ply);
        ent.setType(dtmScore > 0 ? TType::T_GE : TType::T_LE);
        hasDtm = true;
    }

    // Try WDL probe. If the result is not draw, it can only be trusted if hmc == 0.
    // 5-men GTB WDL probes can only be trusted if the score is draw, because they
    // don't take the 50 move draw rule into account.
    bool hasResult = false;
    bool checkABBound = false;
    int wdlScore;
    if (nPieces <= Syzygy::TBLargest && rtbProbeWDL(pos, ply, wdlScore, ent)) {
        if ((wdlScore == 0) || (hmc == 0))
            hasResult = true;
        else
            checkABBound = true;
    } else if (nPieces <= gtbMaxPieces && gtbProbeWDL(pos, ply, wdlScore)) {
        if ((wdlScore == 0) || (hmc == 0 && nPieces <= 4))
            hasResult = true;
        else
            checkABBound = true;
    }
    if (checkABBound) {
        if ((wdlScore > 0) && (beta <= 0)) { // WDL says win but could be draw due to 50move rule
            ent.setScore(0, ply);
            ent.setType(TType::T_GE);
            return true;
        }
        if ((wdlScore < 0) && (alpha >= 0)) { // WDL says loss but could be draw due to 50move rule
            ent.setScore(0, ply);
            ent.setType(TType::T_LE);
            return true;
        }
    }
    bool frustrated = false;
    if (hasResult) {
        ent.setScore(wdlScore, ply);
        if (wdlScore > 0) {
            ent.setType(TType::T_GE);
            if (wdlScore >= beta)
                return true;
        } else if (wdlScore < 0) {
            ent.setType(TType::T_LE);
            if (wdlScore <= alpha)
                return true;
        } else {
            ent.setType(TType::T_EXACT);
            int evScore = ent.getEvalScore();
            if (evScore == 0) {
                return true;
            } else if (evScore > 0) {
                if (beta <= SearchConst::minFrustrated)
                    return true;
                frustrated = true;
            } else {
                if (alpha >= -SearchConst::minFrustrated)
                    return true;
                frustrated = true;
            }
        }
    }

    const bool dtmFirst = frustrated || SearchConst::isLoseScore(alpha) || SearchConst::isWinScore(beta);
    // Try GTB DTM probe if searching for fastest mate
    if (dtmFirst && !hasDtm && nPieces <= gtbMaxPieces) {
        if (gtbProbeDTM(pos, ply, dtmScore)) {
            if ((dtmScore == 0) || (rule50Margin(dtmScore, ply, hmc, ent) >= 0)) {
                ent.setScore(dtmScore, ply);
                ent.setType(TType::T_EXACT);
                return true;
            }
            ent.setScore(0, ply);
            ent.setType(dtmScore > 0 ? TType::T_GE : TType::T_LE);
            hasDtm = true;
        }
    }

    // Try RTB DTZ probe
    int dtzScore;
    if (nPieces <= Syzygy::TBLargest && rtbProbeDTZ(pos, ply, dtzScore, ent)) {
        hasResult = true;
        ent.setScore(dtzScore, ply);
        if (dtzScore > 0) {
            ent.setType(TType::T_GE);
            if (dtzScore >= beta)
                return true;
        } else if (dtzScore < 0) {
            ent.setType(TType::T_LE);
            if (dtzScore <= alpha)
                return true;
        } else {
            ent.setType(TType::T_EXACT);
            return true;
        }
    }

    // Try GTB DTM probe if not searching for fastest mate
    if (!dtmFirst && !hasDtm && nPieces <= gtbMaxPieces) {
        if (gtbProbeDTM(pos, ply, dtmScore)) {
            if ((dtmScore == 0) || (rule50Margin(dtmScore, ply, hmc, ent) >= 0)) {
                ent.setScore(dtmScore, ply);
                ent.setType(TType::T_EXACT);
                return true;
            }
            ent.setScore(0, ply);
            ent.setType(dtmScore > 0 ? TType::T_GE : TType::T_LE);
            hasDtm = true;
        }
    }

    return hasResult || hasDtm;
}

bool
TBProbe::getSearchMoves(Position& pos, const MoveList& legalMoves,
                        std::vector<Move>& movesToSearch,
                        const TranspositionTable& tt) {
    const int mate0 = SearchConst::MATE0;
    const int ply = 0;
    TranspositionTable::TTEntry rootEnt;
    if (!tbProbe(pos, ply, -mate0, mate0, tt, rootEnt) || rootEnt.getType() == TType::T_LE)
        return false;
    const int rootScore = rootEnt.getScore(ply);
    if (!SearchConst::isWinScore(rootScore))
        return false;

    // Root position is TB win
    bool hasProgress = false;
    UndoInfo ui;
    for (int mi = 0; mi < legalMoves.size; mi++) {
        const Move& m = legalMoves[mi];
        pos.makeMove(m, ui);
        TranspositionTable::TTEntry ent;
        bool progressMove = false;
        bool badMove = false;
        if (tbProbe(pos, ply+1, -mate0, mate0, tt, ent)) {
            const int type = ent.getType();
            const int score = -ent.getScore(ply+1);
            if (score >= rootScore && (type == TType::T_EXACT || type == TType::T_LE))
                progressMove = true;
            if ((score < rootScore - 1)) // -1 to handle +/-1 uncertainty in RTB tables
                badMove = true;
        }
        if (progressMove)
            hasProgress = true;
        if (!badMove)
            movesToSearch.push_back(m);
        pos.unMakeMove(m, ui);
    }

    return !hasProgress && !movesToSearch.empty();
}

bool
TBProbe::dtmProbe(Position& pos, int ply, const TranspositionTable& tt, int& score) {
    const int nPieces = BitBoard::bitCount(pos.occupiedBB());
    if (nPieces <= 4 && tt.probeDTM(pos, ply, score))
        return true;
    if (TBProbe::gtbProbeDTM(pos, ply, score))
        return true;
    return false;
}

void
TBProbe::extendPV(const Position& rootPos, std::vector<Move>& pv, const TranspositionTable& tt) {
    Position pos(rootPos);
    UndoInfo ui;
    int ply = 0;
    int score;
    for (int i = 0; i < (int)pv.size(); i++) {
        const Move& m = pv[i];
        pos.makeMove(m, ui);
        if (dtmProbe(pos, ply, tt, score) && SearchConst::isWinScore(std::abs(score)) &&
            (SearchConst::MATE0 - 1 - abs(score) - ply <= 100 - pos.getHalfMoveClock())) {
            // TB win, replace rest of PV since it may be inaccurate
            pv.erase(pv.begin()+i+1, pv.end());
            break;
        }
    }
    if (!dtmProbe(pos, ply, tt, score) || !SearchConst::isWinScore(std::abs(score)))
        return; // No TB win
    if (SearchConst::MATE0 - 1 - abs(score) - ply > 100 - pos.getHalfMoveClock())
        return; // Mate too far away, perhaps 50-move draw
    if (!pos.isWhiteMove())
        score = -score;
    while (true) {
        MoveList moveList;
        MoveGen::pseudoLegalMoves(pos, moveList);
        MoveGen::removeIllegal(pos, moveList);
        bool extended = false;
        for (int mi = 0; mi < moveList.size; mi++) {
            const Move& m = moveList[mi];
            pos.makeMove(m, ui);
            int newScore;
            if (dtmProbe(pos, ply+1, tt, newScore)) {
                if (!pos.isWhiteMove())
                    newScore = -newScore;
                if (newScore == score) {
                    pv.push_back(m);
                    ply++;
                    extended = true;
                    break;
                }
            }
            pos.unMakeMove(m, ui);
        }
        if (!extended)
            break;
    }
}

template <typename ProbeFunc>
static void handleEP(Position& pos, int ply, int& score, bool& ret, ProbeFunc probeFunc) {
    const bool inCheck = MoveGen::inCheck(pos);
    MoveList moveList;
    if (inCheck) MoveGen::checkEvasions(pos, moveList);
    else         MoveGen::pseudoLegalMoves(pos, moveList);
    const int pawn = pos.isWhiteMove() ? Piece::WPAWN : Piece::BPAWN;
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

bool
TBProbe::rtbProbeDTZ(Position& pos, int ply, int& score,
                     TranspositionTable::TTEntry& ent) {
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
        ent.setEvalScore(0);
        return true;
    }
    const int maxHalfMoveClock = std::abs(dtz) + pos.getHalfMoveClock();
    const int sgn = dtz > 0 ? 1 : -1;
    if ((maxHalfMoveClock == 100) && (pos.getHalfMoveClock() > 0) &&
            approxDTZ(pos.materialId())) // DTZ can be off by one
        return false;
    if (abs(dtz) <= 2) {
        if (maxHalfMoveClock > 101) {
            score = 0;
            updateEvScore(ent, sgn * (maxHalfMoveClock - 100));
            return true;
        } else if (maxHalfMoveClock == 101)
            return false; // DTZ can be wrong when mate-in-1
    } else {
        if (maxHalfMoveClock > 100) {
            score = 0;
            if (std::abs(dtz) <= 100)
                updateEvScore(ent, sgn * (maxHalfMoveClock - 100));
            else
                updateEvScore(ent, sgn * maxFrustratedDist);
            return true;
        }
    }
    int plyToMate = getMaxSubMate(pos) + std::abs(dtz);
    if (dtz > 0) {
        score = SearchConst::MATE0 - ply - plyToMate - 2;
    } else {
        score = -(SearchConst::MATE0 - ply - plyToMate - 2);
    }
    return true;
}

bool
TBProbe::rtbProbeWDL(Position& pos, int ply, int& score,
                     TranspositionTable::TTEntry& ent) {
    if (BitBoard::bitCount(pos.occupiedBB()) > Syzygy::TBLargest)
        return false;
    if (pos.getCastleMask())
        return false;

    int success;
    int wdl = Syzygy::probe_wdl(pos, &success);
    if (!success)
        return false;
    int plyToMate;
    switch (wdl) {
    case 0:
        score = 0;
        break;
    case 1:
        score = 0;
        if (ent.getEvalScore() == 0)
            ent.setEvalScore(maxFrustratedDist);
        break;
    case -1:
        score = 0;
        if (ent.getEvalScore() == 0)
            ent.setEvalScore(-maxFrustratedDist);
        break;
    case 2:
        plyToMate = getMaxSubMate(pos) + getMaxDTZ(pos.materialId());
        score = SearchConst::MATE0 - ply - plyToMate - 2;
        break;
    case -2:
        plyToMate = getMaxSubMate(pos) + getMaxDTZ(pos.materialId());
        score = -(SearchConst::MATE0 - ply - plyToMate - 2);
        break;
    default:
        return false;
    }

    return true;
}

void
TBProbe::gtbInitialize(const std::string& path, int cacheMB, int wdlFraction) {
    static_assert((int)tb_A1 == (int)A1, "Incompatible square numbering");
    static_assert((int)tb_A8 == (int)A8, "Incompatible square numbering");
    static_assert((int)tb_H1 == (int)H1, "Incompatible square numbering");
    static_assert((int)tb_H8 == (int)H8, "Incompatible square numbering");

    tbpaths_done(gtbPaths);

    gtbMaxPieces = 0;
    gtbPaths = tbpaths_init();
    gtbPaths = tbpaths_add(gtbPaths, path.c_str());

    TB_compression_scheme scheme = tb_CP4;
    int verbose = 0;
    int cacheSize = 1024 * 1024 * cacheMB;
    static bool isInitialized = false;
    if (isInitialized) {
        tb_restart(verbose, scheme, gtbPaths);
        tbcache_restart(cacheSize, wdlFraction);
    } else {
        tb_init(verbose, scheme, gtbPaths);
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
    gtbData.stm = pos.isWhiteMove() ? tb_WHITE_TO_MOVE : tb_BLACK_TO_MOVE;
    gtbData.epsq = pos.getEpSquare() >= 0 ? pos.getEpSquare() : tb_NOSQUARE;

    gtbData.castles = 0;
    if (pos.a1Castle()) gtbData.castles |= tb_WOOO;
    if (pos.h1Castle()) gtbData.castles |= tb_WOO;
    if (pos.a8Castle()) gtbData.castles |= tb_BOOO;
    if (pos.h8Castle()) gtbData.castles |= tb_BOO;

    int cnt = 0;
    U64 m = pos.whiteBB();
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
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
    }
    gtbData.wSq[cnt] = tb_NOSQUARE;
    gtbData.wP[cnt] = tb_NOPIECE;

    cnt = 0;
    m = pos.blackBB();
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
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
        score = maxDTM[gtbData.materialId] - ply;
        break;
    case tb_BMATE:
        score = -(maxDTM[gtbData.materialId] - ply);
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
    initMaxDTM();
    initMaxDTZ();

    // Pre-calculate all interesting maxSubDTM values
    int nNonKings = 5;
    for (int wp = 0; wp <= nNonKings; wp++) {
        std::vector<int> pieces(Piece::nPieceTypes);
        pieces[Piece::WPAWN] = wp;
        pieces[Piece::BPAWN] = nNonKings - wp;
        getMaxSubMate(pieces, nNonKings*5);
    }
}

int
TBProbe::getMaxDTZ(int matId) {
    auto it = maxDTZ.find(matId);
    if (it == maxDTZ.end())
        return 100;
    int val = it->second;
    if (val < 0)
        return 0;
    else
        return std::min(val+2, 100); // RTB DTZ values are not exact
}

bool
TBProbe::approxDTZ(int matId) {
    auto it = maxDTZ.find(matId);
    return it == maxDTZ.end() || it->second != 100;
}

static int
getMaxPawnMoves(const Position& pos) {
    int maxPawnMoves = 0;
    U64 m = pos.pieceTypeBB(Piece::WPAWN);
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        maxPawnMoves += 6 - Square::getY(sq);
    }
    m = pos.pieceTypeBB(Piece::BPAWN);
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        maxPawnMoves += Square::getY(sq) - 1;
    }
    return maxPawnMoves;
}

int
TBProbe::getMaxSubMate(const Position& pos) {
    int maxPawnMoves = getMaxPawnMoves(pos);
    int matId = pos.materialId();
    matId = std::min(matId, MatId::mirror(matId));
    auto it = maxSubDTM.find(std::make_pair(matId,maxPawnMoves));
    if (it != maxSubDTM.end())
        return it->second;

    std::vector<int> pieces(Piece::nPieceTypes);
    for (int p = 0; p < Piece::nPieceTypes; p++)
        pieces[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));
    pieces[Piece::EMPTY] = pieces[Piece::WKING] = pieces[Piece::BKING] = 0;
    return getMaxSubMate(pieces, maxPawnMoves);
}

int
TBProbe::getMaxSubMate(std::vector<int>& pieces, int pawnMoves) {
    assert(pawnMoves >= 0);
    if (pawnMoves > (pieces[Piece::WPAWN] + pieces[Piece::BPAWN]) * 5)
        return 0;

    MatId matId;
    for (int p = 0; p < Piece::nPieceTypes; p++)
        matId.addPieceCnt(p, pieces[p]);

    const int matIdMin = std::min(matId(), MatId::mirror(matId()));
    auto it = maxSubDTM.find(std::make_pair(matIdMin, pawnMoves));
    if (it != maxSubDTM.end())
        return it->second;

    int maxSubMate = 0;
    if (pawnMoves > 0) { // Pawn move
        maxSubMate = getMaxSubMate(pieces, pawnMoves-1) + getMaxDTZ(matId());
    }
    for (int p = 0; p < Piece::nPieceTypes; p++) { // Capture
        if (pieces[p] > 0) {
            pieces[p]--;
            matId.removePiece(p);
            int maxRemovedPawnMoves = 0;
            if (p == Piece::WPAWN || p == Piece::BPAWN)
                maxRemovedPawnMoves = 5;
            for (int i = 0; i <= maxRemovedPawnMoves; i++) {
                int newPawnMoves = pawnMoves - i;
                if (newPawnMoves >= 0) {
                    int tmp = getMaxSubMate(pieces, newPawnMoves) + getMaxDTZ(matId());
                    maxSubMate = std::max(maxSubMate, tmp);
                }
            }
            pieces[p]++;
            matId.addPiece(p);
        }
    }
    for (int c = 0; c < 2; c++) { // Promotion
        const int pawn = (c == 0) ? Piece::WPAWN : Piece::BPAWN;
        if (pieces[pawn] > 0) {
            const int p0 = (c == 0) ? Piece::WQUEEN : Piece::BQUEEN;
            const int p1 = (c == 0) ? Piece::WKNIGHT : Piece::BKNIGHT;
            for (int p = p0; p <= p1; p++) {
                pieces[pawn]--;
                pieces[p]++;
                matId.removePiece(pawn);
                matId.addPiece(p);
                int tmp = getMaxSubMate(pieces, pawnMoves) + getMaxDTZ(matId());
                maxSubMate = std::max(maxSubMate, tmp);
                pieces[pawn]++;
                pieces[p]--;
                matId.addPiece(pawn);
                matId.removePiece(p);
            }
        }
    }

#if 0
    std::cout << "wQ:" << pieces[Piece::WQUEEN]
              << " wR:" << pieces[Piece::WROOK]
              << " wB:" << pieces[Piece::WBISHOP]
              << " wN:" << pieces[Piece::WKNIGHT]
              << " wP:" << pieces[Piece::WPAWN]
              << " bQ:" << pieces[Piece::BQUEEN]
              << " bR:" << pieces[Piece::BROOK]
              << " bB:" << pieces[Piece::BBISHOP]
              << " bN:" << pieces[Piece::BKNIGHT]
              << " bP:" << pieces[Piece::BPAWN]
              << " pMoves:" << pawnMoves << " : " << maxSubMate << std::endl;
#endif
    maxSubDTM[std::make_pair(matIdMin, pawnMoves)] = maxSubMate;
    return maxSubMate;
}

void
TBProbe::initMaxDTM() {
    const int Q = MatId::WQ;
    const int R = MatId::WR;
    const int B = MatId::WB;
    const int N = MatId::WN;
    const int P = MatId::WP;
    const int q = MatId::BQ;
    const int r = MatId::BR;
    const int b = MatId::BB;
    const int n = MatId::BN;
    const int p = MatId::BP;

    static int table[][2] = {
        { Q, 31979 }, { R, 31967 }, { P, 31943 },

        { Q+Q, 31979 }, { Q+R, 31967 }, { Q+B, 31979 }, { Q+N, 31979 },
        { Q+P, 31943 }, { R+R, 31967 }, { R+B, 31967 }, { R+N, 31967 },
        { R+P, 31943 }, { B+B, 31961 }, { B+N, 31933 }, { B+P, 31937 },
        { N+N, 31998 }, { N+P, 31943 }, { P+P, 31935 }, { Q+q, 31974 },
        { R+q, 31929 }, { R+r, 31961 }, { B+q, 31965 }, { B+r, 31941 },
        { B+b, 31998 }, { N+q, 31957 }, { N+r, 31919 }, { N+b, 31998 },
        { N+n, 31998 }, { P+q, 31942 }, { P+r, 31914 }, { P+b, 31942 },
        { P+n, 31942 }, { P+p, 31933 },

        { Q+Q+Q, 31991 }, { Q+Q+R, 31987 }, { Q+Q+B, 31983 }, { Q+Q+N, 31981 },
        { Q+Q+P, 31979 }, { Q+R+R, 31985 }, { Q+R+B, 31967 }, { Q+R+N, 31967 },
        { Q+R+P, 31967 }, { Q+B+B, 31961 }, { Q+B+N, 31933 }, { Q+B+P, 31937 },
        { Q+N+N, 31981 }, { Q+N+P, 31945 }, { Q+P+P, 31935 }, { R+R+R, 31985 },
        { R+R+B, 31967 }, { R+R+N, 31967 }, { R+R+P, 31967 }, { R+B+B, 31961 },
        { R+B+N, 31933 }, { R+B+P, 31937 }, { R+N+N, 31967 }, { R+N+P, 31945 },
        { R+P+P, 31935 }, { B+B+B, 31961 }, { B+B+N, 31933 }, { B+B+P, 31937 },
        { B+N+N, 31931 }, { B+N+P, 31933 }, { B+P+P, 31935 }, { N+N+N, 31957 },
        { N+N+P, 31943 }, { N+P+P, 31935 }, { P+P+P, 31933 }, { Q+Q+q, 31939 },
        { Q+Q+r, 31929 }, { Q+Q+b, 31965 }, { Q+Q+n, 31957 }, { Q+Q+p, 31939 },
        { Q+R+q, 31865 }, { Q+R+r, 31929 }, { Q+R+b, 31941 }, { Q+R+n, 31919 },
        { Q+R+p, 31865 }, { Q+B+q, 31933 }, { Q+B+r, 31919 }, { Q+B+b, 31965 },
        { Q+B+n, 31957 }, { Q+B+p, 31933 }, { Q+N+q, 31917 }, { Q+N+r, 31918 },
        { Q+N+b, 31965 }, { Q+N+n, 31957 }, { Q+N+p, 31917 }, { Q+P+q, 31752 },
        { Q+P+r, 31913 }, { Q+P+b, 31941 }, { Q+P+n, 31939 }, { Q+P+p, 31755 },
        { R+R+q, 31901 }, { R+R+r, 31937 }, { R+R+b, 31941 }, { R+R+n, 31919 },
        { R+R+p, 31900 }, { R+B+q, 31859 }, { R+B+r, 31870 }, { R+B+b, 31939 },
        { R+B+n, 31919 }, { R+B+p, 31860 }, { R+N+q, 31861 }, { R+N+r, 31918 },
        { R+N+b, 31937 }, { R+N+n, 31919 }, { R+N+p, 31864 }, { R+P+q, 31792 },
        { R+P+r, 31851 }, { R+P+b, 31853 }, { R+P+n, 31891 }, { R+P+p, 31794 },
        { B+B+q, 31837 }, { B+B+r, 31938 }, { B+B+b, 31955 }, { B+B+n, 31843 },
        { B+B+p, 31834 }, { B+N+q, 31893 }, { B+N+r, 31918 }, { B+N+b, 31921 },
        { B+N+n, 31786 }, { B+N+p, 31791 }, { B+P+q, 31899 }, { B+P+r, 31910 },
        { B+P+b, 31898 }, { B+P+n, 31800 }, { B+P+p, 31865 }, { N+N+q, 31855 },
        { N+N+r, 31918 }, { N+N+b, 31992 }, { N+N+n, 31986 }, { N+N+p, 31770 },
        { N+P+q, 31875 }, { N+P+r, 31866 }, { N+P+b, 31914 }, { N+P+n, 31805 },
        { N+P+p, 31884 }, { P+P+q, 31752 }, { P+P+r, 31892 }, { P+P+b, 31913 },
        { P+P+n, 31899 }, { P+P+p, 31745 },
    };
    for (int i = 0; i < (int)COUNT_OF(table); i++) {
        int id = table[i][0];
        int value = table[i][1];
        maxDTM[id] = value;
        maxDTM[MatId::mirror(id)] = value;
    }
}

void
TBProbe::initMaxDTZ() {
    const int Q = MatId::WQ;
    const int R = MatId::WR;
    const int B = MatId::WB;
    const int N = MatId::WN;
    const int P = MatId::WP;
    const int q = MatId::BQ;
    const int r = MatId::BR;
    const int b = MatId::BB;
    const int n = MatId::BN;
    const int p = MatId::BP;

    static int table[][2] = {
        { 0, -1 },

        { Q, 20 }, { R, 32 }, { B, -1 }, { N, -1 }, { P, 20 },

        { Q+q, 19 }, { N+N,  1 }, { Q+Q,  6 }, { P+P, 14 }, { R+R, 10 },
        { R+r,  7 }, { Q+B, 12 }, { Q+R,  8 }, { Q+N, 14 }, { R+b, 35 },
        { B+b,  1 }, { Q+P,  6 }, { B+B, 37 }, { B+n,  2 }, { R+P,  6 },
        { N+n,  1 }, { R+n, 53 }, { P+p, 21 }, { B+p,  7 }, { R+B, 24 },
        { Q+n, 38 }, { R+N, 24 }, { B+P, 26 }, { N+p, 16 }, { N+P, 26 },
        { Q+r, 62 }, { Q+b, 24 }, { R+p, 25 }, { Q+p, 52 }, { B+N, 65 },

        { Q+Q+Q,   6 }, { Q+Q+R,   6 }, { R+R+R,   8 }, { Q+Q+B,   6 },
        { Q+Q+N,   8 }, { Q+Q+P,   6 }, { Q+R+N,   8 }, { Q+R+R,   8 },
        { Q+R+B,   8 }, { Q+P+P,   6 }, { Q+B+N,   8 }, { R+R+P,   6 },
        { Q+B+B,  12 }, { B+B+B,  20 }, { R+R+N,  10 }, { R+R+B,  10 },
        { Q+R+P,   6 }, { Q+N+N,  14 }, { Q+B+P,   6 }, { Q+N+P,   6 },
        { R+P+P,   6 }, { R+B+B,  20 }, { P+P+P,  14 }, { R+N+N,  20 },
        { Q+Q+q,  50 }, { Q+Q+n,   8 }, { Q+Q+b,   8 }, { R+B+N,  14 },
        { B+P+P,  18 }, { B+B+P,  24 }, { Q+Q+r,  28 }, { B+B+N,  26 },
        { N+P+P,  12 }, { Q+B+q,  59 }, { B+N+N,  26 }, { N+N+P,  16 },
        { Q+Q+p,   6 }, { N+N+N,  41 }, { Q+N+q,  69 }, { Q+R+q, 100 },
        { Q+R+n,  10 }, { Q+R+b,  10 }, { Q+R+r,  30 }, { R+B+P,   8 },
        { Q+B+n,  14 }, { Q+B+r,  38 }, { Q+B+b,  16 }, { B+N+P,  10 },
        { R+N+P,   8 }, { R+R+q,  40 }, { Q+N+n,  18 }, { R+B+r, 100 },
        { Q+N+b,  18 }, { Q+R+p,   6 }, { R+B+q,  82 }, { Q+P+q, 100 },
        { Q+P+p,  10 }, { Q+B+p,  22 }, { R+N+r,  64 }, { R+R+n,  14 },
        { R+R+p,  18 }, { Q+N+r,  44 }, { R+N+q,  92 }, { R+R+b,  20 },
        { Q+N+p,  34 }, { R+R+r,  50 }, { B+B+r,  16 }, { B+B+b,  11 },
        { Q+P+n,  12 }, { R+B+n,  42 }, { Q+P+b,  10 }, { B+N+r,  24 },
        { B+N+b,  24 }, { B+B+n, 100 }, { B+N+n, 100 }, { Q+P+r,  34 },
        { R+P+p,  19 }, { R+P+r,  70 }, { R+B+b,  50 }, { B+B+p,  42 },
        { B+B+q, 100 }, { R+B+p,  22 }, { N+N+r,  20 }, { N+N+b,   6 },
        { B+P+r,  36 }, { N+N+n,  12 }, { B+P+b,  50 }, { R+N+n,  48 },
        { N+P+r,  78 }, { N+N+q, 100 }, { R+N+b,  50 }, { R+N+p,  29 },
        { B+P+n,  60 }, { B+N+q,  84 }, { B+P+p,  74 }, { N+N+p, 100 },
        { N+P+b,  48 }, { P+P+b,  24 }, { P+P+q,  58 }, { P+P+p,  42 },
        { P+P+n,  27 }, { P+P+r,  30 }, { N+P+n,  59 }, { N+P+p,  46 },
        { R+P+n,  62 }, { R+P+b, 100 }, { N+P+q,  86 }, { B+N+p,  40 },
        { R+P+q, 100 }, { B+P+q,  84 },

        { B+B+B+B,  20 }, { B+B+B+b,  40 }, { B+B+B+n,  28 }, { B+B+B+p,  24 },
        { B+B+B+q, 100 }, { B+B+B+r, 100 }, { B+B+B+N,  26 }, { B+B+B+P,  24 },
        { B+B+b+b,  11 }, { B+B+b+n,  40 }, { B+B+b+p,  69 }, { B+B+n+n,  56 },
        { B+B+n+p, 100 }, { B+B+p+p,  39 }, { B+B+N+b,  72 }, { B+B+N+n,  62 },
        { B+B+N+p,  32 }, { B+B+N+q, 100 }, { B+B+N+r, 100 }, { B+B+N+N,  20 },
        { B+B+N+P,  10 }, { B+B+P+b,  56 }, { B+B+P+n, 100 }, { B+B+P+p,  29 },
        { B+B+P+q, 100 }, { B+B+P+r, 100 }, { B+B+P+P,  12 }, { B+N+b+n,  17 },
        { B+N+b+p,  56 }, { B+N+n+n,  24 }, { B+N+n+p,  98 }, { B+N+p+p,  48 },
        { B+N+N+b,  76 }, { B+N+N+n,  58 }, { B+N+N+p,  33 }, { B+N+N+q,  98 },
        { B+N+N+r,  96 }, { B+N+N+N,  20 }, { B+N+N+P,  10 }, { B+N+P+b,  86 },
        { B+N+P+n,  77 }, { B+N+P+p,  21 }, { B+N+P+q, 100 }, { B+N+P+r, 100 },
        { B+N+P+P,  10 }, { B+P+b+p,  65 }, { B+P+n+n,  48 }, { B+P+n+p,  62 },
        { B+P+p+p,  75 }, { B+P+P+b,  86 }, { B+P+P+n, 100 }, { B+P+P+p,  61 },
        { B+P+P+q,  78 }, { B+P+P+r,  66 }, { B+P+P+P,  18 }, { N+N+n+n,  13 },
        { N+N+n+p,  56 }, { N+N+p+p, 100 }, { N+N+N+b, 100 }, { N+N+N+n, 100 },
        { N+N+N+p,  41 }, { N+N+N+q,  70 }, { N+N+N+r,  22 }, { N+N+N+N,  22 },
        { N+N+N+P,  12 }, { N+N+P+b, 100 }, { N+N+P+n, 100 }, { N+N+P+p,  33 },
        { N+N+P+q, 100 }, { N+N+P+r,  91 }, { N+N+P+P,  12 }, { N+P+n+p,  57 },
        { N+P+p+p,  66 }, { N+P+P+b,  97 }, { N+P+P+n,  96 }, { N+P+P+p,  40 },
        { N+P+P+q,  78 }, { N+P+P+r,  81 }, { N+P+P+P,  10 }, { P+P+p+p,  31 },
        { P+P+P+b,  36 }, { P+P+P+n,  42 }, { P+P+P+p,  40 }, { P+P+P+q,  65 },
        { P+P+P+r,  44 }, { P+P+P+P,  14 }, { Q+B+B+B,  12 }, { Q+B+B+b,  16 },
        { Q+B+B+n,  14 }, { Q+B+B+p,  10 }, { Q+B+B+q, 100 }, { Q+B+B+r,  40 },
        { Q+B+B+N,  10 }, { Q+B+B+P,   6 }, { Q+B+b+b,  26 }, { Q+B+b+n,  32 },
        { Q+B+b+p,  44 }, { Q+B+n+n,  26 }, { Q+B+n+p,  53 }, { Q+B+p+p,  34 },
        { Q+B+q+b,  91 }, { Q+B+q+n,  72 }, { Q+B+q+p, 100 }, { Q+B+r+b,  83 },
        { Q+B+r+n,  54 }, { Q+B+r+p,  77 }, { Q+B+r+r, 100 }, { Q+B+N+b,  14 },
        { Q+B+N+n,  12 }, { Q+B+N+p,   8 }, { Q+B+N+q, 100 }, { Q+B+N+r,  44 },
        { Q+B+N+N,  10 }, { Q+B+N+P,   6 }, { Q+B+P+b,  12 }, { Q+B+P+n,  12 },
        { Q+B+P+p,   8 }, { Q+B+P+q, 100 }, { Q+B+P+r,  62 }, { Q+B+P+P,   8 },
        { Q+N+b+b,  30 }, { Q+N+b+n,  34 }, { Q+N+b+p,  67 }, { Q+N+n+n,  32 },
        { Q+N+n+p,  62 }, { Q+N+p+p,  44 }, { Q+N+q+n,  57 }, { Q+N+q+p, 100 },
        { Q+N+r+b,  52 }, { Q+N+r+n,  80 }, { Q+N+r+p,  83 }, { Q+N+r+r, 100 },
        { Q+N+N+b,  22 }, { Q+N+N+n,  18 }, { Q+N+N+p,  20 }, { Q+N+N+q, 100 },
        { Q+N+N+r,  44 }, { Q+N+N+N,  10 }, { Q+N+N+P,   6 }, { Q+N+P+b,  12 },
        { Q+N+P+n,  12 }, { Q+N+P+p,  12 }, { Q+N+P+q, 100 }, { Q+N+P+r,  42 },
        { Q+N+P+P,  10 }, { Q+P+b+b,  44 }, { Q+P+b+n,  36 }, { Q+P+b+p,  99 },
        { Q+P+n+n,  92 }, { Q+P+n+p,  54 }, { Q+P+p+p,  35 }, { Q+P+q+p, 100 },
        { Q+P+r+b, 100 }, { Q+P+r+n, 100 }, { Q+P+r+p, 100 }, { Q+P+r+r, 100 },
        { Q+P+P+b,  12 }, { Q+P+P+n,  12 }, { Q+P+P+p,  10 }, { Q+P+P+q, 100 },
        { Q+P+P+r,  42 }, { Q+P+P+P,   6 }, { Q+Q+B+B,   6 }, { Q+Q+B+b,  10 },
        { Q+Q+B+n,  10 }, { Q+Q+B+p,   6 }, { Q+Q+B+q,  58 }, { Q+Q+B+r,  52 },
        { Q+Q+B+N,   8 }, { Q+Q+B+P,   6 }, { Q+Q+b+b,  16 }, { Q+Q+b+n,  16 },
        { Q+Q+b+p,  12 }, { Q+Q+n+n,  14 }, { Q+Q+n+p,  11 }, { Q+Q+p+p,   6 },
        { Q+Q+q+b, 100 }, { Q+Q+q+n, 100 }, { Q+Q+q+p,  79 }, { Q+Q+q+q,  87 },
        { Q+Q+q+r, 100 }, { Q+Q+r+b,  27 }, { Q+Q+r+n,  28 }, { Q+Q+r+p,  38 },
        { Q+Q+r+r,  36 }, { Q+Q+N+b,   8 }, { Q+Q+N+n,  10 }, { Q+Q+N+p,   6 },
        { Q+Q+N+q,  56 }, { Q+Q+N+r,  48 }, { Q+Q+N+N,   8 }, { Q+Q+N+P,   6 },
        { Q+Q+P+b,   8 }, { Q+Q+P+n,  10 }, { Q+Q+P+p,   6 }, { Q+Q+P+q,  70 },
        { Q+Q+P+r,  48 }, { Q+Q+P+P,   6 }, { Q+Q+Q+B,   6 }, { Q+Q+Q+b,   6 },
        { Q+Q+Q+n,   8 }, { Q+Q+Q+p,   6 }, { Q+Q+Q+q,  38 }, { Q+Q+Q+r,  40 },
        { Q+Q+Q+N,   6 }, { Q+Q+Q+P,   6 }, { Q+Q+Q+Q,   6 }, { Q+Q+Q+R,   6 },
        { Q+Q+R+B,   6 }, { Q+Q+R+b,   8 }, { Q+Q+R+n,  10 }, { Q+Q+R+p,   6 },
        { Q+Q+R+q,  56 }, { Q+Q+R+r,  48 }, { Q+Q+R+N,   8 }, { Q+Q+R+P,   6 },
        { Q+Q+R+R,   6 }, { Q+R+B+B,   8 }, { Q+R+B+b,  10 }, { Q+R+B+n,  10 },
        { Q+R+B+p,   6 }, { Q+R+B+q,  98 }, { Q+R+B+r,  50 }, { Q+R+B+N,   8 },
        { Q+R+B+P,   8 }, { Q+R+b+b,  24 }, { Q+R+b+n,  22 }, { Q+R+b+p,  28 },
        { Q+R+n+n,  21 }, { Q+R+n+p,  26 }, { Q+R+p+p,  12 }, { Q+R+q+b, 100 },
        { Q+R+q+n, 100 }, { Q+R+q+p, 100 }, { Q+R+q+r, 100 }, { Q+R+r+b,  42 },
        { Q+R+r+n,  42 }, { Q+R+r+p,  44 }, { Q+R+r+r,  68 }, { Q+R+N+b,   8 },
        { Q+R+N+n,  12 }, { Q+R+N+p,   7 }, { Q+R+N+q, 100 }, { Q+R+N+r,  48 },
        { Q+R+N+N,   8 }, { Q+R+N+P,   8 }, { Q+R+P+b,   8 }, { Q+R+P+n,  10 },
        { Q+R+P+p,   7 }, { Q+R+P+q, 100 }, { Q+R+P+r,  60 }, { Q+R+P+P,   6 },
        { Q+R+R+B,   8 }, { Q+R+R+b,   8 }, { Q+R+R+n,  10 }, { Q+R+R+p,   6 },
        { Q+R+R+q,  82 }, { Q+R+R+r,  46 }, { Q+R+R+N,   8 }, { Q+R+R+P,   6 },
        { Q+R+R+R,   8 }, { R+B+B+B,  20 }, { R+B+B+b,  36 }, { R+B+B+n,  23 },
        { R+B+B+p,  24 }, { R+B+B+q,  88 }, { R+B+B+r,  71 }, { R+B+B+N,  14 },
        { R+B+B+P,  10 }, { R+B+b+b, 100 }, { R+B+b+n, 100 }, { R+B+b+p,  76 },
        { R+B+n+n, 100 }, { R+B+n+p,  90 }, { R+B+p+p,  47 }, { R+B+r+b,  33 },
        { R+B+r+n,  40 }, { R+B+r+p,  94 }, { R+B+N+b,  26 }, { R+B+N+n,  24 },
        { R+B+N+p,  31 }, { R+B+N+q, 100 }, { R+B+N+r,  72 }, { R+B+N+N,  14 },
        { R+B+N+P,  10 }, { R+B+P+b,  20 }, { R+B+P+n,  20 }, { R+B+P+p,  21 },
        { R+B+P+q, 100 }, { R+B+P+r, 100 }, { R+B+P+P,   8 }, { R+N+b+b, 100 },
        { R+N+b+n, 100 }, { R+N+b+p, 100 }, { R+N+n+n, 100 }, { R+N+n+p, 100 },
        { R+N+p+p,  48 }, { R+N+r+n,  41 }, { R+N+r+p,  72 }, { R+N+N+b,  24 },
        { R+N+N+n,  25 }, { R+N+N+p,  30 }, { R+N+N+q,  81 }, { R+N+N+r,  78 },
        { R+N+N+N,  14 }, { R+N+N+P,   8 }, { R+N+P+b,  26 }, { R+N+P+n,  20 },
        { R+N+P+p,  27 }, { R+N+P+q, 100 }, { R+N+P+r, 100 }, { R+N+P+P,  10 },
        { R+P+b+b,  79 }, { R+P+b+n, 100 }, { R+P+b+p, 100 }, { R+P+n+n,  84 },
        { R+P+n+p, 100 }, { R+P+p+p,  31 }, { R+P+r+p,  73 }, { R+P+P+b,  36 },
        { R+P+P+n,  36 }, { R+P+P+p,  26 }, { R+P+P+q, 100 }, { R+P+P+r,  90 },
        { R+P+P+P,   6 }, { R+R+B+B,  12 }, { R+R+B+b,  14 }, { R+R+B+n,  12 },
        { R+R+B+p,   8 }, { R+R+B+q, 100 }, { R+R+B+r,  62 }, { R+R+B+N,  12 },
        { R+R+B+P,   8 }, { R+R+b+b,  74 }, { R+R+b+n,  51 }, { R+R+b+p,  52 },
        { R+R+n+n,  66 }, { R+R+n+p,  50 }, { R+R+p+p,  50 }, { R+R+r+b, 100 },
        { R+R+r+n, 100 }, { R+R+r+p, 100 }, { R+R+r+r,  35 }, { R+R+N+b,  14 },
        { R+R+N+n,  14 }, { R+R+N+p,  18 }, { R+R+N+q, 100 }, { R+R+N+r,  66 },
        { R+R+N+N,  12 }, { R+R+N+P,   8 }, { R+R+P+b,  14 }, { R+R+P+n,  12 },
        { R+R+P+p,  22 }, { R+R+P+q, 100 }, { R+R+P+r,  56 }, { R+R+P+P,   6 },
        { R+R+R+B,  10 }, { R+R+R+b,  10 }, { R+R+R+n,  12 }, { R+R+R+p,   6 },
        { R+R+R+q, 100 }, { R+R+R+r,  42 }, { R+R+R+N,  10 }, { R+R+R+P,   8 },
        { R+R+R+R,   8 },
    };
    for (int i = 0; i < (int)COUNT_OF(table); i++) {
        int id = table[i][0];
        int value = table[i][1];
        maxDTZ[id] = value;
        maxDTZ[MatId::mirror(id)] = value;
    }
}
