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
 * treeLogger.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TREELOGGER_HPP_
#define TREELOGGER_HPP_

#include "util.hpp"
#include "move.hpp"

#include <vector>
#include <fstream>

//#define TREELOG


class Position;

/**
 * Base class for logging a search tree to file.
 */
class TreeLoggerBase {
protected:
    unsigned char entryBuffer[16];

    int putInt(int idx, int nBytes, U64 value) {
        for (int i = 0; i < nBytes; i++) {
            entryBuffer[idx + nBytes - 1 - i] = value & 0xff;
            value >>= 8;
        }
        return idx + nBytes;
    }

    U64 getLong(int idx, int nBytes) const {
        U64 ret = 0;
        for (int i = 0; i < nBytes; i++)
            ret = (ret << 8) | entryBuffer[idx++];
        return ret;
    }

    int getInt(int idx, int nBytes) const {
        return (int)getLong(idx, nBytes);
    }

    /* This is the on-disk format. Little-endian byte-order is used.
     * First there is one header entry. Then there is a set of start/end entries.
     * A StartEntry can be identified by its first 4 bytes (endIndex/startIndex)
     * being either -1 (endIndex not computed), or > the entry index.
     *
     * private static final class Header {
     *     byte fenLen; // Used length of fen array
     *     byte[] fen; // 126 bytes, 0-padded
     *     byte flags; // bit 0: 1 if endIndex has been computed for all StartEntries.
     * }
     *
     * private static final class StartEntry {
     *     int endIndex;
     *     int parentIndex;                 // -1 for root node
     *     short move;
     *     short alpha;
     *     short beta;
     *     byte ply;
     *     byte depth;
     * }
     *
     * private static final class EndEntry {
     *     int startIndex;
     *     short score;
     *     short scoreType;
     *     short evalScore;
     *     byte[] hashKey; // lower 6 bytes of position hash key
     * }
     */

    struct StartEntry {
        int endIndex;
        int parentIndex;                 // -1 for root node
        Move move;
        short alpha;
        short beta;
        byte ply;
        byte depth;
    };
    struct EndEntry {
        int startIndex;
        short score;
        short scoreType;
        short evalScore;
        U64 hashKey;    // Note! Upper 2 bytes are not valid (ie 0)
    };
};

#ifdef TREELOG
/**
 * Writer class for logging a search tree to file.
 */
class TreeLoggerWriter : public TreeLoggerBase {
private:
    bool opened;
    std::ofstream os;
    U64 nextIndex;

public:
    /** Constructor. */
    TreeLoggerWriter() : opened(false) { }

    void open(const std::string& filename, const Position& pos) {
        os.open(filename.c_str(), std::ios_base::out |
                                  std::ios_base::binary |
                                  std::ios_base::trunc);
        opened = true;
        writeHeader(pos);
        nextIndex = 0;
    }

    void close() {
        opened = false;
        os.close();
    }

    bool isOpened() const { return opened; }

    // ----------------------------------------------------------------------------
    // Functions used for tree logging

    /**
     * Log information when entering a search node.
     * @param parentId     Index of parent node.
     * @param m            Move made to go from parent node to this node
     * @param alpha        Search parameter
     * @param beta         Search parameter
     * @param ply          Search parameter
     * @param depth        Search parameter
     * @return node index
     */
    U64 logNodeStart(U64 parentIndex, const Move& m, int alpha, int beta, int ply, int depth) {
        if (!opened)
            return 0;
        int idx = 0;
        idx = putInt(idx, 4, -1);
        idx = putInt(idx, 4, parentIndex);
        idx = putInt(idx, 2, m.from() + (m.to() << 6) + (m.promoteTo() << 12));
        idx = putInt(idx, 2, alpha);
        idx = putInt(idx, 2, beta);
        idx = putInt(idx, 1, ply);
        idx = putInt(idx, 1, depth);
        os.write((const char*)entryBuffer, sizeof(entryBuffer));
        return nextIndex++;
    }

    /**
     * @param startIndex Pointer to corresponding start node entry.
     * @param score      Computed score for this node.
     * @param scoreType  See TranspositionTable, T_EXACT, T_GE, T_LE.
     * @param evalScore  Score returned by evaluation function at this node, if known.
     * @return node index
     */
    U64 logNodeEnd(U64 startIndex, int score, int scoreType, int evalScore, U64 hashKey) {
        if (!opened)
            return 0;
        int idx = 0;
        idx = putInt(idx, 4, startIndex);
        idx = putInt(idx, 2, score);
        idx = putInt(idx, 2, scoreType);
        idx = putInt(idx, 2, evalScore);
        idx = putInt(idx, 6, hashKey);
        os.write((const char*)entryBuffer, sizeof(entryBuffer));
        return nextIndex++;
    }

private:
    void writeHeader(const Position& pos);
};
#else
class TreeLoggerWriter {
public:
    TreeLoggerWriter() { }
    void open(const std::string& filename, const Position& pos) { }
    void close() { }
    bool isOpened() const { return false; }
    U64 logNodeStart(U64 parentIndex, const Move& m, int alpha, int beta, int ply, int depth) { return 0; }
    U64 logNodeEnd(U64 startIndex, int score, int scoreType, int evalScore, U64 hashKey) { return 0; }
};
#endif

/**
 * Reader/analysis class for a search tree dumped to a file.
 */
class TreeLoggerReader : public TreeLoggerBase {
    std::fstream fs;
    S64 filePos;
    S64 fileLen;
    int numEntries;

public:
    /** Constructor. */
    TreeLoggerReader(const std::string& filename)
        : fs(filename.c_str(), std::ios_base::out |
                               std::ios_base::in |
                               std::ios_base::binary),
          filePos(-1), fileLen(0), numEntries(0)
    {
        fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fs.seekg(0, std::ios_base::end);
        fileLen = fs.tellg();
        numEntries = (int) ((fileLen - 128) / 16);
        computeForwardPointers();
    }

    void close() {
        fs.close();
    }

    /** Main loop of the interactive tree browser. */
    static void main(const std::string& filename);

private:

    static S64 indexToFileOffs(int index) {
        return 128 + 16 * (S64)index;
    }

    void writeInt(std::streamoff pos, int nBytes, U64 value) {
        fs.seekp(pos, std::ios_base::beg);
        putInt(0, nBytes, value);
        fs.write((const char*)entryBuffer, nBytes);
        filePos = -1;
    }

    U64 readInt(std::streamoff pos, int nBytes) {
        fs.seekg(pos, std::ios_base::beg);
        fs.read((char*)entryBuffer, nBytes);
        filePos = -1;
        return getInt(0, nBytes);
    }

    /** Compute endIndex for all StartNode entries. */
    void computeForwardPointers();

    /** Write forward pointer data to disk. */
    void flushForwardPointerData(std::vector<std::pair<S64,int> >& toWrite);

    /** Get FEN string for root node position. */
    std::string getRootNodeFEN();

    /** Read a start/end entry.
     * @return True if entry was a start entry, false if it was an end entry. */
    bool readEntry(int index, StartEntry& se, EndEntry& ee);

private:
    void mainLoop(Position rootPos);

    bool isMove(std::string cmdStr) const;

    /** Return all nodes with a given hash key. */
    void getNodesForHashKey(U64 hashKey, std::vector<int>& nodes, int maxEntry);

    /** Get hash key from an input string. */
    U64 getHashKey(std::string& s, U64 defKey) const;

    /** Get integer parameter from an input string. */
    static int getArg(const std::string& s, int defVal);

    /** Get a list of integer parameters from an input string. */
    void getArgs(const std::string& s, int defVal, std::vector<int>& args);

    /** Get a string parameter from an input string. */
    static std::string getArgStr(const std::string& s, const std::string& defVal);

    void printHelp();

    /** Read start/end entries for a tree node. Return true if the end entry exists. */
    bool readEntries(int index, StartEntry& se, EndEntry& ee);

    /** Find the parent node to a node. */
    int findParent(int index);

    /** Find all children of a node. */
    void findChildren(int index, std::vector<int>& childs);

    /** Get node position in parents children list. */
    int getChildNo(int index);

    /** Get list of nodes from root position to a node. */
    void getNodeSequence(int index, std::vector<int>& nodes);

    /** Find list of moves from root node to a node. */
    void getMoveSequence(int index, std::vector<Move>& moves);

    /** Find the position corresponding to a node. */
    Position getPosition(const Position& rootPos, int index);

    void printNodeInfo(const Position& rootPos, int index);

    void printNodeInfo(const Position& rootPos, int index, const std::string& filterMove);
};

#endif /* TREELOGGER_HPP_ */
