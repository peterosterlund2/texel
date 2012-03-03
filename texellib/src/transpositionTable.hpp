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
        U64 key;                 // Zobrist hash key
    private:
        short move;              // from + (to<<6) + (promote<<12)
        short score;             // Score from search
        unsigned short depthSlot;// Search depth (bit 0-14) and hash slot (bit 15).
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

        /** Return true if entry is good enough to spend extra time trying to avoid overwriting it. */
        bool valuable(int currGen) const {
            if (generation != currGen)
                return false;
            return (type == TType::T_EXACT) || (getDepth() > 3 * SearchConst::plyScale);
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
            return depthSlot & 0x7fff;
        }

        /** Set depth. */
        void setDepth(int d) {
            depthSlot &= 0x8000;
            depthSlot |= ((short)d) & 0x7fff;
        }

        int getHashSlot() const {
            return depthSlot >> 15;
        }

        void setHashSlot(int s) {
            depthSlot &= 0x7fff;
            depthSlot |= (s << 15);
        }
    };
    std::vector<TTEntry> table;
    TTEntry emptySlot;
    byte generation;

public:
    /** Constructor. Creates an empty transposition table with numEntries slots. */
    TranspositionTable(int log2Size) {
        reSize(log2Size);
    }

    void reSize(int log2Size) {
        const int numEntries = (1 << log2Size);
        table.resize(numEntries);
        for (int i = 0; i < numEntries; i++) {
            TTEntry& ent = table[i];
            ent.key = 0;
            ent.depthSlot = 0;
            ent.type = TType::T_EMPTY;
        }
        emptySlot.type = TType::T_EMPTY;
        generation = 0;
    }

    /** Insert an entry in the hash table. */
    void insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore);

    /** Retrieve an entry from the hash table corresponding to "pos". */
    void probe(U64 key, TTEntry& result) {
        int idx0 = h0(key);
        TTEntry& ent = table[idx0];
        if (ent.key == key) {
            ent.generation = (byte)generation;
            result = ent;
            return;
        }
        int idx1 = h1(key);
        TTEntry& ent2 = table[idx1];
        if (ent2.key == key) {
            ent2.generation = (byte)generation;
            result = ent2;
            return;
        }
        result = emptySlot;
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
        for (int i = 0; i < (int)table.size(); i++)
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
    int h0(U64 key) const;
    int h1(U64 key) const;
};

inline int
TranspositionTable::h0(U64 key) const {
    return (int)(key & (table.size() - 1));
}

inline int
TranspositionTable::h1(U64 key) const {
    return (int)((key >> 32) & (table.size() - 1));
}

#endif /* TRANSPOSITIONTABLE_HPP_ */
