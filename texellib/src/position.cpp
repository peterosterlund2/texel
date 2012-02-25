/*
 * position.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "position.hpp"
#include <iostream>

U64 Position::psHashKeys[Piece::nPieceTypes][64];
U64 Position::whiteHashKey;
U64 Position::castleHashKeys[16];
U64 Position::epHashKeys[9];
U64 Position::moveCntKeys[101];


std::ostream&
Position::operator<<(std::ostream& os) {
//    os << TextIO::asciiBoard(this) << (whiteMove ? "white\n" : "black\n")
//       << Long.toHexString(zobristHash());
    return os;
}
