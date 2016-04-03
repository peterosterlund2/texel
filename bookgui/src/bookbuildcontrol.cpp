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


BookBuildControl::BookBuildControl(ChangeNotifier& notifier0)
    : notifier(notifier0), tt(27), pd(tt),
      analysisScore(0) {
}

void
BookBuildControl::getChanges(std::vector<Change>& changes) {

}

// --------------------------------------------------------------------------------

void
BookBuildControl::newBook() {

}

void
BookBuildControl::readFromFile(const std::string& filename) {

}

void
BookBuildControl::saveToFile(const std::string& filename) {

}

std::string
BookBuildControl::getBookFile() const {
    return "";
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

void
BookBuildControl::getFocus(Position& pos) {

}

// --------------------------------------------------------------------------------

void
BookBuildControl::importPGN(const std::string pgn, int maxPly) {

}

// --------------------------------------------------------------------------------

void
BookBuildControl::startAnalysis(const Position& pos) {

}

void
BookBuildControl::stopAnalysis() {

}

void
BookBuildControl::getPV(int& score, std::string& pv) {

}

// --------------------------------------------------------------------------------
