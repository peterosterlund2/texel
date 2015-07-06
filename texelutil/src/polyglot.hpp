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
    static U16 getMove(const Position& pos, const Move& move);

    struct PGEntry {
        U8 data[16];
    };

    /** Store book information in a PGEntry object. */
    static void serialize(U64 hash, U16 move, U16 weight, PGEntry& ent);

private:
    static U64 hashRandoms[];
};

#endif /* POLYGLOT_HPP_ */
