/*
 * computerPlayer.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef COMPUTERPLAYER_HPP_
#define COMPUTERPLAYER_HPP_

#include "player.hpp"
#include "transpositionTable.hpp"
#include "book.hpp"
#include "search.hpp"

#include <string>
#include <memory>

/**
 * A computer algorithm player.
 */
class ComputerPlayer : public Player {
private:

    int minTimeMillis;
public:
    int maxTimeMillis;
    int maxDepth;
private:
    int maxNodes;
    TranspositionTable tt;
    Book book;
    bool bookEnabled;
    Search* currentSearch;
    std::shared_ptr <Search::Listener> listener;

    // Not implemented.
    ComputerPlayer(const ComputerPlayer& other);
    ComputerPlayer& operator=(const ComputerPlayer& other);

public:
    static std::string engineName;
    bool verbose;

    ComputerPlayer()
        : tt(15),
          book(verbose)
    {
        minTimeMillis = 10000;
        maxTimeMillis = 10000;
        maxDepth = 100;
        maxNodes = -1;
        verbose = true;
        bookEnabled = true;
        currentSearch = NULL;
    }

    void setTTLogSize(int logSize) {
        tt.reSize(logSize);
    }

    void setListener(const std::shared_ptr<Search::Listener>& listener) {
        this->listener = listener;
    }

    std::string getCommand(const Position& posIn, bool drawOffer, const std::vector<Position>& history);

    bool isHumanPlayer() {
        return false;
    }

    void useBook(bool bookOn) {
        bookEnabled = bookOn;
    }

    void timeLimit(int minTimeLimit, int maxTimeLimit) {
        minTimeMillis = minTimeLimit;
        maxTimeMillis = maxTimeLimit;
        if (currentSearch != NULL)
            currentSearch->timeLimit(minTimeLimit, maxTimeLimit);
    }

    void clearTT() {
        tt.clear();
    }

    /** Search a position and return the best move and score. Used for test suite processing. */
    std::pair<Move, std::string> searchPosition(Position& pos, int maxTimeMillis);

    /** Initialize static data. */
    static void staticInitialize();

private:
    /** Check if a draw claim is allowed, possibly after playing "move".
     * @param move The move that may have to be made before claiming draw.
     * @return The draw string that claims the draw, or empty string if draw claim not valid.
     */
    std::string canClaimDraw(Position& pos, std::vector<U64>& posHashList,
                             int posHashListSize, const Move& move);


    // FIXME!!! Test Botvinnik-Markoff extension
};


#endif /* COMPUTERPLAYER_HPP_ */
