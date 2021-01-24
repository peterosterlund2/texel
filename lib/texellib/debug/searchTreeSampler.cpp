/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * searchTreeSampler.cpp
 *
 *  Created on: Feb 7, 2021
 *      Author: petero
 */

#include "searchTreeSampler.hpp"
#include "search.hpp"
#include "transpositionTable.hpp"
#include "killerTable.hpp"
#include "history.hpp"
#include "textio.hpp"

#include <iostream>
#include <fstream>

using namespace SearchConst;

static std::mutex mutex;

void
SearchTreeSamplerReal::doSample(const Position& pos, Evaluate& eval, int q0Eval) {
    if (q0Eval == UNKNOWN_SCORE)
        q0Eval = eval.evalPos(pos);
    if (!pos.isWhiteMove())
        q0Eval = -q0Eval;

    Data d;
    pos.serialize(d.pos);
    std::lock_guard<std::mutex> L(mutex);
    samples.push_back(d);
}

void
SearchTreeSamplerReal::writeToFile(int searchScore) {
    std::lock_guard<std::mutex> L(mutex);

    if (samples.empty())
        return;

    static std::ofstream of("positions.txt",
                            std::ios_base::out | std::ios_base::app);

    TranspositionTable tt(512*1024);
    Notifier notifier;
    ThreadCommunicator comm(nullptr, tt, notifier, false);

    KillerTable kt;
    History ht;
    Evaluate::EvalHashTables et;
    Search::SearchTables st(comm.getCTT(), kt, ht, et);

    std::vector<U64> nullHist = std::vector<U64>(SearchConst::MAX_SEARCH_DEPTH * 2);
    TreeLogger treeLog;

    Position pos;
    Search sc(pos, nullHist, 0, st, comm, treeLog);

    for (Data d : samples) {
        pos.deSerialize(d.pos);

        sc.init(pos, nullHist, 0);
        auto ret = sc.quiescePos(-MATE0, MATE0, 0, 0, MoveGen::inCheck(pos));
        int eval = ret.first * (pos.isWhiteMove() ? 1 : -1);

        pos.deSerialize(ret.second);

        of << TextIO::toFEN(pos)
           << " eval:" << eval << " search:" << searchScore << std::endl;
    }
    samples.clear();
}

