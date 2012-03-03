/*
 * humanPlayer.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "humanPlayer.hpp"
#include "position.hpp"

#include <stdio.h>


std::string
HumanPlayer::getCommand(const Position& pos, bool drawOffer, const std::vector<Position>& history) {
    const char* color = pos.whiteMove ? "white" : "black";
    printf("Enter move (%s):", color);
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
