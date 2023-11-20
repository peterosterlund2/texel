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


/** A square on a chess board. Squares also have an integer representation,
 *  corresponding to the SquareName enum. */
class Square {
public:
    /** Create an invalid square. */
    Square();
    /** Create a square with given x/y coordinates. */
    Square(int x, int y);
    /** Create a square from the integer representation. */
    explicit Square(int sq);
    /** Create a square from a square name. */
    Square(SquareName s);

    /** Return true if the square is valid. */
    bool isValid() const;

    /** Return the integer representation of the square. */
    int asInt() const;

    /** Return square x position (file). */
    int getX() const;

    /** Return square y position (rank). */
    int getY() const;

    /** Return square mirrored in x direction. */
    Square mirrorX() const;

    /** Return square mirrored in y direction. */
    Square mirrorY() const;

    /** Return square rotated 180 degrees around center, i.e. mirrored in both X and Y direction. */
    Square rot180() const;

    /** Return true if square is dark. */
    bool isDark() const;

    /** Comparison operators. */
    bool operator==(Square other) const;
    bool operator!=(Square other) const;
    bool operator==(SquareName s) const;
    bool operator!=(SquareName s) const;

    Square& operator+=(int d);

private:
    int sq;
};


/** An iterator-like class that makes it possible to iterate over
 * all 64 chess board squares in order, without having an actual
 * container of squares to iterate over. For example:
 * for (Square sq : AllSquares()) {
 *     // Do something with sq
 * } */
class AllSquares {
public:
    AllSquares() : sq(0) {}
    AllSquares begin() const { return AllSquares(); }
    AllSquares end() const { return AllSquares(64); }
    Square operator*() const { return Square(sq); }
    AllSquares& operator++() { ++sq; return *this; }
    bool operator!=(const AllSquares& o) const { return sq != o.sq; }
private:
    explicit AllSquares(int sq) : sq(sq) {}
    int sq; // Current square
};


inline Square::Square()
    : sq(-1) {
}

inline Square::Square(int x, int y)
    : sq(y * 8 + x) {
}

inline Square::Square(SquareName s)
    : sq(static_cast<int>(s)) {
}

inline Square::Square(int sq)
    : sq(sq) {
}

inline bool
Square::isValid() const {
    return sq != -1;
}

inline int
Square::asInt() const {
    return sq;
}

inline int
Square::getX() const {
    return sq & 7;
}

inline int
Square::getY() const {
    return sq >> 3;
}

inline Square
Square::mirrorX() const {
    return Square(sq ^ 0x7);
}

inline Square
Square::mirrorY() const {
    return Square(sq ^ 0x38);
}

inline Square
Square::rot180() const {
    return Square(63 - sq);
}

inline bool
Square::isDark() const {
    return (getX() & 1) == (getY() & 1);
}

inline bool
Square::operator==(Square other) const {
    return sq == other.sq;
}

inline bool
Square::operator!=(Square other) const {
    return sq != other.sq;
}

inline bool
Square::operator==(SquareName s) const {
    return sq == s;
}

inline bool
Square::operator!=(SquareName s) const {
    return sq != s;
}


// Operator overloading

inline Square&
Square::operator+=(int d) {
    sq += d;
    return *this;
}

inline Square operator+(Square a, int b) {
    return Square(a.asInt() + b);
}

inline Square operator-(Square a, int b) {
    return Square(a.asInt() - b);
}


#endif /* BITBOARD_HPP_ */
