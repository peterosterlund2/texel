/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * enginecontrol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef ENGINECONTROL_HPP_
#define ENGINECONTROL_HPP_

#include "search.hpp"
#include "transpositionTable.hpp"
#include "position.hpp"
#include "move.hpp"
#include "parallel.hpp"

#include <vector>
#include <iosfwd>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>

class SearchParams;
class SearchListener;
class EngineControl;

/** State needed by the main engine search thread. */
class EngineMainThread {
public:
    EngineMainThread(SearchListener& listener);

    /** Called by the main search thread. Waits for and acts upon start and quit
     *  calls from another thread. */
    void mainLoop();

    /** Tells the main loop to terminate. */
    void quit();

    /** Tell the search thread to start searching. */
    void startSearch(EngineControl* engineControl,
                     std::shared_ptr<Search>& sc, const Position& pos,
                     std::shared_ptr<MoveList>& moves,
                     bool ownBook, bool analyseMode,
                     int maxDepth, int maxNodes,
                     int maxPV, int minProbeDepth,
                     std::atomic<bool>& ponder, std::atomic<bool>& infinite);

    /** Wait for the search thread to stop searching. */
    void waitStop();

private:
    void doSearch();

    SearchListener& listener;

    std::mutex mutex;
    std::condition_variable newCommand;
    std::condition_variable searchStopped;
    bool search = false;
    bool quitFlag = false;

    EngineControl* engineControl = nullptr;
    std::shared_ptr<Search> sc;
    Position pos;
    std::shared_ptr<MoveList> moves;
    bool ownBook = false;
    bool analyseMode = false;
    int maxDepth = -1;
    int maxNodes = -1;
    int maxPV = 1;
    int minProbeDepth = 0;
    std::atomic<bool>* ponder = nullptr;
    std::atomic<bool>* infinite = nullptr;
};

/**
 * Control the search thread.
 */
class EngineControl {
public:
    EngineControl(std::ostream& o, EngineMainThread& engineThread, SearchListener& listener);
    ~EngineControl();

    void startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void ponderHit();

    void stopSearch();

    void newGame();

    static void printOptions(std::ostream& os);

    void setOption(const std::string& optionName, const std::string& optionValue,
                   bool deferIfBusy);

    void finishSearch(Position& pos, const Move& bestMove);

private:
    /**
     * Compute thinking time for current search.
     */
    void computeTimeLimit(const SearchParams& sPar);

    void startThread(int minTimeLimit, int maxTimeLimit, int earlyStopPercentage,
                     int maxDepth, int maxNodes);

    void stopThread();

    void setupTT();

    void setupPosition(Position pos, const std::vector<Move>& moves);

    /**
     * Try to find a move to ponder from the transposition table.
     */
    Move getPonderMove(Position pos, const Move& m);


    std::ostream& os;

    int hashParListenerId;
    int clearHashParListenerId;

    std::mutex searchingMutex;
    bool isSearching = false;
    std::map<std::string, std::string> pendingOptions;

    EngineMainThread& engineThread;
    SearchListener& listener;
    std::shared_ptr<Search> sc;
    TranspositionTable tt;
    ParallelData pd;
    KillerTable kt;
    History ht;
    std::unique_ptr<Evaluate::EvalHashTables> et;
    TreeLogger treeLog;

    Position pos;
    std::vector<U64> posHashList;
    int posHashListSize;
    std::atomic<bool> ponder;     // True if currently doing pondering
    bool onePossibleMove;
    std::atomic<bool> infinite;

    int minTimeLimit;
    int maxTimeLimit;
    int earlyStopPercentage;
    int maxDepth;
    int maxNodes;
    std::vector<Move> searchMoves;

    // Random seed for reduced strength
    U64 randomSeed;
};


#endif /* ENGINECONTROL_HPP_ */
