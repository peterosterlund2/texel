/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
 * transpositionTableTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "transpositionTableTest.hpp"
#include "transpositionTable.hpp"
#include "position.hpp"
#include "textio.hpp"
#include "searchTest.hpp"
#include <iostream>

#include "cute.h"

using TTEntry = TranspositionTable::TTEntry;

/**
 * Test of TTEntry nested class, of class TranspositionTable.
 */
static void
testTTEntry() {
    const int mate0 = SearchConst::MATE0;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    Move move = TextIO::stringToMove(pos, "e4");

    // Test "normal" (non-mate) score
    int score = 17;
    int ply = 3;
    TTEntry ent1;
    ent1.setKey(1);
    ent1.setMove(move);
    ent1.setScore(score, ply);
    ent1.setDepth(3);
    ent1.setGeneration(0);
    ent1.setType(TType::T_EXACT);
    Move tmpMove;
    ent1.getMove(tmpMove);
    ASSERT_EQUAL(move, tmpMove);
    ASSERT_EQUAL(score, ent1.getScore(ply));
    ASSERT_EQUAL(score, ent1.getScore(ply + 3));    // Non-mate score, should be ply-independent

    // Test positive mate score
    TTEntry ent2;
    score = mate0 - 6;
    ply = 3;
    ent2.setKey(3);
    move = Move(8, 0, Piece::BQUEEN);
    ent2.setMove(move);
    ent2.setScore(score, ply);
    ent2.setDepth(99);
    ent2.setGeneration(0);
    ent2.setType(TType::T_EXACT);
    ent2.getMove(tmpMove);
    ASSERT_EQUAL(move, tmpMove);
    ASSERT_EQUAL(score, ent2.getScore(ply));
    ASSERT_EQUAL(score + 2, ent2.getScore(ply - 2));

    // Compare ent1 and ent2
    ASSERT(!ent1.replaceBetterThan(ent2, 0));  // More depth is good
    ASSERT(ent2.replaceBetterThan(ent1, 0));

    ent2.setGeneration(1);
    ASSERT(!ent2.replaceBetterThan(ent1, 0));  // ent2 has wrong generation
    ASSERT(ent2.replaceBetterThan(ent1, 1));   // ent1 has wrong generation

    ent2.setGeneration(0);
    ent1.setDepth(7); ent2.setDepth(7);
    ent1.setType(TType::T_GE);
    ASSERT(ent2.replaceBetterThan(ent1, 0));
    ent2.setType(TType::T_LE);
    ASSERT(!ent2.replaceBetterThan(ent1, 0));  // T_GE is equally good as T_LE
    ASSERT(!ent1.replaceBetterThan(ent2, 0));

    // Test negative mate score
    TTEntry ent3;
    ent3.clear();
    score = -mate0 + 5;
    ply = 3;
    ent3.setKey(3);
    move = Move(8, 0, Piece::BQUEEN);
    ent3.setMove(move);
    ent3.setScore(score, ply);
    ent3.setDepth(99);
    ent3.setGeneration(0);
    ent3.setType(TType::T_EXACT);
    ent3.getMove(tmpMove);
    ASSERT_EQUAL(move, tmpMove);
    ASSERT_EQUAL(score, ent3.getScore(ply));
    ASSERT_EQUAL(score - 2, ent3.getScore(ply - 2));
}

static void
testTTEntryValueGE() {
    {
        TTEntry ent1;
        ent1.clear();
        ent1.setDepth(5);
        ent1.setScore(10, 0);
        ent1.setType(TType::T_EXACT);
        ASSERT(ent1.valueGE(ent1));

        TTEntry ent2;
        ent2.clear();
        ent2.setDepth(7);
        ent2.setScore(8, 0);
        ent2.setType(TType::T_EXACT);
        ASSERT(ent2.valueGE(ent2));

        // ent2 has better depth, equally valuable score
        ASSERT(!ent1.valueGE(ent2));
        ASSERT(ent2.valueGE(ent1));

        ent2.setType(TType::T_GE);
        // ent2 has better depth but worse score
        ASSERT(!ent1.valueGE(ent2));
        ASSERT(!ent2.valueGE(ent1));

        ent1.setType(TType::T_GE);
        ent1.setScore(6, 0);
        // ent2 has better depth and better score
        ASSERT(!ent1.valueGE(ent2));
        ASSERT(ent2.valueGE(ent1));

        ent2.setDepth(5);
        ent1.setType(TType::T_EXACT);
        ent2.setType(TType::T_EXACT);
        // equal depth and equally good (but different) score
        ASSERT(ent1.valueGE(ent2));
        ASSERT(ent2.valueGE(ent1));
    }

    {
        TTEntry ent1;
        ent1.clear();
        ent1.setDepth(5);
        ent1.setScore(10, 0);
        ent1.setType(TType::T_GE);

        TTEntry ent2;
        ent2.clear();
        ent2.setDepth(5);
        ent2.setScore(8, 0);
        ent2.setType(TType::T_LE);

        // Different score type, not comparable
        ASSERT(!ent1.valueGE(ent2));
        ASSERT(!ent2.valueGE(ent1));

        ent2.setType(TType::T_GE);
        ent2.setScore(10, 0);
        ASSERT(ent1.valueGE(ent2));
        ASSERT(ent2.valueGE(ent1));

        ent2.setScore(9, 0);
        ASSERT(ent1.valueGE(ent2));
        ASSERT(!ent2.valueGE(ent1));

        ent2.setScore(11, 0);
        ASSERT(!ent1.valueGE(ent2));
        ASSERT(ent2.valueGE(ent1));

        ent1.setType(TType::T_LE);
        ent2.setType(TType::T_LE);
        ASSERT(ent1.valueGE(ent2));
        ASSERT(!ent2.valueGE(ent1));

        ent1.setScore(12, 0);
        ASSERT(!ent1.valueGE(ent2));
        ASSERT(ent2.valueGE(ent1));
    }

    static_assert(TType::T_EXACT + 1 == TType::T_GE, "broken test");
    static_assert(TType::T_GE + 1 == TType::T_LE, "broken test");
    for (int d1 = 5; d1 <= 15; d1 += 5) {
        for (int t1 = TType::T_EXACT; t1 <= TType::T_LE; t1++) {
            for (int s1 = -5; s1 <= 5; s1 += 5) {
                TTEntry ent1;
                ent1.clear();
                ent1.setDepth(d1);
                ent1.setType(t1);
                ent1.setScore(s1, 0);
                ASSERT(ent1.valueGE(ent1));

                for (int d2 = 5; d2 <= 15; d2 += 5) {
                    for (int t2 = TType::T_EXACT; t2 <= TType::T_LE; t2++) {
                        for (int s2 = -5; s2 <= 5; s2 += 5) {
                            TTEntry ent2;
                            ent2.clear();
                            ent2.setDepth(d2);
                            ent2.setType(t2);
                            ent2.setScore(s2, 0);
                            ASSERT(ent2.valueGE(ent2));

                            if (ent1.valueGE(ent2) && ent2.valueGE(ent1)) {
                                // ent1 >= ent2 && ent2 >= ent1 => same depth and score type
                                ASSERT_EQUAL(ent1.getDepth(), ent2.getDepth());
                                ASSERT_EQUAL(ent1.getType(), ent2.getType());
                                if (ent1.getType() != TType::T_EXACT)
                                    ASSERT_EQUAL(ent1.getScore(0), ent2.getScore(0));
                            }

                            for (int d3 = 5; d3 <= 15; d3 += 5) {
                                for (int t3 = TType::T_EXACT; t3 <= TType::T_LE; t3++) {
                                    for (int s3 = -5; s3 <= 5; s3 += 5) {
                                        TTEntry ent3;
                                        ent3.clear();
                                        ent3.setDepth(d3);
                                        ent3.setType(t3);
                                        ent3.setScore(s3, 0);
                                        ASSERT(ent3.valueGE(ent3));

                                        // Test transitivity
                                        if (ent1.valueGE(ent2) && ent2.valueGE(ent3))
                                            ASSERT(ent1.valueGE(ent3));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Test of insert method, of class TranspositionTable.
 */
static void
testInsert() {
    TranspositionTable tt(16);
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::string moves[] = {
        "e4", "e5", "Nf3", "Nc6", "Bb5", "a6", "Ba4", "b5", "Bb3", "Nf6", "O-O", "Be7", "Re1"
    };
    UndoInfo ui;
    for (size_t i = 0; i < COUNT_OF(moves); i++) {
        Move m = TextIO::stringToMove(pos, moves[i]);
        pos.makeMove(m, ui);
        int score = i * 17 + 3;
        m.setScore(score);
        int type = TType::T_EXACT;
        int ply = i + 1;
        int depth = (i * 2 + 5) * SearchConst::plyScale;
        tt.insert(pos.historyHash(), m, type, ply, depth, score * 2 + 3);
    }

    pos = TextIO::readFEN(TextIO::startPosFEN);
    for (size_t i = 0; i < COUNT_OF(moves); i++) {
        Move m = TextIO::stringToMove(pos, moves[i]);
        pos.makeMove(m, ui);
        TranspositionTable::TTEntry ent;
        ent.clear();
        const int mate0 = SearchConst::MATE0;
        tt.probe(pos.historyHash(), -mate0, mate0, 0, 0, ent);
        ASSERT_EQUAL(TType::T_EXACT, ent.getType());
        int score = i * 17 + 3;
        int ply = i + 1;
        int depth = (i * 2 + 5) * SearchConst::plyScale;
        ASSERT_EQUAL(score, ent.getScore(ply));
        ASSERT_EQUAL(depth, ent.getDepth());
        ASSERT_EQUAL(score * 2 + 3, ent.getEvalScore());
        Move tmpMove;
        ent.getMove(tmpMove);
        ASSERT_EQUAL(m, tmpMove);
    }
}

/**
 * Test special depth logic for mate scores.
 */
static void
testMateDepth() {
    TranspositionTable& tt(SearchTest::tt);
    Position pos = TextIO::readFEN("rnbqkbnr/pppp1ppp/8/4p3/8/5P1P/PPPPP1P1/RNBQKBNR b KQkq - 0 2");
    Search sc(pos, SearchTest::nullHist, 0, SearchTest::st, SearchTest::pd,
              nullptr, SearchTest::treeLog);
    Move m = SearchTest::idSearch(sc, 2, 100);
    ASSERT_EQUAL("d8h4", TextIO::moveToUCIString(m));
    UndoInfo ui;
    pos.makeMove(m, ui);

    TranspositionTable::TTEntry ent;
    ent.clear();
    const int mate0 = SearchConst::MATE0;
    const int ply = 5;
    tt.probe(pos.historyHash(), -mate0, mate0, ply, 0, ent);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(-(mate0 - 3 - ply), ent.getScore(ply));
    int plyScale = SearchConst::plyScale;
    ASSERT_EQUAL(1*plyScale, ent.getDepth());
    ASSERT(ent.isCutOff(-mate0, mate0, ply, 1*plyScale));
    ASSERT(!ent.isCutOff(-mate0, mate0, ply, 2*plyScale));

    ent.setDepth(2*plyScale);
    ASSERT(ent.isCutOff(-mate0, mate0, ply, 2*plyScale));
    ASSERT(!ent.isCutOff(-mate0, mate0, ply, 3*plyScale));

    ent.setDepth(3*plyScale);
    ASSERT(ent.isCutOff(-mate0, mate0, ply, 3*plyScale));
    ASSERT(ent.isCutOff(-mate0, mate0, ply, 4*plyScale));
}

/**
 * Test storing more than one entry for a single position.
 */
static void
testMultiInsert() {
    TranspositionTable tt(16);
    U64 key1 = 1;
    U64 key2 = (1ULL<<63) + 1;
    Move m = TextIO::uciStringToMove("e2e4");

    // No entry better than the other, both should be stored
    const int ply = 17;
    m.setScore(20);
    tt.insert(key1, m, TType::T_GE, ply, 2, 0);
    m.setScore(10);
    tt.insert(key1, m, TType::T_GE, ply, 3, 0);

    // Neither entry causes a cutoff, highest depth entry should be returned
    TTEntry ent;
    tt.probe(key1, 0, 30, ply, 2, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(3, ent.getDepth());
    ASSERT_EQUAL(10, ent.getScore(ply));

    // Only one entry causes a cutoff, make sure that entry is returned
    tt.probe(key1, 0, 15, ply, 2, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(2, ent.getDepth());
    ASSERT_EQUAL(20, ent.getScore(ply));

    // Only one entry causes a cutoff, make sure that entry is returned
    tt.probe(key1, 0, 5, ply, 3, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(3, ent.getDepth());
    ASSERT_EQUAL(10, ent.getScore(ply));

    // Neither entry causes a cutoff, highest depth entry should be returned
    tt.probe(key1, 0, 15, ply, 3, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(3, ent.getDepth());
    ASSERT_EQUAL(10, ent.getScore(ply));

    tt.probe(key2, 0, 15, ply, 3, ent);
    ASSERT_EQUAL(TType::T_EMPTY, ent.getType());

    // both entries causes a cutoff, highest depth entry should be returned
    tt.probe(key1, 0, 5, ply, 2, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(3, ent.getDepth());
    ASSERT_EQUAL(10, ent.getScore(ply));

    // Should replace the least valuable entry
    m.setScore(40);
    tt.insert(key2, m, TType::T_LE, ply, 7, 0);

    tt.probe(key1, -30, 30, ply, 4, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(3, ent.getDepth());
    ASSERT_EQUAL(10, ent.getScore(ply));

    // (5,20) > (3,10), replace old entry
    m.setScore(20);
    tt.insert(key1, m, TType::T_GE, ply, 5, 0);
    tt.probe(key1, -30, 30, ply, 1, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(5, ent.getDepth());
    ASSERT_EQUAL(20, ent.getScore(ply));
    tt.probe(key2, -30, 30, ply, 1, ent);
    ASSERT_EQUAL(TType::T_LE, ent.getType());

    // (8,10) not comparable to (5,20), but key2 entry has higher depth, so keep it
    m.setScore(10);
    tt.insert(key1, m, TType::T_GE, ply, 8, 0);
    tt.probe(key2, -30, 30, ply, 1, ent);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    tt.probe(key1, 0, 15, ply, 1, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(8, ent.getDepth());
    ASSERT_EQUAL(10, ent.getScore(ply));

    // (10, 8) not comparable to (8,10), replace key2 entry
    m.setScore(8);
    tt.insert(key1, m, TType::T_GE, ply, 10, 0);
    tt.probe(key1, 0, 9, ply, 6, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(8, ent.getDepth());
    ASSERT_EQUAL(10, ent.getScore(ply));
    tt.probe(key1, 0, 9, ply, 14, ent);
    ASSERT_EQUAL(TType::T_GE, ent.getType());
    ASSERT_EQUAL(10, ent.getDepth());
    ASSERT_EQUAL(8, ent.getScore(ply));
    tt.probe(key2, -30, 30, ply, 1, ent);
    ASSERT_EQUAL(TType::T_EMPTY, ent.getType());
}

cute::suite
TranspositionTableTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testTTEntry));
    s.push_back(CUTE(testTTEntryValueGE));
    s.push_back(CUTE(testInsert));
    s.push_back(CUTE(testMateDepth));
    s.push_back(CUTE(testMultiInsert));
    return s;
}
