/*
 * piece.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef PIECE_HPP_
#define PIECE_HPP_

/**
 * Constants for different piece types.
 */
class Piece {
public:
    static const int EMPTY = 0;
    static const int WKING = 1;
    static const int WQUEEN = 2;
    static const int WROOK = 3;
    static const int WBISHOP = 4;
    static const int WKNIGHT = 5;
    static const int WPAWN = 6;

    static const int BKING = 7;
    static const int BQUEEN = 8;
    static const int BROOK = 9;
    static const int BBISHOP = 10;
    static const int BKNIGHT = 11;
    static const int BPAWN = 12;

    static const int nPieceTypes = 13;

    /**
     * Return true if p is a white piece, false otherwise.
     * Note that if p is EMPTY, an unspecified value is returned.
     */
    static bool isWhite(int pType) {
        return pType < BKING;
    }

    static int makeWhite(int pType) {
        return pType < BKING ? pType : pType - (BKING - WKING);
    }

    static int makeBlack(int pType) {
        return ((pType > EMPTY) && (pType < BKING)) ? pType + (BKING - WKING) : pType;
    }
};

#endif /* PIECE_HPP_ */
