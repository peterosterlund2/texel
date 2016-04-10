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
 * bookbuildcontrol.cpp
 *
 *  Created on: Apr 2, 2016
 *      Author: petero
 */

#include "bookbuildcontrol.hpp"
#include "textio.hpp"
#include "search.hpp"
#include "computerPlayer.hpp"


BookBuildControl::BookBuildControl(ChangeListener& listener0)
    : listener(listener0), tt(27), pd(tt) {
    ComputerPlayer::initEngine();
    et = Evaluate::getEvalHashTables();
}

BookBuildControl::~BookBuildControl() {
    stopAnalysis();
}

void
BookBuildControl::getChanges(std::vector<Change>& changes) {
    std::lock_guard<std::mutex> L(mutex);
    for (Change c : this->changes)
        changes.push_back(c);
    this->changes.clear();
}

void
BookBuildControl::notify(Change change) {
    {
        std::lock_guard<std::mutex> L(mutex);
        changes.insert(change);
    }
    listener.notify();
}

// --------------------------------------------------------------------------------

void
BookBuildControl::newBook() {
    // Create new book

    filename.clear();
}

void
BookBuildControl::readFromFile(const std::string& newFileName) {
    filename = newFileName;

    // Read in different thread
}

void
BookBuildControl::saveToFile(const std::string& newFileName) {
    if (!newFileName.empty())
        filename = newFileName;

    std::cout << "save to file: " << filename << std::endl;

    // Save in different thread
}

std::string
BookBuildControl::getBookFileName() const {
    return filename;
}

// --------------------------------------------------------------------------------

void
BookBuildControl::setParams(const Params& params) {

}

void
BookBuildControl::getParams(Params& params) {

}

// --------------------------------------------------------------------------------

void
BookBuildControl::startSearch() {

}

void
BookBuildControl::stopSearch(bool immediate) {

}

void
BookBuildControl::nextGeneration() {

}

int
BookBuildControl::nRunningThreads() const {
    return 0;
}

// --------------------------------------------------------------------------------

void
BookBuildControl::getTreeData(const Position& pos, TreeData& treeData) const {

}

void
BookBuildControl::getBookData(BookData& bookData) const {

}

void
BookBuildControl::getQueueData(QueueData& queueData) const {

}

// --------------------------------------------------------------------------------

void
BookBuildControl::setFocus(const Position& pos) {

}

bool
BookBuildControl::getFocus(Position& pos, std::vector<Move>& movesBefore,
                           std::vector<Move>& movesAfter) {
    return false;
}

// --------------------------------------------------------------------------------

void
BookBuildControl::importPGN(const std::string pgn, int maxPly) {

}

// --------------------------------------------------------------------------------

void
BookBuildControl::startAnalysis(const std::vector<Move>& moves) {
    if (engineThread)
        stopAnalysis();

    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    std::vector<U64> posHashList(200 + moves.size());
    int posHashListSize = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        const Move& m = moves[i];
        posHashList[posHashListSize++] = pos.zobristHash();
        pos.makeMove(m, ui);
        if (pos.getHalfMoveClock() == 0)
            posHashListSize = 0;
    }

    class SearchListener : public Search::Listener {
    public:
        SearchListener(BookBuildControl& bbc0, const Position& pos) : bbc(bbc0), pos0(pos) {}
        void notifyDepth(int depth) override {}
        void notifyCurrMove(const Move& m, int moveNr) override {}
        void notifyPV(int depth, int score, int time, U64 nodes, int nps,
                              bool isMate, bool upperBound, bool lowerBound,
                              const std::vector<Move>& pv, int multiPVIndex,
                              U64 tbHits) override {
            Position pos(pos0);
            std::stringstream ss;
            ss << "[" << depth << "] ";
            bool negateScore = !pos.isWhiteMove();
            if (upperBound || lowerBound) {
                bool upper = upperBound ^ negateScore;
                ss << (upper ? "<=" : ">=");
            }
            int s = negateScore ? -score : score;
            if (isMate) {
                ss << 'm' << s;
            } else {
                ss.precision(2);
                ss << std::fixed << (s / 100.0);
            }
            UndoInfo ui;
            for (const Move& m : pv) {
                ss << ' ' << TextIO::moveToString(pos, m, false);
                pos.makeMove(m, ui);
            }
            {
                std::lock_guard<std::mutex> L(bbc.mutex);
                bbc.analysisPV = ss.str();
            }
            bbc.notify(BookBuildControl::Change::PV);
        }
        void notifyStats(U64 nodes, int nps, U64 tbHits, int time) override {}
    private:
        BookBuildControl& bbc;
        Position pos0;
    };

    Search::SearchTables st(tt, kt, ht, *et);
    sc = std::make_shared<Search>(pos, posHashList, posHashListSize, st, pd, nullptr, treeLog);
    sc->setListener(make_unique<SearchListener>(*this, pos));
    std::shared_ptr<MoveList> moveList(std::make_shared<MoveList>());
    MoveGen::pseudoLegalMoves(pos, *moveList);
    MoveGen::removeIllegal(pos, *moveList);
    pd.addRemoveWorkers(0);
    pd.wq.resetSplitDepth();
    pd.startAll();
    sc->timeLimit(-1, -1);
    int minProbeDepth = UciParams::minProbeDepth->getIntPar();
    auto f = [this,moveList,minProbeDepth]() {
        sc->iterativeDeepening(*moveList, -1, -1, false, 1, false, minProbeDepth);
    };
    engineThread = std::make_shared<std::thread>(f);
}

void
BookBuildControl::stopAnalysis() {
    if (engineThread) {
        sc->timeLimit(0, 0);
        engineThread->join();
        engineThread.reset();
        sc.reset();
        pd.stopAll();
    }
}

void
BookBuildControl::getPVInfo(std::string& pv) {
    std::lock_guard<std::mutex> L(mutex);
    pv = analysisPV;
}

// --------------------------------------------------------------------------------
