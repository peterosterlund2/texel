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
 * proofkernel.cpp
 *
 *  Created on: Oct 16, 2021
 *      Author: petero
 */

#include "proofkernel.hpp"
#include "position.hpp"
#include "proofgame.hpp"
#include "textio.hpp"


ProofKernel::ProofKernel(const Position& initialPos, const Position& goalPos) {
    for (int i = 0; i < 8; i++)
        columns[i] = PawnColumn(i);
    posToState(initialPos, columns, pieceCnt);

    std::array<PawnColumn,8> goalColumns;
    posToState(goalPos, goalColumns, goalCnt);

    U64 blocked;
    ProofGame pg(TextIO::toFEN(goalPos), 1, 1, false, true);
    if (!pg.computeBlocked(initialPos, blocked))
        blocked = 0xffffffffffffffffULL; // If goalPos not reachable, consider all pieces blocked
    auto isBlocked = [blocked](int x, int y) -> bool {
        int sq = Square::getSquare(x, y);
        return blocked & (1ULL << sq);
    };
    auto getPiece = [&goalPos](int x, int y) -> int {
        return goalPos.getPiece(Square::getSquare(x, y));
    };
    auto blockedByKing = [&](int x, int y, PieceColor c) -> bool {
        if (x < 0 || x > 7)
            return false;
        int oKing = c == PieceColor::WHITE ? Piece::BKING : Piece::WKING;
        return isBlocked(x, y) && getPiece(x, y) == oKing;
    };

    for (int ci = 0; ci < 2; ci++) {
        PieceColor c = static_cast<PieceColor>(ci);
        int promY = c == PieceColor::WHITE ? 7 : 0;
        int yDir  = c == PieceColor::WHITE ? 1 : -1;
        for (int x = 0; x < 8; x++) {
            PawnColumn& col = columns[x];
            bool promForward = !isBlocked(x, promY) && !isBlocked(x, promY - yDir);
            bool kingDiagBlock = blockedByKing(x-1, promY, c) || blockedByKing(x+1, promY, c);
            promForward &= !kingDiagBlock;
            bool promLeft = !kingDiagBlock && x > 0 && !isBlocked(x-1, promY);
            bool promRight = !kingDiagBlock && x < 7 && !isBlocked(x+1, promY);
            bool rqPromote = !blockedByKing(x, promY, c);
            col.setCanPromote(c, promLeft, promForward, promRight, rqPromote);
        }
    }

    for (int i = 0; i < 8; i++)
        columns[i].setGoal(goalColumns[i]);

    for (int c = 0; c < 2; c++)
        for (int p = 0; p < nPieceTypes; p++)
            excessCnt[c][p] = pieceCnt[c][p] - goalCnt[c][p];
}

void ProofKernel::posToState(const Position& pos, std::array<PawnColumn,8>& columns,
                             int (&pieceCnt)[2][nPieceTypes]) {
    for (int c = 0; c < 2; c++) {
        pieceCnt[c][QUEEN]  = BitBoard::bitCount(pos.pieceTypeBB(c == WHITE ? Piece::WQUEEN  : Piece::BQUEEN));
        pieceCnt[c][ROOK]   = BitBoard::bitCount(pos.pieceTypeBB(c == WHITE ? Piece::WROOK   : Piece::BROOK));
        pieceCnt[c][KNIGHT] = BitBoard::bitCount(pos.pieceTypeBB(c == WHITE ? Piece::WKNIGHT : Piece::BKNIGHT));
        pieceCnt[c][PAWN]   = BitBoard::bitCount(pos.pieceTypeBB(c == WHITE ? Piece::WPAWN   : Piece::BPAWN));
        U64 bishopMask = pos.pieceTypeBB(c == WHITE ? Piece::WBISHOP : Piece::BBISHOP);
        pieceCnt[c][DARK_BISHOP]  = BitBoard::bitCount(bishopMask & BitBoard::maskDarkSq);
        pieceCnt[c][LIGHT_BISHOP] = BitBoard::bitCount(bishopMask & BitBoard::maskLightSq);
    }

    for (int x = 0; x < 8; x++) {
        PawnColumn& col = columns[x];
        for (int y = 1; y < 7; y++) {
            int p = pos.getPiece(Square::getSquare(x, y));
            if (p == Piece::WPAWN) {
                col.addPawn(col.nPawns(), WHITE);
            } else if (p == Piece::BPAWN) {
                col.addPawn(col.nPawns(), BLACK);
            }
        }
    }
}

ProofKernel::PawnColumn::PawnColumn(int x) {
    bool even = (x % 2) == 0;
    promSquare[WHITE] = even ? SquareColor::LIGHT : SquareColor::DARK;
    promSquare[BLACK] = even ? SquareColor::DARK  : SquareColor::LIGHT;
}

void
ProofKernel::PawnColumn::setGoal(const PawnColumn& goal) {
    const int goalPawns = goal.nPawns();
    const U8 oldData = data;
    for (int d = 1; d < 128; d++) {
        data = d;
        const int pawns = nPawns();

        // Compute number of possible white/black promotions if the pawns
        // starting at "offs" match the goal pawns.
        auto computePromotions = [this,&goal,goalPawns,pawns](int offs, int& wp, int& bp) {
            bool match = true;
            for (int i = 0; i < goalPawns; i++) {
                if (getPawn(offs + i) != goal.getPawn(i)) {
                    match = false;
                    break;
                }
            }
            if (match) {
                wp = 0;
                for (int i = pawns - 1; i >= offs + goalPawns; i--) {
                    if (getPawn(i) == BLACK)
                        break;
                    wp++;
                }
                bp = 0;
                for (int i = 0; i < offs; i++) {
                    if (getPawn(i) == WHITE)
                        break;
                    bp++;
                }
            } else {
                wp = -1;
                bp = -1;
            }
        };

        int whiteProm = -1;
        int blackProm = -1;
        for (int offs = 0; offs + goalPawns <= pawns; offs++) {
            int wp, bp;
            computePromotions(offs, wp, bp);
            if (wp + bp > whiteProm + blackProm) {
                whiteProm = wp;
                blackProm = bp;
            }
        }
        bool uniqueBest = true;
        for (int offs = 0; offs + goalPawns <= pawns; offs++) {
            int wp, bp;
            computePromotions(offs, wp, bp);
            if (wp > whiteProm || bp > blackProm) {
                uniqueBest = false;
                break;
            }
        }
        whiteProm = std::min(whiteProm, nPromotions(WHITE));
        blackProm = std::min(blackProm, nPromotions(BLACK));
        nProm[WHITE][data] = uniqueBest ? whiteProm : -1;
        nProm[BLACK][data] = uniqueBest ? blackProm : -1;
    }
    data = oldData;
}

int
ProofKernel::PawnColumn::nPromotions(PieceColor c) const {
    if (!canPromote(c, Direction::FORWARD))
        return 0;

    int np = nPawns();
    if (c == WHITE) {
        int cnt = 0;
        for (int i = np - 1; i >= 0; i--) {
            if (getPawn(i) == BLACK)
                break;
            cnt++;
        }
        return cnt;
    } else {
        int cnt = 0;
        for (int i = 0; i < np; i++) {
            if (getPawn(i) == WHITE)
                break;
            cnt++;
        }
        return cnt;
    }
}

void
ProofKernel::PawnColumn::setCanPromote(PieceColor c, bool pLeft, bool pForward, bool pRight,
                                       bool pRookQueen) {
    canProm[(int)c][(int)Direction::LEFT]    = pLeft;
    canProm[(int)c][(int)Direction::FORWARD] = pForward;
    canProm[(int)c][(int)Direction::RIGHT]   = pRight;
    canRQProm[(int)c] = pRookQueen;
}

bool
ProofKernel::isGoal() const {
    for (int ci = 0; ci < 2; ci++) {
        PieceColor c = static_cast<PieceColor>(ci);
        int promNeeded = 0;
        promNeeded += std::max(0, -excessCnt[c][QUEEN]);
        promNeeded += std::max(0, -excessCnt[c][ROOK]);
        int promNeededDark = std::max(0, -excessCnt[c][DARK_BISHOP]);
        promNeeded += promNeededDark;
        int promNeededLight = std::max(0, -excessCnt[c][LIGHT_BISHOP]);
        promNeeded += promNeededLight;
        promNeeded += std::max(0, -excessCnt[c][KNIGHT]);

        int promAvail = 0;
        int promAvailDark = 0;
        int promAvailLight = 0;
        for (int i = 0; i < 8; i++) {
            int nProm = columns[i].nAllowedPromotions(c);
            if (nProm < 0)
                return false;
            promAvail += nProm;
            if (columns[i].promotionSquareType(c) == SquareColor::DARK)
                promAvailDark += nProm;
            else
                promAvailLight += nProm;
        }

        if (promAvail < promNeeded ||
            promAvailDark < promNeededDark ||
            promAvailLight < promNeededLight)
            return false;
    }
    return true;
}

bool
ProofKernel::goalPossible() const {
    for (int c = 0; c < 2; c++) {
        int sparePawns = excessCnt[c][PAWN];
        sparePawns += std::min(0, excessCnt[c][QUEEN]);
        sparePawns += std::min(0, excessCnt[c][ROOK]);
        sparePawns += std::min(0, excessCnt[c][DARK_BISHOP]);
        sparePawns += std::min(0, excessCnt[c][LIGHT_BISHOP]);
        sparePawns += std::min(0, excessCnt[c][KNIGHT]);
        if (sparePawns < 0)
            return false;
    }
    return true;
}
