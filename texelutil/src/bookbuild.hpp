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
#include <deque>
#include <map>
#include <climits>

class BookBuildTest;


namespace BookBuild {

// Node is temporarily ignored because it is currently being searched
const int IGNORE_SCORE = SearchConst::UNKNOWN_SCORE + 1;

// Used in when no search score has been computed
const int INVALID_SCORE = SearchConst::UNKNOWN_SCORE + 2;


/** Bonus per move for staying in the opening book. */
const int BOOK_LENGTH_BONUS = 2;

/** Represents a book position, its connections to parent/child book positions,
 *  and information about the best non-book move. */
class BookNode {
public:
    /** Create an empty node. */
    BookNode(U64 hashKey, bool rootNode = false);

    BookNode(const BookNode& other) = delete;
    BookNode& operator=(const BookNode& other) = delete;

    /** Return book hash key. */
    U64 getHashKey() const;

    /** Return shortest distance to the root node. */
    int getDepth() const;

    /** Get "book score" corresponding to searchScore, from white's perspective.
     * The score is positive if good for the side to move, but the score is better
     * for white the longer the book line is. */
    int bookScoreW() const;

    /** Get "book score" corresponding to searchScore, from black's perspective. */
    int bookScoreB() const;

    /** Get negamax scores. */
    int getNegaMaxScore() const;
    int getNegaMaxBookScoreW() const;
    int getNegaMaxBookScoreB() const;

    struct BookSerializeData {
        U8 data[16];
    };

    /** Serialize/deserialize object. */
    void serialize(BookSerializeData& bsd) const;
    void deSerialize(const BookSerializeData& bsd);

    /** Add a parent/child relationship. */
    void addChild(U16 move, const std::shared_ptr<BookNode>& child);
    void addParent(U16 move, const std::shared_ptr<BookNode>& parent);

    /** Set search result data. */
    void setSearchResult(const std::set<U64>& pendingPositions,
                         const Move& bestMove, int score, int searchTime);

    enum State {
        EMPTY,             // Newly constructed, node contains no useful data
        DESERIALIZED,      // Deserialized but non-serialized data not initialized
        INITIALIZED,       // All data initialized, consistency not yet analyzed
    };

    State getState() const;
    void setState(State s);

    /** Recursively initialize negamax scores of this node and all children and parents. */
    void updateNegaMax(const std::set<U64>& pendingPositions,
                       bool updateThis = true, bool updateChildren = true, bool updateParents = true);

    /** Get all children. */
    const std::map<U16, std::shared_ptr<BookNode>>& getChildren() const { return children; }

    /** Get all parents. */
    const std::multimap<U16, std::weak_ptr<BookNode>>& getParents() const { return parents; }

    const Move& getBestNonBookMove() const;
    const S16 getSearchScore() const;
    const U32 getSearchTime() const;

private:
    /** Update depth of this node and all descendants. */
    void updateDepth();

    /** Negate a score for a child node to produce the corresponding score
     *  for the parent node. Also handles special scores, i.e. mate, invalid, ignore. */
    int negateScore(int score) const;

    /** Compute negaMax scores for this node assuming all child nodes are already up to date.
     * Return true if any score was modified. */
    bool computeNegaMax(const std::set<U64>& pendingPositions);


    U64 hashKey;
    int depth;             // Length of shortest path to the root node

    Move bestNonBookMove;  // Best non-book move. Empty if all legal moves are
                           // included in the book.
    S16 searchScore;       // Score for best non-book move.
                           // IGNORE_SCORE, -MATE0 or 0 (stalemate) if no non-book move.
    U32 searchTime;        // Time in milliseconds spent on computing searchScore and bestNonBookMove

    int negaMaxScore;      // Best score in this position. max(searchScore, -child_i(pos))
    int negaMaxBookScoreW; // Best book score for white, max(bookScoreW(), -child_i.bookScoreW())
    int negaMaxBookScoreB; // Best book score for black, max(bookScoreB(), -child_i.bookScoreB())

    std::map<U16, std::shared_ptr<BookNode>> children;   // Compressed move -> BookNode
    std::multimap<U16, std::weak_ptr<BookNode>> parents; // Compressed move -> BookNode
    State state;
};

/** Represents an opening book and methods that can improve the book
 *  by extension and engine analysis. */
class Book {
    friend class ::BookBuildTest;
public:
    /** Constructor. Create an empty book.
     * @param backupFile The backup path name. Empty string disables backup. */
    Book(const std::string& backupFile);

    Book(const Book& other) = delete;
    Book& operator=(const Book& other) = delete;

    /** Improve the opening book. If fen is a non-empty string, only improve the part
     * of the book rooted at that fen position.
     * This function does not return until no more book moves can be added, which in
     * practice never happens. */
    void improve(const std::string& bookFile, int searchTime, const std::string& fen);

    /** Add all moves from a PGN file to the book. */
    void importPGN(const std::string& bookFile, const std::string& pgnFile, int searchTime);

    /** Convert the book to polyglot format. */
    void exportPolyglot(const std::string& bookFile, const std::string& polyglotFile,
                        int maxPathError);

    /** Query the book interactively, taking query commands from standard input. */
    void interactiveQuery(const std::string& bookFile);

private:
    /** Add root node if not already present. */
    void addRootNode();

    /** Read opening book from file. */
    void readFromFile(const std::string& filename);

    /** Write opening book to file. */
    void writeToFile(const std::string& filename);

    class PositionSelector {
    public:
        virtual ~PositionSelector() {}
        /** Retrieve position and move which can be used to extend the opening book.
         * If move is empty, don't extend book, only search non-book moves in pos.
         * Return true if a position was retrieved, false otherwise. */
        virtual bool getNextPosition(Position& pos, Move& move) = 0;
    };

    /** Extend book using positions provided by the selector. */
    void extendBook(PositionSelector& selector, int searchTime);

    /** Get the list of legal moves to include in the search. */
    std::vector<Move> getMovesToSearch(Position& pos);

    /** Add/remove a position to the set of positions currently being searched. */
    void addPending(U64 hashKey);
    void removePending(U64 hashKey);

    /** Add the position resulting from playing "move" in position "pos" to the
     * book. The new position and its parent position(s) that need to be
     * re-searched are returned in the "toSearch" vector. "pos" must already be
     * in the book and "pos + move" must not already be in the book. */
    void addPosToBook(Position& pos, const Move& move, std::vector<U64>& toSearch);

    /**
     * Retrieve the position corresponding to a hash key. Only works for positions
     * that are included in the opening book. Returns true if the position could
     * be retrieved, false otherwise. "pos" may be modified even if the function
     * returns false.
     */
    bool getPosition(U64 hashKey, Position& pos, std::vector<Move>& moveList) const;

    /** Get the book node corresponding to a hash key.
     * Return null if there is no matching node in the book. */
    std::shared_ptr<BookNode> getBookNode(U64 hashKey) const;

    /** Initialize parent/child relations in all book nodes
     *  by following legal moves from pos. */
    void initPositions(Position& pos);

    /** Find all children of pos in book and update parent/child pointers. */
    void setChildRefs(Position& pos);

    /** Write a book node to the backup file. */
    void writeBackup(const BookNode& bookNode);


    /** Hash key corresponding to initial position. */
    const U64 startPosHash;

    /** Filename where all incremental improvements are stored.
     * The backup file is a valid book file at all times. */
    std::string backupFile;

    /** All positions in the opening book. */
    std::map<U64, std::shared_ptr<BookNode>> bookNodes;

    /** Map from position hash code to all parent book position hash codes. */
    std::set<std::pair<U64, U64>> hashToParent;

    /** Positions that are currently being searched. */
    std::set<U64> pendingPositions;
};

/** Calls Search::iterativeDeepening() to analyze a position. */
class SearchRunner {
public:
    /** Constructor. */
    SearchRunner(int instanceNo, TranspositionTable& tt);

    SearchRunner(const SearchRunner& other) = delete;
    SearchRunner& operator=(const SearchRunner& other) = delete;

    /** Analyze position and return the best move and score. */
    Move analyze(const std::vector<Move>& gameMoves,
                 const std::vector<Move>& movesToSearch,
                 int searchTime);

    int instNo() const { return instanceNo; }

private:
    int instanceNo;
    Evaluate::EvalHashTables et;
    KillerTable kt;
    History ht;
    TranspositionTable& tt;
    TreeLogger treeLog;
};

/** Handles work distribution to the search threads. */
class SearchScheduler {
public:
    /** Constructor. */
    SearchScheduler();

    /** Destructor. Waits for all threads to terminate. */
    ~SearchScheduler();

    /** Add a SearchRunner. */
    void addWorker(const std::shared_ptr<SearchRunner>& sr);

    /** Start the worker threads. Creates one thread for each SearchRunner object. */
    void startWorkers();

    /** Wait for currently running WorkUnits to finish and then stops all threads. */
    void stopWorkers();

    struct WorkUnit {
        // Input
        int id;
        U64 hashKey;
        std::vector<Move> gameMoves;     // Moves leading to the position to search
        std::vector<Move> movesToSearch; // Set of moves to consider in the search
        int searchTime;

        // Output
        Move bestMove;         // Best move and corresponding score
        int instNo;            // Instance number that ran this WorkUnit

        bool operator<(const WorkUnit& other) const { return id < other.id; }
    };

    /** Add a WorkUnit to the queue. */
    void addWorkUnit(const WorkUnit& wu);

    /** Wait until a result is ready and retrieve the corresponding WorkUnit. */
    void getResult(WorkUnit& wu);

    /** Report finished WorkUnit information to cout. */
    void reportResult(const WorkUnit& wu) const;

private:
    /** Worker thread main loop. */
    void workerLoop(SearchRunner& sr);

    bool stopped;
    std::mutex mutex;

    std::vector<std::shared_ptr<SearchRunner>> workers;
    std::vector<std::shared_ptr<std::thread>> threads;

    std::deque<WorkUnit> pending;
    std::condition_variable pendingCv;

    std::deque<WorkUnit> complete;
    std::condition_variable completeCv;
};

// ----------------------------------------------------------------------------

inline
BookNode::BookNode(U64 hashKey0, bool rootNode)
    : hashKey(hashKey0), depth(rootNode ? 0 : INT_MAX),
      searchScore(0), searchTime(0),
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
BookNode::getDepth() const {
    return depth;
}

inline int
BookNode::bookScoreW() const {
    if (searchScore == INVALID_SCORE)
        return INVALID_SCORE;
    bool wtm = depth % 2 == 0;
    int fullMoveCounter = (depth + 1) / 2;
    return searchScore + fullMoveCounter * BOOK_LENGTH_BONUS * (wtm ? 1 : -1);
}

inline int
BookNode::bookScoreB() const {
    if (searchScore == INVALID_SCORE)
        return INVALID_SCORE;
    bool wtm = depth % 2 == 0;
    int fullMoveCounter = depth / 2;
    return searchScore - fullMoveCounter * BOOK_LENGTH_BONUS * (wtm ? 1 : -1);
}

inline int
BookNode::getNegaMaxScore() const {
    return negaMaxScore;
}

inline int
BookNode::getNegaMaxBookScoreW() const {
    return negaMaxBookScoreW;
}

inline int
BookNode::getNegaMaxBookScoreB() const {
    return negaMaxBookScoreB;
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

inline const Move&
BookNode::getBestNonBookMove() const {
    return bestNonBookMove;
}

inline const S16
BookNode::getSearchScore() const {
    return searchScore;
}

inline const U32
BookNode::getSearchTime() const {
    return searchTime;
}


} // Namespace BookBuild

#endif /* BOOKBUILD_HPP_ */
