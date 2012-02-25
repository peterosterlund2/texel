/*
 * killerTable.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef KILLERTABLE_HPP_
#define KILLERTABLE_HPP_

/**
 * Implement a table of killer moves for the killer heuristic.
 */
#if 0
public class KillerTable {
    /** There is one KTEntry for each ply in the search tree. */
    static final class KTEntry {
        public KTEntry() {
            move0 = move1 = 0;
        }
        int move0;
        int move1;
    }
    KTEntry[] ktList;

    /** Create an empty killer table. */
    public KillerTable() {
        ktList = new KTEntry[200];
        for (int i = 0; i < ktList.length; i++)
            ktList[i] = new KTEntry();
    }

    /** Add a killer move to the table. Moves are replaced on an LRU basis. */
    final public void addKiller(int ply, Move m) {
        if (ply >= ktList.length)
            return;
        int move = (short)(m.from + (m.to << 6) + (m.promoteTo << 12));
        KTEntry ent = ktList[ply];
        if (move != ent.move0) {
            ent.move1 = ent.move0;
            ent.move0 = move;
        }
    }

    /**
     * Get a score for move m based on hits in the killer table.
     * The score is 4 for primary   hit at ply.
     * The score is 3 for secondary hit at ply.
     * The score is 2 for primary   hit at ply - 2.
     * The score is 1 for secondary hit at ply - 2.
     * The score is 0 otherwise.
     */
    final public int getKillerScore(int ply, Move m) {
        int move = (short)(m.from + (m.to << 6) + (m.promoteTo << 12));
        if (ply < ktList.length) {
            KTEntry ent = ktList[ply];
            if (move == ent.move0) {
                return 4;
            } else if (move == ent.move1) {
                return 3;
            }
        }
        if ((ply - 2 >= 0) && (ply - 2 < ktList.length)) {
            KTEntry ent = ktList[ply - 2];
            if (move == ent.move0) {
                return 2;
            } else if (move == ent.move1) {
                return 1;
            }
        }
        return 0;
    }
};
#endif



#endif /* KILLERTABLE_HPP_ */
