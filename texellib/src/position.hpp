/*
 * position.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include <iosfwd>

/**
 * Stores the state of a chess position.
 * All required state is stored, except for all previous positions
 * since the last capture or pawn move. That state is only needed
 * for three-fold repetition draw detection, and is better stored
 * in a separate hash table.
 */
class Position {
public:

    /** Return index in squares[] vector corresponding to (x,y). */
    static int getSquare(int x, int y);

    /** Return x position (file) corresponding to a square. */
    static int getX(int square);

    /** Return y position (rank) corresponding to a square. */
    static int getY(int square);

    /** Return true if (x,y) is a dark square. */
    static bool darkSquare(int x, int y);

    /** For debugging. */
    std::ostream& operator<<(std::ostream& os);
};

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
