/*
    Texel - A UCI chess engine.
    Copyright (C) 2018  Peter Österlund, peterosterlund2@gmail.com

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
 * square.hpp
 *
 *  Created on: Jul 9, 2018
 *      Author: petero
 */

#ifndef SQUARE_HPP_
#define SQUARE_HPP_

enum SquareName {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

class Square {
public:
    /** Return index in squares[] vector corresponding to (x,y). */
    static int getSquare(int x, int y);

    /** Return x position (file) corresponding to a square. */
    static int getX(int square);

    /** Return y position (rank) corresponding to a square. */
    static int getY(int square);

    /** Return getSquare(7-getX(square),getY(square)). */
    static int mirrorX(int square);

    /** Return getSquare(getX(square),7-getY(square)). */
    static int mirrorY(int square);

    /** Return true if (x,y) is a dark square. */
    static bool darkSquare(int x, int y);
};

inline int
Square::getSquare(int x, int y) {
    return y * 8 + x;
}

/** Return x position (file) corresponding to a square. */
inline int
Square::getX(int square) {
    return square & 7;
}

/** Return y position (rank) corresponding to a square. */
inline int
Square::getY(int square) {
    return square >> 3;
}

inline int
Square::mirrorX(int square) {
    return square ^ 0x7;
}

inline int
Square::mirrorY(int square) {
    return square ^ 0x38;
}

/** Return true if (x,y) is a dark square. */
inline bool
Square::darkSquare(int x, int y) {
    return (x & 1) == (y & 1);
}

#endif /* BITBOARD_HPP_ */
