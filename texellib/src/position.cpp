/*
 * position.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "position.hpp"
#include <iostream>


std::ostream&
Position::operator<<(std::ostream& os) {
//    os << TextIO.asciiBoard(this) << (whiteMove ? "white\n" : "black\n")
//       << Long.toHexString(zobristHash());
    return os;
}
