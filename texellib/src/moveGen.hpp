/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * moveGen.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef MOVEGEN_HPP_
#define MOVEGEN_HPP_

#include "move.hpp"
#include "position.hpp"
#include "util.hpp"

#include <assert.h>

//#define MOVELIST_DEBUG

#ifdef MOVELIST_DEBUG
# include <set>
# include "textio.hpp"
#endif

/**
 * Generates move lists (pseudo-legal, legal, check evasions, captures).
 */
class MoveGen {
private:
    static const int MAX_MOVES = 256;

public:

    /** A stack-allocated move list object. */
    class MoveList {
    private:
        int buf[sizeof(Move[MAX_MOVES])/sizeof(int)];
    public:
        int size;

        MoveList() : size(0) { }
        void filter(const std::vector<Move>& searchMoves);
        void clear() { size = 0; }

              Move& operator[](int i)        { return ((Move*)&buf[0])[i]; }
        const Move& operator[](int i) const  { return ((Move*)&buf[0])[i]; }

        void addMove(int from, int to, int promoteTo) {
            Move& m = (*this)[size++];
            new (&m) Move(from, to, promoteTo, 0);
        }
    };

    /**
     * Generate and return a list of pseudo-legal moves.
     * Pseudo-legal means that the moves doesn't necessarily defend from check threats.
     */
    static void pseudoLegalMoves(const Position& pos, MoveList& moveList) {
        const U64 occupied = pos.whiteBB | pos.blackBB;
        if (pos.whiteMove) {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::WQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(true);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                const int k0 = 4;
                if (sq == k0) {
                    const U64 OO_SQ = 0x60ULL;
                    const U64 OOO_SQ = 0xEULL;
                    if (((pos.getCastleMask() & (1 << Position::H1_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        moveList.addMove(k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A1_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        moveList.addMove(k0, k0 - 2, Piece::EMPTY);
                    }
                }
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::WKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            U64 m = (pawns << 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m, -8, true)) return;
            m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m, -16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, true)) return;

            m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, true)) return;
        } else {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::BQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(false);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                const int k0 = 60;
                if (sq == k0) {
                    const U64 OO_SQ = 0x6000000000000000ULL;
                    const U64 OOO_SQ = 0xE00000000000000ULL;
                    if (((pos.getCastleMask() & (1 << Position::H8_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        moveList.addMove(k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A8_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        moveList.addMove(k0, k0 - 2, Piece::EMPTY);
                    }
                }
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::BKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            U64 m = (pawns >> 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m, 8, true)) return;
            m = ((m & BitBoard::maskRow6) >> 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m, 16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns >> 9) & BitBoard::maskAToGFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, true)) return;

            m = (pawns >> 7) & BitBoard::maskBToHFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, true)) return;
        }
    }

    /**
     * Generate and return a list of pseudo-legal check evasion moves.
     * Pseudo-legal means that the moves doesn't necessarily defend from check threats.
     */
    static void checkEvasions(const Position& pos, MoveList& moveList) {
        const U64 occupied = pos.whiteBB | pos.blackBB;
        if (pos.whiteMove) {
            U64 kingThreats = pos.pieceTypeBB[Piece::BKNIGHT] & BitBoard::knightAttacks[pos.wKingSq];
            U64 rookPieces = pos.pieceTypeBB[Piece::BROOK] | pos.pieceTypeBB[Piece::BQUEEN];
            if (rookPieces != 0)
                kingThreats |= rookPieces & BitBoard::rookAttacks(pos.wKingSq, occupied);
            U64 bishPieces = pos.pieceTypeBB[Piece::BBISHOP] | pos.pieceTypeBB[Piece::BQUEEN];
            if (bishPieces != 0)
                kingThreats |= bishPieces & BitBoard::bishopAttacks(pos.wKingSq, occupied);
            kingThreats |= pos.pieceTypeBB[Piece::BPAWN] & BitBoard::wPawnAttacks[pos.wKingSq];
            U64 validTargets = 0;
            if ((kingThreats != 0) && ((kingThreats & (kingThreats-1)) == 0)) { // Exactly one attacking piece
                int threatSq = BitBoard::numberOfTrailingZeros(kingThreats);
                validTargets = kingThreats | BitBoard::squaresBetween[pos.wKingSq][threatSq];
            }
            validTargets |= pos.pieceTypeBB[Piece::BKING];
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::WQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) &
                            ~pos.whiteBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.whiteBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.whiteBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(true);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::WKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.whiteBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            U64 m = (pawns << 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m & validTargets, -8, true)) return;
            m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m & validTargets, -16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns << 7) & BitBoard::maskAToGFiles & ((pos.blackBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, true)) return;

            m = (pawns << 9) & BitBoard::maskBToHFiles & ((pos.blackBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, true)) return;
        } else {
            U64 kingThreats = pos.pieceTypeBB[Piece::WKNIGHT] & BitBoard::knightAttacks[pos.bKingSq];
            U64 rookPieces = pos.pieceTypeBB[Piece::WROOK] | pos.pieceTypeBB[Piece::WQUEEN];
            if (rookPieces != 0)
                kingThreats |= rookPieces & BitBoard::rookAttacks(pos.bKingSq, occupied);
            U64 bishPieces = pos.pieceTypeBB[Piece::WBISHOP] | pos.pieceTypeBB[Piece::WQUEEN];
            if (bishPieces != 0)
                kingThreats |= bishPieces & BitBoard::bishopAttacks(pos.bKingSq, occupied);
            kingThreats |= pos.pieceTypeBB[Piece::WPAWN] & BitBoard::bPawnAttacks[pos.bKingSq];
            U64 validTargets = 0;
            if ((kingThreats != 0) && ((kingThreats & (kingThreats-1)) == 0)) { // Exactly one attacking piece
                int threatSq = BitBoard::numberOfTrailingZeros(kingThreats);
                validTargets = kingThreats | BitBoard::squaresBetween[pos.bKingSq][threatSq];
            }
            validTargets |= pos.pieceTypeBB[Piece::WKING];
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::BQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) &
                            ~pos.blackBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.blackBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.blackBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(false);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::BKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.blackBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            U64 m = (pawns >> 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m & validTargets, 8, true)) return;
            m = ((m & BitBoard::maskRow6) >> 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m & validTargets, 16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns >> 9) & BitBoard::maskAToGFiles & ((pos.whiteBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, true)) return;

            m = (pawns >> 7) & BitBoard::maskBToHFiles & ((pos.whiteBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, true)) return;
        }

#ifdef MOVELIST_DEBUG
        {
            // Extra check that all valid evasions were generated
            MoveList allMoves;
            pseudoLegalMoves(pos, allMoves);
            Position tmpPos(pos);
            removeIllegal(tmpPos, allMoves);
            std::set<std::string> evMoves;
            for (int i = 0; i < moveList.size; i++)
                evMoves.insert(TextIO::moveToUCIString(moveList.m[i]));
            for (int i = 0; i < allMoves.size; i++)
                assert(evMoves.find(TextIO::moveToUCIString(allMoves.m[i])) != evMoves.end());
        }
#endif
    }

    /** Generate captures, checks, and possibly some other moves that are too hard to filter out. */
    static void pseudoLegalCapturesAndChecks(const Position& pos, MoveList& moveList) {
        U64 occupied = pos.whiteBB | pos.blackBB;
        if (pos.whiteMove) {
            int bKingSq = pos.getKingSq(false);
            U64 discovered = 0; // Squares that could generate discovered checks
            U64 kRookAtk = BitBoard::rookAttacks(bKingSq, occupied);
            if ((BitBoard::rookAttacks(bKingSq, occupied & ~kRookAtk) &
                    (pos.pieceTypeBB[Piece::WQUEEN] | pos.pieceTypeBB[Piece::WROOK])) != 0)
                discovered |= kRookAtk;
            U64 kBishAtk = BitBoard::bishopAttacks(bKingSq, occupied);
            if ((BitBoard::bishopAttacks(bKingSq, occupied & ~kBishAtk) &
                    (pos.pieceTypeBB[Piece::WQUEEN] | pos.pieceTypeBB[Piece::WBISHOP])) != 0)
                discovered |= kBishAtk;

            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::WQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied));
                if ((discovered & (1ULL<<sq)) == 0) m &= (pos.blackBB | kRookAtk | kBishAtk);
                m &= ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= (pos.blackBB | kRookAtk);
                m &= ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= (pos.blackBB | kBishAtk);
                m &= ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(true);
                U64 m = BitBoard::kingAttacks[sq];
                m &= ((discovered & (1ULL<<sq)) == 0) ? pos.blackBB : ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                const int k0 = 4;
                if (sq == k0) {
                    const U64 OO_SQ = 0x60ULL;
                    const U64 OOO_SQ = 0xEULL;
                    if (((pos.getCastleMask() & (1 << Position::H1_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        moveList.addMove(k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A1_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        moveList.addMove(k0, k0 - 2, Piece::EMPTY);
                    }
                }
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::WKNIGHT];
            U64 kKnightAtk = BitBoard::knightAttacks[bKingSq];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.whiteBB;
                if ((discovered & (1ULL<<sq)) == 0) m &= (pos.blackBB | kKnightAtk);
                m &= ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // Pawn moves
            // Captures
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            U64 m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, false)) return;
            m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, false)) return;

            // Discovered checks and promotions
            U64 pawnAll = discovered | BitBoard::maskRow7;
            m = ((pawns & pawnAll) << 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m, -8, false)) return;
            m = ((m & BitBoard::maskRow3) << 8) & ~(pos.whiteBB | pos.blackBB);
            addPawnDoubleMovesByMask(moveList, pos, m, -16);

            // Normal checks
            m = ((pawns & ~pawnAll) << 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m & BitBoard::bPawnAttacks[bKingSq], -8, false)) return;
            m = ((m & BitBoard::maskRow3) << 8) & ~(pos.whiteBB | pos.blackBB);
            addPawnDoubleMovesByMask(moveList, pos, m & BitBoard::bPawnAttacks[bKingSq], -16);
        } else {
            int wKingSq = pos.getKingSq(true);
            U64 discovered = 0; // Squares that could generate discovered checks
            U64 kRookAtk = BitBoard::rookAttacks(wKingSq, occupied);
            if ((BitBoard::rookAttacks(wKingSq, occupied & ~kRookAtk) &
                    (pos.pieceTypeBB[Piece::BQUEEN] | pos.pieceTypeBB[Piece::BROOK])) != 0)
                discovered |= kRookAtk;
            U64 kBishAtk = BitBoard::bishopAttacks(wKingSq, occupied);
            if ((BitBoard::bishopAttacks(wKingSq, occupied & ~kBishAtk) &
                    (pos.pieceTypeBB[Piece::BQUEEN] | pos.pieceTypeBB[Piece::BBISHOP])) != 0)
                discovered |= kBishAtk;

            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::BQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied));
                if ((discovered & (1ULL<<sq)) == 0) m &= pos.whiteBB | kRookAtk | kBishAtk;
                m &= ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= pos.whiteBB | kRookAtk;
                m &= ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= pos.whiteBB | kBishAtk;
                m &= ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(false);
                U64 m = BitBoard::kingAttacks[sq];
                m &= ((discovered & (1ULL<<sq)) == 0) ? pos.whiteBB : ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                const int k0 = 60;
                if (sq == k0) {
                    const U64 OO_SQ = 0x6000000000000000ULL;
                    const U64 OOO_SQ = 0xE00000000000000ULL;
                    if (((pos.getCastleMask() & (1 << Position::H8_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        moveList.addMove(k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A8_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        moveList.addMove(k0, k0 - 2, Piece::EMPTY);
                    }
                }
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::BKNIGHT];
            U64 kKnightAtk = BitBoard::knightAttacks[wKingSq];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.blackBB;
                if ((discovered & (1ULL<<sq)) == 0) m &= pos.whiteBB | kKnightAtk;
                m &= ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // Pawn moves
            // Captures
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            U64 m = (pawns >> 9) & BitBoard::maskAToGFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, false)) return;
            m = (pawns >> 7) & BitBoard::maskBToHFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, false)) return;

            // Discovered checks and promotions
            U64 pawnAll = discovered | BitBoard::maskRow2;
            m = ((pawns & pawnAll) >> 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m, 8, false)) return;
            m = ((m & BitBoard::maskRow6) >> 8) & ~(pos.whiteBB | pos.blackBB);
            addPawnDoubleMovesByMask(moveList, pos, m, 16);

            // Normal checks
            m = ((pawns & ~pawnAll) >> 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m & BitBoard::wPawnAttacks[wKingSq], 8, false)) return;
            m = ((m & BitBoard::maskRow6) >> 8) & ~(pos.whiteBB | pos.blackBB);
            addPawnDoubleMovesByMask(moveList, pos, m & BitBoard::wPawnAttacks[wKingSq], 16);
        }
    }

    static void pseudoLegalCaptures(const Position& pos, MoveList& moveList) {
        U64 occupied = pos.whiteBB | pos.blackBB;
        if (pos.whiteMove) {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::WQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::WKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // King moves
            int sq = pos.getKingSq(true);
            U64 m = BitBoard::kingAttacks[sq] & pos.blackBB;
            if (addMovesByMask(moveList, pos, sq, m)) return;

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            m = (pawns << 8) & ~(pos.whiteBB | pos.blackBB);
            m &= BitBoard::maskRow8;
            if (addPawnMovesByMask(moveList, pos, m, -8, false)) return;

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, false)) return;
            m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, false)) return;
        } else {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::BQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                squares &= squares-1;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::BKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return;
                knights &= knights-1;
            }

            // King moves
            int sq = pos.getKingSq(false);
            U64 m = BitBoard::kingAttacks[sq] & pos.whiteBB;
            if (addMovesByMask(moveList, pos, sq, m)) return;

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            m = (pawns >> 8) & ~(pos.whiteBB | pos.blackBB);
            m &= BitBoard::maskRow1;
            if (addPawnMovesByMask(moveList, pos, m, 8, false)) return;

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0L;
            m = (pawns >> 9) & BitBoard::maskAToGFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, false)) return;
            m = (pawns >> 7) & BitBoard::maskBToHFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, false)) return;
        }
    }

    /**
     * Return true if the side to move is in check.
     */
    static bool inCheck(const Position& pos) {
        int kingSq = pos.getKingSq(pos.whiteMove);
        return sqAttacked(pos, kingSq);
    }

    /**
     * Return true if making a move delivers check to the opponent
     */
    static bool givesCheck(const Position& pos, const Move& m) {
        bool wtm = pos.whiteMove;
        int oKingSq = pos.getKingSq(!wtm);
        int oKing = wtm ? Piece::BKING : Piece::WKING;
        int p = Piece::makeWhite(m.promoteTo() == Piece::EMPTY ? pos.getPiece(m.from()) : m.promoteTo());
        int d1 = BitBoard::getDirection(m.to(), oKingSq);
        switch (d1) {
        case 8: case -8: case 1: case -1: // Rook direction
            if ((p == Piece::WQUEEN) || (p == Piece::WROOK))
                if ((d1 != 0) && (nextPiece(pos, m.to(), d1) == oKing))
                    return true;
            break;
        case 9: case 7: case -9: case -7: // Bishop direction
            if ((p == Piece::WQUEEN) || (p == Piece::WBISHOP)) {
                if ((d1 != 0) && (nextPiece(pos, m.to(), d1) == oKing))
                    return true;
            } else if (p == Piece::WPAWN) {
                if (((d1 > 0) == wtm) && (pos.getPiece(m.to() + d1) == oKing))
                    return true;
            }
            break;
        default:
            if (d1 != 0) { // Knight direction
                if (p == Piece::WKNIGHT)
                    return true;
            }
            break;
        }
        int d2 = BitBoard::getDirection(m.from(), oKingSq);
        if ((d2 != 0) && (d2 != d1) && (nextPiece(pos, m.from(), d2) == oKing)) {
            int p2 = nextPieceSafe(pos, m.from(), -d2);
            switch (d2) {
            case 8: case -8: case 1: case -1: // Rook direction
                if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                    (p2 == (wtm ? Piece::WROOK : Piece::BROOK)))
                    return true;
                break;
            case 9: case 7: case -9: case -7: // Bishop direction
                if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                    (p2 == (wtm ? Piece::WBISHOP : Piece::BBISHOP)))
                    return true;
                break;
            }
        }
        if ((m.promoteTo() != Piece::EMPTY) && (d1 != 0) && (d1 == d2)) {
            switch (d1) {
            case 8: case -8: case 1: case -1: // Rook direction
                if ((p == Piece::WQUEEN) || (p == Piece::WROOK))
                    if ((d1 != 0) && (nextPiece(pos, m.from(), d1) == oKing))
                        return true;
                break;
            case 9: case 7: case -9: case -7: // Bishop direction
                if ((p == Piece::WQUEEN) || (p == Piece::WBISHOP)) {
                    if ((d1 != 0) && (nextPiece(pos, m.from(), d1) == oKing))
                        return true;
                }
                break;
            }
        }
        if (p == Piece::WKING) {
            if (m.to() - m.from() == 2) { // O-O
                if (nextPieceSafe(pos, m.from(), -1) == oKing)
                    return true;
                if (nextPieceSafe(pos, m.from() + 1, wtm ? 8 : -8) == oKing)
                    return true;
            } else if (m.to() - m.from() == -2) { // O-O-O
                if (nextPieceSafe(pos, m.from(), 1) == oKing)
                    return true;
                if (nextPieceSafe(pos, m.from() - 1, wtm ? 8 : -8) == oKing)
                    return true;
            }
        } else if (p == Piece::WPAWN) {
            if (pos.getPiece(m.to()) == Piece::EMPTY) {
                int dx = Position::getX(m.to()) - Position::getX(m.from());
                if (dx != 0) { // en passant
                    int epSq = m.from() + dx;
                    int d3 = BitBoard::getDirection(epSq, oKingSq);
                    switch (d3) {
                    case 9: case 7: case -9: case -7:
                        if (nextPiece(pos, epSq, d3) == oKing) {
                            int p2 = nextPieceSafe(pos, epSq, -d3);
                            if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                                (p2 == (wtm ? Piece::WBISHOP : Piece::BBISHOP)))
                                return true;
                        }
                        break;
                    case 1:
                        if (nextPiece(pos, std::max(epSq, m.from()), d3) == oKing) {
                            int p2 = nextPieceSafe(pos, std::min(epSq, m.from()), -d3);
                            if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                                (p2 == (wtm ? Piece::WROOK : Piece::BROOK)))
                                return true;
                        }
                        break;
                    case -1:
                        if (nextPiece(pos, std::min(epSq, m.from()), d3) == oKing) {
                            int p2 = nextPieceSafe(pos, std::max(epSq, m.from()), -d3);
                            if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                                (p2 == (wtm ? Piece::WROOK : Piece::BROOK)))
                                return true;
                        }
                        break;
                    }
                }
            }
        }
        return false;
    }

    /**
     * Return true if the side to move can take the opponents king.
     */
    static bool canTakeKing(Position& pos) {
        pos.setWhiteMove(!pos.whiteMove);
        bool ret = inCheck(pos);
        pos.setWhiteMove(!pos.whiteMove);
        return ret;
    }

    /**
     * Return true if a square is attacked by the opposite side.
     */
    static bool sqAttacked(const Position& pos, int sq) {
        if (pos.whiteMove) {
            if ((BitBoard::knightAttacks[sq] & pos.pieceTypeBB[Piece::BKNIGHT]) != 0)
                return true;
            if ((BitBoard::kingAttacks[sq] & pos.pieceTypeBB[Piece::BKING]) != 0)
                return true;
            if ((BitBoard::wPawnAttacks[sq] & pos.pieceTypeBB[Piece::BPAWN]) != 0)
                return true;
            U64 occupied = pos.whiteBB | pos.blackBB;
            U64 bbQueen = pos.pieceTypeBB[Piece::BQUEEN];
            if ((BitBoard::bishopAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::BBISHOP] | bbQueen)) != 0)
                return true;
            if ((BitBoard::rookAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::BROOK] | bbQueen)) != 0)
                return true;
        } else {
            if ((BitBoard::knightAttacks[sq] & pos.pieceTypeBB[Piece::WKNIGHT]) != 0)
                return true;
            if ((BitBoard::kingAttacks[sq] & pos.pieceTypeBB[Piece::WKING]) != 0)
                return true;
            if ((BitBoard::bPawnAttacks[sq] & pos.pieceTypeBB[Piece::WPAWN]) != 0)
                return true;
            U64 occupied = pos.whiteBB | pos.blackBB;
            U64 bbQueen = pos.pieceTypeBB[Piece::WQUEEN];
            if ((BitBoard::bishopAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::WBISHOP] | bbQueen)) != 0)
                return true;
            if ((BitBoard::rookAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::WROOK] | bbQueen)) != 0)
                return true;
        }
        return false;
    }

    /**
     * Remove all illegal moves from moveList.
     * "moveList" is assumed to be a list of pseudo-legal moves.
     * This function removes the moves that don't defend from check threats.
     */
    static void removeIllegal(Position& pos, MoveList& moveList) {
        int length = 0;
        UndoInfo ui;

        bool isInCheck = inCheck(pos);
        const U64 occupied = pos.whiteBB | pos.blackBB;
        int kSq = pos.getKingSq(pos.whiteMove);
        U64 kingAtks = BitBoard::rookAttacks(kSq, occupied) | BitBoard::bishopAttacks(kSq, occupied);
        int epSquare = pos.getEpSquare();
        if (isInCheck) {
            kingAtks |= pos.pieceTypeBB[pos.whiteMove ? Piece::BKNIGHT : Piece::WKNIGHT];
            for (int mi = 0; mi < moveList.size; mi++) {
                const Move& m = moveList[mi];
                bool legal;
                if ((m.from() != kSq) && ((kingAtks & (1ULL<<m.to())) == 0) && (m.to() != epSquare)) {
                    legal = false;
                } else {
                    pos.makeMove(m, ui);
                    pos.setWhiteMove(!pos.whiteMove);
                    legal = !inCheck(pos);
                    pos.setWhiteMove(!pos.whiteMove);
                    pos.unMakeMove(m, ui);
                }
                if (legal)
                    moveList[length++] = m;
            }
        } else {
            for (int mi = 0; mi < moveList.size; mi++) {
                const Move& m = moveList[mi];
                bool legal;
                if ((m.from() != kSq) && ((kingAtks & (1ULL<<m.from())) == 0) && (m.to() != epSquare)) {
                    legal = true;
                } else {
                    pos.makeMove(m, ui);
                    pos.setWhiteMove(!pos.whiteMove);
                    legal = !inCheck(pos);
                    pos.setWhiteMove(!pos.whiteMove);
                    pos.unMakeMove(m, ui);
                }
                if (legal)
                    moveList[length++] = m;
            }
        }
        moveList.size = length;
    }

private:
    /**
     * Return the next piece in a given direction, starting from sq.
     */
    static int nextPiece(const Position& pos, int sq, int delta) {
        while (true) {
            sq += delta;
            int p = pos.getPiece(sq);
            if (p != Piece::EMPTY)
                return p;
        }
        assert(false);
        return -1;
    }

    /** Like nextPiece(), but handles board edges. */
    static int nextPieceSafe(const Position& pos, int sq, int delta) {
        int dx = 0, dy = 0;
        switch (delta) {
        case 1: dx=1; dy=0; break;
        case 9: dx=1; dy=1; break;
        case 8: dx=0; dy=1; break;
        case 7: dx=-1; dy=1; break;
        case -1: dx=-1; dy=0; break;
        case -9: dx=-1; dy=-1; break;
        case -8: dx=0; dy=-1; break;
        case -7: dx=1; dy=-1; break;
        }
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        while (true) {
            x += dx;
            y += dy;
            if ((x < 0) || (x > 7) || (y < 0) || (y > 7)) {
                return Piece::EMPTY;
            }
            int p = pos.getPiece(Position::getSquare(x, y));
            if (p != Piece::EMPTY)
                return p;
        }
        assert(false);
        return -1;
    }

    static bool addPawnMovesByMask(MoveList& moveList, const Position& pos, U64 mask,
                                   int delta, bool allPromotions) {
        if (mask == 0)
            return false;
        U64 oKingMask = pos.pieceTypeBB[pos.whiteMove ? Piece::BKING : Piece::WKING];
        if ((mask & oKingMask) != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask & oKingMask);
            moveList.size = 0;
            moveList.addMove(sq + delta, sq, Piece::EMPTY);
            return true;
        }
        U64 promMask = mask & BitBoard::maskRow1Row8;
        mask &= ~promMask;
        while (promMask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(promMask);
            int sq0 = sq + delta;
            if (sq >= 56) { // White promotion
                moveList.addMove(sq0, sq, Piece::WQUEEN);
                moveList.addMove(sq0, sq, Piece::WKNIGHT);
                if (allPromotions) {
                    moveList.addMove(sq0, sq, Piece::WROOK);
                    moveList.addMove(sq0, sq, Piece::WBISHOP);
                }
            } else { // Black promotion
                moveList.addMove(sq0, sq, Piece::BQUEEN);
                moveList.addMove(sq0, sq, Piece::BKNIGHT);
                if (allPromotions) {
                    moveList.addMove(sq0, sq, Piece::BROOK);
                    moveList.addMove(sq0, sq, Piece::BBISHOP);
                }
            }
            promMask &= (promMask - 1);
        }
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            moveList.addMove(sq + delta, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
        return false;
    }

    static void addPawnDoubleMovesByMask(MoveList& moveList, const Position& pos,
                                         U64 mask, int delta) {
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            moveList.addMove(sq + delta, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
    }

    static bool addMovesByMask(MoveList& moveList, const Position& pos, int sq0, U64 mask) {
        U64 oKingMask = pos.pieceTypeBB[pos.whiteMove ? Piece::BKING : Piece::WKING];
        if ((mask & oKingMask) != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask & oKingMask);
            moveList.size = 0;
            moveList.addMove(sq0, sq, Piece::EMPTY);
            return true;
        }
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            moveList.addMove(sq0, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
        return false;
    }

    /** Not implemented. */
    MoveGen();
};


#endif /* MOVEGEN_HPP_ */
