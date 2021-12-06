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
 * chessError.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef CHESSERROR_HPP_
#define CHESSERROR_HPP_

#include <exception>
#include <string>

/** Generic chess related errors. */
class ChessError : public std::exception {
public:
    explicit ChessError(const std::string& msg);

    const char* what() const noexcept override;

private:
    std::string msg_;
};

/** Parse errors in FEN, algebraic move notation, text files, etc. */
class ChessParseError : public ChessError {
public:
    explicit ChessParseError(const std::string& msg);
};


inline
ChessError::ChessError(const std::string& msg)
    : msg_(msg) {
}

inline const char*
ChessError::what() const noexcept {
    return msg_.c_str();
}

inline
ChessParseError::ChessParseError(const std::string& msg)
    : ChessError(msg) {
}

#endif /* CHESSERROR_HPP_ */
