/*
 * chessParseError.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef CHESSPARSEERROR_HPP_
#define CHESSPARSEERROR_HPP_

#include <exception>
#include <string>

/**
 * Exception class to represent parse errors in FEN or algebraic notation.
 */
class ChessParseError : public std::exception {
public:
    ChessParseError();

    ~ChessParseError() throw() {}

    ChessParseError(const std::string& msg) : msg_(msg)
    {
    }

    virtual const char* what() const throw() {
        return msg_.c_str();
    }

private:
    std::string msg_;
};

#endif /* CHESSPARSEERROR_HPP_ */
