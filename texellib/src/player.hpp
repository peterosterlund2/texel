/*
 * player.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef PLAYER_HPP_
#define PLAYER_HPP_

/**
 * Interface for human/computer players.
 */
#if 0
public interface Player {
    /**
     * Get a command from a player.
     * The command can be a valid move string, in which case the move is played
     * and the turn goes over to the other player. The command can also be a special
     * command, such as "quit", "new", "resign", etc.
     * @param history List of earlier positions (not including the current position).
     *                This makes it possible for the player to correctly handle
     *                the draw by repetition rule.
     */
    public std::string getCommand(const Position& pos, bool drawOffer, List<Position> history);
    
    /** Return true if this player is a human player. */
    public bool isHumanPlayer();

    /**
     * Inform player whether or not to use an opening book.
     * Of course, a human player is likely to ignore this.
     */
    public void useBook(bool bookOn);

    /**
     * Inform player about min recommended/max allowed thinking time per move.
     * Of course, a human player is likely to ignore this.
     */
    public void timeLimit(int minTimeLimit, int maxTimeLimit, bool randomMode);

    /** 
     * Inform player that the transposition table should be cleared.
     * Of course, a human player has a hard time implementing this.
     */
    public void clearTT();
};
#endif

#endif /* PLAYER_HPP_ */
