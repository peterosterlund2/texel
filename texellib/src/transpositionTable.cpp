/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * transpositionTable.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "transpositionTable.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "textio.hpp"

#include <iostream>
#include <iomanip>

using namespace std;


void
TranspositionTable::reSize(int log2Size) {
    const size_t numEntries = ((size_t)1) << log2Size;
    table.resize(numEntries);
    generation = 0;
}

void
TranspositionTable::insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore) {
    if (depth < 0) depth = 0;
    const size_t idx0 = getIndex(key);
    TTEntry ent0, ent1, newEnt;
    ent0.load(table[idx0]);
    TTEntry* ent = nullptr;
    size_t idx;
    newEnt.setDepth(depth);
    newEnt.setType(type);
    newEnt.setScore(sm.score(), ply);
    const U64 key2 = getStoredKey(key);
    if (ent0.getKey() == key2) {
        if (newEnt.valueGE(ent0)) {
            ent = &ent0;
            idx = idx0;
        } else if (ent0.valueGE(newEnt))
            return;
    }
    const size_t idx1 = idx0 ^ 1;
    if (ent == nullptr) {
        ent1.load(table[idx1]);
        if (ent1.getKey() == key2) {
            if (newEnt.valueGE(ent1)) {
                ent = &ent1;
                idx = idx1;
            } else if (ent1.valueGE(newEnt))
                return;
        }
    }
    if (ent == nullptr) {
        if (ent0.replaceBetterThan(ent1, generation)) {
            ent = &ent1;
            idx = idx1;
        } else {
            ent = &ent0;
            idx = idx0;
        }
    }

    if ((ent->getKey() != key2) || (sm.from() != sm.to()))
        ent->setMove(sm);
    ent->setKey(key2);
    ent->setScore(sm.score(), ply);
    ent->setDepth(depth);
    ent->setGeneration((S8)generation);
    ent->setType(type);
    ent->setEvalScore(evalScore);
    ent->store(table[idx]);
}

void
TranspositionTable::probe(U64 key, int alpha, int beta, int ply, int depth,
                          TTEntry& result) {
    const size_t idx0 = getIndex(key);
    const U64 key2 = getStoredKey(key);
    TTEntry ent0, ent1;
    TTEntry* ent = nullptr;
    size_t idx;
    ent0.load(table[idx0]);
    const size_t idx1 = idx0 ^ 1;
    if (ent0.getKey() == key2) {
        ent1.load(table[idx1]);
        if (ent1.getKey() == key2) {
            bool cutOff0 = ent0.isCutOff(alpha, beta, ply, depth);
            bool cutOff1 = ent1.isCutOff(alpha, beta, ply, depth);
            if (cutOff0 && !cutOff1) {
                ent = &ent0;
                idx = idx0;
            } else if (cutOff1 && !cutOff0) {
                ent = &ent1;
                idx = idx1;
            } else {
                if (ent0.getDepth() >= ent1.getDepth()) {
                    ent = &ent0;
                    idx = idx0;
                } else {
                    ent = &ent1;
                    idx = idx1;
                }
            }
        } else {
            ent = &ent0;
            idx = idx0;
        }
    } else {
        ent1.load(table[idx1]);
        if (ent1.getKey() == key2) {
            ent = &ent1;
            idx = idx1;
        }
    }

    if (!ent) {
        result.setType(TType::T_EMPTY);
        return;
    }

    if (ent->getGeneration() != generation) {
        ent->setGeneration(generation);
        ent->store(table[idx]);
    }
    result = *ent;
}

void
TranspositionTable::extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv) {
    Position pos(rootPos);
    Move m(mFirst);
    UndoInfo ui;
    std::vector<U64> hashHistory;
    while (true) {
        pv.push_back(m);
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            break;
        hashHistory.push_back(pos.zobristHash());
        TTEntry ent;
        ent.clear();
        const int mate0 = SearchConst::MATE0;
        probe(pos.historyHash(), -mate0, mate0, 0, 0, ent);
        if (ent.getType() == TType::T_EMPTY)
            break;
        ent.getMove(m);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool contains = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi].equals(m)) {
                contains = true;
                break;
            }
        if  (!contains)
            break;
    }
}

/** Extract the PV starting from posIn, using hash entries, both exact scores and bounds. */
std::string
TranspositionTable::extractPV(const Position& posIn) {
    std::string ret;
    Position pos(posIn);
    bool first = true;
    TTEntry ent;
    ent.clear();
    const int mate0 = SearchConst::MATE0;
    probe(pos.historyHash(), -mate0, mate0, 0, 0, ent);
    UndoInfo ui;
    std::vector<U64> hashHistory;
    bool repetition = false;
    while (ent.getType() != TType::T_EMPTY) {
        Move m;
        ent.getMove(m);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool valid = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi].equals(m)) {
                valid = true;
                break;
            }
        if  (!valid)
            break;
        if (repetition)
            break;
        if (!first)
            ret += ' ';
        if (ent.getType() == TType::T_LE)
            ret += '<';
        else if (ent.getType() == TType::T_GE)
            ret += '>';
        std::string moveStr = TextIO::moveToString(pos, m, false);
        ret += moveStr;
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            repetition = true;
        hashHistory.push_back(pos.zobristHash());
        probe(pos.historyHash(), -mate0, mate0, 0, 0, ent);
        first = false;
    }
    return ret;
}

void
TranspositionTable::printStats() const {
    int unused = 0;
    int thisGen = 0;
    std::vector<int> depHist;
    const int maxDepth = 20*8;
    depHist.resize(maxDepth);
    for (size_t i = 0; i < table.size(); i++) {
        TTEntry ent;
        ent.load(table[i]);
        if (ent.getType() == TType::T_EMPTY) {
            unused++;
        } else {
            if (ent.getGeneration() == generation)
                thisGen++;
            if (ent.getDepth() < maxDepth)
                depHist[ent.getDepth()]++;
        }
    }
    double w = 100.0 / table.size();
    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << "hstat: size:" << table.size()
       << " unused:" << unused << " (" << (unused*w) << "%)"
       << " thisGen:" << thisGen << " (" << (thisGen*w) << "%)" << std::endl;
    cout << ss.str();
    for (int i = 0; i < maxDepth; i++) {
        int c = depHist[i];
        if (c > 0) {
            std::stringstream ss;
            ss.precision(2);
            ss << std::setw(4) << i
               << ' ' << std::setw(8) << c
               << " " << std::setw(6) << std::fixed << (c*w);
            std::cout << "hstat:" << ss.str() << std::endl;
        }
    }
}
