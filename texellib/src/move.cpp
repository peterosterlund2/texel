/*
 * move.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "move.hpp"
#include <iostream>

std::ostream&
Move::operator<<(std::ostream& os) {
//    os << TextIO::moveToUCIString(this);
    return os;
}
