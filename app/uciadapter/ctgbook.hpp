/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * ctgbook.hpp
 *
 *  Created on: Jul 15, 2016
 *      Author: petero
 */

#ifndef CTGBOOK_HPP_
#define CTGBOOK_HPP_

#include "util/util.hpp"
#include "util/random.hpp"
#include "position.hpp"

#include <vector>
#include <fstream>


class BitVector {
public:
    void addBit(bool value);
    void addBits(int mask, int numBits);

    /** Number of bits left in current byte. */
    int padBits();

    const std::vector<U8>& getBytes() const;

    int getLength() const;

private:
    std::vector<U8> buf;
    int length = 0;
};

struct BookEntry {
    Move move;
    float weight;
};

class PositionData {
public:
    void setFromPageBuf(const std::vector<U8>& pageBuf, int offs);

    Position pos;
    bool mirrorColor = false;
    bool mirrorLeftRight = false;

    void getBookMoves(std::vector<BookEntry>& entries);

    /** Return (loss * 2 + draws). */
    int getOpponentScore();

    int getRecommendation();

    static void staticInitialize();

    static const int posInfoBytes = 3*4 + 4 + (3+4)*2 + 1 + 1 + 1;

private:
    static int findPiece(const Position& pos, int piece, int pieceNo);

    Move decodeMove(const Position& pos, int moveCode);

    struct MoveInfo {
        int piece;
        int pieceNo;
        int dx;
        int dy;
    };
    static MoveInfo moveInfo[256];

    std::vector<U8> buf;
    int posLen = 0;
    int moveBytes = 0;
};

class CtbFile {
public:
    CtbFile(std::fstream& f);
    int lowerPageBound;
    int upperPageBound;
};

class CtoFile {
public:
    CtoFile(std::fstream& f);

    static void getHashIndices(const std::vector<U8>& encodedPos, const CtbFile& ctb,
                               std::vector<int>& indices);

    int getPage(int hashIndex);

private:
    static int getHashValue(const std::vector<U8>& encodedPos);

    std::fstream& f;
    static const int tbl[];
};

class CtgFile {
public:
    CtgFile(std::fstream& f, CtbFile& ctb, CtoFile& cto);

    bool getPositionData(const Position& pos, PositionData& pd);

private:
    bool findInPage(int page, const std::vector<U8>& encodedPos, PositionData& pd);

    std::fstream& f;
    CtbFile& ctb;
    CtoFile& cto;
};

class CtgBook {
public:
    /** Constructor. */
    CtgBook(const std::string& fileName, bool tournament, bool preferMain);

    /** Get a random book move for a position.
     * @return true if a book move was found, false otherwise. */
    bool getBookMove(Position& pos, Move& out);

private:
    void getBookEntries(const Position& pos, std::vector<BookEntry>& bookMoves);

    std::fstream ctgF;
    std::fstream ctbF;
    std::fstream ctoF;

    bool tournamentMode;
    bool preferMainLines;

    static Random rndGen;
};


#endif /* CTGBOOK_HPP_ */
