/*
 * texelutil.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "position.hpp"
#include "textio.hpp"

#include <iostream>
#include <string>


int main(int argc, char* argv[]) {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Position pos = TextIO::readFEN(fen);
    std::cout << TextIO::asciiBoard(pos);
    return 0;
}
