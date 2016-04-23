/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * bookbuildcontrol.hpp
 *
 *  Created on: Apr 2, 2016
 *      Author: petero
 */

#ifndef BOOKBUILDCONTROL_HPP_
#define BOOKBUILDCONTROL_HPP_

#include "bookbuild.hpp"
#include <vector>
#include <set>
#include <memory>

class GameTree;

/** Provides an asynchronous interface to control the book building process.
 * All time consuming work is performed in separate worker threads.
 * All methods are thread safe.
 */
class BookBuildControl {
public:
    class ChangeListener {
    public:
        /** Called when state has changed. */
        virtual void notify() = 0;
    };

    /** Constructor. */
    BookBuildControl(ChangeListener& listener);
    ~BookBuildControl();

    /** Changes that requires the GUI to be updated. */
    enum class Change {
        TREE,               // A book node has been updated after a finished search.
        QUEUE,              // The queue of pending searches has changed.
        PV,                 // The analysis principal variation has changed.
        PROCESSING_COMPLETE // Processing (reading, writing) of opening book is complete.
    };

    /** Get state changes since last call to this method. */
    void getChanges(std::vector<Change>& changes);


    /** Create empty book. */
    void newBook();

    /** Load book from file. */
    void readFromFile(const std::string& newFileName);

    /** Save book to file. Use empty filename to save to current file.
     *  Throws exception if trying to save to current file when there is no current file. */
    void saveToFile(const std::string& filename);

    /** Get the current book filename. */
    std::string getBookFileName() const;


    /** Search related parameters. */
    struct Params {
        int computationTime = 100000;  // Computation time in milliseconds.
        int nThreads = 23;             // Maximum number of search threads to use.
        int bookDepthCost = 100;
        int ownPathErrorCost = 200;
        int otherPathErrorCost = 50;
    };

    /** Set search parameters. Search parameters can be changed also during search. */
    void setParams(const Params& params);

    /** Get current search parameters. */
    void getParams(Params& params);


    /** Start the search threads. */
    void startSearch();

    /** Stop the search threads. If "immediate" is true, stop all threads immediately.
     *  Otherwise, don't start new search jobs but let already running jobs complete. */
    void stopSearch(bool immediate);

    /** Increase the transposition table generation counter. */
    void nextGeneration();

    /** Return number of unfinished book tasks. */
    int numPendingBookTasks() const;


    struct TreeData {
        struct Parent {
            std::string fen;  // Parent position
            std::string move; // Move from parent position leading to current position
        };
        std::vector<Parent> parents;

        struct Child {
            std::string move;  // Book move
            int score;         // Negamax score from white's point of view
            int pathErrW;      // Accumulated white path error
            int pathErrB;      // Accumulated black path error
            int expandCostW;   // Expansion cost when building book for white
            int expandCostB;   // Expansion cost when building book for black
            double weightW;    // Book move weight for white
            double weightB;    // Book move weight for black
        };
        std::vector<Child> children; // Child moves, including the dropout move
        int searchTime;        // Search time in ms for the dropout move
    };

    /** Get information about the book node given by "pos". */
    void getTreeData(const Position& pos, TreeData& treeData) const;

    struct BookData {
        int nNodes;
        int nZeroTime;
    };

    /** Get book statistics. */
    void getBookData(BookData& bookData) const;

    struct QueueData {
        struct Item {
            std::string fen;  // Root position of search
            int searchTime;   // Time in ms since search started
            bool completed;   // True if search is completed.
        };
        std::vector<Item> items;
    };

    /** Get information about the currently running search jobs,
     *  and the last N completed jobs. */
    void getQueueData(QueueData& queueData) const;


    /** When selecting a position to expand, do not consider positions before "pos".  */
    void setFocus(const Position& pos);

    /** Get the focus position. Return false if no position is focused. */
    bool getFocus(Position& pos, std::vector<Move>& movesBefore,
                  std::vector<Move>& movesAfter);

    /** Get the book hash code for the focus position. */
    U64 getFocusHash() const;


    /** Add all positions up to depth "maxPly" to the book.
     *  This method must not be called when search threads are running. */
    void importPGN(const GameTree& gt, int maxPly);


    /** Start the analysis search thread. If analysis is already running, it
     *  is restarted with the provided position. The position to analyze is
     *  given by playing "moves" from the starting position. */
    void startAnalysis(const std::vector<Move>& moves);

    /** Stop the analysis search thread. Has no effect if the analysis thread
     *  is not running. */
    void stopAnalysis();

    /** Get information about the principal variation for the analysis search thread. */
    void getPVInfo(std::string& pv);

private:
    /** Add change to set of unreported changes and notify the listener. */
    void notify(Change change);

    mutable std::mutex mutex; // Main mutex for providing thread safe API.
    ChangeListener& listener;
    std::set<Change> changes; // Changes not yet reported to the listener.

    std::unique_ptr<BookBuild::Book> book; // Current book.
    std::string filename; // Current book filename, or empty string.
    Params params;        // Search parameters.

    /** Background worker thread for book processing operations. */
    std::shared_ptr<std::thread> bgThread;
    std::shared_ptr<std::thread> bgThread2;

    /** Book hash code for the current focus position. */
    std::atomic<U64> focusHash;
    std::atomic<int> stopFlag; // 0: Continue, 1: Don't start new jobs, 2: stop immediately
    int nPendingBookTasks;

    // Data used by the analysis thread.
    std::shared_ptr<std::thread> engineThread;
    std::shared_ptr<Search> sc;
    TranspositionTable tt;
    ParallelData pd;
    KillerTable kt;
    History ht;
    std::unique_ptr<Evaluate::EvalHashTables> et;
    TreeLogger treeLog;

    // Analysis result
    std::string analysisPV;
};

#endif /* BOOKBUILDCONTROL_HPP_ */
