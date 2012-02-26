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

#include <stdio.h>
#include <iostream>

using namespace std;

const int TranspositionTable::TTEntry::T_EXACT;
const int TranspositionTable::TTEntry::T_GE;
const int TranspositionTable::TTEntry::T_LE;
const int TranspositionTable::TTEntry::T_EMPTY;


void
TranspositionTable::insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore) {
    if (depth < 0) depth = 0;
    int idx0 = h0(key);
    int idx1 = h1(key);
    TTEntry* ent = &table[idx0];
    byte hashSlot = 0;
    if (ent->key != key) {
        ent = &table[idx1];
        hashSlot = 1;
    }
    if (ent->key != key) {
        if (table[idx1].betterThan(table[idx0], generation)) {
            ent = &table[idx0];
            hashSlot = 0;
        }
        if (ent->valuable(generation)) {
            int altEntIdx = (ent->getHashSlot() == 0) ? h1(ent->key) : h0(ent->key);
            if (ent->betterThan(table[altEntIdx], generation)) {
                table[altEntIdx] = (*ent);
                table[altEntIdx].setHashSlot(1 - ent->getHashSlot());
            }
        }
    }
    bool doStore = true;
    if ((ent->key == key) && (ent->getDepth() > depth) && (ent->type == type)) {
        if (type == TTEntry::T_EXACT)
            doStore = false;
        else if ((type == TTEntry::T_GE) && (sm.score() <= ent->getScore(ply)))
            doStore = false;
        else if ((type == TTEntry::T_LE) && (sm.score() >= ent->getScore(ply)))
            doStore = false;
    }
    if (doStore) {
        if ((ent->key != key) || (sm.from() != sm.to()))
            ent->setMove(sm);
        ent->key = key;
        ent->setScore(sm.score(), ply);
        ent->setDepth(depth);
        ent->generation = (byte)generation;
        ent->type = (byte)type;
        ent->setHashSlot(hashSlot);
        ent->evalScore = (short)evalScore;
    }
}

std::vector<Move>
TranspositionTable::extractPVMoves(const Position& rootPos, const Move& mFirst) const {
    Position pos(rootPos);
    Move m(mFirst);
    std::vector<Move> ret;
    UndoInfo ui;
    std::vector<U64> hashHistory;
    while (true) {
        ret.push_back(m);
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            break;
        hashHistory.push_back(pos.zobristHash());
        TTEntry ent = probe(pos.historyHash());
        if (ent.type == TTEntry::T_EMPTY)
            break;
        ent.getMove(m);
        MoveGen::MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool contains = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves.m[mi].equals(m)) {
                contains = true;
                break;
            }
        if  (!contains)
            break;
    }
    return ret;
}

/** Extract the PV starting from posIn, using hash entries, both exact scores and bounds. */
std::string
TranspositionTable::extractPV(const Position& posIn) const {
    std::string ret;
    Position pos(posIn);
    bool first = true;
    TTEntry ent = probe(pos.historyHash());
    UndoInfo ui;
    std::vector<U64> hashHistory;
    bool repetition = false;
    while (ent.type != TTEntry::T_EMPTY) {
        Move m;
        ent.getMove(m);
        MoveGen::MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool valid = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves.m[mi].equals(m)) {
                valid = true;
                break;
            }
        if  (!valid)
            break;
        if (repetition)
            break;
        if (!first)
            ret += ' ';
        if (ent.type == TTEntry::T_LE)
            ret += '<';
        else if (ent.type == TTEntry::T_GE)
            ret += '>';
        std::string moveStr = TextIO::moveToString(pos, m, false);
        ret += moveStr;
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            repetition = true;
        hashHistory.push_back(pos.zobristHash());
        ent = probe(pos.historyHash());
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
    for (int i = 0; i < (int)table.size(); i++) {
        const TTEntry& ent = table[i];
        if (ent.type == TTEntry::T_EMPTY) {
            unused++;
        } else {
            if (ent.generation == generation)
                thisGen++;
            if (ent.getDepth() < maxDepth)
                depHist[ent.getDepth()]++;
        }
    }
    double w = 100.0 / table.size();
    printf("Hash stats: size:%ld unused:%d (%.2f%%) thisGen:%d (%.2f%%)\n",
                      table.size(), unused, unused*w, thisGen, thisGen*w);
    for (int i = 0; i < maxDepth; i++) {
        int c = depHist[i];
        if (c > 0)
            printf("%3d %8d (%6.2f%%)\n", i, c, c*w);
    }
}
