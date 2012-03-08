/*
 * tuigame.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef TUIGAME_HPP_
#define TUIGAME_HPP_

#include "game.hpp"

#include <memory>

/**
 * Handles a game played using a text interface.
 */
class TUIGame : public Game {
public:
    TUIGame(std::shared_ptr<Player> whitePlayer, std::shared_ptr<Player> blackPlayer);

    /**
     * Administrate a game between two players, human or computer.
     */
    void play();

protected:
    bool handleCommand(const std::string& moveStr);

private:
    void showHelp();

    void handleTestSuite(const std::string& cmd);
};


#endif /* TUIGAME_HPP_ */
