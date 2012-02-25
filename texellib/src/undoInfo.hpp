/*
 * undoInfo.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef UNDOINFO_HPP_
#define UNDOINFO_HPP_

/**
 * Contains enough information to undo a previous move.
 * Set by makeMove(). Used by unMakeMove().
 */
struct UndoInfo {
    int capturedPiece;
    int castleMask;
    int epSquare;
    int halfMoveClock;
};

#endif /* UNDOINFO_HPP_ */
