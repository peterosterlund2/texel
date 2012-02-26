/*
 * computerPlayer.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "computerPlayer.hpp"

static StaticInitializer<ComputerPlayer> cpInit;

void
ComputerPlayer::staticInitialize() {
    std::string name = "Texel 1.00";
    if (false)
        name += " 32-bit";
    if (false)
        name += " 64-bit";
    engineName = name;
}
