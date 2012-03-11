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
    enum {
      EMPTY = 0,
      WKING = 1,
      WQUEEN = 2,
      WROOK = 3,
      WBISHOP = 4,
      WKNIGHT = 5,
      WPAWN = 6,

      BKING = 7,
      BQUEEN = 8,
      BROOK = 9,
      BBISHOP = 10,
      BKNIGHT = 11,
      BPAWN = 12,

      nPieceTypes = 13
    };

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
