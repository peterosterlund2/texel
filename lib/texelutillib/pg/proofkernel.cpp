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
#include "extproofkernel.hpp"
#include "textio.hpp"
#include "stloutput.hpp"
#include <cassert>
#include <climits>


ProofKernel::ProofKernel(const Position& initialPos, const Position& goalPos, U64 blocked,
                         std::ostream& log)
    : initialPos(initialPos), goalPos(goalPos), log(log) {
    for (int i = 0; i < 8; i++)
        columns[i] = PawnColumn(i);
    posToState(initialPos, columns, pieceCnt, blocked);
    posToState(goalPos, goalColumns, goalCnt, blocked);
    minRooks[WHITE] = (goalPos.a1Castle() ? 1 : 0) + (goalPos.h1Castle() ? 1 : 0);
    minRooks[BLACK] = (goalPos.a8Castle() ? 1 : 0) + (goalPos.h8Castle() ? 1 : 0);

    auto isBlocked = [&blocked](int x, int y) -> bool {
        return blocked & (1ULL << Square(x, y));
    };
    auto getPiece = [&goalPos](int x, int y) -> int {
        return goalPos.getPiece(Square(x, y));
    };
    auto blockedByKing = [&](int x, int y, PieceColor c) -> bool {
        if (x < 0 || x > 7)
            return false;
        int oKing = c == PieceColor::WHITE ? Piece::BKING : Piece::WKING;
        return isBlocked(x, y) && getPiece(x, y) == oKing;
    };
    U64 dead = 0;
    for (int x = 0; x < 8; x++) {
        if ((x == 0 || isBlocked(x-1, 6)) && (x == 7 || isBlocked(x+1, 6))) {
            if (getPiece(x, 7) == Piece::BBISHOP)
                blocked |= 1ULL << Square(x, 7);
            if (initialPos.getPiece(Square(x, 7)) == Piece::BBISHOP &&
                getPiece(x, 7) != Piece::BBISHOP)
                dead |= 1ULL << Square(x, 7);
        }
        if ((x == 0 || isBlocked(x-1, 1)) && (x == 7 || isBlocked(x+1, 1))) {
            if (getPiece(x, 0) == Piece::WBISHOP)
                blocked |= 1ULL << Square(x, 0);
            if (initialPos.getPiece(Square(x, 0)) == Piece::WBISHOP &&
                getPiece(x, 0) != Piece::WBISHOP)
                dead |= 1ULL << Square(x, 0);
        }
    }
    deadBishops = dead;

    for (int ci = 0; ci < 2; ci++) {
        PieceColor c = static_cast<PieceColor>(ci);
        int promY = c == PieceColor::WHITE ? 7 : 0;
        int yDir  = c == PieceColor::WHITE ? 1 : -1;
        for (int x = 0; x < 8; x++) {
            PawnColumn& col = columns[x];
            bool blocked7 = isBlocked(x, promY - yDir);
            bool promForward = !blocked7 && !isBlocked(x, promY);
            bool kingDiagBlock = blockedByKing(x-1, promY, c) || blockedByKing(x+1, promY, c);
            promForward &= !kingDiagBlock;
            bool promLeft = !blocked7 && !kingDiagBlock && x > 0 && !isBlocked(x-1, promY);
            bool promRight = !blocked7 && !kingDiagBlock && x < 7 && !isBlocked(x+1, promY);
            bool rqPromote = !blockedByKing(x, promY, c);
            if (!rqPromote) {
                int rook  = c == PieceColor::WHITE ? Piece::WROOK  : Piece::BROOK;
                int queen = c == PieceColor::WHITE ? Piece::WQUEEN : Piece::BQUEEN;
                int pieceLeft  = x == 0 ? Piece::EMPTY : getPiece(x - 1, promY);
                int pieceRight = x == 7 ? Piece::EMPTY : getPiece(x + 1, promY);
                if (pieceLeft == rook || pieceLeft == queen ||
                    pieceRight == rook || pieceRight == queen)
                    rqPromote = true;
            }
            col.setCanPromote(c, promLeft, promForward, promRight, rqPromote);
        }
    }

    for (int i = 0; i < 8; i++) {
        columns[i].setGoal(goalColumns[i]);
        columns[i].calcBishopPromotions(initialPos, goalPos, blocked, i);
    }

    remainingMoves = 0;
    for (int c = 0; c < 2; c++) {
        remainingCaptures[c] = 0;
        for (int p = 0; p < nPieceTypes; p++) {
            int tmp = pieceCnt[c][p] - goalCnt[c][p];
            excessCnt[c][p] = tmp;
            remainingMoves += tmp;
            remainingCaptures[c] += tmp;
        }
    }
}

void
ProofKernel::setRandomSeed(U64 seed) {
    rndSeed = hashU64(seed + hashU64(1));
}

void ProofKernel::posToState(const Position& pos, std::array<PawnColumn,8>& columns,
                             int (&pieceCnt)[2][nPieceTypes], U64 blocked) {
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
            int p = pos.getPiece(Square(x, y));
            if (p == Piece::WPAWN) {
                col.addPawn(col.nPawns(), WHITE);
            } else if (p == Piece::BPAWN) {
                col.addPawn(col.nPawns(), BLACK);
            }
        }

        bool canMove[2] = { true, true };
        for (int c = 0; c < 2; c++) {
            int y = c == WHITE ? 1 : 6;
            Square sq(x, y);
            if (blocked & (1ULL << sq)) {
                int pawn = c == WHITE ? Piece::WPAWN : Piece::BPAWN;
                if (pos.getPiece(sq) == pawn)
                    canMove[c] = false;
            }
        }
        col.setFirstCanMove(canMove[0], canMove[1]);
    }
}

bool
ProofKernel::operator==(const ProofKernel& other) const {
    for (int i = 0; i < 8; i++)
        if (columns[i] != other.columns[i])
            return false;

    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < nPieceTypes; p++) {
            if (pieceCnt[c][p] != other.pieceCnt[c][p])
                return false;
            if (goalCnt[c][p] != other.goalCnt[c][p])
                return false;
            if (excessCnt[c][p] != other.excessCnt[c][p])
                return false;
        }
    }

    return true;
}

ProofKernel::SearchResult
ProofKernel::findProofKernel(std::vector<PkMove>& proofKernel,
                             std::vector<ExtPkMove>& extProofKernel) {
    findFirst = true;
    proofKernel.clear();
    extProofKernel.clear();

    initSearch1();

    if (!goalPossible()) {
        proofKernel = path;
        return FAIL;
    }

    initSearch2();

    SearchResult ret = search(0);
    log << "found:" << (int)ret << " nodes:" << nodes
        << " csp:" << nCSPs << " cspNodes:" << nCSPNodes << std::endl;

    proofKernel.insert(proofKernel.end(), path.begin(), path.end());
    extProofKernel.insert(extProofKernel.end(), extPath.begin(), extPath.end());

    return ret;
}

bool
ProofKernel::isGoalPossible() {
    initSearch1();
    return goalPossible();
}

void
ProofKernel::initSearch1() {
    path.clear();
    while (deadBishops) {
        const Square sq = BitBoard::extractSquare(deadBishops);
        PieceColor color = (sq.getY() == 0) ? BLACK : WHITE;
        PieceType bishop = sq.isDark() ? DARK_BISHOP : LIGHT_BISHOP;
        path.push_back(PkMove::pieceXPiece(color, bishop));
        PkUndoInfo ui;
        makeMove(path.back(), ui);
        if (remainingMoves < 0)
            break;
    }
}

void
ProofKernel::initSearch2() {
    onlyPieceXPiece = false;
    nodes = 0;
    nCSPs = 0;
    nCSPNodes = 0;
    moveStack.resize(remainingMoves);
    ht.setSize(19, 24);
}

void
ProofKernel::findAll() {
    findFirst = false;
    initSearch1();
    initSearch2();
    search(0);
}

ProofKernel::SearchResult
ProofKernel::search(int ply) {
    nodes++;
    if ((nodes & ((1ULL << 26) - 1)) == 0 && findFirst) {
        log << "nodes:" << nodes << " csp:" << nCSPs << std::endl;
        log << "path:" << path << std::endl;
    }

    if (remainingMoves == 0 && isGoal()) {
        if (computeExtKernel()) {
            if (!findFirst) {
                for (const PkMove& m : path)
                    log << ' ' << m;
                log << '\n';
            }
            return EXT_PROOF_KERNEL;
        } else {
            return PROOF_KERNEL;
        }
    }

    if (remainingMoves <= 0 || !goalPossible())
        return FAIL;

    State myState;
    getState(myState);
    if (ht.probe(myState))
        return FAIL; // Already searched, no solution exists

    bool hasProofKernel = false;
    std::vector<PkMove>& moves = moveStack[ply];
    genMoves(moves, remainingMoves > 2);
    for (const PkMove& m : moves) {
        PkUndoInfo ui;

        makeMove(m, ui);
        path.push_back(m);

        SearchResult res = search(ply + 1);

        unMakeMove(m, ui);

        if (res == EXT_PROOF_KERNEL) {
            if (findFirst)
                return res;
            hasProofKernel = true;
        }
        if (res == PROOF_KERNEL)
            hasProofKernel = true;

        path.pop_back();
    }
    if (!hasProofKernel) {
        ht.insert(myState);
        return FAIL;
    }
    return PROOF_KERNEL;
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
    for (int d = 1; d < nPawnConfigs; d++) {
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
        bool isComplete = false;
        for (int offs = 0; offs + goalPawns <= pawns; offs++) {
            int wp, bp;
            computePromotions(offs, wp, bp);
            if (wp + bp > whiteProm + blackProm) {
                whiteProm = wp;
                blackProm = bp;
            }
            if (wp >= 0 && bp >= 0 &&
                std::min(wp, nPromotions(WHITE)) + std::min(bp, nPromotions(BLACK)) + goalPawns == pawns)
                isComplete = true;
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
        nProm[WHITE][false][data] = uniqueBest ? whiteProm : -1;
        nProm[BLACK][false][data] = uniqueBest ? blackProm : -1;
        complete[data] = isComplete;
    }
    data = oldData;
}

void
ProofKernel::PawnColumn::calcBishopPromotions(const Position& initialPos,
                                              const Position& goalPos,
                                              U64 blocked, int x) {
    auto isBlocked = [blocked](int x, int y) -> bool {
        return blocked & (1ULL << Square(x, y));
    };
    auto promBlocked = [blocked,x,&isBlocked](int y) -> bool {
        return (x == 0 || isBlocked(x-1, y)) && (x == 7 || isBlocked(x+1, y));
    };
    auto getPiece = [](const Position& pos, int x, int y) {
        return pos.getPiece(Square(x, y));
    };

    S8 nWhiteBishopProm = maxPawns;
    if (promBlocked(6)) {
        if (getPiece(goalPos, x, 7) == Piece::WBISHOP &&
            getPiece(initialPos, x, 7) != Piece::WBISHOP) {
            nWhiteBishopProm = 1;
            bishopPromRequired[WHITE] = true;
        } else {
            nWhiteBishopProm = 0;
        }
    }

    S8 nBlackBishopProm = maxPawns;
    if (promBlocked(1)) {
        if (getPiece(goalPos, x, 0) == Piece::BBISHOP &&
            getPiece(initialPos, x, 0) != Piece::BBISHOP) {
            nBlackBishopProm = 1;
            bishopPromRequired[BLACK] = true;
        } else {
            nBlackBishopProm = 0;
        }
    }

    for (int d = 1; d < nPawnConfigs; d++) {
        nProm[WHITE][true][d] = std::min(nProm[WHITE][false][d], nWhiteBishopProm);
        nProm[BLACK][true][d] = std::min(nProm[BLACK][false][d], nBlackBishopProm);
    }
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
        promNeeded += std::max(0, -excessCnt[c][KNIGHT]);

        int promNeededDark = 0, promNeededLight = 0;
        for (int i = 0; i < 8; i++) {
            if (columns[i].bishopPromotionRequired(c)) {
                if (columns[i].promotionSquareType(c) == SquareColor::DARK)
                    promNeededDark++;
                else
                    promNeededLight++;
            }
        }
        promNeededDark = std::max(promNeededDark, -excessCnt[c][DARK_BISHOP]);
        promNeededLight = std::max(promNeededLight, -excessCnt[c][LIGHT_BISHOP]);
        promNeeded += promNeededDark + promNeededLight;

        int promAvail = 0;
        int promAvailDark = 0;
        int promAvailLight = 0;
        for (int i = 0; i < 8; i++) {
            int nProm = columns[i].nAllowedPromotions(c, false);
            if (nProm < 0)
                return false;
            promAvail += nProm;
            nProm = columns[i].nAllowedPromotions(c, true);
            if (nProm == 0 && columns[i].bishopPromotionRequired(c))
                return false;
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
    if (remainingMoves < minMovesToGoal())
        return false;

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

    for (int c = 0; c < 2; c++)
        if (minMovesToGoalOneColor((PieceColor)c) > remainingCaptures[1-c])
            return false;

    return true;
}

int
ProofKernel::minMovesToGoal() const {
    // A move can in the best case make two adjacent columns "complete", meaning
    // the required pawn structure can be obtained by performing only promotions.
    int minMoves = 0;
    for (int i = 0; i < 8; i++) {
        if (!columns[i].isComplete()) {
            minMoves++; // Not complete, one more move required
            i++;        // The next column could be completed by the same move
        }
    }
    return minMoves;
}

int
ProofKernel::minMovesToGoalOneColor(PieceColor c) const {
    int availIdx = -100;
    int neededPawns[8], minDist[8];
    int maxBishProm[2] = { 0, 0 };
    for (int i = 0; i < 8; i++) {
        int n = goalColumns[i].nPawns(c) - columns[i].nPawns(c);
        neededPawns[i] = n;
        if (n < 0) {
            availIdx = i;
            if (columns[i].canPromote(c, Direction::FORWARD))
                maxBishProm[(int)columns[i].promotionSquareType(c)] += -n;
        }
        minDist[i] = i - availIdx;
    }
    availIdx = 100;
    int cnt = 0;
    for (int i = 7; i >= 0; i--) {
        int n = neededPawns[i];
        if (n < 0) {
            availIdx = i;
        } else if (n > 0) {
            int minDst = std::min(minDist[i], availIdx - i);
            cnt += n * minDst;
        }
    }

    cnt = std::max(cnt, -(maxBishProm[(int)SquareColor::LIGHT] + excessCnt[c][LIGHT_BISHOP]));
    cnt = std::max(cnt, -(maxBishProm[(int)SquareColor::DARK ] + excessCnt[c][DARK_BISHOP]));

    return cnt;
}

void
ProofKernel::genMoves(std::vector<PkMove>& moves, bool sort) {
    moves.clear();
    if (!onlyPieceXPiece)
        genPawnMoves(moves);
    genPieceXPieceMoves(moves);

    if (sort || rndSeed) {
        for (PkMove& m : moves) {
            PkUndoInfo ui;
            makeMove(m, ui);
            m.sortKey = rndSeed ? hashU64(++rndSeed) : minMovesToGoal();
            unMakeMove(m, ui);
        }
        std::stable_sort(moves.begin(), moves.end());
    }
}

void
ProofKernel::genPawnMoves(std::vector<PkMove>& moves) {
    // Return true if a pawn is free to move
    auto canMove = [](const PawnColumn& col, int idx, int colNp) -> bool {
        if ((idx == 0 && !col.firstCanMove(WHITE)) ||
            (idx == colNp - 1 && !col.firstCanMove(BLACK)))
            return false;
        return true;
    };

    // Return true if a pawn can be inserted at a position in the pawn column,
    // without forcing an un-movable pawn to move
    auto canInsert = [](const PawnColumn& col, int idx, int colNp) -> bool {
        if ((idx == 0 && !col.firstCanMove(WHITE)) ||
            (idx == colNp && !col.firstCanMove(BLACK)))
            return false;
        return true;
    };

    // Pawn takes pawn moves
    for (int x = 0; x < 8; x++) {
        PawnColumn& col = columns[x];
        const int colNp = col.nPawns();
        for (int dir = -1; dir <= 1; dir += 2) {
            if ((x == 0 && dir == -1) || (x == 7 && dir == 1))
                continue;
            PawnColumn oCol = columns[x + dir];
            const int oColNp = oCol.nPawns();
            for (int fromIdx = 0; fromIdx < colNp; fromIdx++) {
                if (!canMove(col, fromIdx, colNp))
                    continue;
                PieceColor c = col.getPawn(fromIdx);
                if (remainingCaptures[1-c] <= 0)
                    continue;
                for (int toIdx = 0; toIdx < oColNp; toIdx++) {
                    if (c == oCol.getPawn(toIdx))
                        continue; // Cannot capture own pawn
                    if (!canMove(oCol, toIdx, oColNp))
                        continue;
                    moves.push_back(PkMove::pawnXPawn(c, x, fromIdx, x + dir, toIdx));
                }
            }
        }
    }

    auto canPromote = [](const PawnColumn& col, PieceColor c, int prom, int taken) -> bool {
        if (!col.rookQueenPromotePossible(c) && (prom == QUEEN || prom == ROOK))
            return false;
        if (col.promotionSquareType(c) == SquareColor::DARK) {
            if (prom == DARK_BISHOP || taken == DARK_BISHOP)
                return false;
        } else {
            if (prom == LIGHT_BISHOP || taken == LIGHT_BISHOP)
                return false;
        }
        return true;
    };

    // Pawn takes piece moves
    for (int x = 0; x < 8; x++) {
        PawnColumn& col = columns[x];
        const int colNp = col.nPawns();
        for (int dir = -1; dir <= 1; dir += 2) {
            if ((x == 0 && dir == -1) || (x == 7 && dir == 1))
                continue;
            PawnColumn oCol = columns[x + dir];
            const int oColNp = oCol.nPawns();
            for (int fromIdx = 0; fromIdx < colNp; fromIdx++) {
                if (!canMove(col, fromIdx, colNp))
                    continue;
                PieceColor c = col.getPawn(fromIdx);
                if (remainingCaptures[1-c] <= 0)
                    continue;
                PieceColor oc = otherColor(c);
                for (int t = QUEEN; t < PAWN; t++) {
                    PieceType taken = (PieceType)t;
                    if (pieceCnt[oc][taken] <= (t == ROOK ? minRooks[oc] : 0))
                        continue;
                    for (int toIdx = 0; toIdx <= oColNp; toIdx++) {
                        if (!canInsert(oCol, toIdx, oColNp))
                            continue;
                        moves.push_back(PkMove::pawnXPiece(c, x, fromIdx, x + dir, toIdx, taken));
                    }

                    // Promotion
                    if ((c == WHITE && fromIdx != colNp - 1) ||
                        (c == BLACK && fromIdx != 0))
                        continue; // Only most advanced pawn can promote
                    if (!col.canPromote(c, dir == -1 ? Direction::LEFT : Direction::RIGHT))
                        continue;
                    for (int prom = QUEEN; prom < PAWN; prom++)
                        if (canPromote(col, c, prom, taken))
                            moves.push_back(PkMove::pawnXPieceProm(c, x, fromIdx, x + dir,
                                                                   taken, (PieceType)prom));
                }
            }
        }
    }

    // Pawn takes promoted pawn moves
    for (int x = 0; x < 8; x++) {
        PawnColumn& col = columns[x];
        const int colNp = col.nPawns();
        for (int dir = -1; dir <= 1; dir += 2) {
            if ((x == 0 && dir == -1) || (x == 7 && dir == 1))
                continue;
            PawnColumn oCol = columns[x + dir];
            const int oColNp = oCol.nPawns();
            for (int fromIdx = 0; fromIdx < colNp; fromIdx++) {
                if (!canMove(col, fromIdx, colNp))
                    continue;
                PieceColor c = col.getPawn(fromIdx);
                if (remainingCaptures[1-c] <= 0)
                    continue;
                PieceColor oc = otherColor(c);
                for (int promFile = 0; promFile < 8; promFile++) {
                    if (columns[promFile].nAllowedPromotions(oc, false) <= 0)
                        continue;
                    int fromIdxDelta = (promFile == x && c == WHITE) ? -1 : 0;
                    for (int toIdx = 0; toIdx <= oColNp; toIdx++) {
                        bool promOnToFile = promFile == x + dir;
                        if (!canInsert(oCol, toIdx, oColNp - (promOnToFile ? 1 : 0)))
                            continue;
                        if (promOnToFile && toIdx == oColNp)
                            continue; // Promotion from file x+dir, one less pawn available
                        moves.push_back(PkMove::pawnXPromPawn(c, x, fromIdx + fromIdxDelta,
                                                              x + dir, toIdx, promFile));
                    }

                    // Promotion
                    if ((c == WHITE && fromIdx != colNp - 1) ||
                        (c == BLACK && fromIdx != 0))
                        continue; // Only most advanced pawn can promote
                    if (!col.canPromote(c, dir == -1 ? Direction::LEFT : Direction::RIGHT))
                        continue;
                    for (int prom = QUEEN; prom < PAWN; prom++)
                        if (canPromote(col, c, prom, KNIGHT))
                            moves.push_back(PkMove::pawnXPromPawnProm(c, x, fromIdx + fromIdxDelta,
                                                                      x + dir, promFile, (PieceType)prom));
                }
            }
        }
    }

    // Piece takes pawn moves
    for (int x = 0; x < 8; x++) {
        PawnColumn& col = columns[x];
        const int colNp = col.nPawns();
        for (int toIdx = 0; toIdx < colNp; toIdx++) {
            if (!canMove(col, toIdx, colNp))
                continue;
            PieceColor oc = col.getPawn(toIdx);
            PieceColor c = otherColor(oc);
            if (remainingCaptures[1-c] <= 0)
                continue;
            moves.push_back(PkMove::pieceXPawn(c, x, toIdx));
        }
    }
}

void
ProofKernel::genPieceXPieceMoves(std::vector<PkMove>& moves) {
    for (int i = 0; i < 2; i++) {
        PieceColor c  = i == 0 ? WHITE : BLACK;
        if (remainingCaptures[1-c] <= 0)
            continue;
        PieceColor oc = i == 0 ? BLACK : WHITE;
        for (int j = 0; j < PAWN; j++)
            if (pieceCnt[oc][j] > (j == ROOK ? minRooks[oc] : 0))
                moves.push_back(PkMove::pieceXPiece(c, (PieceType)j));
    }
}

void
ProofKernel::makeMove(const PkMove& m, PkUndoInfo& ui) {
    PieceType taken;
    if (m.otherPromotionFile != -1) {
        PawnColumn& col = columns[m.otherPromotionFile];
        ui.addColData(m.otherPromotionFile, col.getData());
        if (m.color == WHITE)
            col.removePawn(0);
        else
            col.removePawn(col.nPawns() - 1);
        taken = PieceType::PAWN;
    } else {
        taken = m.takenPiece;
    }

    if (m.fromFile != -1) {
        PawnColumn& col = columns[m.fromFile];
        ui.addColData(m.fromFile, col.getData());
        col.removePawn(m.fromIdx);
    }

    PieceColor oc = otherColor(m.color);
    ui.addCntData(oc, taken, -1);
    pieceCnt[oc][taken]--;
    excessCnt[oc][taken]--;
    remainingMoves--;
    remainingCaptures[oc]--;

    if (m.toFile != -1) {
        PawnColumn& col = columns[m.toFile];
        ui.addColData(m.toFile, col.getData());
        if (m.promotedPiece == PieceType::EMPTY) {
            if (m.fromFile != -1) {
                if (m.takenPiece == PieceType::PAWN)
                    col.setPawn(m.toIdx, m.color);
                else
                    col.addPawn(m.toIdx, m.color);
            } else {
                if (m.takenPiece == PieceType::PAWN)
                    col.removePawn(m.toIdx);
            }
        } else {
            ui.addCntData(m.color, m.promotedPiece, 1);
            pieceCnt[m.color][m.promotedPiece]++;
            excessCnt[m.color][m.promotedPiece]++;
            ui.addCntData(m.color, PieceType::PAWN, -1);
            pieceCnt[m.color][PieceType::PAWN]--;
            excessCnt[m.color][PieceType::PAWN]--;
        }
    }

    if (m.fromFile == -1 && m.toFile == -1) {
        ui.onlyPieceXPiece = onlyPieceXPiece;
        onlyPieceXPiece = true;
    }
}

void
ProofKernel::unMakeMove(const PkMove& m, PkUndoInfo& ui) {
    for (int i = ui.nColData-1; i >= 0; i--)
        columns[ui.colData[i].colNo].setData(ui.colData[i].data);
    for (int i = ui.nCntData-1; i >= 0; i--) {
        const PkUndoInfo::CntData& d = ui.cntData[i];
        pieceCnt[d.color][d.piece] -= d.delta;
        excessCnt[d.color][d.piece] -= d.delta;
    }
    onlyPieceXPiece = ui.onlyPieceXPiece;
    remainingMoves++;
    remainingCaptures[otherColor(m.color)]++;
}

std::string pieceName(ProofKernel::PieceType p) {
    switch (p) {
    case ProofKernel::QUEEN:        return "Q";
    case ProofKernel::ROOK:         return "R";
    case ProofKernel::DARK_BISHOP:  return "DB";
    case ProofKernel::LIGHT_BISHOP: return "LB";
    case ProofKernel::KNIGHT:       return "N";
    case ProofKernel::PAWN:         return "P";
    case ProofKernel::KING:         return "K";
    default:                        assert(false); return "";
    }
};

std::string toString(const ProofKernel::PkMove& m) {
    std::string ret;
    ret += m.color == ProofKernel::WHITE ? "w" : "b";

    auto fileToChar = [](int f) -> char {
        return (char)('a' + f);
    };
    auto idxToChar = [](int idx) -> char {
        return (char)('0' + idx);
    };

    if (m.fromFile != -1) {
        ret += "P";
        ret += fileToChar(m.fromFile);
        ret += idxToChar(m.fromIdx);
    }

    ret += "x";

    if (m.otherPromotionFile == -1) {
        ret += pieceName(m.takenPiece);
    } else {
        ret += fileToChar(m.otherPromotionFile);
    }

    if (m.toFile != -1) {
        ret += fileToChar(m.toFile);

        if (m.toIdx != -1) {
            ret += idxToChar(m.toIdx);
        } else {
            ret += pieceName(m.promotedPiece);
        }
    }

    return ret;
}

static void
ensure(bool b, const std::string& str) {
    if (!b)
        throw ChessParseError("Invalid move: " + str);
}

static ProofKernel::PieceType
parsePiece(const std::string& str, int& idx, bool allowPawn, bool allowKing) {
    switch (str.at(idx++)) {
    case 'Q':
        return ProofKernel::QUEEN;
    case 'R':
        return ProofKernel::ROOK;
    case 'D':
        ensure(str.at(idx++) == 'B', str);
        return ProofKernel::DARK_BISHOP;
    case 'L':
        ensure(str.at(idx++) == 'B', str);
        return ProofKernel::LIGHT_BISHOP;
    case 'N':
        return ProofKernel::KNIGHT;
    case 'P':
        if (allowPawn)
            return ProofKernel::PAWN;
        break;
    case 'K':
        if (allowKing)
            return ProofKernel::KING;
        break;
    default:
        break;
    }
    idx--;
    return ProofKernel::EMPTY;
}

ProofKernel::PkMove
strToPkMove(const std::string& str) {
    ProofKernel::PkMove m;
    int idx = 0;
    m.color = str.at(idx++) == 'w' ? ProofKernel::WHITE : ProofKernel::BLACK;
    if (str.at(idx) == 'P') {
        idx++;
        m.fromFile = str.at(idx++) - 'a'; ensure(m.fromFile >= 0 && m.fromFile < 8, str);
        m.fromIdx = str.at(idx++) - '0';  ensure(m.fromIdx >= 0 && m.fromIdx < ProofKernel::maxPawns, str);
    } else {
        m.fromFile = -1;
        m.fromIdx = -1;
    }

    ensure(str.at(idx++) == 'x', str);

    m.otherPromotionFile = -1;
    m.takenPiece = parsePiece(str, idx, true, false);
    if (m.takenPiece == ProofKernel::EMPTY) {
        m.takenPiece = ProofKernel::KNIGHT;
        char taken = str.at(idx++);
        m.otherPromotionFile = taken - 'a';
        ensure(m.otherPromotionFile >= 0 && m.otherPromotionFile < 8, str);
    }

    m.toFile = -1;
    m.toIdx = -1;
    m.promotedPiece = ProofKernel::EMPTY;
    if (idx != (int)str.size()) {
        m.toFile = str.at(idx++) - 'a';
        ensure(m.toFile >= 0 && m.toFile < 8, str);
        m.promotedPiece = parsePiece(str, idx, false, false);
        if (m.promotedPiece == ProofKernel::EMPTY) {
            char rank = str.at(idx++);
            m.toIdx = rank - '0';
            ensure(m.toIdx >= 0 && m.toIdx < ProofKernel::maxPawns, str);
        }
    }

    return m;
}

bool
ProofKernel::ExtPkMove::operator==(const ExtPkMove& other) const {
    return color == other.color &&
        movingPiece == other.movingPiece &&
        fromSquare == other.fromSquare &&
        capture == other.capture &&
        toSquare == other.toSquare &&
        promotedPiece == other.promotedPiece;
}

std::ostream&
operator<<(std::ostream& os, const ProofKernel::PkMove& m) {
    os << toString(m);
    return os;
}

std::string
toString(const ProofKernel::ExtPkMove& m) {
    std::string ret;
    ret += m.color == ProofKernel::WHITE ? 'w' : 'b';
    if (m.movingPiece != ProofKernel::PieceType::EMPTY) {
        ret += pieceName(m.movingPiece);
        ret += TextIO::squareToString(m.fromSquare);
    }
    ret += m.capture ? 'x' : '-';
    ret += TextIO::squareToString(m.toSquare);
    if (m.promotedPiece != ProofKernel::PieceType::EMPTY)
        ret += pieceName(m.promotedPiece);
    return ret;
}

ProofKernel::ExtPkMove
strToExtPkMove(const std::string& str) {
    int idx = 0;

    auto getSquare = [&str,&idx]() -> Square {
        int x = str.at(idx++) - 'a';
        int y = str.at(idx++) - '1';
        ensure(x >= 0 && x < 8 && y >= 0 && y < 8, str);
        return Square(x, y);
    };

    auto color = str.at(idx++) == 'w' ? ProofKernel::WHITE : ProofKernel::BLACK;
    auto movingPiece = ProofKernel::EMPTY;
    Square fromSq;

    if (str.at(idx) != 'x' && str.at(idx) != '-') {
        movingPiece = parsePiece(str, idx, true, true);
        fromSq = getSquare();
    }

    bool capture = str.at(idx++) == 'x';
    Square toSq = getSquare();
    auto promPiece = idx < (int)str.size() ? parsePiece(str, idx, false, false) : ProofKernel::EMPTY;

    return ProofKernel::ExtPkMove(color, movingPiece, fromSq, capture, toSq, promPiece);
}

std::ostream& operator<<(std::ostream& os, const ProofKernel::ExtPkMove& m) {
    os << toString(m);
    return os;
}

void
ProofKernel::getState(State& state) const {
    U64 pawns = 0;
    for (int i = 0; i < 8; i++)
        pawns = (pawns << 8) | columns[i].getData();
    state.pawnColumns = pawns;

    U64 counts = 0;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < nPieceTypes; j++)
            counts = (counts << 4) | pieceCnt[i][j];
    counts <<= 1;
    if (onlyPieceXPiece)
        counts |= 1;
    counts <<= 8;
    counts |= remainingMoves;
    state.pieceCounts = counts;
}

void
ProofKernel::HashTable::setSize(int logSizeMin, int logSizeMax) {
    int sizeMin = 1 << logSizeMin;
    failed.resize(sizeMin);
    freeSpace = sizeMin * 3 / 4;
    this->logSizeMax = logSizeMax;
}

bool
ProofKernel::HashTable::probe(const State& myState) const {
    const U64 idx = myState.hashKey() & (failed.size() - 1);
    const U64 bucket = idx & ~3;
    for (int i = 0; i < 4; i++)
        if (failed[bucket+i] == myState)
            return true;
    return false;
}

void
ProofKernel::HashTable::insert(const State& myState) {
    const U64 idx = myState.hashKey() & (failed.size() - 1);
    const U64 bucket = idx & ~3;
    const int slot = idx & 3;
    int minDepth = 32;
    U64 insertIdx = 0;
    for (int i = 0; i < 4; i++) {
        U64 idx2 = bucket + ((slot + i) & 3);
        int d = failed[idx2].getDepth();
        if (d < minDepth) {
            insertIdx = idx2;
            minDepth = d;
        }
    }
    failed[insertIdx] = myState;
    freeSpace--;
    if (freeSpace <= 0) {
        freeSpace = INT_MAX;
        if ((int)failed.size() < (1 << logSizeMax))
            grow();
    }
}

void
ProofKernel::HashTable::grow() {
    HashTable tmp;
    tmp.setSize(logSizeMax, logSizeMax);
    for (const State& s : failed)
        if (s.getDepth() > 0)
            tmp.insert(s);
    failed.swap(tmp.failed);
}

bool ProofKernel::computeExtKernel() {
    nCSPs++;
    bool cspLog = nCSPs <= 1000 || (BitUtil::lastBit(nCSPs) - BitUtil::firstBit(nCSPs) <= 2);
    ExtProofKernel epk(initialPos, goalPos, log, !findFirst || !cspLog);
    bool ret = epk.findExtKernel(path, extPath);
    nCSPNodes += epk.getNumNodes();
    return ret;
}

Piece::Type
ProofKernel::toPieceType(bool white, PieceType p, bool allowPawn, bool allowKing) {
    switch (p) {
    case PieceType::QUEEN:
        return white ? Piece::WQUEEN : Piece::BQUEEN;
    case PieceType::ROOK :
        return white ? Piece::WROOK : Piece::BROOK;
    case PieceType::DARK_BISHOP:
    case PieceType::LIGHT_BISHOP:
        return white ? Piece::WBISHOP : Piece::BBISHOP;
    case PieceType::KNIGHT:
        return white ? Piece::WKNIGHT : Piece::BKNIGHT;
    case PieceType::PAWN:
        if (allowPawn)
            return white ? Piece::WPAWN : Piece::BPAWN;
        break;
    case PieceType::KING:
        if (allowKing)
            return white ? Piece::WKING : Piece::BKING;
        break;
    case PieceType::EMPTY:
        break;
    }
    assert(false);
    throw ChessError("Invalid piece type");
}

ProofKernel::PieceType
ProofKernel::toPieceType(int p, Square sq) {
    switch (p) {
    case Piece::WKING: case Piece::BKING:
        return PieceType::KING;
    case Piece::WQUEEN: case Piece::BQUEEN:
        return PieceType::QUEEN;
    case Piece::WROOK: case Piece::BROOK:
        return PieceType::ROOK;
    case Piece::WBISHOP: case Piece::BBISHOP:
        return sq.isDark() ? PieceType::DARK_BISHOP : PieceType::LIGHT_BISHOP;
    case Piece::WKNIGHT: case Piece::BKNIGHT:
        return PieceType::KNIGHT;
    default:
        assert(false);
        return PieceType::EMPTY;
    }
}
