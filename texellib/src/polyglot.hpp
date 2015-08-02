/*
 * polyglot.hpp
 *
 *  Created on: Jul 5, 2015
 *      Author: petero
 */

#ifndef POLYGLOT_HPP_
#define POLYGLOT_HPP_

#include "position.hpp"

/**
 * Utility methods for handling of polyglot book entries.
 */
class PolyglotBook {
public:
    /** Compute a polyglot hash key corresponding to a position. */
    static U64 getHashKey(const Position& pos);

    /** Compute polyglot move value from a Move object. */
    static U16 getPGMove(const Position& pos, const Move& move);

    /** Compute move corresponding to a polyglot move value. */
    static Move getMove(const Position& pos, U16 move);

    struct PGEntry {
        U8 data[16];
    };

    /** Store book information in a PGEntry object. */
    static void serialize(U64 hash, U16 move, U16 weight, PGEntry& ent);

    /** Retrieve book information from a PGEntry object. */
    static void deSerialize(const PGEntry& ent, U64& hash, U16& move, U16& weight);

private:
    static U64 hashRandoms[];
};

#endif /* POLYGLOT_HPP_ */
