/*
 * transpositionTableTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "transpositionTableTest.hpp"

#include "cute.h"

#if 0
    /**
     * Test of TTEntry nested class, of class TranspositionTable.
     */
    public void testTTEntry() {
        final int mate0 = Search.MATE0;
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        Move move = TextIO::stringToMove(pos, "e4");

        // Test "normal" (non-mate) score
        int score = 17;
        int ply = 3;
        TTEntry ent1 = new TTEntry();
        ent1.key = 1;
        ent1.setMove(move);
        ent1.setScore(score, ply);
        ent1.setDepth(3);
        ent1.generation = 0;
        ent1.type = TranspositionTable.TTEntry.T_EXACT;
        ent1.setHashSlot(0);
        Move tmpMove = new Move(0,0,0);
        ent1.getMove(tmpMove);
        ASSERT_EQUAL(move, tmpMove);
        ASSERT_EQUAL(score, ent1.getScore(ply));
        ASSERT_EQUAL(score, ent1.getScore(ply + 3));    // Non-mate score, should be ply-independent

        // Test positive mate score
        TTEntry ent2 = new TTEntry();
        score = mate0 - 6;
        ply = 3;
        ent2.key = 3;
        move = new Move(8, 0, Piece::BQUEEN);
        ent2.setMove(move);
        ent2.setScore(score, ply);
        ent2.setDepth(99);
        ent2.generation = 0;
        ent2.type = TranspositionTable.TTEntry.T_EXACT;
        ent2.setHashSlot(0);
        ent2.getMove(tmpMove);
        ASSERT_EQUAL(move, tmpMove);
        ASSERT_EQUAL(score, ent2.getScore(ply));
        ASSERT_EQUAL(score + 2, ent2.getScore(ply - 2));
        
        // Compare ent1 and ent2
        ASSERT(!ent1.betterThan(ent2, 0));  // More depth is good
        ASSERT(ent2.betterThan(ent1, 0));

        ent2.generation = 1;
        ASSERT(!ent2.betterThan(ent1, 0));  // ent2 has wrong generation
        ASSERT(ent2.betterThan(ent1, 1));  // ent1 has wrong generation

        ent2.generation = 0;
        ent1.setDepth(7); ent2.setDepth(7);
        ent1.type = TranspositionTable.TTEntry.T_GE;
        ASSERT(ent2.betterThan(ent1, 0));
        ent2.type = TranspositionTable.TTEntry.T_LE;
        ASSERT(!ent2.betterThan(ent1, 0));  // T_GE is equally good as T_LE
        ASSERT(!ent1.betterThan(ent2, 0));
        
        // Test negative mate score
        TTEntry ent3 = new TTEntry();
        score = -mate0 + 5;
        ply = 3;
        ent3.key = 3;
        move = new Move(8, 0, Piece::BQUEEN);
        ent3.setMove(move);
        ent3.setScore(score, ply);
        ent3.setDepth(99);
        ent3.generation = 0;
        ent3.type = TranspositionTable.TTEntry.T_EXACT;
        ent3.setHashSlot(0);
        ent3.getMove(tmpMove);
        ASSERT_EQUAL(move, tmpMove);
        ASSERT_EQUAL(score, ent3.getScore(ply));
        ASSERT_EQUAL(score - 2, ent3.getScore(ply - 2));
    }
    
    /**
     * Test of insert method, of class TranspositionTable.
     */
    public void testInsert() {
        TranspositionTable tt = new TranspositionTable(16);
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
	std::string[] moves = {
            "e4", "e5", "Nf3", "Nc6", "Bb5", "a6", "Ba4", "b5", "Bb3", "Nf6", "O-O", "Be7", "Re1"
        };
        UndoInfo ui = new UndoInfo();
        for (int i = 0; i < moves.length; i++) {
            Move m = TextIO::stringToMove(pos, moves[i]);
            pos.makeMove(m, ui);
            int score = i * 17 + 3;
            m.score = score;
            int type = TranspositionTable.TTEntry.T_EXACT;
            int ply = i + 1;
            int depth = i * 2 + 5;
            tt.insert(pos.historyHash(), m, type, ply, depth, score * 2 + 3);
        }

        pos = TextIO::readFEN(TextIO::startPosFEN);
        for (int i = 0; i < moves.length; i++) {
            Move m = TextIO::stringToMove(pos, moves[i]);
            pos.makeMove(m, ui);
            TranspositionTable.TTEntry ent = tt.probe(pos.historyHash());
            ASSERT_EQUAL(TranspositionTable.TTEntry.T_EXACT, ent.type);
            int score = i * 17 + 3;
            int ply = i + 1;
            int depth = i * 2 + 5;
            ASSERT_EQUAL(score, ent.getScore(ply));
            ASSERT_EQUAL(depth, ent.getDepth());
            ASSERT_EQUAL(score * 2 + 3, ent.evalScore);
            Move tmpMove = new Move(0,0,0);
            ent.getMove(tmpMove);
            ASSERT_EQUAL(m, tmpMove);
        }
    }
#endif

cute::suite
TranspositionTableTest::getSuite() const {
    cute::suite s;
#if 0
    s.push_back(CUTE(testTTEntry));
    s.push_back(CUTE(testInsert));
#endif
    return s;
}
