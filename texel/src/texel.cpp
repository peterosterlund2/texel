/*
 * texel.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "computerPlayer.hpp"
#include "humanPlayer.hpp"
#include "tuigame.hpp"
#include "treeLogger.hpp"
#include "uciprotocol.hpp"

/**
 * Texel chess engine main function.
 */
int main(int argc, char* argv[]) {
    if ((argc == 2) && (std::string(argv[1]) == "txt")) {
        HumanPlayer whitePlayer;
        ComputerPlayer blackPlayer;
        blackPlayer.setTTLogSize(21);
        TUIGame game(whitePlayer, blackPlayer);
        game.play();
    } else if ((argc == 3) && (std::string(argv[1]) == "tree")) {
        TreeLoggerReader::main(argv[2]);
    } else {
        UCIProtocol::main(false);
    }
}
