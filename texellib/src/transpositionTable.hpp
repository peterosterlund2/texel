/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * transpositionTable.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TRANSPOSITIONTABLE_HPP_
#define TRANSPOSITIONTABLE_HPP_

#include "util.hpp"
#include "move.hpp"
#include "constants.hpp"

#include <vector>

class Position;

/**
 * Implements the main transposition table using Cuckoo hashing.
 */
class TranspositionTable {
public:
    class TTEntry {
        friend class TranspositionTable;
    public:
        int key;                 // Zobrist hash key
    private:
        short move;              // from + (to<<6) + (promote<<12)
        short score;             // Score from search
        unsigned short depth;    // Search depth
    public:
        byte generation;         // Increase when OTB position changes
        byte type;               // exact score, lower bound, upper bound
        short evalScore;         // Score from static evaluation
        // FIXME!!! Test storing both upper and lower bound in each hash entry.

        /** Return true if this object is more valuable than the other, false otherwise. */
        bool betterThan(const TTEntry& other, int currGen) const {
            if ((generation == currGen) != (other.generation == currGen))
                return generation == currGen;   // Old entries are less valuable
            if ((type == TType::T_EXACT) != (other.type == TType::T_EXACT))
                return type == TType::T_EXACT;         // Exact score more valuable than lower/upper bound
            if (getDepth() != other.getDepth())
                return getDepth() > other.getDepth();     // Larger depth is more valuable
            return false;   // Otherwise, pretty much equally valuable
        }

        void getMove(Move& m) const {
            m.setMove(move & 63, (move >> 6) & 63, (move >> 12) & 15, m.score());
        }

        void setMove(const Move& move) {
            this->move = (short)(move.from() + (move.to() << 6) + (move.promoteTo() << 12));
        }

        /** Get the score from the hash entry, and convert from "mate in x" to "mate at ply". */
        int getScore(int ply) const {
            int sc = score;
            if (sc > SearchConst::MATE0 - 1000) {
                sc -= ply;
            } else if (sc < -(SearchConst::MATE0 - 1000)) {
                sc += ply;
            }
            return sc;
        }

        /** Convert score from "mate at ply" to "mate in x", and store in hash entry. */
        void setScore(int score, int ply) {
            if (score > SearchConst::MATE0 - 1000)
                score += ply;
            else if (score < -(SearchConst::MATE0 - 1000))
                score -= ply;
            this->score = (short)score;
        }

        /** Get depth from the hash entry. */
        int getDepth() const {
            return depth;
        }

        /** Set depth. */
        void setDepth(int d) {
            depth = d;
        }
    };
    std::vector<TTEntry> table;
    byte generation;

public:
    /** Constructor. Creates an empty transposition table with numEntries slots. */
    TranspositionTable(int log2Size) {
        reSize(log2Size);
    }

    void reSize(int log2Size);

    /** Insert an entry in the hash table. */
    void insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore);

    /** Retrieve an entry from the hash table corresponding to position with zobrist key "key". */
    void probe(U64 key, TTEntry& result) {
        size_t idx0 = getIndex(key);
        int key2 = getStoredKey(key);
        TTEntry& ent = table[idx0];
        if (ent.key == key2) {
            ent.generation = (byte)generation;
            result = ent;
            return;
        }
        size_t idx1 = idx0 ^ 1;
        TTEntry& ent2 = table[idx1];
        if (ent2.key == key2) {
            ent2.generation = (byte)generation;
            result = ent2;
            return;
        }
        result.type = TType::T_EMPTY;
    }

    /**
     * Increase hash table generation. This means that subsequent inserts will be considered
     * more valuable than the entries currently present in the hash table.
     */
    void nextGeneration() {
        generation++;
    }

    /** Clear the transposition table. */
    void clear() {
        for (size_t i = 0; i < table.size(); i++)
            table[i].type = TType::T_EMPTY;
    }

    /**
     * Extract a list of PV moves, starting from "rootPos" and first move "mFirst".
     */
    void extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv);

    /** Extract the PV starting from posIn, using hash entries, both exact scores and bounds. */
    std::string extractPV(const Position& posIn);

    /** Print hash table statistics. */
    void printStats() const;

private:
    /** Get position in hash table given zobrist key. */
    size_t getIndex(U64 key) const;

    /** Get part of zobrist key to store in hash table. */
    static int getStoredKey(U64 key);
};

inline size_t
TranspositionTable::getIndex(U64 key) const {
    return (size_t)(key & (table.size() - 1));
}

inline int
TranspositionTable::getStoredKey(U64 key) {
    return (int)(key >> 32);
}

#endif /* TRANSPOSITIONTABLE_HPP_ */
