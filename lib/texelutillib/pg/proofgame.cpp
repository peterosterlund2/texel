/*
    Texel - A UCI chess engine.
    Copyright (C) 2015-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * proofgame.cpp
 *
 *  Created on: Aug 15, 2015
 *      Author: petero
 */

#include "proofgame.hpp"
#include "moveGen.hpp"
#include "revmovegen.hpp"
#include "proofkernel.hpp"
#include "textio.hpp"
#include "timeUtil.hpp"

#include <iostream>
#include <climits>
#include <functional>
#include <mutex>


U64 ProofGame::wPawnReachable[64][maxPawnCapt+1];
U64 ProofGame::bPawnReachable[64][maxPawnCapt+1];

const int ProofGame::bigCost;


static StaticInitializer<ProofGame> pgInit;

void
ProofGame::staticInitialize() {
    for (int y = 6; y >= 1; y--) {
        for (int x = 0; x < 8; x++) {
            int sq = Square(x, y).asInt();
            U64 mask = 1ULL << sq;
            if (y < 7) {
                mask |= wPawnReachable[sq + 8][maxPawnCapt];
                if (x > 0)
                    mask |= wPawnReachable[sq + 7][maxPawnCapt];
                if (x < 7)
                    mask |= wPawnReachable[sq + 9][maxPawnCapt];
            }
            wPawnReachable[sq][maxPawnCapt] = mask;
        }
    }

    for (int y = 1; y < 7; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Square(x, y).asInt();
            U64 mask = 1ULL << sq;
            if (y > 0) {
                mask |= bPawnReachable[sq - 8][maxPawnCapt];
                if (x > 0)
                    mask |= bPawnReachable[sq - 9][maxPawnCapt];
                if (x < 7)
                    mask |= bPawnReachable[sq - 7][maxPawnCapt];
            }
            bPawnReachable[sq][maxPawnCapt] = mask;
        }
    }

    for (int c = 0; c < 2; c++) {
        for (int nCapt = maxPawnCapt - 1; nCapt >= 0; nCapt--) {
            for (int sq = 0; sq < 64; sq++) {
                int x = Square(sq).getX();
                U64 m = (c ? wPawnReachable : bPawnReachable)[sq][nCapt + 1];
                if (x - (nCapt+1) >= 0)
                    m &= ~BitBoard::maskFile[x - (nCapt+1)];
                if (x + (nCapt+1) < 8)
                    m &= ~BitBoard::maskFile[x + (nCapt+1)];
                (c ? wPawnReachable : bPawnReachable)[sq][nCapt] = m;
            }
        }
    }
}

static void
resetMoveCnt(Position& pos) {
    pos.setFullMoveCounter(1);
    pos.setHalfMoveClock(0);
}

ProofGame::ProofGame(const std::string& start, const std::string& goal,
                     bool analyzeLastMoves,
                     const std::vector<Move>& initialPath,
                     bool useNonForcedIrreversible, std::ostream& log)
    : initialFen(start), initialPath(initialPath), log(log) {
    setRandomSeed(1);

    Position startPos = TextIO::readFEN(initialFen);
    UndoInfo ui;
    for (Move m : initialPath)
        startPos.makeMove(m, ui);
    resetMoveCnt(startPos);

    goalPos = TextIO::readFEN(goal);
    if (TextIO::toFEN(goalPos) != goal)
        throw ChessParseError("Lossy FEN conversion");
    resetMoveCnt(goalPos);

    validatePieceCounts(goalPos);

    if (analyzeLastMoves)
        computeLastMoves(startPos, goalPos, useNonForcedIrreversible, lastMoves, log);

    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        goalPieceCnt[p] = BitBoard::bitCount(goalPos.pieceTypeBB((Piece::Type)p));

    pathDataCache.resize(1);

    Assign::Matrix<int> m(8, 8);
    captureAP[0] = Assignment<int>(m);
    captureAP[1] = Assignment<int>(m);
    for (int c = 0; c < 2; c++) {
        for (int n = 0; n <= maxMoveAPSize; n++) {
            Assign::Matrix<int> m(n, n);
            moveAP[c][n] = Assignment<int>(m);
        }
    }
}

void
ProofGame::computeLastMoves(const Position& startPos, Position& goalPos,
                            bool useNonForcedIrreversible,
                            std::vector<Move>& lastMoves,
                            std::ostream& log) {
    while (true) {
        if (startPos == goalPos)
            break;

        std::vector<UnMove> unMoves;
        RevMoveGen::genMoves(goalPos, unMoves, false);

        auto canAnalyze = [](const Position& pos) {
            return BitBoard::bitCount(pos.occupiedBB()) >= 25; // Hack to avoid expensive endgame positions
        };

        std::vector<UnMove> quiets, irreversibles;
        for (const UnMove& um : unMoves) {
            bool capture = um.ui.capturedPiece != Piece::EMPTY;
            bool pawnMove = um.move.promoteTo() != Piece::EMPTY ||
                Piece::makeWhite(goalPos.getPiece(um.move.to())) == Piece::WPAWN;
            if (capture || (pawnMove && canAnalyze(goalPos)))
                irreversibles.push_back(um);
            else
                quiets.push_back(um);
        }

        std::vector<UnMove> quietsInCheck;
        unMoves.clear();
        bool rejected = false;
        for (const UnMove& um : quiets) {
            Position tmpPos(goalPos);
            tmpPos.unMakeMove(um.move, um.ui);
            resetMoveCnt(tmpPos);
            bool valid = tmpPos == startPos;
            if (!valid) {
                if (canAnalyze(tmpPos) && MoveGen::inCheck(tmpPos)) {
                    quietsInCheck.push_back(um);
                    continue;
                } else {
                    std::vector<UnMove> unMoves2;
                    RevMoveGen::genMoves(tmpPos, unMoves2, false);
                    valid = !unMoves2.empty();
                }
            }
            if (valid) {
                unMoves.push_back(um);
                if (unMoves.size() > 1)
                    break;
            } else {
                rejected = true;
            }
        }

        auto knownIllegal = [&startPos,&goalPos,&log](const UnMove& um) -> bool {
            log << "Checking move: " << um << std::endl;
            Position tmpPos(goalPos);
            tmpPos.unMakeMove(um.move, um.ui);
            resetMoveCnt(tmpPos);
            U64 blocked;
            if (!computeBlocked(startPos, tmpPos, blocked))
                blocked = 0xffffffffffffffffULL;
            ProofKernel pk(startPos, tmpPos, blocked, log);
            std::vector<ProofKernel::PkMove> kernel;
            std::vector<ProofKernel::ExtPkMove> extKernel;
            if (pk.findProofKernel(kernel, extKernel) != ProofKernel::EXT_PROOF_KERNEL)
                return true;

            bool ret;
            try {
                ProofGame ps(TextIO::toFEN(startPos), TextIO::toFEN(tmpPos), true, {}, false, log);
                auto opts = ProofGame::Options().setSmallCache(true).setMaxNodes(2);
                ProofGame::Result result;
                ret = ps.search(opts, result) == INT_MAX;
            } catch (ChessError&) {
                ret = true;
            }
            if (ret)
                log << "Move rejected by recursive proof game search" << std::endl;
            return ret;
        };

        for (const UnMove& um : quietsInCheck) {
            if (unMoves.size() > 1)
                break;
            if (knownIllegal(um)) {
                rejected = true;
            } else {
                unMoves.push_back(um);
            }
        }
        bool validQuiet = !unMoves.empty();

        for (const UnMove& um : irreversibles) {
            if (unMoves.size() > 1)
                break;
            if (knownIllegal(um)) {
                rejected = true;
            } else {
                unMoves.push_back(um);
            }
        }

        if (unMoves.empty()) {
            if (rejected)
                throw ChessError("No possible last move, all moves rejected");
            else
                throw ChessError("No possible last move");
        }

        if (unMoves.size() == 1) {
            const UnMove& um = unMoves[0];
            log << "Forced last move: " << um << std::endl;
            goalPos.unMakeMove(um.move, um.ui);
            resetMoveCnt(goalPos);
            lastMoves.push_back(um.move);
            log << "New goalPos: " << TextIO::toFEN(goalPos) << std::endl;
        } else if (useNonForcedIrreversible && !validQuiet) {
            const UnMove& um = unMoves[0];
            log << "Only irreversible moves possible, assuming move: " << um << std::endl;
            goalPos.unMakeMove(um.move, um.ui);
            resetMoveCnt(goalPos);
            lastMoves.push_back(um.move);
            log << "New goalPos: " << TextIO::toFEN(goalPos) << std::endl;
        } else {
            std::reverse(lastMoves.begin(), lastMoves.end());
            break;
        }
    }
}

void
ProofGame::setRandomSeed(U64 seed) {
    rndSeed = hashU64(seed + hashU64(1));
}

void
ProofGame::validatePieceCounts(const Position& pos) {
    int pieceCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieceCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    // White must not have too many pieces
    int maxWPawns = 8;
    maxWPawns -= std::max(0, pieceCnt[Piece::WKNIGHT] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WBISHOP] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WROOK  ] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WQUEEN ] - 1);
    if (pieceCnt[Piece::WPAWN] > maxWPawns)
        throw ChessParseError("Too many white pieces");

    // Black must not have too many pieces
    int maxBPawns = 8;
    maxBPawns -= std::max(0, pieceCnt[Piece::BKNIGHT] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BBISHOP] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BROOK  ] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BQUEEN ] - 1);
    if (pieceCnt[Piece::BPAWN] > maxBPawns)
        throw ChessParseError("Too many black pieces");
}

int
ProofGame::search(const Options& opts, Result& result) {
    if (!opts.smallCache)
        pathDataCache.resize(1024*512);
    useNonAdmissible = opts.useNonAdmissible;

    Position startPos = TextIO::readFEN(initialFen);
    {
        int N = opts.dynamic ? distLowerBound(startPos) * 2 : 0;
        queue = make_unique<Queue>(TreeNodeCompare(nodes, opts.weightA, opts.weightB, N));
    }

    validatePieceCounts(startPos);
    int best = INT_MAX;
    addPosition(startPos, 0, true, false, best);
    {
        // Ignore moves causing position repetition
        struct HashMove {
            U64 hash;
            Move m;
        };
        std::vector<HashMove> hmVec;
        Position pos(startPos);
        UndoInfo ui;
        for (Move m : initialPath) {
            U64 hash = pos.zobristHash();
            hmVec.push_back({hash, m});
            pos.makeMove(m, ui);
        }
        for (int i = 0; i < (int)hmVec.size(); i++) {
            for (int j = 0; j < i; j++) {
                if (hmVec[j].hash == hmVec[i].hash) {
                    hmVec.erase(hmVec.begin() + j, hmVec.begin() + i);
                    i = j;
                }
            }
        }

        // Add all moves so that the full path for a solution can later be reconstructed
        pos = startPos;
        for (auto& e : hmVec) {
            assert(!queue->empty());
            U32 idx = queue->top();
            queue->pop();
            pos.makeMove(e.m, ui);
            addPosition(pos, idx, false, false, best);
        }
    }

    double t0 = currentTime();
    Position pos;
    S64 numNodes = 0;
    int minCost = -1;
    int smallestBound = INT_MAX;

    std::string delayedLog;
    int nNodesAtLogTime = -1;
    auto flushDelayed = [this,&numNodes,&delayedLog,&nNodesAtLogTime](bool force = true) {
        if (nNodesAtLogTime != -1) {
            if (force || numNodes >= nNodesAtLogTime + 100) {
                log << delayedLog << std::flush;
                delayedLog.clear();
                nNodesAtLogTime = -1;
            }
        }
    };
    auto logDelayed = [&numNodes,&delayedLog,&nNodesAtLogTime](std::stringstream& os) {
        delayedLog = os.str();
        nNodesAtLogTime = numNodes;
    };

    UndoInfo ui;
    while (!queue->empty() && (opts.maxNodes == -1 || numNodes < opts.maxNodes)) {
        const U32 idx = queue->top();
        queue->pop();
        const TreeNode& tn = nodes[idx];
        if (tn.ply + tn.bound >= best)
            continue;
        if (tn.ply + tn.bound > minCost) {
            flushDelayed();
            minCost = tn.ply + tn.bound;
            log << "min cost: " << minCost << " queue: " << queue->size()
                << " nodes: " << numNodes
                << " time: " << (currentTime() - t0) << std::endl;
        }

        numNodes++;
        pos.deSerialize(tn.psd);

        if (numNodes == 1)
            showPieceStats(pos);
        flushDelayed(false);

        if (tn.ply < best && isSolution(pos)) {
            flushDelayed();
            double elapsed = currentTime() - t0;
            log << tn.ply << " -w " << opts.weightA << ":" << opts.weightB
                << " queue: " << queue->size() << " nodes: " << numNodes
                << " time: " << elapsed << std::endl;
            getMoves(log, startPos, idx, true, result.proofGame);
            best = tn.ply;
            if (opts.acceptFirst)
                break;
        }

        U64 blocked;
        if (!computeBlocked(pos, blocked))
            continue;

        if (opts.verbose && (numNodes % 1000000) == 0) {
            flushDelayed();
            log << "ply: " << tn.ply << " bound: " << tn.bound
                << " queue: " << queue->size() << " nodes: " << numNodes
                << " time: " << (currentTime() - t0)
                << " " << TextIO::toFEN(pos) << std::endl;
        }

        bool anyChildren = false;
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        for (int i = 0; i < moves.size; i++) {
            if (((1ULL << moves[i].from()) | (1ULL << moves[i].to())) & blocked)
                continue;
            pos.makeMove(moves[i], ui);
            anyChildren |= addPosition(pos, idx, false, true, best);
            pos.unMakeMove(moves[i], ui);
        }
        if (opts.verbose && anyChildren) {
            const TreeNode& tn = nodes[idx]; // Old "tn" no longer valid
            if (tn.bound > 0 && tn.bound < smallestBound) {
                smallestBound = tn.bound;
                std::stringstream os;
                os << "bound: " << tn.bound << " -w " << opts.weightA << ":" << opts.weightB
                   << " queue: " << queue->size() << " nodes: " << numNodes
                   << " time: " << (currentTime() - t0) << std::endl;
                getMoves(os, startPos, idx, false, result.closestPath);
                result.smallestBound = smallestBound;
                logDelayed(os);
            }
        }
    }
    flushDelayed();
    double t1 = currentTime();
    log << "nodes: " << numNodes
        << " time: " << t1 - t0 <<  std::endl;

    result.numNodes = numNodes;
    result.computationTime = t1 - t0;

    if (best < INT_MAX)
        return best + lastMoves.size(); // Return best found solution length
    if (numNodes == opts.maxNodes)
        return -1;                      // Gave up, unknown if solution exists
    return INT_MAX;                     // No solution exists
}

bool
ProofGame::isInfeasible(Square& fromSq, Square& toSq) {
    Position startPos = TextIO::readFEN(initialFen);
    infeasibleFrom = infeasibleTo = Square();
    findInfeasible = true;
    if (distLowerBound(startPos) == INT_MAX) {
        fromSq = infeasibleFrom;
        toSq = infeasibleTo;
        return true;
    }
    return false;
}

bool
ProofGame::addPosition(const Position& pos, U32 parent, bool isRoot, bool checkBound, int best) {
    const int ply = isRoot ? 0 : nodes[parent].ply + 1;
    auto it = nodeHash.find(pos.zobristHash());
    if ((it != nodeHash.end()) && (it->second <= ply))
        return false;

    int bound = distLowerBound(pos);
    if (checkBound) {
        if (bound == INT_MAX)
            return false;
        if (best < INT_MAX && ply + bound >= best)
            return false;
    }

    TreeNode tn;
    pos.serialize(tn.psd);
    tn.parent = parent;
    tn.ply = ply;
    tn.bound = bound;
    U32 idx = nodes.size();
    U64 rnd = hashU64(rndSeed + idx);
    tn.computePrio(pos, goalPos, rnd);

    nodes.push_back(tn);
    nodeHash[pos.zobristHash()] = ply;
    queue->push(idx);
    return true;
}

void
ProofGame::getMoves(std::ostream& logStream, const Position& startPos, int idx,
                    bool includeLastMoves, std::vector<Move>& movePath) const {
    movePath.clear();
    while (true) {
        const TreeNode& tn = nodes[idx];
        if (tn.ply == 0)
            break;

        Position target;
        target.deSerialize(tn.psd);

        Position pos;
        pos.deSerialize(nodes[tn.parent].psd);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        UndoInfo ui;
        for (int i = 0; i < moves.size; i++) {
            pos.makeMove(moves[i], ui);
            if (pos == target) {
                pos.unMakeMove(moves[i], ui);
                movePath.push_back(moves[i]);
                break;
            }
            pos.unMakeMove(moves[i], ui);
        }

        idx = tn.parent;
    }
    std::reverse(movePath.begin(), movePath.end());

    if (includeLastMoves)
        movePath.insert(movePath.end(), lastMoves.begin(), lastMoves.end());
    Position pos = startPos;
    UndoInfo ui;
    for (size_t i = 0; i < movePath.size(); i++) {
        if (i > 0)
            logStream << ' ';
        logStream << TextIO::moveToString(pos, movePath[i], false);
        pos.makeMove(movePath[i], ui);
    }
    logStream << std::endl;
}

// --------------------------------------------------------------------------------

int
ProofGame::distLowerBound(const Position& pos) {
    int pieceCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieceCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    if (!enoughRemainingPieces(pieceCnt))
        return INT_MAX;

    U64 blocked;
    if (!computeBlocked(pos, blocked))
        return INT_MAX;

    const int numWhiteExtraPieces = (BitBoard::bitCount(pos.whiteBB()) -
                                     BitBoard::bitCount(goalPos.whiteBB()));
    const int numBlackExtraPieces = (BitBoard::bitCount(pos.blackBB()) -
                                     BitBoard::bitCount(goalPos.blackBB()));
    const int excessWPawns = pieceCnt[Piece::WPAWN] - goalPieceCnt[Piece::WPAWN];
    const int excessBPawns = pieceCnt[Piece::BPAWN] - goalPieceCnt[Piece::BPAWN];

    if (!capturesFeasible(pos, pieceCnt, numWhiteExtraPieces, numBlackExtraPieces,
                          excessWPawns, excessBPawns))
        return INT_MAX;

    int neededMoves[2] = {0, 0};
    if (!computeNeededMoves(pos, blocked, numWhiteExtraPieces, numBlackExtraPieces,
                            excessWPawns, excessBPawns, neededMoves))
        return INT_MAX;

    // Compute number of needed moves to perform all captures
    int nBlackToCapture = BitBoard::bitCount(pos.blackBB()) - BitBoard::bitCount(goalPos.blackBB());
    int nWhiteToCapture = BitBoard::bitCount(pos.whiteBB()) - BitBoard::bitCount(goalPos.whiteBB());
    neededMoves[0] = std::max(neededMoves[0], nBlackToCapture);
    neededMoves[1] = std::max(neededMoves[1], nWhiteToCapture);

    // Compute number of needed plies from number of needed white/black moves
    int wNeededPlies = neededMoves[0] * 2;
    int bNeededPlies = neededMoves[1] * 2;
    if (pos.isWhiteMove())
        bNeededPlies++;
    else
        wNeededPlies++;
    if (goalPos.isWhiteMove())
        bNeededPlies--;
    else
        wNeededPlies--;
    int ret = std::max(wNeededPlies, bNeededPlies);
    assert(ret >= 0);
    return ret;
}

bool
ProofGame::enoughRemainingPieces(int pieceCnt[]) const {
    int wProm = pieceCnt[Piece::WPAWN] - goalPieceCnt[Piece::WPAWN];
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WQUEEN] - pieceCnt[Piece::WQUEEN]);
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WROOK] - pieceCnt[Piece::WROOK]);
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WBISHOP] - pieceCnt[Piece::WBISHOP]);
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WKNIGHT] - pieceCnt[Piece::WKNIGHT]);
    if (wProm < 0)
        return false;

    int bProm = pieceCnt[Piece::BPAWN] - goalPieceCnt[Piece::BPAWN];
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BQUEEN] - pieceCnt[Piece::BQUEEN]);
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BROOK] - pieceCnt[Piece::BROOK]);
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BBISHOP] - pieceCnt[Piece::BBISHOP]);
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BKNIGHT] - pieceCnt[Piece::BKNIGHT]);
    if (bProm < 0)
        return false;
    return true;
}

void
ProofGame::showPieceStats(const Position& pos) const {
    int currCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        currCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    auto print = [this,&currCnt](int piece, const std::string& pieceName) -> int {
        int cnt = goalPieceCnt[piece] - currCnt[piece];
        log << pieceName << ':' << cnt << ' ';
        return cnt;
    };
    auto printB = [this,&pos](int piece, const std::string& pieceName, bool dark) -> int {
        U64 mask = dark ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
        int cnt = BitBoard::bitCount(goalPos.pieceTypeBB((Piece::Type)piece) & mask);
        cnt -=    BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)piece) & mask);
        log << pieceName << ':' << cnt << ' ';
        return cnt;
    };

    int spare = 0;
    spare -= std::max(0, print (Piece::WQUEEN,  "Q"));
    spare -= std::max(0, print (Piece::WROOK,   "R"));
    spare -= std::max(0, printB(Piece::WBISHOP, "Bd", true));
    spare -= std::max(0, printB(Piece::WBISHOP, "Bl", false));
    spare -= std::max(0, print (Piece::WKNIGHT, "N"));
    spare -= std::min(0, print (Piece::WPAWN,   "P"));
    log << "sP:" << spare << std::endl;

    spare = 0;
    spare -= std::max(0, print (Piece::BQUEEN,  "q"));
    spare -= std::max(0, print (Piece::BROOK,   "r"));
    spare -= std::max(0, printB(Piece::BBISHOP, "bd", true));
    spare -= std::max(0, printB(Piece::BBISHOP, "bl", false));
    spare -= std::max(0, print (Piece::BKNIGHT, "n"));
    spare -= std::min(0, print (Piece::BPAWN,   "p"));
    log << "sp:" << spare << std::endl;
}

bool
ProofGame::capturesFeasible(const Position& pos, int pieceCnt[],
                            int numWhiteExtraPieces, int numBlackExtraPieces,
                            int excessWPawns, int excessBPawns) {
    for (int c = 0; c < 2; c++) {
        const Piece::Type  p = (c == 0) ? Piece::WPAWN : Piece::BPAWN;
        Assignment<int>& as = captureAP[c];
        U64 from = pos.pieceTypeBB(p);
        int fi = 0;
        while (from) {
            Square fromSq = BitBoard::extractSquare(from);
            U64 to = goalPos.pieceTypeBB(p);
            int ti = 0;
            while (to) {
                Square toSq = BitBoard::extractSquare(to);
                int d = std::abs(fromSq.getX() - toSq.getX());
                as.setCost(fi, ti, d);
                ti++;
            }
            for ( ; ti < 8; ti++)
                as.setCost(fi, ti, 0);    // sq -> captured, no cost
            fi++;
        }
        for (; fi < 8; fi++) {
            int ti;
            for (ti = 0; ti < goalPieceCnt[p]; ti++)
                as.setCost(fi, ti, bigCost); // captured -> sq, not possible
            for ( ; ti < 8; ti++)
                as.setCost(fi, ti, 0);       // captured -> captured, no cost
        }

        const std::vector<int>& s = as.optWeightMatch();
        int cost = 0;
        for (int i = 0; i < 8; i++)
            cost += as.getCost(i, s[i]);
        if (c == 0) {
            int neededBCaptured = cost;
            if (neededBCaptured > numBlackExtraPieces)
                return false;

            int neededBProm = (std::max(0, goalPieceCnt[Piece::BQUEEN]  - pieceCnt[Piece::BQUEEN]) +
                               std::max(0, goalPieceCnt[Piece::BROOK]   - pieceCnt[Piece::BROOK]) +
                               std::max(0, goalPieceCnt[Piece::BBISHOP] - pieceCnt[Piece::BBISHOP]) +
                               std::max(0, goalPieceCnt[Piece::BKNIGHT] - pieceCnt[Piece::BKNIGHT]));
            int excessBPieces = (std::max(0, pieceCnt[Piece::BQUEEN]  - goalPieceCnt[Piece::BQUEEN]) +
                                 std::max(0, pieceCnt[Piece::BROOK]   - goalPieceCnt[Piece::BROOK]) +
                                 std::max(0, pieceCnt[Piece::BBISHOP] - goalPieceCnt[Piece::BBISHOP]) +
                                 std::max(0, pieceCnt[Piece::BKNIGHT] - goalPieceCnt[Piece::BKNIGHT]));
            if (neededBCaptured + neededBProm > excessBPawns + excessBPieces)
                return false;
        } else {
            int neededWCaptured = cost;
            if (neededWCaptured > numWhiteExtraPieces)
                return false;

            int neededWProm = (std::max(0, goalPieceCnt[Piece::WQUEEN]  - pieceCnt[Piece::WQUEEN]) +
                               std::max(0, goalPieceCnt[Piece::WROOK]   - pieceCnt[Piece::WROOK]) +
                               std::max(0, goalPieceCnt[Piece::WBISHOP] - pieceCnt[Piece::WBISHOP]) +
                               std::max(0, goalPieceCnt[Piece::WKNIGHT] - pieceCnt[Piece::WKNIGHT]));
            int excessWPieces = (std::max(0, pieceCnt[Piece::WQUEEN]  - goalPieceCnt[Piece::WQUEEN]) +
                                 std::max(0, pieceCnt[Piece::WROOK]   - goalPieceCnt[Piece::WROOK]) +
                                 std::max(0, pieceCnt[Piece::WBISHOP] - goalPieceCnt[Piece::WBISHOP]) +
                                 std::max(0, pieceCnt[Piece::WKNIGHT] - goalPieceCnt[Piece::WKNIGHT]));

            if (neededWCaptured + neededWProm > excessWPawns + excessWPieces)
                return false;
        }
    }
    return true;
}

bool
ProofGame::computeNeededMoves(const Position& pos, U64 blocked,
                              int numWhiteExtraPieces, int numBlackExtraPieces,
                              int excessWPawns, int excessBPawns,
                              int neededMoves[]) {
    std::vector<SqPathData> sqPathData;
    SqPathData promPath[2][8]; // Pawn cost to each possible promotion square
    if (!computeShortestPathData(pos, numWhiteExtraPieces, numBlackExtraPieces,
                                 promPath, sqPathData, blocked))
        return false;

    // Sets of squares where pieces have to be captured
    U64 captureSquares[2][16]; // [c][idx], 0 terminated.
    // Sum of zero bits in captureSquares
    int nCaptureConstraints[2];

    // Compute initial estimate of captureSquares[0]. Further computations are based
    // on analyzing the assignment problem cost matrix for the other side.
    for (int c = 0; c < 1; c++) {
        // If a pawn has to go to a square in front of a blocked square, a capture has
        // to be made on the pawn goal square.
        const bool whiteToBeCaptured = (c == 0);
        const Piece::Type pawn = whiteToBeCaptured ? Piece::BPAWN : Piece::WPAWN;
        U64 capt = goalPos.pieceTypeBB(pawn) & ~pos.pieceTypeBB(pawn);
        capt &= whiteToBeCaptured ? (blocked >> 8) : (blocked << 8);
        int idx = 0;
        while (capt) {
            captureSquares[c][idx++] = 1ULL << BitBoard::extractSquare(capt);
        }
        nCaptureConstraints[c] = 63 * idx;
        captureSquares[c][idx++] = 0;
    }
    captureSquares[1][0] = 0;
    nCaptureConstraints[1] = 0;

    Square rowToSq[2][16], colToSq[2][16];
    int c = 0;
    bool solved[2] = { false, false };
    while (!solved[c]) {
        const bool wtm = (c == 0);
        const int maxCapt = wtm ? numBlackExtraPieces : numWhiteExtraPieces;
        int cost = 0;
        U64 fromPieces = (wtm ? pos.whiteBB() : pos.blackBB()) & ~blocked;
        int N = BitBoard::bitCount(fromPieces);
        if (N > 0) {
            assert(N <= maxMoveAPSize);
            Assignment<int>& as = moveAP[c][N];
            for (int f = 0; f < N; f++) {
                assert(fromPieces != 0);
                const Square fromSq = BitBoard::extractSquare(fromPieces);
                rowToSq[c][f] = fromSq;
                bool canPromote = wtm ? (excessWPawns > 0) && (pos.getPiece(fromSq) == Piece::WPAWN)
                                      : (excessBPawns > 0) && (pos.getPiece(fromSq) == Piece::BPAWN);
                int t = 0;
                for (size_t ci = 0; ci < sqPathData.size(); ci++) {
                    Square toSq = sqPathData[ci].square;
                    int p = goalPos.getPiece(toSq);
                    if (Piece::isWhite(p) == wtm) {
                        assert(t < N);
                        colToSq[c][t] = toSq;
                        int pLen;
                        if (p == pos.getPiece(fromSq)) {
                            pLen = sqPathData[ci].spd->pathLen[fromSq.asInt()];
                            if (pLen < 0)
                                pLen = bigCost;
                        } else
                            pLen = bigCost;
                        if (canPromote) {
                            int cost2 = promPathLen(wtm, fromSq, p, blocked, maxCapt,
                                                    *sqPathData[ci].spd, promPath[c]);
                            pLen = std::min(pLen, cost2);
                        }
                        as.setCost(f, t, pLen < 0 ? bigCost : pLen);
                        t++;
                    }
                }

                // Handle pieces to be captured
                int idx = 0;
                for (; t < N; t++) {
                    int cost = 0;
                    U64 captSquares = captureSquares[c][idx];
                    if (captSquares) {
                        int p = pos.getPiece(fromSq);
                        cost = minDistToSquares(p, fromSq, getBlocked(blocked, pos, p),
                                                maxCapt, promPath[c], captSquares, canPromote);
                        idx++;
                    }
                    colToSq[c][t] = Square(-1);
                    as.setCost(f, t, cost);
                }
                if (captureSquares[c][idx])
                    return false;
            }
            cost = solveAssignment(as);
            if (cost >= bigCost) {
                if (findInfeasible)
                    findInfeasibleMove(pos, as, N, rowToSq[c], colToSq[c]);
                return false;
            }

            int nConstr;
            if (!computeAllCutSets(as, rowToSq[c], colToSq[c], wtm, blocked, maxCapt,
                                   captureSquares[1-c], nConstr))
                return false;
            assert(nCaptureConstraints[1-c] <= nConstr);
            if (nCaptureConstraints[1-c] < nConstr) {
                nCaptureConstraints[1-c] = nConstr;
                solved[1-c] = false;
            }
        }
        neededMoves[c] = cost;
        solved[c] = true;
        c = 1 - c;
    }

    if (useNonAdmissible) {
        for (int c = 0; c < 2; c++) {
            const bool wtm = (c == 0);
            U64 fromPieces = (wtm ? pos.whiteBB() : pos.blackBB()) & ~blocked;
            int N = BitBoard::bitCount(fromPieces);
            Assignment<int>& as = moveAP[c][N];
            const std::vector<int>& s = as.optWeightMatch();
            for (int i = 0; i < N; i++) {
                int cost = as.getCost(i, s[i]);
                if (cost == 0)
                    continue;

                const Square fromSq = rowToSq[c][i];
                const Square toSq = colToSq[c][s[i]];
                if (!toSq.isValid())
                    continue; // Piece is captured

                const ShortestPathData* spd = nullptr;
                for (const SqPathData& d : sqPathData) {
                    if (d.square == toSq) {
                        spd = d.spd.get();
                        break;
                    }
                }

                U64 obstacles = getMovePathObstacles(pos, blocked, fromSq, toSq, *spd);
                int wObst = BitBoard::bitCount(obstacles & pos.whiteBB());
                int bObst = BitBoard::bitCount(obstacles & pos.blackBB());
                neededMoves[0] += wObst * 2;
                neededMoves[1] += bObst * 2;
            }
        }
    }

    return true;
}

void
ProofGame::printAssignment(const Assignment<int>& as, int N,
                           const Square (&rowToSq)[16], const Square (&colToSq)[16]) const {
    auto sq2Str = [](Square sq) -> std::string {
        if (!sq.isValid())
            return "--";
        return TextIO::squareToString(sq);
    };
    std::ostream& os = log;
    os << "  ";
    for (int c = 0; c < N; c++)
        os << "   " << sq2Str(colToSq[c]);
    os << '\n';
    for (int r = 0; r < N; r++) {
        os << sq2Str(rowToSq[r]);
        for (int c = 0; c < N; c++)
            os << ' ' << std::setw(4) << as.getCost(r, c);
        os << '\n';
    }
}

void
ProofGame::findInfeasibleMove(const Position& pos,
                              const Assignment<int>& as, int N,
                              const Square (&rowToSq)[16], const Square (&colToSq)[16]) {
    findInfeasible = false;

    auto wrongBishop = [](int p, Square fromSq, Square toSq) -> bool {
        return Piece::makeWhite(p) == Piece::WBISHOP &&
            fromSq.isDark() != toSq.isDark();
    };

    for (int r = 0; r < N; r++) {
        bool allBig = true;
        for (int c = 0; c < N; c++) {
            if (as.getCost(r, c) != bigCost) {
                allBig = false;
                break;
            }
        }
        if (allBig) {
            Square fromSq = rowToSq[r];
            int p = pos.getPiece(fromSq);
            U64 mask = goalPos.pieceTypeBB((Piece::Type)p);
            while (mask != 0) {
                Square toSq = BitBoard::extractSquare(mask);
                if (fromSq == toSq || wrongBishop(p, fromSq, toSq))
                    continue;
                infeasibleFrom = fromSq;
                infeasibleTo = toSq;
                return;
            }
        }
    }

    for (int c = 0; c < N; c++) {
        bool allBig = true;
        for (int r = 0; r < N; r++) {
            if (as.getCost(r, c) != bigCost) {
                allBig = false;
                break;
            }
        }
        if (allBig) {
            Square toSq = colToSq[c];
            if (toSq.isValid()) {
                int p = goalPos.getPiece(toSq);
                U64 mask = pos.pieceTypeBB((Piece::Type)p);
                while (mask != 0) {
                    Square fromSq = BitBoard::extractSquare(mask);
                    if (fromSq == toSq || wrongBishop(p, fromSq, toSq))
                        continue;
                    infeasibleFrom = fromSq;
                    infeasibleTo = toSq;
                    return;
                }
            }
        }
    }
}

U64
ProofGame::ShortestPathData::getNextSquares(int piece, Square fromSq, U64 blocked) const {
    U64 reachable = 0;
    switch (piece) {
    case Piece::WQUEEN: case Piece::BQUEEN:
        reachable = (BitBoard::rookAttacks(fromSq, blocked) |
                     BitBoard::bishopAttacks(fromSq, blocked));
        break;
    case Piece::WROOK: case Piece::BROOK:
        reachable = BitBoard::rookAttacks(fromSq, blocked);
        break;
    case Piece::WBISHOP: case Piece::BBISHOP:
        reachable = BitBoard::bishopAttacks(fromSq, blocked);
        break;
    case Piece::WKNIGHT: case Piece::BKNIGHT:
        reachable = BitBoard::knightAttacks(fromSq);
        break;
    case Piece::WKING: case Piece::BKING:
        reachable = BitBoard::kingAttacks(fromSq);
        break;
    case Piece::WPAWN:
        reachable = BitBoard::wPawnAttacks(fromSq);
        reachable |= 1ULL << (fromSq + 8);
        if (fromSq.getY() == 1 && ((1ULL << (fromSq + 8)) & blocked) == 0)
            reachable |= 1ULL << (fromSq + 16);
        break;
    case Piece::BPAWN:
        reachable = BitBoard::bPawnAttacks(fromSq);
        reachable |= 1ULL << (fromSq - 8);
        if (fromSq.getY() == 6 && ((1ULL << (fromSq - 8)) & blocked) == 0)
            reachable |= 1ULL << (fromSq - 16);
        break;
    }
    reachable &= ~blocked;
    reachable &= fromSquares;

    int dist = pathLen[fromSq.asInt()];
    U64 ret = 0;
    while (reachable) {
        Square sq = BitBoard::extractSquare(reachable);
        if (pathLen[sq.asInt()] < dist)
            ret |= 1ULL << sq;
    }
    return ret;
}

U64
ProofGame::getMovePathObstacles(const Position& pos, U64 blocked,
                                Square fromSq, Square toSq,
                                const ShortestPathData& spd) const {
    U64 path = 0;

    bool promotion = false;
    int piece = pos.getPiece(fromSq);
    if (piece != goalPos.getPiece(toSq)) { // Pawn promotion needed
        switch (piece) {
        case Piece::WPAWN:
            path |= BitBoard::northFill(1ULL << (fromSq + 8));
            break;
        case Piece::BPAWN:
            path |= BitBoard::southFill(1ULL << (fromSq - 8));
            break;
        default:
            assert(false);
        }
        piece = goalPos.getPiece(toSq);
        promotion = true;
    }

    U64 obstacles = 0; // Squares having same piece in pos and goalPos
    {
        U64 m = pos.occupiedBB() & goalPos.occupiedBB();
        while (m) {
            Square sq = BitBoard::extractSquare(m);
            if (pos.getPiece(sq) == goalPos.getPiece(sq))
                obstacles |= 1ULL << sq;
        }
    }

    while (fromSq != toSq) {
        U64 reachable = spd.getNextSquares(piece, fromSq, blocked);
        if (promotion && reachable == 0)
            return 0; // Give up, need to find other promotion square

        assert(reachable != 0);
        Square sq1, sq2;
        while (reachable) {
            Square sq = BitBoard::extractSquare(reachable);
            if ((1ULL << sq) & obstacles) {
                sq2 = sq;
            } else {
                sq1 = sq;
                break;
            }
        }
        Square sq = sq1.isValid() ? sq1 : sq2;
        assert(sq.isValid());
        switch (piece) {
        case Piece::WQUEEN: case Piece::BQUEEN:
        case Piece::WROOK: case Piece::BROOK:
        case Piece::WBISHOP: case Piece::BBISHOP:
        case Piece::WPAWN: case Piece::BPAWN:
            path |= BitBoard::squaresBetween(fromSq, sq);
            break;
        default:
            break;
        }
        path |= 1ULL << sq;
        fromSq = sq;
    }
    return path & obstacles;
}

U64
ProofGame::getBlocked(U64 blocked, const Position& pos, int pieceType) const {
    if (pieceType == Piece::WKING) {
        blocked |= BitBoard::bPawnAttacksMask(blocked & pos.pieceTypeBB(Piece::BPAWN));
        blocked &= ~pos.pieceTypeBB(Piece::WKING);
    } else if (pieceType == Piece::BKING) {
        blocked |= BitBoard::wPawnAttacksMask(blocked & pos.pieceTypeBB(Piece::WPAWN));
        blocked &= ~pos.pieceTypeBB(Piece::BKING);
    }
    return blocked;
}

bool
ProofGame::computeShortestPathData(const Position& pos,
                                   int numWhiteExtraPieces, int numBlackExtraPieces,
                                   SqPathData promPath[2][8],
                                   std::vector<SqPathData>& sqPathData, U64& blocked) {
    std::vector<SqPathData> pending;
    U64 pieces = goalPos.occupiedBB() & ~blocked;
    while (pieces) {
        Square sq = BitBoard::extractSquare(pieces);
        pending.emplace_back(sq, nullptr);
    }
    while (!pending.empty()) {
        const auto e = pending.back();
        pending.pop_back();
        const Square sq = e.square;
        const Piece::Type p = (Piece::Type)goalPos.getPiece(sq);
        const bool wtm = Piece::isWhite(p);
        const int maxCapt = wtm ? numBlackExtraPieces : numWhiteExtraPieces;
        auto spd = shortestPaths(p, sq, getBlocked(blocked, pos, p), maxCapt);
        bool testPromote = false;
        switch (p) {
        case Piece::WQUEEN: case Piece::WROOK: case Piece::WBISHOP: case Piece::WKNIGHT:
            if (wtm && sq.getY() == 7)
                testPromote = true;
            break;
        case Piece::BQUEEN: case Piece::BROOK: case Piece::BBISHOP: case Piece::BKNIGHT:
            if (!wtm && sq.getY() == 0)
                testPromote = true;
            break;
        default:
            break;
        }
        bool promotionPossible = false;
        if (testPromote) {
            int c = wtm ? 0 : 1;
            int x = sq.getX();
            Piece::Type pawn = wtm ? Piece::WPAWN : Piece::BPAWN;
            if (!promPath[c][x].spd)
                promPath[c][x].spd = shortestPaths(pawn, sq, blocked, maxCapt);
            if (promPath[c][x].spd->fromSquares & pos.pieceTypeBB(pawn))
                promotionPossible = true;
        }
        if (!findInfeasible && (spd->fromSquares == (1ULL << sq)) && !promotionPossible) {
            // Piece on goal square can not move, must be same piece on sq in pos
            if (pos.getPiece(sq) != p)
                return false;
            blocked |= 1ULL << sq;
            pending.insert(pending.end(), sqPathData.begin(), sqPathData.end());
            sqPathData.clear();
            for (int c = 0; c < 2; c++)
                for (int x = 0; x < 8; x++)
                    promPath[c][x].spd = nullptr;
        } else {
            sqPathData.emplace_back(sq, spd);
        }
    }
    return true;
}

int
ProofGame::promPathLen(bool wtm, Square fromSq, int targetPiece, U64 blocked, int maxCapt,
                       const ShortestPathData& toSqPath, SqPathData promPath[8]) {
    int pLen = INT_MAX;
    switch (targetPiece) {
    case Piece::WQUEEN:  case Piece::BQUEEN:
    case Piece::WROOK:   case Piece::BROOK:
    case Piece::WBISHOP: case Piece::BBISHOP:
    case Piece::WKNIGHT: case Piece::BKNIGHT: {
        for (int x = 0; x < 8; x++) {
            Square promSq(x, wtm ? 7 : 0);
            if (!promPath[x].spd)
                promPath[x].spd = shortestPaths(wtm ? Piece::WPAWN : Piece::BPAWN,
                                                promSq, blocked, maxCapt);
            int promCost = promPath[x].spd->pathLen[fromSq.asInt()];
            if (promCost >= 0) {
                int tmp = toSqPath.pathLen[promSq.asInt()];
                if (tmp >= 0)
                    pLen = std::min(pLen, promCost + tmp);
            }
        }
        break;
    default:
        break;
    }
    }
    return pLen;
}

bool
ProofGame::computeAllCutSets(const Assignment<int>& as, Square rowToSq[16], Square colToSq[16],
                             bool wtm, U64 blocked, int maxCapt,
                             U64 cutSets[16], int& nConstraints) {
    const int N = as.getSize();
    int nCutSets = 0;
    for (int t = 0; t < N; t++) {
        Square toSq = colToSq[t];
        if (!toSq.isValid())
            break;
        int p = goalPos.getPiece(toSq);
        if (p == (wtm ? Piece::WPAWN : Piece::BPAWN)) {
            U64 fromSqMask = 0;
            for (int f = 0; f < N; f++)
                if (as.getCost(f, t) < bigCost)
                    fromSqMask |= 1ULL << rowToSq[f];
            if (!computeCutSets(wtm, fromSqMask, toSq, blocked, maxCapt, cutSets, nCutSets))
                return false;
        }
    }
    cutSets[nCutSets++] = 0;

    int nConstr = 0;
    for (int i = 0; cutSets[i] != 0; i++) {
        assert(i < 16);
        nConstr += BitBoard::bitCount(~cutSets[i]);
    }
    nConstraints = nConstr;

    return true;
}

bool
ProofGame::computeCutSets(bool wtm, U64 fromSqMask, Square toSq, U64 blocked, int maxCapt,
                          U64 cutSets[16], int& nCutSets) {
    U64 allPaths = 0;
    U64 m = fromSqMask;
    while (m) {
        Square fromSq = BitBoard::extractSquare(m);
        allPaths |= allPawnPaths(wtm, fromSq, toSq, blocked, maxCapt);
    }
    if (!allPaths)
        return true;

    int n = nCutSets;
    U64 oldSquares = 0;
    U64 newSquares = 1ULL << toSq;
    while (true) {
        // Add squares reachable by non-capture moves
        while (true) {
            U64 tmp = (wtm ? (newSquares >> 8) : (newSquares << 8)) & allPaths;
            if ((newSquares | tmp) == newSquares)
                break;
            newSquares |= tmp;
        }

        if (newSquares & fromSqMask)
            break;

        if (n >= 15)
            return false;
        cutSets[n++] = newSquares & ~oldSquares;
        oldSquares = newSquares;

        // Add squares reachable by capture moves
        newSquares |= (wtm ? BitBoard::bPawnAttacksMask(newSquares)
                           : BitBoard::wPawnAttacksMask(newSquares)) & allPaths;
    }
    cutSets[n] = 0;
    nCutSets = n;
    return true;
}

U64
ProofGame::allPawnPaths(bool wtm, Square fromSq, Square toSq, U64 blocked, int maxCapt) {
    int yDelta = fromSq.getY() - toSq.getY();
    maxCapt = std::min(maxCapt, std::abs(yDelta)); // Can't make use of more than yDelta captures
    Piece::Type pawn = wtm ? Piece::WPAWN : Piece::BPAWN;
    Piece::Type oPawn = wtm ? Piece::BPAWN : Piece::WPAWN;
    U64 mask = 0;
    for (int c = 0; c <= maxCapt; c++) {
        auto tData = shortestPaths(pawn, toSq, blocked, c);
        auto fData = shortestPaths(oPawn, fromSq, blocked, maxCapt-c);
        mask |= tData->fromSquares & fData->fromSquares;
    }
    return mask;
}

int
ProofGame::minDistToSquares(int piece, Square fromSq, U64 blocked, int maxCapt,
                            SqPathData promPath[8], U64 targetSquares, bool canPromote) {
    const bool wtm = Piece::isWhite(piece);
    int best = bigCost;
    while (targetSquares) {
        Square captSq = BitBoard::extractSquare(targetSquares);
        auto spd = shortestPaths((Piece::Type)piece, captSq, blocked, maxCapt);
        int pLen = spd->pathLen[fromSq.asInt()];
        if (pLen < 0)
            pLen = bigCost;
        if (canPromote)
            pLen = promPathLen(wtm, fromSq, blocked, maxCapt, captSq, promPath, pLen);
        best = std::min(best, pLen);
    }
    return best;
}

int
ProofGame::promPathLen(bool wtm, Square fromSq, U64 blocked, int maxCapt,
                       Square toSq, SqPathData promPath[8], int pLen) {
    int firstP = wtm ? Piece::WQUEEN : Piece::BQUEEN;
    int lastP = wtm ? Piece::WKNIGHT : Piece::BKNIGHT;
    for (int x = 0; x < 8; x++) {
        Square promSq(x, wtm ? 7 : 0);
        if (!promPath[x].spd)
            promPath[x].spd = shortestPaths(wtm ? Piece::WPAWN : Piece::BPAWN,
                                            promSq, blocked, maxCapt);
        int promCost = promPath[x].spd->pathLen[fromSq.asInt()];
        if (promCost >= 0 && promCost < pLen) {
            int cost2 = INT_MAX;
            for (int p = firstP; p <= lastP; p++) {
                auto spd2 = shortestPaths((Piece::Type)p, toSq, blocked, maxCapt);
                int tmp = spd2->pathLen[promSq.asInt()];
                if (tmp >= 0)
                    cost2 = std::min(cost2, promCost + tmp);
            }
            pLen = std::min(pLen, cost2);
        }
    }
    return pLen;
}

int
ProofGame::solveAssignment(Assignment<int>& as) {
    const int N = as.getSize();

    // Count number of choices for each row/column
    int nValidR[16];
    int nValidC[16];
    for (int i = 0; i < N; i++)
        nValidR[i] = nValidC[i] = 0;
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            if (as(r,c) < bigCost) {
                nValidR[r]++;
                nValidC[c]++;
            }
        }
    }

    // Find rows/columns with exactly one choice
    U64 rowsToCheck = 0;
    U64 colsToCheck = 0;
    for (int i = 0; i < N; i++) {
        if (nValidR[i] == 1)
            rowsToCheck |= (1 << i);
        if (nValidC[i] == 1)
            colsToCheck |= (1 << i);
    }

    // If a row/column only has one choice, remove all other choices from the corresponding column/row
    U64 rowsHandled = 0;
    U64 colsHandled = 0;
    while (rowsToCheck | colsToCheck) {
        if (rowsToCheck) {
            int r = BitUtil::extractBit(rowsToCheck);
            if ((nValidR[r] == 1) && !(rowsHandled & (1 << r))) {
                int c;
                for (c = 0; c < N; c++)
                    if (as(r, c) < bigCost)
                        break;
                for (int r2 = 0; r2 < N; r2++) {
                    if ((r2 != r) && (as(r2, c) < bigCost)) {
                        as.setCost(r2, c, bigCost);
                        if (--nValidR[r2] == 1)
                            rowsToCheck |= 1 << r2;
                    }
                }
                rowsHandled |= 1 << r;
                colsHandled |= 1 << c;
            }
        }
        if (colsToCheck) {
            int c = BitUtil::extractBit(colsToCheck);
            if ((nValidC[c] == 1) && !(colsHandled & (1 << c))) {
                int r;
                for (r = 0; r < N; r++)
                    if (as(r, c) < bigCost)
                        break;
                for (int c2 = 0; c2 < N; c2++) {
                    if ((c2 != c) && (as(r, c2) < bigCost)) {
                        as.setCost(r, c2, bigCost);
                        if (--nValidC[c2] == 1)
                            colsToCheck |= 1 << c2;
                    }
                }
                rowsHandled |= 1 << r;
                colsHandled |= 1 << c;
            }
        }
    }

    // Solve the assignment problem and return the optimal cost
    int cost = 0;
    const std::vector<int>& s = as.optWeightMatch();
    for (int i = 0; i < N; i++)
        cost += as.getCost(i, s[i]);
    return cost;
}

bool
ProofGame::computeBlocked(const Position& pos, U64& blocked) const {
    return computeBlocked(pos, goalPos, blocked, findInfeasible);
}

bool
ProofGame::computeBlocked(const Position& pos, const Position& goalPos, U64& blocked,
                          bool findInfeasible) {
    blocked = 0;
    const U64 wGoalPawns = goalPos.pieceTypeBB(Piece::WPAWN);
    const U64 bGoalPawns = goalPos.pieceTypeBB(Piece::BPAWN);
    const U64 wCurrPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 bCurrPawns = pos.pieceTypeBB(Piece::BPAWN);

    U64 goalUnMovedPawns = (wGoalPawns & BitBoard::maskRow2) | (bGoalPawns & BitBoard::maskRow7);
    U64 currUnMovedPawns = (wCurrPawns & BitBoard::maskRow2) | (bCurrPawns & BitBoard::maskRow7);
    if (goalUnMovedPawns & ~currUnMovedPawns)
        return false;
    blocked |= goalUnMovedPawns;

    const int nWhiteExtraPieces = (BitBoard::bitCount(pos.whiteBB()) -
                                   BitBoard::bitCount(goalPos.whiteBB()));
    const int nBlackExtraPieces = (BitBoard::bitCount(pos.blackBB()) -
                                   BitBoard::bitCount(goalPos.blackBB()));

    U64 wUsefulPawnSquares = 0;
    // Compute pawns that are blocked because advancing them would leave too few
    // remaining pawns in the cone of squares that can reach the pawn square.
    U64 m = wGoalPawns & ~blocked;
    while (m) {
        int sq = BitUtil::firstBit(m); m &= ~(1ULL << sq);
        U64 mask = bPawnReachable[sq][maxPawnCapt];
        wUsefulPawnSquares |= mask;
        int nGoal = BitBoard::bitCount(wGoalPawns & mask);
        int nCurr = BitBoard::bitCount(wCurrPawns & mask);
        if (nCurr < nGoal)
            return false;
        if ((nCurr == nGoal) && (wCurrPawns & (1ULL << sq))) {
            blocked |= (1ULL << sq);
        } else if (nBlackExtraPieces < maxPawnCapt) {
            mask = bPawnReachable[sq][nBlackExtraPieces];
            if ((wCurrPawns & mask & ~blocked) == (1ULL << sq))
                blocked |= (1ULL << sq);
        }
    }

    if (BitBoard::bitCount(wGoalPawns) == BitBoard::bitCount(wCurrPawns)) {
        // Compute pawns that are blocked because advancing them would put
        // them on a square from which no goal pawn square can be reached.
        m = wGoalPawns & wCurrPawns & ~blocked;
        while (m) {
            Square sq = BitBoard::extractSquare(m);
            U64 tgt = BitBoard::wPawnAttacks(sq) | (1ULL << (sq + 8));
            if ((tgt & wUsefulPawnSquares) == 0)
                blocked |= (1ULL << sq);
        }
    }

    U64 bUsefulPawnSquares = 0;
    // Compute pawns that are blocked because advancing them would leave too few
    // remaining pawns in the cone of squares that can reach the pawn square.
    m = bGoalPawns & ~blocked;
    while (m) {
        int sq = BitUtil::lastBit(m); m &= ~(1ULL << sq);
        U64 mask = wPawnReachable[sq][maxPawnCapt];
        bUsefulPawnSquares |= mask;
        int nGoal = BitBoard::bitCount(bGoalPawns & mask);
        int nCurr = BitBoard::bitCount(bCurrPawns & mask);
        if (nCurr < nGoal)
            return false;
        if ((nCurr == nGoal) && (bCurrPawns & (1ULL << sq))) {
            blocked |= (1ULL << sq);
        } else if (nWhiteExtraPieces < maxPawnCapt) {
            mask = wPawnReachable[sq][nWhiteExtraPieces];
            if ((bCurrPawns & mask & ~blocked) == (1ULL << sq))
                blocked |= (1ULL << sq);
        }
    }

    if (BitBoard::bitCount(bGoalPawns) == BitBoard::bitCount(bCurrPawns)) {
        // Compute pawns that are blocked because advancing them would put
        // them on a square from which no goal pawn square can be reached.
        m = bGoalPawns & bCurrPawns & ~blocked;
        while (m) {
            Square sq = BitBoard::extractSquare(m);
            U64 tgt = BitBoard::bPawnAttacks(sq) | (1ULL << (sq - 8));
            if ((tgt & bUsefulPawnSquares) == 0)
                blocked |= (1ULL << sq);
        }
    }

    // Handle castling rights
    int cMask = goalPos.getCastleMask();
    if (cMask & ~pos.getCastleMask())
        return false;
    if (goalPos.h1Castle())
        blocked |= BitBoard::sqMask(E1,H1);
    if (goalPos.a1Castle())
        blocked |= BitBoard::sqMask(E1,A1);
    if (goalPos.h8Castle())
        blocked |= BitBoard::sqMask(E8,H8);
    if (goalPos.a8Castle())
        blocked |= BitBoard::sqMask(E8,A8);

    if (!findInfeasible && !computeDeadlockedPieces(pos, goalPos, blocked))
        return false;

    return true;
}

bool
ProofGame::computeDeadlockedPieces(const Position& pos, const Position& goalPos,
                                   U64& blocked) {
    if (BitBoard::bitCount(pos.occupiedBB()) > BitBoard::bitCount(goalPos.occupiedBB()))
        return true; // Captures can break a deadlock

    auto pieceCanMove = [&pos](Square sq, U64 occ) -> bool {
        switch (pos.getPiece(sq)) {
        case Piece::WKING: {
            U64 toMask = BitBoard::kingAttacks(sq) & ~occ;
            toMask &= ~BitBoard::bPawnAttacksMask(pos.pieceTypeBB(Piece::BPAWN) & occ);
            return toMask != 0;
        }
        case Piece::BKING: {
            U64 toMask = BitBoard::kingAttacks(sq) & ~occ;
            toMask &= ~BitBoard::wPawnAttacksMask(pos.pieceTypeBB(Piece::WPAWN) & occ);
            return toMask != 0;
        }
        case Piece::WQUEEN: case Piece::BQUEEN:
            return ((BitBoard::bishopAttacks(sq, occ) & ~occ) != 0 ||
                    (BitBoard::rookAttacks(sq, occ) & ~occ) != 0);
        case Piece::WROOK: case Piece::BROOK:
            return (BitBoard::rookAttacks(sq, occ) & ~occ) != 0;
        case Piece::WBISHOP: case Piece::BBISHOP:
            return (BitBoard::bishopAttacks(sq, occ) & ~occ) != 0;
        case Piece::WKNIGHT: case Piece::BKNIGHT:
            return (BitBoard::knightAttacks(sq) & ~occ) != 0;
        case Piece::WPAWN:
            return ((1ULL << (sq+8)) & ~occ) != 0;
        case Piece::BPAWN:
            return ((1ULL << (sq-8)) & ~occ) != 0;
        default:
            assert(false);
        }
        return true;
    };

    U64 deadlocked = pos.occupiedBB() & ~blocked;
    while (true) {
        bool modified = false;
        U64 tmp = deadlocked;
        while (tmp) {
            Square sq = BitBoard::extractSquare(tmp);
            U64 occ = (blocked | deadlocked) & ~(1ULL << sq);
            if (pieceCanMove(sq, occ)) {
                deadlocked &= ~(1ULL << sq);
                modified = true;
            }
        }
        if (!modified)
            break;
    }

    blocked |= deadlocked;

    while (deadlocked) {
        Square sq = BitBoard::extractSquare(deadlocked);
        if (pos.getPiece(sq) != goalPos.getPiece(sq))
            return false;
    }

    return true;
}

// --------------------------------------------------------------------------------

#if 0
template <typename T>
static void printTable(const T* tbl) {
    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++)
            std::cout << ' ' << std::setw(2) << (int)tbl[y*8+x];
        std::cout << '\n';
    }
}
#endif

void
ProofGame::shortestPaths(Piece::Type p, Square toSq, U64 blocked,
                         const ShortestPathData* pawnSub,
                         ShortestPathData& spd) {
    for (int i = 0; i < 64; i++)
        spd.pathLen[i] = -1;
    spd.pathLen[toSq.asInt()] = 0;
    U64 reached = 1ULL << toSq;

    if (p == Piece::WPAWN || p == Piece::BPAWN) {
        if (pawnSub == nullptr) {
            int sq = toSq.asInt();
            int d = (p == Piece::WPAWN) ? -8 : 8;
            int dist = 1;
            while (true) {
                sq += d;
                if ((sq < 0) || (sq > 63) || (blocked & (1ULL << sq)))
                    break;
                spd.pathLen[sq] = dist;
                reached |= 1ULL << sq;
                if (Square(sq).getY() != ((d > 0) ? 5 : 2))
                    dist++;
            }
        } else {
            auto minLen = [](int a, int b) {
                if (b != -1)
                    b++;
                if (a == -1) return b;
                if (b == -1) return a;
                return std::min(a, b);
            };
            if (p == Piece::WPAWN) {
                for (int y = toSq.getY() - 1; y >= 0; y--) {
                    bool newReached = false;
                    for (int x = 0; x < 8; x++) {
                        int sq = Square(x, y).asInt();
                        if (blocked & (1ULL << sq))
                            continue;
                        int best = pawnSub->pathLen[sq];
                        best = minLen(best, spd.pathLen[sq+8]);
                        if ((y == 1) && !(blocked & (1ULL << (sq+8))))
                            best = minLen(best, spd.pathLen[sq+16]);
                        if (x > 0)
                            best = minLen(best, pawnSub->pathLen[sq+7]);
                        if (x < 7)
                            best = minLen(best, pawnSub->pathLen[sq+9]);
                        spd.pathLen[sq] = best;
                        if (best != -1) {
                            reached |= 1ULL << sq;
                            newReached = true;
                        }
                    }
                    if (!newReached)
                        break;
                }
            } else {
                for (int y = toSq.getY() + 1; y < 8; y++) {
                    bool newReached = false;
                    for (int x = 0; x < 8; x++) {
                        int sq = Square(x, y).asInt();
                        if (blocked & (1ULL << sq))
                            continue;
                        int best = pawnSub->pathLen[sq];
                        best = minLen(best, spd.pathLen[sq-8]);
                        if ((y == 6) && !(blocked & (1ULL << (sq-8))))
                            best = minLen(best, spd.pathLen[sq-16]);
                        if (x > 0)
                            best = minLen(best, pawnSub->pathLen[sq-9]);
                        if (x < 7)
                            best = minLen(best, pawnSub->pathLen[sq-7]);
                        spd.pathLen[sq] = best;
                        if (best != -1) {
                            reached |= 1ULL << sq;
                            newReached = true;
                        }
                    }
                    if (!newReached)
                        break;
                }
            }
        }
    } else {
        int dist = 1;
        U64 newSquares = reached;
        while (true) {
            U64 neighbors = computeNeighbors(p, newSquares, blocked);
            newSquares = neighbors & ~reached;
            if (newSquares == 0)
                break;
            U64 m = newSquares;
            while (m) {
                Square sq = BitBoard::extractSquare(m);
                spd.pathLen[sq.asInt()] = dist;
            }
            reached |= newSquares;
            dist++;
        }
    }
    spd.fromSquares = reached;
}

std::shared_ptr<ProofGame::ShortestPathData>
ProofGame::shortestPaths(Piece::Type p, Square toSq, U64 blocked, int maxCapt) {
    bool pawn = (p == Piece::WPAWN || p == Piece::BPAWN);
    if (!pawn)
        maxCapt = 6;
    U64 h = hashU64(hashU64(blocked) + (p * 64 + toSq.asInt()) * 16 + maxCapt);
    h &= pathDataCache.size() - 1;
    PathCacheEntry& entry = pathDataCache[h];
    if (entry.blocked == blocked && entry.toSq == toSq.asInt() &&
        entry.piece == (int)p && entry.maxCapt == maxCapt)
        return entry.spd;

    auto spd = std::make_shared<ShortestPathData>();

    std::shared_ptr<ShortestPathData> pawnSub;
    if (pawn && maxCapt > 0)
        pawnSub = shortestPaths(p, toSq, blocked, maxCapt - 1);
    shortestPaths(p, toSq, blocked, pawnSub.get(), *spd);

    entry.piece = p;
    entry.toSq = toSq.asInt();
    entry.maxCapt = maxCapt;
    entry.blocked = blocked;
    entry.spd = spd;

    return spd;
}

U64
ProofGame::computeNeighbors(Piece::Type p, U64 toSquares, U64 blocked) {
    U64 ret = 0;
    switch (p) {
    case Piece::WKING: case Piece::BKING:
        toSquares &= ~blocked;
        while (toSquares) {
            Square sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::kingAttacks(sq);
        }
        break;
    case Piece::WQUEEN: case Piece::BQUEEN:
        while (toSquares) {
            Square sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::rookAttacks(sq, blocked);
            ret |= BitBoard::bishopAttacks(sq, blocked);
        }
        break;
    case Piece::WROOK: case Piece::BROOK:
        while (toSquares) {
            Square sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::rookAttacks(sq, blocked);
        }
        break;
    case Piece::WBISHOP: case Piece::BBISHOP:
        while (toSquares) {
            Square sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::bishopAttacks(sq, blocked);
        }
        break;
    case Piece::WKNIGHT: case Piece::BKNIGHT:
        while (toSquares) {
            Square sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::knightAttacks(sq);
        }
        break;
    case Piece::WPAWN: {
        U64 tmp = (toSquares >> 8) & ~blocked;
        ret |= tmp;
        ret |= (tmp & BitBoard::maskRow3) >> 8;
        ret |= BitBoard::bPawnAttacksMask(toSquares);
        break;
    }
    case Piece::BPAWN: {
        U64 tmp = (toSquares << 8) & ~blocked;
        ret |= tmp;
        ret |= (tmp & BitBoard::maskRow6) << 8;
        ret |= BitBoard::wPawnAttacksMask(toSquares);
        break;
    }
    default:
        assert(false);
    }
    return ret & ~blocked;
}

// --------------------------------------------------------------------------------

/** Return penalty for white/black kings far away from their goal positions. */
static int kingDistPenalty(const Position& pos, const Position& goalPos) {
    int dw = std::max(1, BitBoard::getKingDistance(pos.wKingSq(), goalPos.wKingSq()));
    int db = std::max(1, BitBoard::getKingDistance(pos.bKingSq(), goalPos.bKingSq()));
    return dw + db;
}

/** Return the sum of all pawn advances for both white and black. */
static int nPawnAdvances(const Position& pos) {
    int adv = 0;

    U64 m = pos.pieceTypeBB(Piece::WPAWN);
    while (m != 0) {
        Square sq = BitBoard::extractSquare(m);
        adv += sq.getY() - 1;
    }

    m = pos.pieceTypeBB(Piece::BPAWN);
    while (m != 0) {
        Square sq = BitBoard::extractSquare(m);
        adv += 6 - sq.getY();
    }

    return adv;
}

bool
ProofGame::TreeNodeCompare::higherPrio(int a, int b) const {
    const TreeNode& n1 = nodes[a];
    const TreeNode& n2 = nodes[b];
    int min1 = n1.sortWeight(k0, k1, N);
    int min2 = n2.sortWeight(k0, k1, N);
    if (min1 != min2)
        return min1 < min2;
    if (n1.ply != n2.ply)
        return n1.ply > n2.ply;

    if (n1.prio != n2.prio)
        return n1.prio > n2.prio;

    return a > b;
}

void
ProofGame::TreeNode::computePrio(const Position& pos, const Position& goalPos, U64 rnd) {
    int p = 0;

    int nPiece = BitBoard::bitCount(pos.occupiedBB());
    p = p * 32 + (32 - nPiece); // Fewer pieces hopefully closer to the goal

    int kp = kingDistPenalty(pos, goalPos);
    p = p * 16 + (14 - kp); // Kings closer to goal position hopefully closer to the goal

    int nP = BitBoard::bitCount(pos.pieceTypeBB(Piece::WPAWN, Piece::BPAWN));
    p = p * 17 + (16 - nP); // More promoted pawns hopefully closer to the goal

    int nPAdv = nPawnAdvances(pos);
    p = p * 41 + nPAdv; // More advanced pawns hopefully closer to the goal

    p = p * 1024 + (rnd & 1023); // Randomize

    prio = p;
}
