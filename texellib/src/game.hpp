/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * game.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef GAME_HPP_
#define GAME_HPP_

#include "move.hpp"
#include "undoInfo.hpp"
#include "position.hpp"
#include "player.hpp"

#include <vector>
#include <string>
#include <memory>

/**
 * Handles a game between two players.
 */
class Game {
public:
    enum GameState {
        ALIVE,
        WHITE_MATE,         // White mates
        BLACK_MATE,         // Black mates
        WHITE_STALEMATE,    // White is stalemated
        BLACK_STALEMATE,    // Black is stalemated
        DRAW_REP,           // Draw by 3-fold repetition
        DRAW_50,            // Draw by 50 move rule
        DRAW_NO_MATE,       // Draw by impossibility of check mate
        DRAW_AGREE,         // Draw by agreement
        RESIGN_WHITE,       // White resigns
        RESIGN_BLACK        // Black resigns
    };

    Position pos;
    std::shared_ptr<Player> whitePlayer;
    std::shared_ptr<Player> blackPlayer;

protected:
    std::vector<Move> moveList;
    std::vector<UndoInfo> uiInfoList;
    std::vector<bool> drawOfferList;
    int currentMove;

private:
    std::string drawStateMoveStr; // Move required to claim DRAW_REP or DRAW_50
    GameState resignState;

public:
    bool pendingDrawOffer;
    GameState drawState;

    Game(const std::shared_ptr<Player>& whitePlayer,
         const std::shared_ptr<Player>& blackPlayer);

    virtual ~Game() { }

    /**
     * Update the game state according to move/command string from a player.
     * @param str The move or command to process.
     * @return True if str was understood, false otherwise.
     */
    bool processString(const std::string& str);

    std::string getGameStateString();

    /**
     * Get the last played move, or null if no moves played yet.
     */
    Move getLastMove();

    /**
     * Get the current state of the game.
     */
    GameState getGameState();

    /**
     * Check if a draw offer is available.
     * @return True if the current player has the option to accept a draw offer.
     */
    bool haveDrawOffer();

    void getPosHistory(std::vector<std::string> ret);

    std::string getMoveListString(bool compressed);

    std::string getPGNResultString();

    /** Return a list of previous positions in this game, back to the last "zeroing" move. */
    void getHistory(std::vector<Position>& posList);

    static U64 perfT(Position& pos, int depth);

protected:
    /**
     * Handle a special command.
     * @param moveStr  The command to handle
     * @return  True if command handled, false otherwise.
     */
    virtual bool handleCommand(const std::string& moveStr);

    /** Swap players around if needed to make the human player in control of the next move. */
    void activateHumanPlayer();

private:
    /**
     * Print a list of all moves.
     */
    void listMoves();

    bool handleDrawCmd(std::string drawCmd);

    bool handleBookCmd(const std::string& bookCmd);

    bool insufficientMaterial();

    // Not implemented
    Game(const Game& other);
    Game& operator=(const Game& other);
};


#endif /* GAME_HPP_ */
