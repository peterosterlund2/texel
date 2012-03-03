/*
 * humanPlayer.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef HUMANPLAYER_HPP_
#define HUMANPLAYER_HPP_

#include "player.hpp"

#include <string>


/**
 * A player that reads input from the keyboard.
 */
class HumanPlayer : public Player {
private:
    std::string lastCmd;

public:
    HumanPlayer() {
    }

    std::string getCommand(const Position& pos, bool drawOffer, const std::vector<Position>& history);

    bool isHumanPlayer() {
        return true;
    }

    void useBook(bool bookOn) {
    }

    void timeLimit(int minTimeLimit, int maxTimeLimit) {
    }

    void clearTT() {
    }
};


#endif /* HUMANPLAYER_HPP_ */
