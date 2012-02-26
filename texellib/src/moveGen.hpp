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
#include "types.hpp"

/**
 *
 */
class MoveGen {
private:
    static const int MAX_MOVES = 256;

public:

    class MoveList {
    public:
#if 0
        Move m[MAX_MOVES];
#endif
        int size;
        MoveList() : size(0) { }
#if 0
        void filter(List<Move> searchMoves) {
            int used = 0;
            for (int i = 0; i < size; i++)
                if (searchMoves.contains(m[i]))
                    m[used++] = m[i];
            size = used;
        }
#endif
    };

#if 0
    static final MoveGen instance;
    static {
        instance = new MoveGen();
    }


    /**
     * Generate and return a list of pseudo-legal moves.
     * Pseudo-legal means that the moves doesn't necessarily defend from check threats.
     */
    public final MoveList pseudoLegalMoves(Position pos) {
        MoveList moveList = getMoveListObj();
        final U64 occupied = pos.whiteBB | pos.blackBB;
        if (pos.whiteMove) {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::WQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(true);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                final int k0 = 4;
                if (sq == k0) {
                    final U64 OO_SQ = 0x60ULL;
                    final U64 OOO_SQ = 0xEULL;
                    if (((pos.getCastleMask() & (1 << Position::H1_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        setMove(moveList, k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A1_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        setMove(moveList, k0, k0 - 2, Piece::EMPTY);
                    }
                }
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::WKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            U64 m = (pawns << 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m, -8, true)) return moveList;
            m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m, -16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, true)) return moveList;

            m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, true)) return moveList;
        } else {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::BQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(false);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                final int k0 = 60;
                if (sq == k0) {
                    final U64 OO_SQ = 0x6000000000000000ULL;
                    final U64 OOO_SQ = 0xE00000000000000ULL;
                    if (((pos.getCastleMask() & (1 << Position::H8_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        setMove(moveList, k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A8_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        setMove(moveList, k0, k0 - 2, Piece::EMPTY);
                    }
                }
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::BKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            U64 m = (pawns >>> 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m, 8, true)) return moveList;
            m = ((m & BitBoard::maskRow6) >>> 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m, 16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns >>> 9) & BitBoard::maskAToGFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, true)) return moveList;

            m = (pawns >>> 7) & BitBoard::maskBToHFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, true)) return moveList;
        }
        return moveList;
    }

    /**
     * Generate and return a list of pseudo-legal check evasion moves.
     * Pseudo-legal means that the moves doesn't necessarily defend from check threats.
     */
    public final MoveList checkEvasions(Position pos) {
        MoveList moveList = getMoveListObj();
        final U64 occupied = pos.whiteBB | pos.blackBB;
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
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.whiteBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.whiteBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(true);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::WKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.whiteBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            U64 m = (pawns << 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m & validTargets, -8, true)) return moveList;
            m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m & validTargets, -16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns << 7) & BitBoard::maskAToGFiles & ((pos.blackBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, true)) return moveList;

            m = (pawns << 9) & BitBoard::maskBToHFiles & ((pos.blackBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, true)) return moveList;
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
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.blackBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.blackBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(false);
                U64 m = BitBoard::kingAttacks[sq] & ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::BKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & ~pos.blackBB & validTargets;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            U64 m = (pawns >>> 8) & ~occupied;
            if (addPawnMovesByMask(moveList, pos, m & validTargets, 8, true)) return moveList;
            m = ((m & BitBoard::maskRow6) >>> 8) & ~occupied;
            addPawnDoubleMovesByMask(moveList, pos, m & validTargets, 16);

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns >>> 9) & BitBoard::maskAToGFiles & ((pos.whiteBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, true)) return moveList;

            m = (pawns >>> 7) & BitBoard::maskBToHFiles & ((pos.whiteBB & validTargets) | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, true)) return moveList;
        }

        /* Extra debug checks
        {
            ArrayList<Move> allMoves = pseudoLegalMoves(pos);
            allMoves = removeIllegal(pos, allMoves);
            HashSet<std::string> evMoves = new HashSet<std::string>();
            for (Move m : moveList)
                evMoves.add(TextIO::moveToUCIString(m));
            for (Move m : allMoves)
                if (!evMoves.contains(TextIO::moveToUCIString(m)))
                    throw new RuntimeException();
        }
        */

        return moveList;
    }

    /** Generate captures, checks, and possibly some other moves that are too hard to filter out. */
    public final MoveList pseudoLegalCapturesAndChecks(Position pos) {
        MoveList moveList = getMoveListObj();
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
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= (pos.blackBB | kRookAtk);
                m &= ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= (pos.blackBB | kBishAtk);
                m &= ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(true);
                U64 m = BitBoard::kingAttacks[sq];
                m &= ((discovered & (1ULL<<sq)) == 0) ? pos.blackBB : ~pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                final int k0 = 4;
                if (sq == k0) {
                    final U64 OO_SQ = 0x60ULL;
                    final U64 OOO_SQ = 0xEULL;
                    if (((pos.getCastleMask() & (1 << Position::H1_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        setMove(moveList, k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A1_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::WROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        setMove(moveList, k0, k0 - 2, Piece::EMPTY);
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
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // Pawn moves
            // Captures
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            U64 m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, false)) return moveList;
            m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, false)) return moveList;

            // Discovered checks and promotions
            U64 pawnAll = discovered | BitBoard::maskRow7;
            m = ((pawns & pawnAll) << 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m, -8, false)) return moveList;
            m = ((m & BitBoard::maskRow3) << 8) & ~(pos.whiteBB | pos.blackBB);
            addPawnDoubleMovesByMask(moveList, pos, m, -16);

            // Normal checks
            m = ((pawns & ~pawnAll) << 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m & BitBoard::bPawnAttacks[bKingSq], -8, false)) return moveList;
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
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= pos.whiteBB | kRookAtk;
                m &= ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied);
                if ((discovered & (1ULL<<sq)) == 0) m &= pos.whiteBB | kBishAtk;
                m &= ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // King moves
            {
                int sq = pos.getKingSq(false);
                U64 m = BitBoard::kingAttacks[sq];
                m &= ((discovered & (1ULL<<sq)) == 0) ? pos.whiteBB : ~pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                final int k0 = 60;
                if (sq == k0) {
                    final U64 OO_SQ = 0x6000000000000000ULL;
                    final U64 OOO_SQ = 0xE00000000000000ULL;
                    if (((pos.getCastleMask() & (1 << Position::H8_CASTLE)) != 0) &&
                        ((OO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 + 3) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 + 1)) {
                        setMove(moveList, k0, k0 + 2, Piece::EMPTY);
                    }
                    if (((pos.getCastleMask() & (1 << Position::A8_CASTLE)) != 0) &&
                        ((OOO_SQ & (pos.whiteBB | pos.blackBB)) == 0) &&
                        (pos.getPiece(k0 - 4) == Piece::BROOK) &&
                        !sqAttacked(pos, k0) &&
                        !sqAttacked(pos, k0 - 1)) {
                        setMove(moveList, k0, k0 - 2, Piece::EMPTY);
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
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // Pawn moves
            // Captures
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            U64 m = (pawns >>> 9) & BitBoard::maskAToGFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, false)) return moveList;
            m = (pawns >>> 7) & BitBoard::maskBToHFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, false)) return moveList;

            // Discovered checks and promotions
            U64 pawnAll = discovered | BitBoard::maskRow2;
            m = ((pawns & pawnAll) >>> 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m, 8, false)) return moveList;
            m = ((m & BitBoard::maskRow6) >>> 8) & ~(pos.whiteBB | pos.blackBB);
            addPawnDoubleMovesByMask(moveList, pos, m, 16);

            // Normal checks
            m = ((pawns & ~pawnAll) >>> 8) & ~(pos.whiteBB | pos.blackBB);
            if (addPawnMovesByMask(moveList, pos, m & BitBoard::wPawnAttacks[wKingSq], 8, false)) return moveList;
            m = ((m & BitBoard::maskRow6) >>> 8) & ~(pos.whiteBB | pos.blackBB);
            addPawnDoubleMovesByMask(moveList, pos, m & BitBoard::wPawnAttacks[wKingSq], 16);
        }

        return moveList;
    }

    public final MoveList pseudoLegalCaptures(Position pos) {
        MoveList moveList = getMoveListObj();
        U64 occupied = pos.whiteBB | pos.blackBB;
        if (pos.whiteMove) {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::WQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::WROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::WBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::WKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & pos.blackBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // King moves
            int sq = pos.getKingSq(true);
            U64 m = BitBoard::kingAttacks[sq] & pos.blackBB;
            if (addMovesByMask(moveList, pos, sq, m)) return moveList;

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
            m = (pawns << 8) & ~(pos.whiteBB | pos.blackBB);
            m &= BitBoard::maskRow8;
            if (addPawnMovesByMask(moveList, pos, m, -8, false)) return moveList;

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
            m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -7, false)) return moveList;
            m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.blackBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, -9, false)) return moveList;
        } else {
            // Queen moves
            U64 squares = pos.pieceTypeBB[Piece::BQUEEN];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Rook moves
            squares = pos.pieceTypeBB[Piece::BROOK];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::rookAttacks(sq, occupied) & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Bishop moves
            squares = pos.pieceTypeBB[Piece::BBISHOP];
            while (squares != 0) {
                int sq = BitBoard::numberOfTrailingZeros(squares);
                U64 m = BitBoard::bishopAttacks(sq, occupied) & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                squares &= squares-1;
            }

            // Knight moves
            U64 knights = pos.pieceTypeBB[Piece::BKNIGHT];
            while (knights != 0) {
                int sq = BitBoard::numberOfTrailingZeros(knights);
                U64 m = BitBoard::knightAttacks[sq] & pos.whiteBB;
                if (addMovesByMask(moveList, pos, sq, m)) return moveList;
                knights &= knights-1;
            }

            // King moves
            int sq = pos.getKingSq(false);
            U64 m = BitBoard::kingAttacks[sq] & pos.whiteBB;
            if (addMovesByMask(moveList, pos, sq, m)) return moveList;

            // Pawn moves
            U64 pawns = pos.pieceTypeBB[Piece::BPAWN];
            m = (pawns >>> 8) & ~(pos.whiteBB | pos.blackBB);
            m &= BitBoard::maskRow1;
            if (addPawnMovesByMask(moveList, pos, m, 8, false)) return moveList;

            int epSquare = pos.getEpSquare();
            U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0L;
            m = (pawns >>> 9) & BitBoard::maskAToGFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 9, false)) return moveList;
            m = (pawns >>> 7) & BitBoard::maskBToHFiles & (pos.whiteBB | epMask);
            if (addPawnMovesByMask(moveList, pos, m, 7, false)) return moveList;
        }
        return moveList;
    }
#endif
    /**
     * Return true if the side to move is in check.
     */
    static bool inCheck(const Position& pos) {
        int kingSq = pos.getKingSq(pos.whiteMove);
        return sqAttacked(pos, kingSq);
    }

#if 0
    /**
     * Return the next piece in a given direction, starting from sq.
     */
    private static final int nextPiece(Position pos, int sq, int delta) {
        while (true) {
            sq += delta;
            int p = pos.getPiece(sq);
            if (p != Piece::EMPTY)
                return p;
        }
    }

    /** Like nextPiece(), but handles board edges. */
    private static final int nextPieceSafe(Position pos, int sq, int delta) {
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
    }

    /**
     * Return true if making a move delivers check to the opponent
     */
    public static final bool givesCheck(Position pos, Move m) {
        bool wtm = pos.whiteMove;
        int oKingSq = pos.getKingSq(!wtm);
        int oKing = wtm ? Piece::BKING : Piece::WKING;
        int p = Piece::makeWhite(m.promoteTo == Piece::EMPTY ? pos.getPiece(m.from()) : m.promoteTo);
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
        if ((m.promoteTo != Piece::EMPTY) && (d1 != 0) && (d1 == d2)) {
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
                        if (nextPiece(pos, Math.max(epSq, m.from()), d3) == oKing) {
                            int p2 = nextPieceSafe(pos, Math.min(epSq, m.from()), -d3);
                            if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                                (p2 == (wtm ? Piece::WROOK : Piece::BROOK)))
                                return true;
                        }
                        break;
                    case -1:
                        if (nextPiece(pos, Math.min(epSq, m.from()), d3) == oKing) {
                            int p2 = nextPieceSafe(pos, Math.max(epSq, m.from()), -d3);
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
    public static final bool canTakeKing(Position pos) {
        pos.setWhiteMove(!pos.whiteMove);
        bool ret = inCheck(pos);
        pos.setWhiteMove(!pos.whiteMove);
        return ret;
    }
#endif

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
#if 0

    /**
     * Remove all illegal moves from moveList.
     * "moveList" is assumed to be a list of pseudo-legal moves.
     * This function removes the moves that don't defend from check threats.
     */
    public static final void removeIllegal(Position pos, MoveList moveList) {
        int length = 0;
        UndoInfo ui = new UndoInfo();

        bool isInCheck = inCheck(pos);
        final U64 occupied = pos.whiteBB | pos.blackBB;
        int kSq = pos.getKingSq(pos.whiteMove);
        U64 kingAtks = BitBoard::rookAttacks(kSq, occupied) | BitBoard::bishopAttacks(kSq, occupied);
        int epSquare = pos.getEpSquare();
        if (isInCheck) {
            kingAtks |= pos.pieceTypeBB[pos.whiteMove ? Piece::BKNIGHT : Piece::WKNIGHT];
            for (int mi = 0; mi < moveList.size; mi++) {
                Move m = moveList.m[mi];
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
                    moveList.m[length++].copyFrom(m);
            }
        } else {
            for (int mi = 0; mi < moveList.size; mi++) {
                Move m = moveList.m[mi];
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
                    moveList.m[length++].copyFrom(m);
            }
        }
        moveList.size = length;
    }

    private final static bool addPawnMovesByMask(MoveList moveList, Position pos, U64 mask,
                                                    int delta, bool allPromotions) {
        if (mask == 0)
            return false;
        U64 oKingMask = pos.pieceTypeBB[pos.whiteMove ? Piece::BKING : Piece::WKING];
        if ((mask & oKingMask) != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask & oKingMask);
            moveList.size = 0;
            setMove(moveList, sq + delta, sq, Piece::EMPTY);
            return true;
        }
        U64 promMask = mask & BitBoard::maskRow1Row8;
        mask &= ~promMask;
        while (promMask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(promMask);
            int sq0 = sq + delta;
            if (sq >= 56) { // White promotion
                setMove(moveList, sq0, sq, Piece::WQUEEN);
                setMove(moveList, sq0, sq, Piece::WKNIGHT);
                if (allPromotions) {
                    setMove(moveList, sq0, sq, Piece::WROOK);
                    setMove(moveList, sq0, sq, Piece::WBISHOP);
                }
            } else { // Black promotion
                setMove(moveList, sq0, sq, Piece::BQUEEN);
                setMove(moveList, sq0, sq, Piece::BKNIGHT);
                if (allPromotions) {
                    setMove(moveList, sq0, sq, Piece::BROOK);
                    setMove(moveList, sq0, sq, Piece::BBISHOP);
                }
            }
            promMask &= (promMask - 1);
        }
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            setMove(moveList, sq + delta, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
        return false;
    }

    private final static void addPawnDoubleMovesByMask(MoveList moveList, Position pos,
                                                       U64 mask, int delta) {
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            setMove(moveList, sq + delta, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
    }

    private final static bool addMovesByMask(MoveList moveList, Position pos, int sq0, U64 mask) {
        U64 oKingMask = pos.pieceTypeBB[pos.whiteMove ? Piece::BKING : Piece::WKING];
        if ((mask & oKingMask) != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask & oKingMask);
            moveList.size = 0;
            setMove(moveList, sq0, sq, Piece::EMPTY);
            return true;
        }
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            setMove(moveList, sq0, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
        return false;
    }

    private final static void setMove(MoveList moveList, int from, int to, int promoteTo) {
        Move m = moveList.m[moveList.size++];
        m.from = from;
        m.to = to;
        m.promoteTo = promoteTo;
        m.score = 0;
    }

    // Code to handle the Move cache.
    private Object[] moveListCache = new Object[200];
    private int moveListsInCache = 0;

    private final MoveList getMoveListObj() {
        MoveList ml;
        if (moveListsInCache > 0) {
            ml = (MoveList)moveListCache[--moveListsInCache];
            ml.size = 0;
        } else {
            ml = new MoveList();
            for (int i = 0; i < MAX_MOVES; i++)
                ml.m[i] = new Move(0, 0, Piece::EMPTY);
        }
        return ml;
    }

    /** Return all move objects in moveList to the move cache. */
    public final void returnMoveList(MoveList moveList) {
        if (moveListsInCache < moveListCache.length) {
            moveListCache[moveListsInCache++] = moveList;
        }
    }
#endif
};

#endif /* MOVEGEN_HPP_ */
