/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * move.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef MOVE_HPP_
#define MOVE_HPP_

#include <iosfwd>
#include "util.hpp"
#include "square.hpp"

/** Represents a chess move. */
class Move {
public:
    /** Create empty move object. */
    Move();

    /** Create a move object. */
    Move(Square from, Square to, int promoteTo, int score = 0);

    /** Set move properties. */
    void setMove(Square from, Square to, int promoteTo, int score);

    void setScore(int score);

    /** Get 16 bit compressed representation of move, not including score. */
    U16 getCompressedMove() const;

    /** Set move from 16 bit compressed representation. Score not changed. */
    void setFromCompressed(U16 move);

    Square from() const;
    Square to() const;
    int promoteTo() const;
    int score() const;

    bool isEmpty() const;

    /** Note that score is not included in the comparison. */
    bool operator==(const Move& other) const;

    /** Not declared "nothrow". Avoids nullptr check in generated assembly code when using placement new. */
    void* operator new (std::size_t size, void* ptr);

    /** For debugging. */
    std::ostream& operator<<(std::ostream& os);

private:
    /** From square, 0-63. */
    Square from_;

    /** To square, 0-63. */
    Square to_;

    /** Promotion piece. */
    int promoteTo_;

    /** Score. */
    int score_;
};

inline
Move::Move()
    : from_(Square(0)), to_(Square(0)), promoteTo_(0), score_(0) {
}

inline
Move::Move(Square from, Square to, int promoteTo, int score)
    : from_(from), to_(to), promoteTo_(promoteTo), score_(score) {
}

inline void
Move::setMove(Square from, Square to, int promoteTo, int score) {
    from_ = from;
    to_ = to;
    promoteTo_ = promoteTo;
    score_ = score;
}

inline void
Move::setScore(int score) {
    score_ = score;
}

inline U16
Move::getCompressedMove() const {
    return (U16)(from().asInt() + (to().asInt() << 6) + (promoteTo() << 12));
}

inline void
Move::setFromCompressed(U16 move) {
    setMove(Square(move & 63), Square((move >> 6) & 63),
            (move >> 12) & 15, score());
}

inline Square
Move::from() const {
    return from_;
}

inline Square
Move::to() const {
    return to_;
}

inline int
Move::promoteTo() const {
    return promoteTo_;
}

inline int
Move::score() const {
    return score_;
}

inline bool
Move::isEmpty() const {
    return (from_.asInt() == 0) && (to_.asInt() == 0);
}

inline bool
Move::operator==(const Move& other) const {
    if (from_ != other.from_)
        return false;
    if (to_ != other.to_)
        return false;
    if (promoteTo_ != other.promoteTo_)
        return false;
    return true;
}

inline void*
Move::operator new (std::size_t size, void* ptr) {
    return ptr;
}

#endif /* MOVE_HPP_ */
