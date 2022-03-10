/*
    Texel - A UCI chess engine.
    Copyright (C) 2022  Peter Österlund, peterosterlund2@gmail.com

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
 * pkseq.cpp
 *
 *  Created on: Feb 5, 2022
 *      Author: petero
 */

#include "pkseq.hpp"
#include "proofgame.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "stloutput.hpp"
#include <cassert>
#include <limits.h>

using PieceColor = ProofKernel::PieceColor;
using PieceType = ProofKernel::PieceType;

PkSequence::PkSequence(const std::vector<ExtPkMove>& extKernel,
                       const Position& initPos, const Position& goalPos,
                       std::ostream& log)
    : extKernel(extKernel), initPos(initPos), goalPos(goalPos), log(log) {
}

void
PkSequence::improve() {
    if (extKernel.empty())
        return;

    splitPawnMoves();

    log << "extKernel: " << extKernel << std::endl;

    Graph kernel;
    for (const ExtPkMove& m : extKernel) {
        kernel.addNode(m);
    }

    Position pos(initPos);
    SearchLimits lim;
    lim.maxNodes = 1000000;
    nodes = 0;
    if (improveKernel(kernel, 0, pos, lim)) {
        extKernel.clear();
        for (const MoveData& md : kernel.nodes)
            extKernel.push_back(md.move);
    }

    log << "nodes: " << nodes << " new extKernel: " << extKernel << std::endl;

    combinePawnMoves();
}

static bool
isNonCapturePawnMove(const ProofKernel::ExtPkMove& m) {
    if (m.movingPiece == PieceType::PAWN &&
        Square::getX(m.fromSquare) == Square::getX(m.toSquare)) {
        assert(!m.capture);
        return true;
    }
    return false;
}

void
PkSequence::splitPawnMoves() {
    std::vector<ExtPkMove> seq;
    for (const ExtPkMove& m : extKernel) {
        if (isNonCapturePawnMove(m)) {
            int x = Square::getX(m.fromSquare);
            int y1 = Square::getY(m.fromSquare);
            int y2 = Square::getY(m.toSquare);
            int d = y1 < y2 ? 1 : -1;
            for (int y = y1 + d; y != y2 + d; y += d) {
                ExtPkMove tmpM(m);
                tmpM.fromSquare = Square::getSquare(x, y1);
                tmpM.toSquare = Square::getSquare(x, y);
                if (y != y2)
                    tmpM.promotedPiece = PieceType::EMPTY;
                seq.push_back(tmpM);
                y1 = y;
            }
        } else {
            seq.push_back(m);
        }
    }
    extKernel = seq;
}

void
PkSequence::combinePawnMoves() {
    std::vector<ExtPkMove> seq;
    for (const ExtPkMove& m : extKernel) {
        bool merged = false;
        if (!seq.empty() && isNonCapturePawnMove(m) && isNonCapturePawnMove(seq.back())) {
            const ExtPkMove& m0 = seq.back();
            int x = Square::getX(m.fromSquare);
            if (x == Square::getX(m0.fromSquare) &&
                Square::getY(m0.toSquare) == Square::getY(m.fromSquare)) {
                int y0 = Square::getY(m0.fromSquare);
                int y1 = Square::getY(m.toSquare);
                bool white = (m.color == PieceColor::WHITE);
                if (y0 == (white ? 1 : 6) && y1 == (white ? 3 : 4)) {
                    seq.back() = m;
                    seq.back().fromSquare = Square::getSquare(x, y0);
                    merged = true;
                }
            }
        }
        if (!merged)
            seq.push_back(m);
    }
    extKernel = seq;
}

static U64
moveMask(const ProofKernel::ExtPkMove& m) {
    U64 mask = 0;
    if (m.fromSquare != -1)
        mask |= 1ULL << m.fromSquare;
    mask |= 1ULL << m.toSquare;
    return mask;
}

bool
PkSequence::improveKernel(Graph& kernel, int idx, const Position& pos,
                          const SearchLimits lim) {
    nodes++;
    if ((nodes % 100000) == 0) {
        log << "improveKernel nodes: " << nodes << std::endl;
        kernel.print(log, idx);
    }

    if ((int)searchData.size() < lim.level + 1)
        searchData.resize(lim.level + 1);
    Graph& tmpKernel = searchData[lim.level].graph;
    Position& tmpPos = searchData[lim.level].pos;

    auto nextLim = [&lim]() -> SearchLimits {
        return SearchLimits(lim).nextLev();
    };

    if (idx >= (int)kernel.nodes.size()) {
        int fromSq = -1, toSq = -1;
        if (lim.d3 <= 0)
            return true;
        try {
            auto pg = std::unique_ptr<ProofGame>(new ProofGame(TextIO::toFEN(pos),
                                                               TextIO::toFEN(goalPos),
                                                               false, {}, true, log));
            if (!pg->isInfeasible(fromSq, toSq))
                return true;
        } catch (const ChessError& e) {
            return true; // Cannot determine feasibility
        }
        if (fromSq != -1 && toSq != -1) {
            int p = pos.getPiece(fromSq);
            if (p != Piece::WPAWN && p != Piece::BPAWN) {
                tmpKernel = kernel;
                ExtPkMove em(Piece::isWhite(p) ? PieceColor::WHITE : PieceColor::BLACK,
                             ProofKernel::toPieceType(p, fromSq), fromSq, false, toSq,
                             PieceType::EMPTY);
                tmpKernel.addNode(em);
                tmpKernel.adjustPrevNextMove(idx);
                if (improveKernel(tmpKernel, idx, pos, nextLim().decD3())) {
                    kernel = tmpKernel;
                    return true;
                }
            }
        }
        return false;
    }

    MoveData& md = kernel.nodes[idx];
    ExtPkMove& m = md.move;

    if (m.fromSquare == m.toSquare)
        return improveKernel(kernel, idx+1, pos, nextLim());

    if (!m.capture) {
        // If piece at target square, try to move it away
        int p = pos.getPiece(m.toSquare);
        if (p != Piece::EMPTY && Piece::makeWhite(p) != Piece::WPAWN) {
            int fromSquare = m.toSquare;
            std::vector<int> toSquares;
            getPieceEvasions(pos, fromSquare, toSquares);
            for (int toSq : toSquares) {
                tmpKernel = kernel;
                ExtPkMove em(Piece::isWhite(p) ? PieceColor::WHITE : PieceColor::BLACK,
                             ProofKernel::toPieceType(p, fromSquare),
                             fromSquare, false, toSq, PieceType::EMPTY);
                tmpKernel.addNode(em);
                tmpKernel.nodes.back().movedEarly = true;
                tmpKernel.nodes[idx].dependsOn.push_back(tmpKernel.nodes.back().id);
                if (!tmpKernel.topoSort())
                    continue;
                tmpKernel.adjustPrevNextMove(idx);
                if (improveKernel(tmpKernel, idx, pos, nextLim())) {
                    kernel = tmpKernel;
                    return true;
                }
                if (nodes > lim.maxNodes)
                    return false;
            }
            return false;
        }
    }

    if (m.movingPiece == PieceType::PAWN) {
        UndoInfo ui;
        tmpPos = pos;
        if (!makeMove(tmpPos, ui, m))
            return false;
        return improveKernel(kernel, idx + 1, tmpPos, nextLim());
    }

    auto updatePos = [](const Graph& tmpKernel, int idx, int id,
                        Position& pos) -> bool {
        UndoInfo ui;
        for (int i = idx; i < (int)tmpKernel.nodes.size(); i++) {
            if (tmpKernel.nodes[i].id == id)
                break;
            if (!makeMove(pos, ui, tmpKernel.nodes[i].move)) {
                return false;
            }
        }
        return true;
    };

    if (!md.pseudoLegal) {
        if (m.movingPiece == PieceType::EMPTY) {
            assert(m.capture);
            if (!assignPiece(kernel, idx, pos))
                return false;
        }

        // Try moving piece without moving away any other pawns/pieces
        if (m.capture || pos.getPiece(m.toSquare) == Piece::EMPTY) {
            tmpKernel = kernel;
            U64 occupied = pos.occupiedBB() & ~moveMask(m);
            std::vector<ExtPkMove> expanded;
            if (expandPieceMove(m, occupied, expanded)) {
                tmpKernel.replaceNode(idx, expanded);
                if (!improveKernel(tmpKernel, idx, pos, nextLim()))
                    return false;
                kernel = tmpKernel;
                return true;
            }
        }

        // Try moving later pawn moves earlier to make the piece movement possible
        for (int i = idx + 1; i < (int)kernel.nodes.size(); i++) {
            const MoveData& nextMd = kernel.nodes[i];
            const ExtPkMove& em = nextMd.move;
            if (em.movingPiece != PieceType::PAWN || em.promotedPiece != PieceType::EMPTY)
                continue;

            tmpKernel = kernel;
            tmpKernel.nodes[idx].dependsOn.push_back(tmpKernel.nodes[i].id);
            if (!tmpKernel.topoSort())
                continue;

            tmpPos = pos;
            if (!updatePos(tmpKernel, idx, md.id, tmpPos))
                continue;

            U64 occupied = tmpPos.occupiedBB() & ~moveMask(m);
            std::vector<ExtPkMove> expanded;
            if (expandPieceMove(m, occupied, expanded)) {
                if (!improveKernel(tmpKernel, idx, pos, nextLim()))
                    return false;
                kernel = tmpKernel;
                return true;
            }
        }

        // Try adding a pawn move to make piece movement possible
        std::vector<ExtPkMove> pawnMoves;
        getPawnMoves(kernel, idx, pos, pawnMoves);
        for (const ExtPkMove& pawnMove : pawnMoves) {
            tmpKernel = kernel;
            tmpKernel.addNode(pawnMove);
            tmpKernel.nodes[idx].dependsOn.push_back(tmpKernel.nodes.back().id);
            if (!tmpKernel.topoSort())
                continue;

            tmpPos = pos;
            if (!updatePos(tmpKernel, idx, md.id, tmpPos))
                continue;

            U64 occupied = tmpPos.occupiedBB() & ~moveMask(m);
            std::vector<ExtPkMove> expanded;
            if (expandPieceMove(m, occupied, expanded)) {
                if (!improveKernel(tmpKernel, idx, pos, nextLim()))
                    return false;
                kernel = tmpKernel;
                return true;
            }
        }

        if (lim.d2 > 0) {
            // For a non-pawn move, if other non-pawn piece needs to move, insert
            // ExtPkMove to move it.
            U64 expandedMask = 0;
            {
                U64 blocked;
                if (ProofGame::computeBlocked(pos, goalPos, blocked)) {
                    std::vector<ExtPkMove> blockedExpanded;
                    if (expandPieceMove(m, blocked, blockedExpanded)) {
                        for (const ExtPkMove& em : blockedExpanded) {
                            expandedMask |= moveMask(em);
                            if (em.fromSquare != -1)
                                expandedMask |= BitBoard::squaresBetween(em.fromSquare, em.toSquare);
                        }
                    }
                }
            }
            U64 mask = expandedMask & ~moveMask(m);
            while (mask != 0) {
                int fromSquare = BitBoard::extractSquare(mask);
                int p = pos.getPiece(fromSquare);
                if (p != Piece::EMPTY && Piece::makeWhite(p) != Piece::WPAWN) {
                    std::vector<int> toSquares;
                    getPieceEvasions(pos, fromSquare, toSquares);
                    int tries = 0;
                    for (int toSq : toSquares) {
                        if (((1ULL << toSq) & expandedMask) != 0)
                            continue;
                        tmpKernel = kernel;
                        ExtPkMove em(Piece::isWhite(p) ? PieceColor::WHITE : PieceColor::BLACK,
                                     ProofKernel::toPieceType(p, fromSquare),
                                     fromSquare, false, toSq, PieceType::EMPTY);
                        tmpKernel.addNode(em);
                        tmpKernel.nodes.back().movedEarly = true;
                        tmpKernel.nodes[idx].dependsOn.push_back(tmpKernel.nodes.back().id);
                        if (!tmpKernel.topoSort())
                            continue;
                        tmpKernel.adjustPrevNextMove(idx);
                        if (improveKernel(tmpKernel, idx, pos, nextLim().decD2())) {
                            kernel = tmpKernel;
                            return true;
                        }
                        if (nodes > lim.maxNodes)
                            return false;
                        if (++tries >= 3)
                            break;
                    }
                    break;
                }
            }

            for (const ExtPkMove& pawnMove : pawnMoves) {
                if (((1ULL << pawnMove.fromSquare) & expandedMask) != 0) {
                    tmpKernel = kernel;
                    tmpKernel.addNode(pawnMove);
                    tmpKernel.nodes[idx].dependsOn.push_back(tmpKernel.nodes.back().id);
                    if (!tmpKernel.topoSort())
                        continue;
                    tmpPos = pos;
                    if (!updatePos(tmpKernel, idx, md.id, tmpPos))
                        continue;
                    if (improveKernel(tmpKernel, idx, pos, nextLim().decD2())) {
                        kernel = tmpKernel;
                        return true;
                    }
                    if (nodes > lim.maxNodes)
                        return false;
                }
            }
        }

        if (lim.d1 > 0 && !kernel.nodes[idx].movedEarly) {
            // Try moving the piece move earlier in the proof kernel
            for (int i = idx - 1; i >= 0; i--) {
                if (kernel.nodes[i].move.movingPiece != PieceType::PAWN)
                    continue;
                tmpKernel = kernel;
                tmpKernel.nodes[i].dependsOn.push_back(tmpKernel.nodes[idx].id);
                tmpKernel.nodes[idx].movedEarly = true;
                if (!tmpKernel.topoSort())
                    continue;

                tmpPos = initPos;
                if (!updatePos(tmpKernel, 0, tmpKernel.nodes[i].id, tmpPos))
                    continue;

                if (improveKernel(tmpKernel, i, tmpPos, nextLim().decD1())) {
                    kernel = tmpKernel;
                    return true;
                }
                if (nodes > lim.maxNodes)
                    return false;
            }
        }

        return false;
    }

    if (md.pseudoLegal) {
        tmpPos = pos;
        UndoInfo ui;
        if (!makeMove(tmpPos, ui, m))
            return false;
        return improveKernel(kernel, idx + 1, tmpPos, nextLim());
    }

    // After each move, test if the king is in check.
    // - If so, generate king evasion move.
    // - If king has nowhere to go, generate move to make space for the king
    //   before the checking move.

    // If a rook cannot reach its target square, check if the other rook can
    // reach it instead.
    // - If so, swap which rook is moved.
    // - If there later is a move of the other rook, that move must be swapped
    //   too.

    return false;
}

bool
PkSequence::makeMove(Position& pos, UndoInfo& ui, const ExtPkMove& move) {
    bool white = move.color == PieceColor::WHITE;

    int p = pos.getPiece(move.toSquare);
    if (move.capture) {
        if (p == Piece::EMPTY || Piece::isWhite(p) == white)
            return false; // Wrong capture piece
    } else {
        if (p != Piece::EMPTY)
            return false; // Target square occupied
    }

    if (move.movingPiece == PieceType::EMPTY)
        return false;

    int promoteTo = Piece::EMPTY;
    if (move.promotedPiece != PieceType::EMPTY)
        promoteTo = ProofKernel::toPieceType(white, move.promotedPiece, false, false);
    Move m(move.fromSquare, move.toSquare, promoteTo);
    pos.makeMove(m, ui);

    pos.setWhiteMove(!white);
    if (MoveGen::canTakeKing(pos) && !MoveGen::inCheck(pos))
        pos.setWhiteMove(white);

    return true;
}

bool
PkSequence::assignPiece(Graph& kernel, int idx, const Position& pos) const {
    ExtPkMove& move = kernel.nodes[idx].move;
    bool whiteMoving = !Piece::isWhite(pos.getPiece(move.toSquare));
    U64 candidates = whiteMoving ? pos.whiteBB() : pos.blackBB();
    candidates &= ~pos.pieceTypeBB(whiteMoving ? Piece::WPAWN : Piece::BPAWN);
    candidates &= ~pos.pieceTypeBB(whiteMoving ? Piece::WKING : Piece::BKING);
    if (pos.a1Castle()) candidates &= ~(1ULL << A1);
    if (pos.h1Castle()) candidates &= ~(1ULL << H1);
    if (pos.a8Castle()) candidates &= ~(1ULL << A8);
    if (pos.h8Castle()) candidates &= ~(1ULL << H8);

    int bestDist = INT_MAX;
    ProofGame::ShortestPathData spd;
    while (candidates != 0) {
        int sq = BitBoard::extractSquare(candidates);
        Piece::Type p = (Piece::Type)pos.getPiece(sq);

        U64 occupied = pos.occupiedBB() & ~(1ULL << sq) & ~(1ULL << move.toSquare);
        ProofGame::shortestPaths(p, move.toSquare, occupied, nullptr, spd);
        int dist = spd.pathLen[sq];
        if (dist > 0 && dist < bestDist) {
            move.movingPiece = ProofKernel::toPieceType(p, sq);
            move.fromSquare = sq;
            bestDist = dist;
        }
    }

    if (bestDist == INT_MAX)
        return false;

    kernel.adjustPrevNextMove(idx);
    return true;
}

bool
PkSequence::expandPieceMove(const ExtPkMove& move, U64 occupied,
                            std::vector<ExtPkMove>& outMoves) {
    if ((moveMask(move) & occupied) != 0)
        return false;

    if (move.fromSquare == move.toSquare) {
        outMoves.push_back(move);
        return true;
    }

    bool white = move.color == PieceColor::WHITE;
    Piece::Type p = ProofKernel::toPieceType(white, move.movingPiece, false, true);

    ProofGame::ShortestPathData spd;
    ProofGame::shortestPaths(p, move.toSquare, occupied, nullptr, spd);
    if (spd.pathLen[move.fromSquare] < 0)
        return false;

    int fromSq = move.fromSquare;
    int toSq = move.toSquare;
    while (fromSq != toSq) {
        U64 nextMask = spd.getNextSquares(p, fromSq, occupied);
        assert(nextMask != 0);
        int nextSq = BitBoard::firstSquare(nextMask);

        ExtPkMove m(move);
        m.fromSquare = fromSq;
        m.toSquare = nextSq;
        if (nextSq != toSq)
            m.capture = false;
        outMoves.push_back(m);

        fromSq = nextSq;
    }

    return true;
}

void
PkSequence::getPawnMoves(const Graph& kernel, int idx, const Position& inPos,
                         std::vector<ExtPkMove>& pawnMoves) const {
    // Remove all but pawns and kings
    Position tmpPos(inPos);
    for (int sq = 0; sq < 64; sq++) {
        int p = tmpPos.getPiece(sq);
        switch (p) {
        case Piece::WKING: case Piece::BKING:
        case Piece::WPAWN: case Piece::BPAWN:
        case Piece::EMPTY:
            break;
        default:
            tmpPos.setPiece(sq, Piece::EMPTY);
        }
    }

    for (int i = idx; i < (int)kernel.nodes.size(); i++) {
        const ExtPkMove& m = kernel.nodes[i].move;
        int p = Piece::EMPTY;
        if (m.fromSquare != -1) {
            p = tmpPos.getPiece(m.fromSquare);
            tmpPos.setPiece(m.fromSquare, Piece::EMPTY);
        }
        if (m.promotedPiece != PieceType::EMPTY)
            p = Piece::EMPTY;
        tmpPos.setPiece(m.toSquare, p);
    }

    auto countPawns = [](const Position& pos, int sq, bool white) -> int {
        U64 mask = 1ULL << sq;
        mask = white ? BitBoard::southFill(mask) : BitBoard::northFill(mask);
        mask &= pos.pieceTypeBB(white ? Piece::WPAWN : Piece::BPAWN);
        return BitBoard::bitCount(mask);
    };

    auto pawnsOk = [this,&tmpPos,&countPawns](bool white, int x) -> bool {
        U64 mask = goalPos.pieceTypeBB(white ? Piece::WPAWN : Piece::BPAWN);
        mask &= BitBoard::maskFile[x];
        while (mask != 0) {
            int sq = BitBoard::extractSquare(mask);
            int cntNow = countPawns(tmpPos, sq, white);
            int cntGoal = countPawns(goalPos, sq, white);
            if (cntNow < cntGoal)
                return false;
        }
        return true;
    };

    for (int c = 0; c < 2; c++) {
        bool white = c == 0;
        U64 mask = tmpPos.pieceTypeBB(white ? Piece::WPAWN : Piece::BPAWN);
        while (mask != 0) {
            int sq = BitBoard::extractSquare(mask);
            int x0 = Square::getX(sq);
            int y0 = Square::getY(sq);
            for (int d = 1; d <= 2; d++) {
                if (d == 2 && y0 != (white ? 1 : 6))
                    break;
                int y1 = y0 + (white ? d : -d);
                if (y1 == 0 || y1 == 7)
                    break; // Cannot move pawn to first/last rank
                int toSq = Square::getSquare(x0, y1);
                if (tmpPos.getPiece(toSq) != Piece::EMPTY)
                    break;

                Move m(sq, toSq, Piece::EMPTY);
                UndoInfo ui;
                tmpPos.makeMove(m, ui);

                if (pawnsOk(white, x0)) {
                    ExtPkMove em(white ? PieceColor::WHITE : PieceColor::BLACK,
                                 PieceType::PAWN, sq, false, toSq, PieceType::EMPTY);
                    pawnMoves.push_back(em);
                }

                tmpPos.unMakeMove(m, ui);
            }
        }
    }
}

void
PkSequence::getPieceEvasions(const Position& pos, int fromSq,
                             std::vector<int>& toSquares) const {
    Piece::Type p = (Piece::Type)pos.getPiece(fromSq);
    ProofGame::ShortestPathData spd;
    ProofGame::shortestPaths(p, fromSq, 0, nullptr, spd);

    struct SquareCost {
        int square;
        int cost;
    };

    std::vector<SquareCost> squares;
    for (int sq = 0; sq < 64; sq++) {
        int d = spd.pathLen[sq];
        if (d <= 0 || pos.getPiece(sq) != Piece::EMPTY)
            continue;
        int cost = d * 8 + BitBoard::getKingDistance(fromSq, sq);
        squares.push_back(SquareCost{sq, cost});
    }

    std::sort(squares.begin(), squares.end(),
              [](const SquareCost& a, const SquareCost& b) {
                  if (a.cost != b.cost)
                      return a.cost < b.cost;
                  return a.square < b.square;
              });
    for (const SquareCost sc : squares)
        toSquares.push_back(sc.square);
}

// --------------------------------------------------------------------------------

void
PkSequence::Graph::addNode(const ExtPkMove& m) {
    nodes.emplace_back(nextId++, m);

    MoveData& md = nodes.back();
    if (m.movingPiece == PieceType::PAWN) {
        md.pseudoLegal = true;
        if (m.capture && nodes.size() > 1) {
            const MoveData& prev = nodes[nodes.size() - 2];
            if (m.toSquare == prev.move.toSquare)
                md.dependsOn.push_back(prev.id);
        }
        U64 mMask = moveMask(m);
        for (int i = (int)nodes.size() - 2; i >= 0; i--) {
            const MoveData& prev = nodes[i];
            if (prev.move.movingPiece == PieceType::PAWN) {
                U64 iMask = moveMask(prev.move);
                if ((mMask & iMask) != 0)
                    md.dependsOn.push_back(prev.id);
            }
        }
    }
}

void
PkSequence::Graph::replaceNode(int idx, const std::vector<ExtPkMove>& moves) {
    const int oldId = nodes[idx].id;
    const bool early = nodes[idx].movedEarly;
    auto dependsOn = nodes[idx].dependsOn;
    nodes[idx] = MoveData(nextId++, moves[0]);
    nodes[idx].pseudoLegal = true;
    nodes[idx].dependsOn = dependsOn;
    nodes[idx].movedEarly = early;

    std::vector<MoveData> toInsert;
    int prevId = nodes[idx].id;
    for (int i = 1; i < (int)moves.size(); i++) {
        MoveData md(nextId++, moves[i]);
        md.pseudoLegal = true;
        md.dependsOn.push_back(prevId);
        md.movedEarly = early;
        toInsert.push_back(md);
        prevId = md.id;
    }
    nodes.insert(nodes.begin() + (idx + 1), toInsert.begin(), toInsert.end());

    for (MoveData& md : nodes)
        for (int& d : md.dependsOn)
            if (d == oldId)
                d = prevId;
}

bool
PkSequence::Graph::topoSort() {
    const int n = nodes.size();
    std::vector<bool> visited(n, false);
    std::vector<bool> currPath(n, false);

    std::vector<int> idToIdx(nextId, -1);
    for (int i = 0; i < n; i++)
        idToIdx[nodes[i].id] = i;

    std::vector<MoveData> result;
    for (int i = 0; i < n; i++)
        if (!sortRecursive(i, visited, currPath, idToIdx, result))
            return false;

    nodes = result;
    return true;
}

bool
PkSequence::Graph::sortRecursive(int i,
                                 std::vector<bool>& visited,
                                 std::vector<bool>& currPath,
                                 const std::vector<int>& idToIdx,
                                 std::vector<MoveData>& result) {
    if (currPath[i])
        return false; // Cyclic

    if (visited[i])
        return true;
    visited[i] = true;

    currPath[i] = true;
    for (int dep : nodes[i].dependsOn)
        if (!sortRecursive(idToIdx[dep], visited, currPath, idToIdx, result))
            return false;
    currPath[i] = false;

    result.reserve(nodes.size());
    result.push_back(nodes[i]);
    return true;
}

void
PkSequence::Graph::adjustPrevNextMove(int idx) {
    auto targetPiece = [](const ExtPkMove& move) -> PieceType {
        if (move.promotedPiece != PieceType::EMPTY)
            return move.promotedPiece;
        return move.movingPiece;
    };

    const ExtPkMove& move = nodes[idx].move;
    for (int i = idx + 1; i < (int)nodes.size(); i++) {
        ExtPkMove& m = nodes[i].move;
        if (m.color == move.color &&
            m.movingPiece == targetPiece(move) &&
            m.fromSquare == move.fromSquare) {
            m.fromSquare = move.toSquare;
            nodes[i].dependsOn.push_back(nodes[idx].id);
            break;
        }
    }
    for (int i = idx - 1; i >= 0; i--) {
        ExtPkMove& m = nodes[i].move;
        if (m.color == move.color &&
            targetPiece(m) == move.movingPiece &&
            m.toSquare == move.fromSquare) {
            nodes[idx].dependsOn.push_back(nodes[i].id);
            break;
        }
    }
}

void
PkSequence::Graph::print(std::ostream& os, int idx) const {
    for (int i = 0; i < (int)nodes.size(); i++) {
        os << ((i == idx) ? '*' : ' ');
        os << nodes[i].move;
        os << (nodes[i].pseudoLegal ? 'p' : ' ');
        os << (nodes[i].movedEarly ? 'e' : ' ');
        os << ' ';
    }
    os << std::endl;
}
