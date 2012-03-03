/*
 * book.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BOOK_HPP_
#define BOOK_HPP_

#include "move.hpp"
#include "util.hpp"
#include "random.hpp"

#include <map>
#include <vector>
#include <cmath>

class Position;

/**
 * Implements an opening book.
 */
class Book {
private:
    struct BookEntry {
        Move move;
        int count;
        BookEntry(const Move& m) : move(m), count(1) { }
    };

    typedef std::map<U64, std::vector<BookEntry> > BookMap;
    static BookMap bookMap;
    static Random rndGen;
    static int numBookMoves;
    bool verbose;

    static const char* bookLines[];

public:
    Book(bool verbose) {
        this->verbose = verbose;
    }

    /** Return a random book move for a position, or empty move if out of book. */
    void getBookMove(Position& pos, Move& out);

    /** Return a string describing all book moves. */
    std::string getAllBookMoves(const Position& pos);

private:

    void initBook();

    /** Add a move to a position in the opening book. */
    void addToBook(const Position& pos, const Move& moveToAdd);

    int getWeight(int count);

    static void createBinBook(std::vector<byte>& binBook);

    /** Add a sequence of moves, starting from the initial position, to the binary opening book. */
    static bool addBookLine(const std::string& line, std::vector<byte>& binBook);

    static int pieceToProm(int p);

    static int promToPiece(int prom, bool whiteMove);
};

#endif /* BOOK_HPP_ */
