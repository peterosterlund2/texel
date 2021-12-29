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
 * extproofkernel.cpp
 *
 *  Created on: Nov 30, 2021
 *      Author: petero
 */

#include "extproofkernel.hpp"
#include "stloutput.hpp"
#include <cassert>

//#define EPKDEBUG

#ifdef EPKDEBUG
#define LOG(x) std::cerr << x << std::endl
#else
#define LOG(x) do { } while (false)
#endif

const auto LE = CspSolver::LE;
const auto GE = CspSolver::GE;

// Maximum number of non-capture promotion moves on one file for one color.
const int maxPromoteOneFile = 7;


ExtProofKernel::ExtProofKernel(const Position& initialPos,
                               const Position& goalPos)
    : initialPos(initialPos), goalPos(goalPos) {
    allPawns.reserve(16);
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int sq = Square::getSquare(x, y);
            int p = initialPos.getPiece(sq);
            if (p == Piece::WPAWN || p == Piece::BPAWN) {
                bool w = p == Piece::WPAWN;
                int idx = allPawns.size();
                allPawns.emplace_back(idx, w);
                int var = csp.addVariable(w ? PrefVal::SMALL : PrefVal::LARGE, y, y);
                allPawns[idx].addVar(var, csp);
                columns[x].addPawn(columns[x].nPawns(), idx);
            }
        }
    }
}

bool
ExtProofKernel::findExtKernel(const std::vector<PkMove>& path,
                              std::vector<ExtPkMove>& extPath) {
    std::cerr << "kernel: " << path << std::endl;

    struct PromPiece {
        bool white;
        PieceType piece;
        int x, y; // Promotion square
    };
    PromPiece promoted[16];
    int nPromoted = 0;

    Position currPos(initialPos); // Keep track of pieces for "pawn/piece takes piece" moves
    // Return square of a non-pawn piece of given type
    auto getSquare = [&promoted,&nPromoted,&currPos](bool white, PieceType p) -> VarSquare {
        for (int i = nPromoted - 1; i >= 0; i--) {
            const PromPiece& pp = promoted[i];
            if (pp.white == white && pp.piece == p) {
                int x = pp.x;
                int y = pp.y;
                for ( ; i+1 < nPromoted; i++)
                    promoted[i] = promoted[i+1];
                nPromoted--;
                return { x, y, -1 };
            }
        }

        Piece::Type pt = ProofKernel::toPieceType(white, p, false);
        U64 m = currPos.pieceTypeBB(pt);
        if (p == PieceType::DARK_BISHOP)
            m &= BitBoard::maskDarkSq;
        else if (p == PieceType::LIGHT_BISHOP)
            m &= BitBoard::maskLightSq;
        assert(m != 0);

        int sq = BitBoard::firstSquare(m);
        currPos.clearPiece(sq);
        return { Square::getX(sq), Square::getY(sq), -1 };
    };

    std::vector<ExtMove> varExtPath;
    auto addExtMove = [&varExtPath](ExtMove&& move) {
         LOG("extMove: " << move);
         varExtPath.push_back(std::move(move));
    };

    // For each move in "path", create corresponding ExtMoves
    for (const PkMove& m : path) {
        LOG("m:" << m);
        const bool white = m.color == PieceColor::WHITE;
        VarSquare otherFromSq = { -1, -1, -1 };
        VarSquare otherToSq = { -1, -1, -1 };
        if (m.otherPromotionFile != -1) { // A just promoted piece to be captured
            otherFromSq.x = otherToSq.x = m.otherPromotionFile;
            PawnColumn& col = columns[m.otherPromotionFile];
            if (white) {
                Pawn& pawn = allPawns[col.getPawn(0)];
                otherFromSq.yVar = pawn.varIds.back();
                otherToSq.y = 0;
                col.removePawn(0);
            } else {
                Pawn& pawn = allPawns[col.getPawn(col.nPawns() - 1)];
                otherFromSq.yVar = pawn.varIds.back();
                otherToSq.y = 7;
                col.removePawn(col.nPawns() - 1);
            }
            auto oc = ProofKernel::otherColor(m.color);
            addExtMove({ oc, PieceType::PAWN,
                         otherFromSq, false, otherToSq, PieceType::KNIGHT });
        }

        int fromYVar = -1; // Y position variable for pawn capturing pawn/piece
        int pawnIdx = -1;  // Pawn that moves, or -1 if non-pawn move
        if (m.fromFile != -1) { // A pawn is moved
            int x = m.fromFile;
            PawnColumn& col = columns[x];
            pawnIdx = col.getPawn(m.fromIdx);
            Pawn& pawn = allPawns[pawnIdx];
            int initYVar = pawn.varIds.back();
            fromYVar = csp.addVariable(white ? PrefVal::MIDDLE_SMALL : PrefVal::MIDDLE_LARGE);
            pawn.addVar(fromYVar, csp);
            addColumnIneqs(col);
            col.removePawn(m.fromIdx);
            addExtMove({ m.color, PieceType::PAWN,
                         { x, -1, initYVar }, false,
                         { x, -1, fromYVar }, PieceType::EMPTY });
        }

        if (m.toFile != -1) {
            PawnColumn& col = columns[m.toFile];
            if (m.promotedPiece == PieceType::EMPTY) {
                if (m.fromFile != -1) {
                    Pawn& pawn = allPawns[pawnIdx];
                    int toYVar = csp.addVariable(white ? PrefVal::SMALL : PrefVal::LARGE);
                    pawn.addVar(toYVar, csp, false);
                    csp.addEq(toYVar, fromYVar, white ? 1 : -1);
                    movePawns(m.toFile, col, varExtPath);
                    if (m.takenPiece == PieceType::PAWN) { // pawn takes pawn
                        Pawn& captPawn = allPawns[col.getPawn(m.toIdx)];
                        csp.addEq(captPawn.varIds.back(), toYVar, 0);
                        col.setPawn(m.toIdx, pawnIdx);
                    } else {                               // pawn takes piece
                        col.addPawn(m.toIdx, pawnIdx);
                        if (m.takenPiece == PieceType::DARK_BISHOP ||
                            m.takenPiece == PieceType::LIGHT_BISHOP) {
                            bool even = (m.toFile % 2 == 0) == (m.takenPiece == PieceType::DARK_BISHOP);
                            if (even)
                                csp.makeEven(toYVar);
                            else
                                csp.makeOdd(toYVar);
                        }
                        auto oc = ProofKernel::otherColor(m.color);
                        if (m.otherPromotionFile == -1) {
                            addExtMove({ oc, m.takenPiece,
                                         getSquare(!white, m.takenPiece), false,
                                         { m.toFile, -1, toYVar }, PieceType::EMPTY });
                        } else {
                            addExtMove({ oc, m.takenPiece,
                                         otherToSq, false,
                                         { m.toFile, -1, toYVar }, PieceType::EMPTY });
                        }
                    }
                    addColumnIneqs(col);
                    addExtMove({ m.color, PieceType::PAWN,
                                 { m.fromFile, -1, fromYVar }, true,
                                 { m.toFile, -1, toYVar }, PieceType::EMPTY });
                } else {
                    if (m.takenPiece == PieceType::PAWN) {  // piece takes pawn
                        Pawn& captPawn = allPawns[col.getPawn(m.toIdx)];
                        int yVar = captPawn.varIds.back();
                        csp.addMinVal(yVar, 1);
                        csp.addMaxVal(yVar, 6);
                        col.removePawn(m.toIdx);
                        addExtMove({ m.color, PieceType::EMPTY,
                                     { -1, -1, -1}, true,
                                     { m.toFile, -1, yVar }, PieceType::EMPTY });
                    }
                }
            } else { // pawn capture and promotion
                if (white) {
                    csp.addMinVal(fromYVar, 6);
                    csp.addMaxVal(fromYVar, 6);
                } else {
                    csp.addMinVal(fromYVar, 1);
                    csp.addMaxVal(fromYVar, 1);
                }
                VarSquare toSq = { m.toFile, white ? 7 : 0, -1 };
                auto oc = ProofKernel::otherColor(m.color);
                if (m.otherPromotionFile == -1) {
                    addExtMove({ oc, m.takenPiece,
                                 getSquare(!white, m.takenPiece), false,
                                 toSq, PieceType::EMPTY });
                } else {
                    addExtMove({ oc, m.takenPiece,
                                 otherToSq, false,
                                 toSq, PieceType::EMPTY });
                }
                addExtMove({ m.color, PieceType::PAWN,
                             { m.fromFile, -1, fromYVar }, true,
                             toSq, m.promotedPiece });
                promoted[nPromoted++] = { white, m.promotedPiece, toSq.x, toSq.y };
            }
        }

        if (m.fromFile == -1 && m.toFile == -1) { // piece takes piece
            addExtMove({ m.color, PieceType::EMPTY,
                         { -1, -1, -1}, true,
                         getSquare(!white, m.takenPiece), PieceType::EMPTY});
        }
    }

    // Add constraints preventing remaining pawns from moving too far
    for (int x = 0; x < 8; x++) {
        int goalYPos[6];  // Required y position of i:th pawn, or -1
        getGoalPawnYPos(x, goalYPos);
        int np = columns[x].nPawns();
        for (int i = 0; i < np; i++) {
            int y = goalYPos[i];
            if (y == -1)
                continue;
            Pawn& pawn = allPawns[columns[x].getPawn(i)];
            int var = pawn.varIds.back();
            if (pawn.white)
                csp.addMaxVal(var, y);
            else
                csp.addMinVal(var, y);
        }
    }

    extPath.clear();
    std::vector<int> values;
    if (!csp.solve(values))
        return false;

    // Substitute variable values to create extPath
    for (const ExtMove& m : varExtPath) {
        int fromY = m.from.yVar == -1 ? m.from.y : values[m.from.yVar];
        int toY = m.to.yVar == -1 ? m.to.y : values[m.to.yVar];

        // Adjust moves involving already promoted pawns
        fromY = clamp(fromY, 0, 7);
        toY = clamp(toY, 0, 7);

        int fromSq = Square::getSquare(m.from.x, fromY);
        int toSq = Square::getSquare(m.to.x, toY);
        if (fromSq != toSq)
            extPath.emplace_back(m.color, m.movingPiece, fromSq, m.capture, toSq, m.promotedPiece);
    }

    return true;
}

void
ExtProofKernel::addColumnIneqs(const PawnColumn& col) {
    int np = col.nPawns();
    for (int i = 1; i < np; i++) {
        Pawn& p1 = allPawns[col.getPawn(i - 1)];
        Pawn& p2 = allPawns[col.getPawn(i)];
        int var1 = p1.varIds.back();
        int var2 = p2.varIds.back();
        csp.addIneq(var1, LE, var2, -1);
    }
}

void
ExtProofKernel::movePawns(int x, const PawnColumn& col,
                          std::vector<ExtMove>& varExtPath) {
    // Consecutive white pawn moves are added in reverse order to "varExtPath",
    // in order for the pawns to not collide with each other. The "whiteMoves"
    // variable keeps track of white moves not yet added to "varExtPath".
    std::vector<ExtMove> whiteMoves;

    auto addWhiteMoves = [&]() {
        std::reverse(whiteMoves.begin(), whiteMoves.end());
        varExtPath.insert(varExtPath.end(), whiteMoves.begin(), whiteMoves.end());
        whiteMoves.clear();
    };

    for (int i = 0; i < col.nPawns(); i++) {
        Pawn& pawn = allPawns[col.getPawn(i)];
        int fromYVar = pawn.varIds.back();
        int toYVar = csp.addVariable(pawn.white ? PrefVal::SMALL : PrefVal::LARGE,
                                     1 - maxPromoteOneFile, 6 + maxPromoteOneFile);
        pawn.addVar(toYVar, csp);
        PieceColor color = pawn.white ? PieceColor::WHITE : PieceColor::BLACK;
        ExtMove m { color, PieceType::PAWN,
                    { x, -1, fromYVar }, false,
                    { x, -1, toYVar }, PieceType::EMPTY };
        if (pawn.white) {
            whiteMoves.push_back(m);
        } else {
            addWhiteMoves();
            varExtPath.push_back(m);
        }
    }
    addWhiteMoves();
}

void
ExtProofKernel::getGoalPawnYPos(int x, int (&goalYPos)[6]) const {
    bool allBlack = true;
    std::vector<std::pair<bool,int>> goalPawns; // idx -> (white, y)
    for (int y = 1; y < 7; y++) {
        int p = goalPos.getPiece(Square::getSquare(x, y));
        if (p == Piece::WPAWN) {
            goalPawns.emplace_back(true, y);
            allBlack = false;
        } else if (p == Piece::BPAWN) {
            goalPawns.emplace_back(false, y);
        }
    }

    int nGoalPawns = goalPawns.size();
    int nPawns = columns[x].nPawns();
    assert(nPawns >= nGoalPawns);

    auto match = [this,x,&goalPawns,nGoalPawns](int offs) -> bool {
        for (int i = 0; i < nGoalPawns; i++)
            if (allPawns[columns[x].getPawn(i+offs)].white != goalPawns[i].first)
                return false;
        return true;
    };

    int offs = -1;
    if (allBlack) {
        for (int o = nPawns - nGoalPawns; o >= 0; o--) {
            if (match(o)) {
                offs = o;
                break;
            }
        }
    } else {
        for (int o = 0; o <= nPawns - nGoalPawns; o++) {
            if (match(o)) {
                offs = o;
                break;
            }
        }
    }
    assert(offs != -1);

    for (int i = 0; i < offs; i++)
        goalYPos[i] = -1; // Black pawns to be promoted
    for (int i = 0; i < nGoalPawns; i++)
        goalYPos[i+offs] = goalPawns[i].second;
    for (int i = nGoalPawns + offs; i < nPawns; i++)
        goalYPos[i] = -1; // White pawns to be promoted
}

void
ExtProofKernel::Pawn::addVar(int varNo, CspSolver& csp, bool addIneq) {
    varIds.push_back(varNo);
    LOG("pawn:" << idx << " vars:" << varIds);
    int n = varIds.size();
    if (addIneq && n >= 2)
        csp.addIneq(varIds[n-1], white ? GE : LE, varIds[n-2], 0);
}


std::ostream& operator<<(std::ostream& os, const ExtProofKernel::VarSquare& vsq) {
    os << (char)(vsq.x + 'a');
    if (vsq.yVar == -1)
        os << (vsq.y + 1);
    else
        os << 'v' << vsq.yVar;
    return os;
}

std::ostream& operator<<(std::ostream& os, const ExtProofKernel::ExtMove& move) {
    os << (move.color == ProofKernel::PieceColor::WHITE ? 'w' : 'b');
    if (move.movingPiece != ProofKernel::PieceType::EMPTY)
        os << pieceName(move.movingPiece);
    os << move.from << (move.capture ? 'x' : '-') << move.to;
    if (move.promotedPiece != ProofKernel::PieceType::EMPTY)
        os << pieceName(move.promotedPiece);
    return os;
}
