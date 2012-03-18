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
 * position.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef POSITION_HPP_
#define POSITION_HPP_

#include <iosfwd>

#include "move.hpp"
#include "undoInfo.hpp"
#include "bitBoard.hpp"
#include "piece.hpp"
#include <algorithm>
#include <iostream>

/**
 * Stores the state of a chess position.
 * All required state is stored, except for all previous positions
 * since the last capture or pawn move. That state is only needed
 * for three-fold repetition draw detection, and is better stored
 * in a separate hash table.
 */
class Position {
private:
    int squares[64];

public:
    // Piece square table scores
    short psScore1[Piece::nPieceTypes];
    short psScore2[Piece::nPieceTypes];

    // Bitboards
    U64 pieceTypeBB[Piece::nPieceTypes];
    U64 whiteBB, blackBB;

    bool whiteMove;

    /** Number of half-moves since last 50-move reset. */
    int halfMoveClock;

    /** Game move number, starting from 1. */
    int fullMoveCounter;

private:
    int castleMask;
    int epSquare;

    U64 hashKey;           // Cached Zobrist hash key
    U64 pHashKey;          // Cached Zobrist pawn hash key

public:
    /** Bit definitions for the castleMask bit mask. */
    static const int A1_CASTLE = 0; /** White long castle. */
    static const int H1_CASTLE = 1; /** White short castle. */
    static const int A8_CASTLE = 2; /** Black long castle. */
    static const int H8_CASTLE = 3; /** Black short castle. */

    int wKingSq, bKingSq;   // Cached king positions
    int wMtrl;              // Total value of all white pieces and pawns
    int bMtrl;              // Total value of all black pieces and pawns
    int wMtrlPawns;         // Total value of all white pawns
    int bMtrlPawns;         // Total value of all black pawns

    /** Initialize board to empty position. */
    Position();

    bool equals(const Position& other) const {
        if (!drawRuleEquals(other))
            return false;
        if (halfMoveClock != other.halfMoveClock)
            return false;
        if (fullMoveCounter != other.fullMoveCounter)
            return false;
        if (hashKey != other.hashKey)
            return false;
        if (pHashKey != other.pHashKey)
            return false;
        return true;
    }

    int hashCode() const {
        return (int)hashKey;
    }

    /**
     * Return Zobrist hash value for the current position.
     * Everything except the move counters are included in the hash value.
     */
    uint64_t zobristHash() const {
        return hashKey;
    }
    uint64_t pawnZobristHash() const {
        return pHashKey;
    }
    uint64_t kingZobristHash() const {
        return psHashKeys[Piece::WKING][wKingSq] ^
               psHashKeys[Piece::BKING][bKingSq];
    }

    uint64_t historyHash() const {
        uint64_t ret = hashKey;
        if (halfMoveClock >= 80)
            ret ^= moveCntKeys[std::min(halfMoveClock, 100)];
        return ret;
    }

    /**
     * Decide if two positions are equal in the sense of the draw by repetition rule.
     * @return True if positions are equal, false otherwise.
     */
    bool drawRuleEquals(Position other) const {
        for (int i = 0; i < 64; i++)
            if (squares[i] != other.squares[i])
                return false;
        if (whiteMove != other.whiteMove)
            return false;
        if (castleMask != other.castleMask)
            return false;
        if (epSquare != other.epSquare)
            return false;
        return true;
    }

    void setWhiteMove(bool whiteMove) {
        if (whiteMove != this->whiteMove) {
            hashKey ^= whiteHashKey;
            this->whiteMove = whiteMove;
        }
    }

    /** Return piece occupying a square. */
    int getPiece(int square) const {
        return squares[square];
    }

    /** Set a square to a piece value. */
    void setPiece(int square, int piece);

    /**
     * Set a square to a piece value.
     * Special version that only updates enough of the state for the SEE function to be happy.
     */
    void setSEEPiece(int square, int piece) {
        int removedPiece = squares[square];

        // Update board
        squares[square] = piece;

        // Update bitboards
        U64 sqMask = 1ULL << square;
        pieceTypeBB[removedPiece] &= ~sqMask;
        pieceTypeBB[piece] |= sqMask;
        if (removedPiece != Piece::EMPTY) {
            if (Piece::isWhite(removedPiece))
                whiteBB &= ~sqMask;
            else
                blackBB &= ~sqMask;
        }
        if (piece != Piece::EMPTY) {
            if (Piece::isWhite(piece))
                whiteBB |= sqMask;
            else
                blackBB |= sqMask;
        }
    }

    /** Return true if white long castling right has not been lost. */
    bool a1Castle() const {
        return (castleMask & (1 << A1_CASTLE)) != 0;
    }
    /** Return true if white short castling right has not been lost. */
    bool h1Castle() const {
        return (castleMask & (1 << H1_CASTLE)) != 0;
    }
    /** Return true if black long castling right has not been lost. */
    bool a8Castle() const {
        return (castleMask & (1 << A8_CASTLE)) != 0;
    }
    /** Return true if black short castling right has not been lost. */
    bool h8Castle() const {
        return (castleMask & (1 << H8_CASTLE)) != 0;
    }
    /** Bitmask describing castling rights. */
    int getCastleMask() const {
        return castleMask;
    }
    void setCastleMask(int castleMask) {
        hashKey ^= castleHashKeys[this->castleMask];
        hashKey ^= castleHashKeys[castleMask];
        this->castleMask = castleMask;
    }

    /** En passant square, or -1 if no ep possible. */
    int getEpSquare() const {
        return epSquare;
    }
    void setEpSquare(int epSquare) {
        if (this->epSquare != epSquare) {
            hashKey ^= epHashKeys[(this->epSquare >= 0) ? getX(this->epSquare) + 1 : 0];
            hashKey ^= epHashKeys[(epSquare >= 0) ? getX(epSquare) + 1 : 0];
            this->epSquare = epSquare;
        }
    }

    int getKingSq(bool white) const {
        return white ? wKingSq : bKingSq;
    }

public:
    /** Apply a move to the current position. */
    void makeMove(const Move& move, UndoInfo& ui) {
        ui.capturedPiece = squares[move.to()];
        ui.castleMask = castleMask;
        ui.epSquare = epSquare;
        ui.halfMoveClock = halfMoveClock;
        bool wtm = whiteMove;

        const int p = squares[move.from()];
        int capP = squares[move.to()];
        U64 fromMask = 1ULL << move.from();

        int prevEpSquare = epSquare;
        setEpSquare(-1);

        if ((capP != Piece::EMPTY) || (((pieceTypeBB[Piece::WPAWN] | pieceTypeBB[Piece::BPAWN]) & fromMask) != 0)) {
            halfMoveClock = 0;

            // Handle en passant and epSquare
            if (p == Piece::WPAWN) {
                if (move.to() - move.from() == 2 * 8) {
                    int x = getX(move.to());
                    if (BitBoard::epMaskW[x] & pieceTypeBB[Piece::BPAWN])
                        setEpSquare(move.from() + 8);
                } else if (move.to() == prevEpSquare) {
                    setPiece(move.to() - 8, Piece::EMPTY);
                }
            } else if (p == Piece::BPAWN) {
                if (move.to() - move.from() == -2 * 8) {
                    int x = getX(move.to());
                    if (BitBoard::epMaskB[x] & pieceTypeBB[Piece::WPAWN])
                        setEpSquare(move.from() - 8);
                } else if (move.to() == prevEpSquare) {
                    setPiece(move.to() + 8, Piece::EMPTY);
                }
            }

            if (((pieceTypeBB[Piece::WKING] | pieceTypeBB[Piece::BKING]) & fromMask) != 0) {
                if (wtm) {
                    setCastleMask(castleMask & ~(1 << A1_CASTLE));
                    setCastleMask(castleMask & ~(1 << H1_CASTLE));
                } else {
                    setCastleMask(castleMask & ~(1 << A8_CASTLE));
                    setCastleMask(castleMask & ~(1 << H8_CASTLE));
                }
            }

            // Perform move
            setPiece(move.from(), Piece::EMPTY);
            // Handle promotion
            if (move.promoteTo() != Piece::EMPTY) {
                setPiece(move.to(), move.promoteTo());
            } else {
                setPiece(move.to(), p);
            }
        } else {
            halfMoveClock++;

            // Handle castling
            if (((pieceTypeBB[Piece::WKING] | pieceTypeBB[Piece::BKING]) & fromMask) != 0) {
                int k0 = move.from();
                if (move.to() == k0 + 2) { // O-O
                    movePieceNotPawn(k0 + 3, k0 + 1);
                } else if (move.to() == k0 - 2) { // O-O-O
                    movePieceNotPawn(k0 - 4, k0 - 1);
                }
                if (wtm) {
                    setCastleMask(castleMask & ~(1 << A1_CASTLE));
                    setCastleMask(castleMask & ~(1 << H1_CASTLE));
                } else {
                    setCastleMask(castleMask & ~(1 << A8_CASTLE));
                    setCastleMask(castleMask & ~(1 << H8_CASTLE));
                }
            }

            // Perform move
            movePieceNotPawn(move.from(), move.to());
        }
        if (wtm) {
            // Update castling rights when rook moves
            if ((BitBoard::maskCorners & fromMask) != 0) {
                if (p == Piece::WROOK)
                    removeCastleRights(move.from());
            }
            if ((BitBoard::maskCorners & (1ULL << move.to())) != 0) {
                if (capP == Piece::BROOK)
                    removeCastleRights(move.to());
            }
        } else {
            fullMoveCounter++;
            // Update castling rights when rook moves
            if ((BitBoard::maskCorners & fromMask) != 0) {
                if (p == Piece::BROOK)
                    removeCastleRights(move.from());
            }
            if ((BitBoard::maskCorners & (1ULL << move.to())) != 0) {
                if (capP == Piece::WROOK)
                    removeCastleRights(move.to());
            }
        }

        hashKey ^= whiteHashKey;
        whiteMove = !wtm;
    }

    void unMakeMove(const Move& move, UndoInfo& ui) {
        hashKey ^= whiteHashKey;
        whiteMove = !whiteMove;
        int p = squares[move.to()];
        setPiece(move.from(), p);
        setPiece(move.to(), ui.capturedPiece);
        setCastleMask(ui.castleMask);
        setEpSquare(ui.epSquare);
        halfMoveClock = ui.halfMoveClock;
        bool wtm = whiteMove;
        if (move.promoteTo() != Piece::EMPTY) {
            p = wtm ? Piece::WPAWN : Piece::BPAWN;
            setPiece(move.from(), p);
        }
        if (!wtm) {
            fullMoveCounter--;
        }

        // Handle castling
        int king = wtm ? Piece::WKING : Piece::BKING;
        if (p == king) {
            int k0 = move.from();
            if (move.to() == k0 + 2) { // O-O
                movePieceNotPawn(k0 + 1, k0 + 3);
            } else if (move.to() == k0 - 2) { // O-O-O
                movePieceNotPawn(k0 - 1, k0 - 4);
            }
        }

        // Handle en passant
        if (move.to() == epSquare) {
            if (p == Piece::WPAWN) {
                setPiece(move.to() - 8, Piece::BPAWN);
            } else if (p == Piece::BPAWN) {
                setPiece(move.to() + 8, Piece::WPAWN);
            }
        }
    }

    /**
     * Apply a move to the current position.
     * Special version that only updates enough of the state for the SEE function to be happy.
     */
    void makeSEEMove(const Move& move, UndoInfo& ui) {
        ui.capturedPiece = squares[move.to()];
        int p = squares[move.from()];

        // Handle en passant
        if (move.to() == epSquare) {
            if (p == Piece::WPAWN) {
                setSEEPiece(move.to() - 8, Piece::EMPTY);
            } else if (p == Piece::BPAWN) {
                setSEEPiece(move.to() + 8, Piece::EMPTY);
            }
        }

        // Perform move
        setSEEPiece(move.from(), Piece::EMPTY);
        setSEEPiece(move.to(), p);
        whiteMove = !whiteMove;
    }

    void unMakeSEEMove(const Move& move, UndoInfo& ui) {
        whiteMove = !whiteMove;
        int p = squares[move.to()];
        setSEEPiece(move.from(), p);
        setSEEPiece(move.to(), ui.capturedPiece);

        // Handle en passant
        if (move.to() == epSquare) {
            if (p == Piece::WPAWN) {
                setSEEPiece(move.to() - 8, Piece::BPAWN);
            } else if (p == Piece::BPAWN) {
                setSEEPiece(move.to() + 8, Piece::WPAWN);
            }
        }
    }

    /** Return index in squares[] vector corresponding to (x,y). */
    static int getSquare(int x, int y);

    /** Return x position (file) corresponding to a square. */
    static int getX(int square);

    /** Return y position (rank) corresponding to a square. */
    static int getY(int square);

    /** Return true if (x,y) is a dark square. */
    static bool darkSquare(int x, int y);

    /**
     * Compute the Zobrist hash value non-incrementally. Only useful for test programs.
     */
    U64 computeZobristHash();

    static void staticInitialize();

private:
    /** Move a non-pawn piece to an empty square. */
    void movePieceNotPawn(int from, int to);

    void removeCastleRights(int square) {
        if (square == getSquare(0, 0)) {
            setCastleMask(castleMask & ~(1 << A1_CASTLE));
        } else if (square == getSquare(7, 0)) {
            setCastleMask(castleMask & ~(1 << H1_CASTLE));
        } else if (square == getSquare(0, 7)) {
            setCastleMask(castleMask & ~(1 << A8_CASTLE));
        } else if (square == getSquare(7, 7)) {
            setCastleMask(castleMask & ~(1 << H8_CASTLE));
        }
    }

    /* ------------- Hashing code ------------------ */

public:
    static U64 psHashKeys[Piece::nPieceTypes][64];    // [piece][square]
private:
    static U64 whiteHashKey;
    static U64 castleHashKeys[16];   // [castleMask]
    static U64 epHashKeys[9];        // [epFile + 1] (epFile==-1 for no ep)
    static U64 moveCntKeys[101];     // [min(halfMoveClock, 100)]

    static U64 getRandomHashVal(int rndNo);

    static const U64 zobristRndKeys[];
};

/** For debugging. */
std::ostream& operator<<(std::ostream& os, const Position& pos);

inline int
Position::getSquare(int x, int y) {
    return y * 8 + x;
}

/** Return x position (file) corresponding to a square. */
inline int
Position::getX(int square) {
    return square & 7;
}

/** Return y position (rank) corresponding to a square. */
inline int
Position::getY(int square) {
    return square >> 3;
}

/** Return true if (x,y) is a dark square. */
inline bool
Position::darkSquare(int x, int y) {
    return (x & 1) == (y & 1);
}

#endif /* POSITION_HPP_ */
