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
    @Test
    public void testTTEntry() throws ChessParseError {
        System.out.println("TTEntry");
        final int mate0 = Search.MATE0;
        Position pos = TextIO.readFEN(TextIO.startPosFEN);
        Move move = TextIO.stringToMove(pos, "e4");

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
        assertEquals(move, tmpMove);
        assertEquals(score, ent1.getScore(ply));
        assertEquals(score, ent1.getScore(ply + 3));    // Non-mate score, should be ply-independent

        // Test positive mate score
        TTEntry ent2 = new TTEntry();
        score = mate0 - 6;
        ply = 3;
        ent2.key = 3;
        move = new Move(8, 0, Piece.BQUEEN);
        ent2.setMove(move);
        ent2.setScore(score, ply);
        ent2.setDepth(99);
        ent2.generation = 0;
        ent2.type = TranspositionTable.TTEntry.T_EXACT;
        ent2.setHashSlot(0);
        ent2.getMove(tmpMove);
        assertEquals(move, tmpMove);
        assertEquals(score, ent2.getScore(ply));
        assertEquals(score + 2, ent2.getScore(ply - 2));
        
        // Compare ent1 and ent2
        assertTrue(!ent1.betterThan(ent2, 0));  // More depth is good
        assertTrue(ent2.betterThan(ent1, 0));

        ent2.generation = 1;
        assertTrue(!ent2.betterThan(ent1, 0));  // ent2 has wrong generation
        assertTrue(ent2.betterThan(ent1, 1));  // ent1 has wrong generation

        ent2.generation = 0;
        ent1.setDepth(7); ent2.setDepth(7);
        ent1.type = TranspositionTable.TTEntry.T_GE;
        assertTrue(ent2.betterThan(ent1, 0));
        ent2.type = TranspositionTable.TTEntry.T_LE;
        assertTrue(!ent2.betterThan(ent1, 0));  // T_GE is equally good as T_LE
        assertTrue(!ent1.betterThan(ent2, 0));
        
        // Test negative mate score
        TTEntry ent3 = new TTEntry();
        score = -mate0 + 5;
        ply = 3;
        ent3.key = 3;
        move = new Move(8, 0, Piece.BQUEEN);
        ent3.setMove(move);
        ent3.setScore(score, ply);
        ent3.setDepth(99);
        ent3.generation = 0;
        ent3.type = TranspositionTable.TTEntry.T_EXACT;
        ent3.setHashSlot(0);
        ent3.getMove(tmpMove);
        assertEquals(move, tmpMove);
        assertEquals(score, ent3.getScore(ply));
        assertEquals(score - 2, ent3.getScore(ply - 2));
    }
    
    /**
     * Test of insert method, of class TranspositionTable.
     */
    @Test
    public void testInsert() throws ChessParseError {
        System.out.println("insert");
        TranspositionTable tt = new TranspositionTable(16);
        Position pos = TextIO.readFEN(TextIO.startPosFEN);
        String[] moves = {
            "e4", "e5", "Nf3", "Nc6", "Bb5", "a6", "Ba4", "b5", "Bb3", "Nf6", "O-O", "Be7", "Re1"
        };
        UndoInfo ui = new UndoInfo();
        for (int i = 0; i < moves.length; i++) {
            Move m = TextIO.stringToMove(pos, moves[i]);
            pos.makeMove(m, ui);
            int score = i * 17 + 3;
            m.score = score;
            int type = TranspositionTable.TTEntry.T_EXACT;
            int ply = i + 1;
            int depth = i * 2 + 5;
            tt.insert(pos.historyHash(), m, type, ply, depth, score * 2 + 3);
        }

        pos = TextIO.readFEN(TextIO.startPosFEN);
        for (int i = 0; i < moves.length; i++) {
            Move m = TextIO.stringToMove(pos, moves[i]);
            pos.makeMove(m, ui);
            TranspositionTable.TTEntry ent = tt.probe(pos.historyHash());
            assertEquals(TranspositionTable.TTEntry.T_EXACT, ent.type);
            int score = i * 17 + 3;
            int ply = i + 1;
            int depth = i * 2 + 5;
            assertEquals(score, ent.getScore(ply));
            assertEquals(depth, ent.getDepth());
            assertEquals(score * 2 + 3, ent.evalScore);
            Move tmpMove = new Move(0,0,0);
            ent.getMove(tmpMove);
            assertEquals(m, tmpMove);
        }
    }
#endif

cute::suite
TranspositionTableTest::getSuite() const {
    cute::suite s;
//    s.push_back(CUTE());
    return s;
}
