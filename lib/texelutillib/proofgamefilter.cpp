/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * proofgamefilter.cpp
 *
 *  Created on: Dec 24, 2021
 *      Author: petero
 */

#include "proofgamefilter.hpp"
#include "textio.hpp"
#include "posutil.hpp"
#include "pkseq.hpp"
#include "util/timeUtil.hpp"
#include "threadpool.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <climits>
#include <mutex>


using PieceColor = ProofKernel::PieceColor;
using PieceType = ProofKernel::PieceType;
using PkMove = ProofKernel::PkMove;

std::vector<ProofGameFilter::InfoData> ProofGameFilter::infoStrData;

void
ProofGameFilter::init() {
    infoStrData.push_back({ ILLEGAL,    "illegal"   });
    infoStrData.push_back({ UNKNOWN,    "unknown"   });
    infoStrData.push_back({ LEGAL,      "legal"     });

    infoStrData.push_back({ FORCED,     "forced"    });

    infoStrData.push_back({ KERNEL,     "kernel"    });
    infoStrData.push_back({ EXT_KERNEL, "extKernel" });
    infoStrData.push_back({ PATH,       "path"      });
    infoStrData.push_back({ STATUS,     "status"    });
    infoStrData.push_back({ FAIL,       "fail"      });
    infoStrData.push_back({ INFO,       "info"      });

    infoStrData.push_back({ PROOF,      "proof"     });
}

ProofGameFilter::ProofGameFilter(int nWorkers, U64 rndSeed, bool rndKernel)
    : nWorkers(nWorkers), rndSeed(rndSeed), rndKernel(rndKernel) {
    std::once_flag flag;
    call_once(flag, init);
    startTime = currentTime();
}

void
ProofGameFilter::filterFens(std::istream& is, std::ostream& os, bool retry) {
    int maxNodes;
    runOneIteration(is, os, true, false, retry, maxNodes);
}

void
ProofGameFilter::filterFensIterated(std::istream& is,
                                    const std::string& outFileBaseName,
                                    bool retry) {
    auto getFileName = [&outFileBaseName](int iter) {
        std::stringstream ss;
        ss << outFileBaseName << std::setfill('0') << std::setw(2) << iter;
        return ss.str();
    };

    int maxNodesAllPos = 0;
    int iter = 0;
    while (true) {
        std::ofstream of(getFileName(iter));
        std::ifstream prev;
        if (iter > 0)
            prev.open(getFileName(iter - 1));
        std::istream& in = iter == 0 ? is : prev;
        if (!runOneIteration(in, of, iter == 0, true, retry, maxNodesAllPos))
            break;
        iter++;
    }
}

bool
ProofGameFilter::runOneIteration(std::istream& is, std::ostream& os,
                                 bool firstIteration,
                                 bool showProgress, bool retry,
                                 int& maxNodesAllPos) {
    std::ostream& log = std::clog;
    bool workRemains = false;
    const Position startPos = TextIO::readFEN(TextIO::startPosFEN);

    struct Result {
        int id;
        Line line;
        Legality status;
        std::string log;
        bool reportProgress;
        bool workRemains = false;
    };
    int nStarted = 0;
    int nFinished = 0;
    bool allStarted = false;
    bool allFinished = false;
    int maxNodesPrevIteration = maxNodesAllPos;
    maxNodesAllPos = 0;

    ThreadPool<Result> pool(nWorkers);
    std::map<int, Result> nonRetired; // Keep track of "out of order" results
    while (true) {
        Result r;
        if (!allStarted && r.line.read(is)) {
            Line& line = r.line;
            if (firstIteration && retry) {
                line.eraseToken(KERNEL);
                line.eraseToken(EXT_KERNEL);
                line.eraseToken(PATH);
                line.eraseToken(STATUS);
                line.eraseToken(FAIL);
                line.eraseToken(INFO);
            }

            if (firstIteration)
                statusCnt[(int)Legality::INITIAL]++;

            r.id = nStarted++;
            r.reportProgress = false;

            auto func = [this,&log,&startPos,maxNodesPrevIteration,r,firstIteration](int workerNo) mutable {
                std::stringstream strLog;
                std::ostream& localLog = nWorkers == 1 ? log : strLog;
                Legality status = r.line.getStatus();
                r.status = firstIteration ? Legality::INITIAL : status;
                switch (status) {
                case Legality::INITIAL:
                    computeExtProofKernel(startPos, r.line, localLog);
                    r.workRemains = true;
                    break;
                case Legality::KERNEL:
                    r.workRemains = computePath(startPos, r.line, localLog);
                    if (r.workRemains && !r.line.hasToken(PATH))
                        r.workRemains = computePath(startPos, r.line, localLog);
                    r.reportProgress = true;
                    break;
                case Legality::PATH: {
                    r.workRemains = computeProofGame(startPos, r.line, localLog);
                    int maxNodes = r.line.getStatusInt("N", 0);
                    if (r.workRemains && maxNodes <= maxNodesPrevIteration) {
                        r.workRemains = computeProofGame(startPos, r.line, localLog);
                    }
                    r.reportProgress = true;
                    break;
                }
                case Legality::ILLEGAL:
                case Legality::LEGAL:
                case Legality::FAIL:
                case Legality::nLegality:
                    break;
                }
                r.log = strLog.str();
                return r;
            };
            pool.addTask(func);
        } else {
            allStarted = true;
        }

        if (allStarted || nStarted >= nFinished + nWorkers * 2) {
            if (!allFinished) {
                Result r;
                if (pool.getResult(r)) {
                    nonRetired.insert({r.id, r});
                } else {
                    allFinished = true;
                }
            }
            while (!nonRetired.empty()) {
                auto it = nonRetired.begin();
                if (it->first != nFinished) {
                    assert(it->first > nFinished);
                    break;
                }
                const Result& r = it->second;
                const Line& line = r.line;

                workRemains |= r.workRemains;
                if (r.workRemains && r.line.hasToken(PATH)) {
                    int maxNodes = r.line.getStatusInt("N", 0);
                    maxNodesAllPos = std::max(maxNodesAllPos, maxNodes);
                }
                if (nWorkers > 1)
                    log << r.log << std::flush;

                Legality newStatus = line.getStatus();
                line.write(os);

                if (newStatus != r.status) {
                    statusCnt[(int)r.status]--;
                    statusCnt[(int)newStatus]++;
                }

                if (showProgress && (r.reportProgress || newStatus != r.status)) {
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(3) << (currentTime() - startTime);
                    std::string elapsed = ss.str();
                    std::cout << "legal: "    << statusCnt[(int)Legality::LEGAL]
                              << " path: "    << statusCnt[(int)Legality::PATH]
                              << " kernel: "  << statusCnt[(int)Legality::KERNEL]
                              << " fail: "    << statusCnt[(int)Legality::FAIL]
                              << " illegal: " << statusCnt[(int)Legality::ILLEGAL]
                              << " time: " << elapsed
                              << std::endl;
                }
                nonRetired.erase(nonRetired.begin());
                nFinished++;
            }
            if (allFinished) {
                assert(nonRetired.empty());
                break;
            }
        }
    }
    assert(nStarted == nFinished);
    return workRemains;
}

static std::vector<std::string>
getMovePath(const Position& startPos, std::vector<Move>& movePath) {
    std::vector<std::string> ret;
    Position pos(startPos);
    UndoInfo ui;
    for (const Move& m : movePath) {
        ret.push_back(TextIO::moveToString(pos, m, false));
        pos.makeMove(m, ui);
    }
    return ret;
}

void
ProofGameFilter::computeExtProofKernel(const Position& startPos, Line& line,
                                       std::ostream& log) const {
    auto setIllegal = [this,&line](const std::string& reason) {
        std::vector<std::string>& illegal = line.tokenData(ILLEGAL);
        illegal.clear();
        illegal.push_back(reason);
    };

    try {
        log << "Finding proof kernel for " << line.fen << std::endl;
        int minCost;
        {
            auto opts = ProofGame::Options().setSmallCache(true).setMaxNodes(2);
            ProofGame pg(TextIO::startPosFEN, line.fen, false, {}, false, log);
            ProofGame::Result result;
            minCost = pg.search(opts, result);
        }
        if (minCost == INT_MAX) {
            setIllegal("Other");
        } else {
            ProofGame pg(TextIO::startPosFEN, line.fen, true, {}, true, log);
            ProofGame::Result result;
            auto opts = ProofGame::Options().setSmallCache(true).setMaxNodes(2);
            minCost = pg.search(opts, result);
            if (minCost == INT_MAX) {
                setIllegal("Other");
            } else if (minCost >= 0) {
                line.tokenData(LEGAL).clear();
                std::vector<std::string>& proof = line.tokenData(PROOF);
                proof = getMovePath(startPos, result.proofGame);
            } else {
                U64 blocked;
                if (!pg.computeBlocked(startPos, blocked))
                    blocked = 0xffffffffffffffffULL; // If goal not reachable, consider all pieces blocked
                ProofKernel pk(startPos, pg.getGoalPos(), blocked, log);
                if (rndKernel)
                    pk.setRandomSeed(rndSeed);

                std::vector<PkMove> kernel;
                std::vector<ExtPkMove> extKernel;
                auto res = pk.findProofKernel(kernel, extKernel);
                if (res == ProofKernel::FAIL) {
                    setIllegal("No proof kernel");
                    if (!kernel.empty()) {
                        std::vector<std::string>& forced = line.tokenData(FORCED);
                        for (const auto& m : kernel)
                            forced.push_back(toString(m));
                    }
                } else if (res == ProofKernel::PROOF_KERNEL) {
                    setIllegal("No extended proof kernel");
                } else {
                    line.tokenData(UNKNOWN).clear();
                    std::vector<std::string>& kernelInfo = line.tokenData(KERNEL);
                    kernelInfo.clear();
                    for (const auto& m : kernel)
                        kernelInfo.push_back(toString(m));
                    std::vector<std::string>& extKernelInfo = line.tokenData(EXT_KERNEL);
                    extKernelInfo.clear();
                    for (const auto& m : extKernel)
                        extKernelInfo.push_back(toString(m));
                }
            }
        }
    } catch (const NotImplementedError& e) {
        line.tokenData(UNKNOWN).clear();
        line.tokenData(FAIL).clear();
        line.tokenData(INFO).clear();
        line.tokenData(INFO).push_back(e.what());
    } catch (const ChessError& e) {
        setIllegal(e.what());
    }
}

bool
ProofGameFilter::computePath(const Position& startPos, Line& line,
                             std::ostream& log) const {
    if (!line.hasToken(EXT_KERNEL))
        return false;

    std::vector<ExtPkMove> extKernel;
    for (const std::string& s : line.tokenData(EXT_KERNEL))
        extKernel.push_back(strToExtPkMove(s));

    const int initMaxNodes =  5000;
    const int maxMaxNodes = 250000;

    int oldMaxNodes = line.getStatusInt("N", 0);
    line.eraseToken(STATUS);
    int maxNodes = clamp(oldMaxNodes * 19 / 16, initMaxNodes, maxMaxNodes);
    if (maxNodes <= oldMaxNodes) {
        line.tokenData(FAIL).clear();
        return false;
    }

    try {
        log << "Finding path for " << line.fen << std::endl;
        Position initPos(startPos);
        ProofGame pg(TextIO::toFEN(startPos), line.fen, true, {}, true, log);
        Position goalPos = pg.getGoalPos();
        initPos.setCastleMask(goalPos.getCastleMask());

        enhanceExtKernel(extKernel, initPos, goalPos, log);

        MultiBoard brd(initPos);
        std::vector<MultiBoard> brdVec;
        brdVec.push_back(brd);
        for (const ExtPkMove& m : extKernel) {
            bool white = m.color == PieceColor::WHITE;
            int movingPiece = Piece::EMPTY;
            if (m.fromSquare != -1) {
                movingPiece = ProofKernel::toPieceType(white, m.movingPiece, true, true);
                if (!brd.hasPiece(m.fromSquare, movingPiece) &&
                    Square::getY(m.fromSquare) == (white ? 7 : 0)) {
                    int pawn = white ? Piece::WPAWN : Piece::BPAWN;
                    if (brd.hasPiece(m.fromSquare, pawn)) { // Moving promoted pawn
                        if (brd.replacePiece(m.fromSquare, pawn, movingPiece)) {
                            for (int i = (int)brdVec.size() - 1; i >= 0; i--)
                                if (!brdVec[i].replacePiece(m.fromSquare, pawn, movingPiece))
                                    break;
                        }
                    }
                }
                brd.removePieceType(m.fromSquare, movingPiece);
            }

            if (m.capture) {
                int np = brd.nPieces(m.toSquare);
                bool found = false;
                for (int i = np - 1; i >= 0; i--) {
                    int p = brd.getPiece(m.toSquare, i);
                    if (Piece::isWhite(p) != white) {
                        brd.removePieceNo(m.toSquare, i);
                        found = true;
                        break;
                    }
                }
                if (!found)
                    throw ChessError("No piece to capture on square " +
                                     TextIO::squareToString(m.toSquare));
            }

            int tgtPiece = movingPiece;
            if (m.promotedPiece != PieceType::EMPTY)
                tgtPiece = ProofKernel::toPieceType(white, m.promotedPiece, false, false);
            if (tgtPiece != Piece::EMPTY)
                brd.addPiece(m.toSquare, tgtPiece);
            brdVec.push_back(brd);
        }

        PathOptions pathOpts;
        pathOpts.maxNodes = maxNodes;
        pathOpts.weightA = 1;
        pathOpts.weightB = 5;

        std::vector<Move> movePath;
        computePath(brdVec, 0, brdVec.size() - 1, initPos, goalPos, pathOpts, movePath, log);

        line.eraseToken(INFO);
        std::vector<std::string>& path = line.tokenData(PATH);
        path = getMovePath(initPos, movePath);

        log << "Path solution: -w " << pathOpts.weightA << ':' << pathOpts.weightB
            << " nodes: " << pathOpts.maxNodes << " len: " << path.size()
            << std::endl;
        return true;
    } catch (const ChessError& e) {
        line.eraseToken(PATH);
        bool workRemains = maxNodes < maxMaxNodes;
        if (workRemains) {
            line.eraseToken(FAIL);
            line.setStatusInt("N", maxNodes);
        } else {
            line.tokenData(FAIL).clear();
        }
        line.tokenData(INFO).clear();
        line.tokenData(INFO).push_back(e.what());
        return workRemains;
    }
}

void
ProofGameFilter::enhanceExtKernel(std::vector<ExtPkMove>& extKernel,
                                  const Position& initPos, const Position& goalPos,
                                  std::ostream& log) const {
    decidePromotions(extKernel, initPos, goalPos);
    PkSequence seq(extKernel, initPos, goalPos, log);
    seq.improve();
    extKernel = seq.getSeq();
}

void
ProofGameFilter::decidePromotions(std::vector<ExtPkMove>& extKernel,
                                  const Position& initPos, const Position& goalPos) const {
    auto hasMissingProm = [](const ExtPkMove& m, PieceColor c, int sq) {
        return m.color == c &&
            m.movingPiece == PieceType::PAWN &&
            !m.capture && m.toSquare == sq &&
            m.promotedPiece == PieceType::EMPTY;
    };

    MultiBoard brd(initPos);
    for (const ExtPkMove& m : extKernel) {
        bool white = m.color == PieceColor::WHITE;
        int movingPiece = Piece::EMPTY;
        if (m.fromSquare != -1) {
            movingPiece = ProofKernel::toPieceType(white, m.movingPiece, true, true);
            if (!brd.hasPiece(m.fromSquare, movingPiece) &&
                Square::getY(m.fromSquare) == (white ? 7 : 0)) {
                int pawn = white ? Piece::WPAWN : Piece::BPAWN;
                if (brd.hasPiece(m.fromSquare, pawn)) { // Moving promoted pawn
                    brd.replacePiece(m.fromSquare, pawn, movingPiece);
                    bool found = false;
                    for (int i = 0; extKernel[i] != m; i++) {
                        if (hasMissingProm(extKernel[i], m.color, m.fromSquare)) {
                            extKernel[i].promotedPiece = PieceType::KNIGHT;
                            found = true;
                            break;
                        }
                    }
                    assert(found);
                }
            }
            brd.removePieceType(m.fromSquare, movingPiece);
        }

        if (m.capture) {
            int np = brd.nPieces(m.toSquare);
            for (int i = np - 1; i >= 0; i--) {
                int p = brd.getPiece(m.toSquare, i);
                if (Piece::isWhite(p) != white) {
                    brd.removePieceNo(m.toSquare, i);
                    break;
                }
            }
        }

        int tgtPiece = movingPiece;
        if (m.promotedPiece != PieceType::EMPTY)
            tgtPiece = ProofKernel::toPieceType(white, m.promotedPiece, false, false);
        if (tgtPiece != Piece::EMPTY)
            brd.addPiece(m.toSquare, tgtPiece);
    }
    MultiBoard& lastBrd = brd;

    bool allPromotionsComplete = true;
    for (int ci = 0; ci < 2; ci++) {
        bool white = ci == 0;
        int pawn = white ? Piece::WPAWN : Piece::BPAWN;
        int y = white ? 7 : 0;
        for (int x = 0; x < 8; x++) {
            if (lastBrd.hasPiece(Square::getSquare(x, y), pawn))
                allPromotionsComplete = false;
        }
    }
    if (allPromotionsComplete)
        return;

    struct FilePromInfo {
        int nPromAvail;
        bool bishopPromAllowed;
        bool bishopPromRequired;
        PieceType bishopType;
    };
    FilePromInfo fpiVec[8][2]; // [x][color]

    U64 blockedPawns = 0;
    for (int ci = 0; ci < 2; ci++) {
        bool white = ci == 0;
        int pawn = white ? Piece::WPAWN : Piece::BPAWN;
        int y = white ? 1 : 6;
        for (int x = 0; x < 8; x++) {
            int sq = Square::getSquare(x, y);
            if ((goalPos.getPiece(sq) == pawn) && (initPos.getPiece(sq) == pawn))
                blockedPawns |= 1ULL << sq;
        }
    }
    auto isBlocked = [blockedPawns](int x, int y) {
        int sq = Square::getSquare(x, y);
        return blockedPawns & (1ULL << sq);
    };
    for (int ci = 0; ci < 2; ci++) {
        bool white = ci == 0;
        Piece::Type pawn = white ? Piece::WPAWN : Piece::BPAWN;
        PieceColor c = (PieceColor)ci;
        for (int x = 0; x < 8; x++) {
            FilePromInfo& fpi = fpiVec[x][c];

            bool dark = ((x % 2) == 0) != white;
            fpi.bishopType = dark ? PieceType::DARK_BISHOP : PieceType::LIGHT_BISHOP;

            int y = white ? 6 : 1;
            bool promBlocked = (x == 0 || isBlocked(x-1, y)) && (x == 7 || isBlocked(x+1, y));
            if (!promBlocked) {
                fpi.bishopPromAllowed = true;
                fpi.bishopPromRequired = false;
            } else {
                y = white ? 7 : 0;
                int bish = white ? Piece::WBISHOP : Piece::BBISHOP;
                int sq = Square::getSquare(x, y);
                bool required = goalPos.getPiece(sq) == bish && !lastBrd.hasPiece(sq, bish);
                fpi.bishopPromRequired = required;
                fpi.bishopPromAllowed = required;
            }

            int nPromAvail = 0;
            for (int y = 0; y < 8; y++)
                nPromAvail += lastBrd.nPiecesOfType(Square::getSquare(x, y), pawn);
            nPromAvail -= BitBoard::bitCount(goalPos.pieceTypeBB(pawn) & BitBoard::maskFile[x]);
            fpi.nPromAvail = nPromAvail;
        }
    }

    int nPromNeeded[ProofKernel::nPieceTypes][2]; // [PieceType][color]
    {
        auto getPieceCnt = [](const Position& pos, PieceType pt, PieceColor c) {
            Piece::Type p = ProofKernel::toPieceType(c == PieceColor::WHITE, pt, false, false);
            U64 mask = pos.pieceTypeBB(p);
            if (pt == PieceType::DARK_BISHOP)
                mask &= BitBoard::maskDarkSq;
            if (pt == PieceType::LIGHT_BISHOP)
                mask &= BitBoard::maskLightSq;
            return BitBoard::bitCount(mask);
        };
        MultiBoard tmpBrd(lastBrd);
        tmpBrd.expel(goalPos.getCastleMask());
        Position lastBrdPos;
        tmpBrd.toPos(lastBrdPos);
        for (int pti = PieceType::QUEEN; pti <= PieceType::KNIGHT; pti++) {
            PieceType pt = (PieceType)pti;
            for (int ci = 0; ci < 2; ci++) {
                PieceColor c = (PieceColor)ci;
                nPromNeeded[pt][c] = (getPieceCnt(goalPos, pt, c) -
                                      getPieceCnt(lastBrdPos, pt, c));
            }
        }
    }

    // A required bishop promotion must be the last promotion in a file
    // because the bishop gets trapped after promotion
    for (int ci = 0; ci < 2; ci++) {
        PieceColor c = (PieceColor)ci;
        bool white = c == PieceColor::WHITE;
        int pawn = white ? Piece::WPAWN   : Piece::BPAWN;
        int bish = white ? Piece::WBISHOP : Piece::BBISHOP;
        int y = white ? 7 : 0;
        for (int x = 0; x < 8; x++) {
            FilePromInfo& fpi = fpiVec[x][c];
            if (fpi.bishopPromRequired) {
                assert(nPromNeeded[fpi.bishopType][c] > 0);
                assert(fpi.nPromAvail > 0);
                assert(fpi.bishopPromAllowed);
                int sq = Square::getSquare(x, y);
                if (fpi.nPromAvail == lastBrd.nPiecesOfType(sq, pawn)) {
                    bool found = false;
                    for (int i = (int)extKernel.size() - 1; i >= 0; i--) {
                        ExtPkMove& m = extKernel[i];
                        if (hasMissingProm(m, c, sq)) {
                            m.promotedPiece = fpi.bishopType;
                            found = true;
                            break;
                        }
                    }
                    assert(found);
                    lastBrd.replacePiece(sq, pawn, bish);
                }
                nPromNeeded[fpi.bishopType][c]--;
                fpi.nPromAvail--;
                fpi.bishopPromAllowed = false;
                fpi.bishopPromRequired = false;
            }
        }
    }

    for (int ci = 0; ci < 2; ci++) {
        PieceColor c = (PieceColor)ci;
        bool white = c == PieceColor::WHITE;
        int pawn = white ? Piece::WPAWN   : Piece::BPAWN;
        int y = white ? 7 : 0;
        for (int x = 0; x < 8; x++) {
            FilePromInfo& fpi = fpiVec[x][c];
            int sq = Square::getSquare(x, y);
            while (lastBrd.hasPiece(sq, pawn)) {
                assert(fpi.nPromAvail > 0);
                fpi.nPromAvail--;

                PieceType prom;
                if (fpi.bishopPromAllowed && nPromNeeded[fpi.bishopType][c] > 0) {
                    prom = fpi.bishopType;
                } else if (nPromNeeded[PieceType::KNIGHT][c] > 0) {
                    prom = PieceType::KNIGHT;
                } else if (nPromNeeded[PieceType::ROOK][c] > 0) {
                    prom = PieceType::ROOK;
                } else {
                    assert(nPromNeeded[PieceType::QUEEN][c] > 0);
                    prom = PieceType::QUEEN;
                }
                nPromNeeded[prom][c]--;

                bool found = false;
                for (ExtPkMove& m : extKernel) {
                    if (hasMissingProm(m, c, sq)) {
                        m.promotedPiece = prom;
                        found = true;
                        break;
                    }
                }
                assert(found);
                Piece::Type promT = ProofKernel::toPieceType(white, prom, false, false);
                lastBrd.replacePiece(sq, pawn, promT);
            }
        }
    }
}

int
ProofGameFilter::pgSearch(const std::string& start, const std::string& goal,
                          const std::vector<Move>& initialPath, std::ostream& log,
                          bool useNonForcedIrreversible,
                          ProofGame::Options& opts, ProofGame::Result& result) const {
    auto getHash = [this,&opts]() -> U64 {
        U64 ret = 1;
        ret = hashU64(ret) + opts.weightA;
        ret = hashU64(ret) + opts.weightB;
        ret = hashU64(ret) + (opts.dynamic ? 1 : 0);
        ret = hashU64(ret) + (opts.useNonAdmissible ? 1 : 0);
        ret = hashU64(ret) + opts.maxNodes;
        ret = hashU64(ret);
        ret += rndSeed;
        return ret;
    };

    {
        ProofGame ps(start, goal, true, initialPath, useNonForcedIrreversible, log);
        ps.setRandomSeed(getHash());
        int ret = ps.search(opts, result);
        if (ret != -1 || result.closestPath.empty())
            return ret;
    }

    ProofGame::Result tmpResult;
    auto updateResult = [&result,&tmpResult](int ret) -> bool {
        if (ret != -1 && ret != INT_MAX) { // solution found
            tmpResult.numNodes += result.numNodes;
            tmpResult.computationTime += result.computationTime;
            result = tmpResult;
            return true;
        }
        result.numNodes += tmpResult.numNodes;
        result.computationTime += tmpResult.computationTime;
        return false;
    };

    opts.maxNodes /= 4;
    opts.setUseNonAdmissible(true);
    {
        ProofGame ps(start, goal, true, result.closestPath, useNonForcedIrreversible, log);
        ps.setRandomSeed(getHash());
        int ret = ps.search(opts, tmpResult);
        if (updateResult(ret))
            return ret;
    }

    opts.maxNodes /= 2;
    ProofGame ps(start, goal, true, initialPath, useNonForcedIrreversible, log);
    ps.setRandomSeed(getHash());
    int ret = ps.search(opts, tmpResult);
    updateResult(ret);
    return ret;
}

void
ProofGameFilter::computePath(std::vector<MultiBoard>& brdVec, int startIdx, int endIdx,
                             const Position& initPos, const Position& goalPos,
                             const PathOptions& pathOpts, std::vector<Move>& path,
                             std::ostream& log) const {
    freePieces(brdVec, endIdx, initPos, goalPos);

    Position startPos(initPos);
    brdVec[startIdx].expel(goalPos.getCastleMask());
    brdVec[startIdx].toPos(startPos);

    Position endPos(initPos);
    brdVec[endIdx].expel(goalPos.getCastleMask());
    brdVec[endIdx].toPos(endPos);

    ProofGame::Result result;
    auto opts = ProofGame::Options()
        .setWeightA(pathOpts.weightA)
        .setWeightB(pathOpts.weightB)
        .setMaxNodes(pathOpts.maxNodes)
        .setVerbose(true)
        .setAcceptFirst(true);
    std::vector<Move> initPath;
    int len = pgSearch(TextIO::toFEN(startPos), TextIO::toFEN(endPos), initPath, log,
                       false, opts, result);

    auto getFenInfo = [&]() -> std::string {
        std::stringstream ss;
        ss << ", fen1= " << TextIO::toFEN(startPos)
           << " fen2= " << TextIO::toFEN(endPos);
        if (endIdx < (int)brdVec.size() - 1) {
            Position lastPos(initPos);
            brdVec[brdVec.size() - 1].expel(goalPos.getCastleMask());
            brdVec[brdVec.size() - 1].toPos(lastPos);
            ss << " fen3= " << TextIO::toFEN(lastPos);
        }
        return ss.str();
    };

    if (len == INT_MAX)
        throw ChessError("No solution exists" + getFenInfo());
    if (len == -1) {
        if (endIdx <= startIdx + 1)
            throw ChessError("No solution found" + getFenInfo());
        int midIdx = (startIdx + endIdx) / 2;
        computePath(brdVec, startIdx, midIdx, initPos, goalPos, pathOpts, path, log);
        computePath(brdVec, midIdx, endIdx, initPos, goalPos, pathOpts, path, log);
    } else {
        path.insert(path.end(), result.proofGame.begin(), result.proofGame.end());
    }
}

void
ProofGameFilter::freePieces(std::vector<MultiBoard>& brdVec, int startIdx,
                            const Position& initPos, const Position& goalPos) const {
    struct Data {
        int pieceType;
        int square;
        std::vector<int> blockingPawns;
        std::vector<int> pawnTargets;
    };
    static const std::vector<Data> dataVec = {
        { Piece::WROOK,   A1, { }, { A4, B4 } },
        { Piece::WROOK,   H1, { }, { H4, G4 } },
        { Piece::WBISHOP, C1, { B2, D2 }, { D4, B4, D3, B3 } },
        { Piece::WBISHOP, F1, { E2, G2 }, { E4, G4, E3, G3 } },
        { Piece::WQUEEN,  D1, { C2, D2, E2 }, { E4, D4, C4, E3, D3, C3 } },
        { Piece::WKING,   E1, { D2, E2, F2 }, { E4, D4, F4, E3, D3, F3 } },

        { Piece::BROOK,   A8, { }, { A5, B5 } },
        { Piece::BROOK,   H8, { }, { H5, G5 } },
        { Piece::BBISHOP, C8, { B7, D7 }, { D5, B5, D6, B6 } },
        { Piece::BBISHOP, F8, { E7, G7 }, { E5, G5, E6, G6 } },
        { Piece::BQUEEN,  D8, { C7, D7, E7 }, { E5, D5, C5, E6, D6, C6 } },
        { Piece::BKING,   E8, { D7, E7, F7 }, { E5, D5, F5, E6, D6, F6 } },
    };

    int nBrds = brdVec.size();

    for (const Data& d : dataVec) {
        if (brdVec[startIdx].hasPiece(d.square, d.pieceType))
            continue;

        bool white = Piece::isWhite(d.pieceType);
        int pawn = white ? Piece::WPAWN : Piece::BPAWN;

        if (!d.blockingPawns.empty()) {
            bool isFree = false;
            for (int pSq : d.blockingPawns) {
                if (!brdVec[startIdx].hasPiece(pSq, pawn)) {
                    isFree = true;
                    break;
                }
            }
            if (isFree)
                continue;
        }

        for (int tgtSq : d.pawnTargets) {
            bool canMove = true;
            for (int b = startIdx; b < nBrds; b++) {
                if (!brdVec[b].canMovePawn(white, tgtSq)) {
                    canMove = false;
                    break;
                }
            }
            if (canMove) {
                if (white) {
                    U64 mask = 1ULL << (tgtSq - 8);
                    mask = BitBoard::southFill(mask);
                    if ((goalPos.pieceTypeBB(Piece::WPAWN) & mask) != 0)
                        canMove = false;
                } else {
                    U64 mask = 1ULL << (tgtSq + 8);
                    mask = BitBoard::northFill(mask);
                    if ((goalPos.pieceTypeBB(Piece::BPAWN) & mask) != 0)
                        canMove = false;
                }
            }
            if (canMove) {
                int sq0 = Square::getSquare(Square::getX(tgtSq), white ? 1 : 6);
                int d = white ? 8 : -8;
                bool moved = false;
                for (int b = startIdx; b < nBrds; b++) {
                    for (int sq = sq0; sq != tgtSq; sq += d) {
                        if (brdVec[b].hasPiece(sq, pawn)) {
                            brdVec[b].removePieceType(sq, pawn);
                            brdVec[b].addPiece(tgtSq, pawn);
                            moved = true;
                            break;
                        }
                    }
                }
                if (moved)
                    break;
            }
        }
    }
}

// ----------------------------------------------------------------------------

bool
ProofGameFilter::computeProofGame(const Position& startPos, Line& line,
                                  std::ostream& log) const {
    std::vector<Move> initPath;
    {
        Position pos(startPos);
        UndoInfo ui;
        for (const std::string& moveS : line.tokenData(PATH)) {
            Move m = TextIO::stringToMove(pos, moveS);
            initPath.push_back(m);
            pos.makeMove(m, ui);
        }
    }

    const int initMaxNodes = 50000;
    const int maxMaxNodes = 800000;

    int oldMaxNodes = line.getStatusInt("N", 0);
    line.eraseToken(STATUS);
    int maxNodes = clamp(oldMaxNodes * 19 / 16, initMaxNodes, maxMaxNodes);
    if (maxNodes <= oldMaxNodes) {
        line.tokenData(FAIL).clear();
        return false;
    }

    const int weightA = 1;
    const int weightB = 5;

    ProofGame::Result result;

    try {
        log << "Finding proof game for " << line.fen << std::endl;
        int len;
        {
            auto opts = ProofGame::Options()
                .setWeightA(weightA)
                .setWeightB(weightB)
                .setMaxNodes(maxNodes)
                .setVerbose(true)
                .setAcceptFirst(true);
            len = pgSearch(TextIO::toFEN(startPos), line.fen, initPath, log,
                           true, opts, result);
        }

        if (len == INT_MAX) {
            line.tokenData(FAIL).clear();
            line.tokenData(INFO).clear();
            line.tokenData(INFO).push_back("No solution exists");
            return false;
        }
        if (len == -1)
            throw ChessError("No solution found");

        std::vector<std::string>& proof = line.tokenData(PROOF);
        proof = getMovePath(startPos, result.proofGame);
        line.eraseToken(UNKNOWN);
        line.tokenData(LEGAL).clear();
        log << "Solution: -w " << weightA << ':' << weightB
            << " len: " << proof.size()
            << " nodes: " << result.numNodes << " time: " << result.computationTime
            << std::endl;
        return false;
    } catch (const ChessError& e) {
        line.eraseToken(PROOF);
        bool workRemains = maxNodes < maxMaxNodes;
        if (workRemains) {
            line.eraseToken(FAIL);
            line.setStatusInt("N", maxNodes);
        } else {
            line.tokenData(FAIL).clear();
        }
        std::vector<std::string>& info = line.tokenData(INFO);
        info.clear();
        info.push_back(e.what() + std::string(","));
        if (result.smallestBound > 0) {
            info.push_back("bound=" + num2Str(result.smallestBound));
            info.push_back("moves");
            for (auto& s : getMovePath(startPos, result.closestPath))
                info.push_back(s);
        }
        return workRemains;
    }
}

// ----------------------------------------------------------------------------

ProofGameFilter::Info
ProofGameFilter::str2Info(const std::string& token) {
    for (auto& e : infoStrData)
        if (e.tokenStr == token)
            return e.info;
    throw ChessParseError("Invalid line format: " + token);
}

std::string
ProofGameFilter::info2Str(Info info) {
    for (auto& e : infoStrData)
        if (e.info == info)
            return e.tokenStr;
    assert(false);
    throw ChessError("internal error");
}

bool
ProofGameFilter::Line::read(std::istream& is) {
    std::string lineStr;
    std::vector<std::string> arr;
    {
        std::getline(is, lineStr);
        if (!is || is.eof())
            return false;
        lineStr = trim(lineStr);

        splitString(lineStr, arr);
    }

    if (arr.size() < 6)
        return false;

    fen.clear();
    for (int i = 0; i < 6; i++) {
        if (i > 0)
            fen += ' ';
        fen += arr[i];
    }

    Info info = STATUS;
    bool infoValid = false;
    for (int i = 6; i < (int)arr.size(); i++) {
        const std::string& token = arr[i];
        if (endsWith(token, ":")) {
            info = str2Info(token.substr(0, token.length() - 1));
            data[info].clear();
            infoValid = true;
        } else {
            if (!infoValid)
                throw ChessParseError("Invalid line format: " + lineStr);
            data[info].push_back(token);
        }
    }

    return true;
}

void
ProofGameFilter::Line::write(std::ostream& os) const {
    os << fen;

    auto printTokenData = [&os,this](Info tokType) {
        if (hasToken(tokType)) {
            os << ' ' << info2Str(tokType) << ':';
            const std::vector<std::string>& d = data.at(tokType);
            for (const std::string& s : d)
                os << ' ' << s;
        }
    };

    if (hasToken(ILLEGAL)) {
        printTokenData(ILLEGAL);
        printTokenData(FORCED);
    } else if (hasToken(UNKNOWN)) {
        printTokenData(UNKNOWN);
        printTokenData(KERNEL);
        printTokenData(EXT_KERNEL);
        printTokenData(PATH);
        printTokenData(STATUS);
        printTokenData(FAIL);
        printTokenData(INFO);
    } else if (hasToken(LEGAL)) {
        printTokenData(LEGAL);
        printTokenData(PROOF);
    }

    os << std::endl;
}

ProofGameFilter::Legality
ProofGameFilter::Line::getStatus() const {
    if (hasToken(ILLEGAL))
        return Legality::ILLEGAL;
    if (hasToken(LEGAL) && hasToken(PROOF))
        return Legality::LEGAL;

    if (hasToken(UNKNOWN)) {
        if (hasToken(FAIL))
            return Legality::FAIL;
        if (hasToken(PATH))
            return Legality::PATH;
        if (hasToken(EXT_KERNEL))
            return Legality::KERNEL;
    }

    return Legality::INITIAL;
}

int
ProofGameFilter::Line::getStatusInt(const std::string& name, int defVal) const {
    if (hasToken(STATUS)) {
        const std::vector<std::string>& status = data.at(STATUS);
        std::string pre = name + "=";
        for (const std::string& s : status) {
            if (startsWith(s, pre)) {
                int val = defVal;
                str2Num(s.substr(pre.length()), val);
                return val;
            }
        }
    }
    return defVal;
}

void
ProofGameFilter::Line::setStatusInt(const std::string& name, int value) {
    std::vector<std::string>& status = tokenData(STATUS);
    std::string pre = name + "=";
    std::string valS = pre + num2Str(value);
    for (std::string& s : status) {
        if (startsWith(s, pre)) {
            s = valS;
            return;
        }
    }
    status.push_back(valS);
}

// ----------------------------------------------------------------------------

MultiBoard::MultiBoard() {
    for (int i = 0; i < 64; i++)
        squares[i][0] = -1;
}

MultiBoard::MultiBoard(const Position& pos)
    : MultiBoard() {
    for (int sq = 0; sq < 64; sq++) {
        int p = pos.getPiece(sq);
        if (p != Piece::EMPTY)
            addPiece(sq, p);
    }
}

int
MultiBoard::nPieces(int square) const {
    for (int i = 0; ; i++)
        if (squares[square][i] == -1)
            return i;
}

int
MultiBoard::getPiece(int square, int pieceNo) const {
    return squares[square][pieceNo];
}

bool
MultiBoard::hasPiece(int square, int piece) const {
    for (int i = 0; ; i++) {
        int p = getPiece(square, i);
        if (p == -1)
            return false;
        if (p == piece)
            return true;
    }
}

int
MultiBoard::nPiecesOfType(int square, int piece) const {
    int n = 0;
    for (int i = 0; ; i++) {
        int p = getPiece(square, i);
        if (p == -1)
            return n;
        if (p == piece)
            n++;
    }
}

void
MultiBoard::addPiece(int square, int piece) {
    for (int i = 0; i < maxPerSquare; i++) {
        if (squares[square][i] == -1) {
            squares[square][i] = piece;
            squares[square][i+1] = -1;
            return;
        }
    }
    throw ChessError("Too many pieces on square " + TextIO::squareToString(square));
}

void
MultiBoard::removePieceType(int square, int piece) {
    for (int i = nPieces(square) - 1; i >= 0; i--) {
        if (squares[square][i] == piece) {
            removePieceNo(square, i);
            return;
        }
    }
    throw ChessError(std::string("No piece of type ") + num2Str(piece) + " on square " +
                     TextIO::squareToString(square));
}

void
MultiBoard::removePieceNo(int square, int pieceNo) {
    for (int i = pieceNo; ; i++) {
        int p = squares[square][i+1];
        squares[square][i] = p;
        if (p == -1)
            return;
    }
}

void
MultiBoard::expel(int castleMask) {
    auto getDist = [this](int fromSq, int toSq, bool isKing) {
        int d = BitBoard::getKingDistance(fromSq, toSq);
        if (isKing) {
            int x = Square::getX(toSq);
            int y = Square::getY(toSq);
            if ((y == 7 && hasPiece(Square::getSquare(x, 6), Piece::WPAWN)) ||
                (y == 0 && hasPiece(Square::getSquare(x, 1), Piece::BPAWN)))
                d += 20; // Avoid king blocking pawn promotion
        }
        return d;
    };

    // Move pieces so there is at most one piece per square
    for (int fromSq = 0; fromSq < 64; fromSq++) {
        while (nPieces(fromSq) > 1) {
            int p = getPiece(fromSq, 0);
            bool isKing = p == Piece::WKING || p == Piece::BKING;
            bool isBishop = p == Piece::WBISHOP || p == Piece::BBISHOP;
            int bestSq = -1;
            for (int toSq = 0; toSq < 64; toSq++) {
                if (nPieces(toSq) > 0)
                    continue;
                if (isBishop && Square::darkSquare(fromSq) != Square::darkSquare(toSq))
                    continue;
                if (bestSq == -1 || getDist(fromSq, toSq, isKing) < getDist(fromSq, bestSq, isKing))
                    bestSq = toSq;
            }
            if (bestSq == -1)
                throw ChessError("Cannot expel piece on square " + TextIO::squareToString(fromSq));
            removePieceNo(fromSq, 0);
            addPiece(bestSq, p);
        }
    }

    // Move kings out of check
    Position pos; toPos(pos);
    int wKingSq = pos.wKingSq(); pos.clearPiece(wKingSq); removePieceNo(wKingSq, 0);
    int bKingSq = pos.bKingSq(); pos.clearPiece(bKingSq); removePieceNo(bKingSq, 0);
    for (int i = 0; i < 2; i++) {
        bool white = i == 0;
        int king = white ? Piece::WKING : Piece::BKING;
        int bestSq = -1;
        if (white && (castleMask & ((1 << Position::A1_CASTLE) | (1 << Position::H1_CASTLE)))) {
            bestSq = wKingSq;
        } else if (!white && (castleMask & ((1 << Position::A8_CASTLE) | (1 << Position::H8_CASTLE)))) {
            bestSq = bKingSq;
        } else {
            int fromSq = white ? wKingSq : bKingSq;
            U64 notAllowed = pos.occupiedBB() | PosUtil::attackedSquares(pos, !white);
            for (int toSq = 0; toSq < 64; toSq++) {
                if (notAllowed & (1ULL << toSq))
                    continue;
                if (bestSq == -1 || getDist(fromSq, toSq, true) < getDist(fromSq, bestSq, true))
                    bestSq = toSq;
            }
            if (bestSq == -1)
                throw ChessError("Cannot expel king on square " + TextIO::squareToString(fromSq));
        }
        pos.setPiece(bestSq, king);
        addPiece(bestSq, king);
    }
}

bool
MultiBoard::canMovePawn(bool white, int toSq) const {
    int x = Square::getX(toSq);
    int yTarget = Square::getY(toSq);
    int yFirst = white ? 1 : 6;
    int d      = white ? 1 : -1;
    int pawn  = white ? Piece::WPAWN : Piece::BPAWN;
    int oPawn = white ? Piece::BPAWN : Piece::WPAWN;

    int y0 = -1;
    for (int y = yFirst; y != yTarget; y += d) {
        if (hasPiece(Square::getSquare(x, y), pawn)) {
            y0 = y;
            break;
        }
    }
    if (y0 == -1)
        return true;

    for (int y = y0 + d; y != yTarget; y += d) {
        int sq = Square::getSquare(x, y);
        if (hasPiece(sq, pawn) || hasPiece(sq, oPawn))
            return false;
    }
    return true;
}

bool
MultiBoard::replacePiece(int square, int oldPiece, int newPiece) {
    int np = nPieces(square);
    for (int i = 0; i < np; i++) {
        if (squares[square][i] == oldPiece) {
            squares[square][i] = newPiece;
            return true;
        }
    }
    return false;
}

void
MultiBoard::toPos(Position& pos) {
    for (int sq = 0; sq < 64; sq++) {
        int np = nPieces(sq);
        switch (np) {
        case 0:
            pos.clearPiece(sq);
            break;
        case 1:
            pos.setPiece(sq, getPiece(sq, 0));
            break;
        default:
            throw ChessError("Too many pieces on square " + TextIO::squareToString(sq));
        }
    }
}

void
MultiBoard::print(std::ostream& os) const {
    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++) {
            int sq = Square::getSquare(x, y);
            std::string s;
            if (hasPiece(sq, Piece::WKING)) s += "K";
            if (hasPiece(sq, Piece::WQUEEN)) s += "Q";
            if (hasPiece(sq, Piece::WROOK)) s += "R";
            if (hasPiece(sq, Piece::WBISHOP)) s += "B";
            if (hasPiece(sq, Piece::WKNIGHT)) s += "N";
            if (hasPiece(sq, Piece::WPAWN)) s += "P";
            if (hasPiece(sq, Piece::BKING)) s += "k";
            if (hasPiece(sq, Piece::BQUEEN)) s += "q";
            if (hasPiece(sq, Piece::BROOK)) s += "r";
            if (hasPiece(sq, Piece::BBISHOP)) s += "b";
            if (hasPiece(sq, Piece::BKNIGHT)) s += "n";
            if (hasPiece(sq, Piece::BPAWN)) s += "p";
            while (s.size() < 3)
                s += '.';
            os << s << ' ';
        }
        os << '\n';
    }
    os << std::endl;
}
