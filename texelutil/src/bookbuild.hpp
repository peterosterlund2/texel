/*
 * bookbuild.hpp
 *
 *  Created on: Apr 27, 2014
 *      Author: petero
 */

#ifndef BOOKBUILD_HPP_
#define BOOKBUILD_HPP_

#include "move.hpp"
#include "position.hpp"
#include "treeLogger.hpp" // Serializer namespace

#include <memory>
#include <vector>
#include <map>
#include <climits>

namespace BookBuild {

// Used in book nodes when negamax scores have not yet been computed
const int INVALID_SCORE = -SearchConst::MATE0 * 2;



/** Bonus per move for staying in the opening book. */
const int BOOK_LENGTH_BONUS = 2;

/** Represents a book position, its connections to parent/child book positions,
 *  and information about the best non-book move. */
class BookNode {
public:
    /** Create an empty node. */
    BookNode();

    /** Return book hash key. */
    U64 getHashKey() const;

    /** Get "book score" corresponding to searchScore, from white's perspective. */
    int bookScoreW() const;

    /** Get "book score" corresponding to searchScore, from black's perspective. */
    int bookScoreB() const;

    struct BookSerializeData {
        U8 data[16];
    };

    /** Serialize/deserialize object. */
    void serialize(BookSerializeData& bsd) const;
    void deSerialize(const BookSerializeData& bsd);

    /** Add a parent/child relationship. */
    void addChild(U16 move, const std::shared_ptr<BookNode>& child);
    void addParent(U16 move, const std::shared_ptr<BookNode>& parent);


    enum State {
        EMPTY,             // Newly constructed, node contains no useful data
        DESERIALIZED,      // Deserialized but non-serialized data not initialized
        INITIALIZED,       // All data initialized, consistency not yet analyzed
        RECOMPUTE,         // Best non-book move needs to be recomputed. Reasons can be:
                           // 1. searchTime too small compared to child nodes
                           // 2. bestNonBookMove leads to a child node
                           // 3. nChildren > 0 and searchScore > best child score
        CONSISTENT         // Node fully consistent with all children and parents
                           // Maybe not consistent with all ancestors/descendants
    };

    State getState() const;
    void setState(State s);

    /** Recursively initialize negamax scores of self and all children. */
    void initScores();


private:
    /** Update depth of this node and all descendants. */
    void updateDepth();


    U64 hashKey;
    int depth;             // Length of shortest path to the root node

    Move bestNonBookMove;  // Best non-book move. Empty if all legal moves are
                           // included in the book.
    S16 searchScore;       // Score for best non-book move. -MATE0 or 0 (stalemate) if no non-book move.
    U32 searchTime;        // Time in milliseconds spent on computing searchScore and bestNonBookMove

    int negaMaxScore;      // Best score in this position. max(searchScore, -child_i(pos))
    int negaMaxBookScoreW; // Best book score for white, max(bookScoreW(), -child_i.bookScoreW())
    int negaMaxBookScoreB; // Best book score for black, max(bookScoreB(), -child_i.bookScoreB())

    std::map<U16, std::shared_ptr<BookNode>> children;   // Compressed move -> BookNode
    std::multimap<U16, std::weak_ptr<BookNode>> parents; // Compressed move -> BookNode
    State state;
};

class Book {
public:
    /** Constructor. Create an empty book. */
    Book();

    /** Read opening book from file. */
    void readFromFile(const std::string& filename);

    /** Write opening book to file. */
    void writeToFile(const std::string& filename);

private:
    /** Initialize parent/child relations in all book nodes
     *  by following legal moves from pos. */
    void initPositions(Position& pos);

    /** Find inconsistencies in the book and correct them.
     * Return when there are no remaining inconsistencies. */
    void makeConsistent();

    /** Extend the opening book by one move for white or black. */
    void extend(bool white);

    /** All positions in the opening book. */
    std::map<U64, std::shared_ptr<BookNode>> bookNodes;

    // Map from position hash code to all parent book positions.
    std::multimap<U64, std::shared_ptr<BookNode>> hashToParent;
};


inline
BookNode::BookNode()
    : hashKey(0), depth(INT_MAX), searchScore(0), searchTime(0),
      negaMaxScore(INVALID_SCORE),
      negaMaxBookScoreW(INVALID_SCORE),
      negaMaxBookScoreB(INVALID_SCORE),
      state(BookNode::EMPTY) {
}


inline U64
BookNode::getHashKey() const {
    return hashKey;
}

inline int
BookNode::bookScoreW() const {
    bool wtm = depth % 2 == 0;
    int fullMoveCounter = depth / 2;
    return searchScore + fullMoveCounter * BOOK_LENGTH_BONUS * (wtm ? 1 : -1);
}

inline int
BookNode::bookScoreB() const {
    bool wtm = depth % 2 == 0;
    int fullMoveCounter = depth / 2;
    return searchScore - fullMoveCounter * BOOK_LENGTH_BONUS * (wtm ? 1 : -1);
}

inline void
BookNode::serialize(BookSerializeData& bsd) const {
    U16 move = bestNonBookMove.getCompressedMove();
    Serializer::serialize<sizeof(bsd.data)>(bsd.data, hashKey, move, searchScore, searchTime);
}

inline void
BookNode::deSerialize(const BookSerializeData& bsd) {
    U16 move;
    Serializer::deSerialize<sizeof(bsd.data)>(bsd.data, hashKey, move, searchScore, searchTime);
    bestNonBookMove.setFromCompressed(move);
    state = DESERIALIZED;
}

inline void
BookNode::addChild(U16 move, const std::shared_ptr<BookNode>& child) {
    children.insert(std::make_pair(move, child));
}

inline void
BookNode::addParent(U16 move, const std::shared_ptr<BookNode>& parent) {
    parents.insert(std::make_pair(move, parent));
    updateDepth();
}

inline BookNode::State
BookNode::getState() const {
    return state;
}

inline void
BookNode::setState(State s) {
    state = s;
}

} // Namespace BookBuild

#endif /* BOOKBUILD_HPP_ */
