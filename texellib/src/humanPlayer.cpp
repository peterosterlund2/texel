/*
 * humanPlayer.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "humanPlayer.hpp"
#include "position.hpp"

#include <iostream>


std::string
HumanPlayer::getCommand(const Position& pos, bool drawOffer, const std::vector<Position>& history) {
    const char* color = pos.whiteMove ? "white" : "black";
    std::cout << "Enter move (" << color << "):" << std::flush;
    std::string moveStr;
    if (getline(std::cin, moveStr) < 0)
        return "quit";
    if (moveStr.length() == 0) {
        return lastCmd;
    } else {
        lastCmd = moveStr;
    }
    return moveStr;
}
