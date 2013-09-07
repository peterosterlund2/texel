/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
#include <type_traits>
#include <utility>
#include <fstream>


class Position;

namespace Serializer {
    template <typename Type>
    typename std::enable_if<std::is_integral<Type>::value, U8*>::type
    putBytes(U8* buffer, Type value) {
        typedef typename std::make_unsigned<Type>::type UType;
        UType uVal = static_cast<UType>(value);
        for (size_t i = 0; i < sizeof(Type); i++) {
            *buffer++ = uVal & 0xff;
            uVal >>= 8;
        }
        return buffer;
    }

    template <int N>
    static U8* serialize(U8* buffer) {
        static_assert(N >= 0, "Buffer too small");
        return buffer;
    }

    /** Store a sequence of values in a byte buffer. */
    template <int N, typename Type0, typename... Types>
    static U8* serialize(U8* buffer, Type0 value0, Types... values) {
        const int s = sizeof(Type0);
        buffer = putBytes(buffer, value0);
        return serialize<N-s>(buffer, values...);
    }

    template <typename Type>
    typename std::enable_if<std::is_integral<Type>::value, const U8*>::type
    getBytes(const U8* buffer, Type& value) {
        typedef typename std::make_unsigned<Type>::type UType;
        UType uVal = 0;
        for (size_t i = 0; i < sizeof(Type); i++)
            uVal |= static_cast<UType>(*buffer++) << (i * 8);
        value = static_cast<Type>(uVal);
        return buffer;
    }

    template <int N>
    static const U8* deSerialize(const U8* buffer) {
        static_assert(N >= 0, "Buffer too small");
        return buffer;
    }

    /** Retrieve a sequence of values from a byte buffer. */
    template <int N, typename Type0, typename... Types>
    static const U8* deSerialize(const U8* buffer, Type0& value0, Types&... values) {
        const int s = sizeof(Type0);
        buffer = getBytes(buffer, value0);
        return deSerialize<N-s>(buffer, values...);
    }
}

/**
 * Base class for logging a search tree to file.
 */
class TreeLoggerBase {
    friend class TreeLoggerTest;
protected:
    /**
     * Rules for the log file format:
     * - The log file contains information for a single search.
     * - A start entry may not have a corresponding end entry. This happens
     *   if the search was interrupted.
     * - Start and end entries are properly nested (assuming the end entries exist)
     *   s1.index < s2.index => e1.index > e2.index
     */

    enum class EntryType : U8 {
        POSITION_INCOMPLETE, // First entry in file has this type when endIndex
                             // has not yet been computed for all StartEntries.
        POSITION_PART0,      // Position entry, first part.
        POSITION_PART1,      // Position entry, second part.
        NODE_START,          // Start of a search node.
        NODE_END             // End of a search node.
    };

    struct Position0 {
        U64 word0;
        U64 word1;
        U16 word2a;

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, word0, word1, word2a);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, word0, word1, word2a);
        }
    };

    struct Position1 {
        U16 word2b;
        U32 word2c;
        U64 word3;
        U64 word4;

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, word2b, word2c, word3, word4);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, word2b, word2c, word3, word4);
        }
    };

    struct StartEntry {
        U32 endIndex;
        S32 parentIndex;    // -1 for root node
        U16 move;
        S16 alpha;
        S16 beta;
        U8 ply;
        U16 depth;

        Move getMove() const {
            Move ret;
            ret.setMove(move & 63, (move >> 6) & 63, (move >> 12) & 15, 0);
            return ret;
        }

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, endIndex, parentIndex, move,
                                            alpha, beta, ply, depth);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, endIndex, parentIndex, move,
                                       alpha, beta, ply, depth);
        }
    };

    struct EndEntry {
        S32 startIndex;
        S16 score;
        U8 scoreType;
        S16 evalScore;
        U64 hashKey;

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, startIndex, score, scoreType,
                                            evalScore, hashKey);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, startIndex, score, scoreType,
                                       evalScore, hashKey);
        }
    };

    struct Entry {
        EntryType type;
        union {
            Position0 h0;
            Position1 h1;
            StartEntry se;
            EndEntry ee;
        };

        static const int bufSize = 23;
        typedef U8 Buffer[bufSize];

        void serialize(U8 buffer[bufSize]) const {
            U8* ptr = buffer;
            typedef typename std::underlying_type<EntryType>::type UType;
            const int su = sizeof(UType);
            UType uType = static_cast<UType>(type);
            ptr = Serializer::serialize<bufSize>(ptr, uType);
            switch (type) {
            case EntryType::POSITION_INCOMPLETE: h0.serialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART0:      h0.serialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART1:      h1.serialize<bufSize-su>(ptr); break;
            case EntryType::NODE_START:          se.serialize<bufSize-su>(ptr); break;
            case EntryType::NODE_END:            ee.serialize<bufSize-su>(ptr); break;
            }
        }

        void deSerialize(U8 buffer[bufSize]) {
            const U8* ptr = buffer;
            typedef typename std::underlying_type<EntryType>::type UType;
            const int su = sizeof(UType);
            UType uType;
            ptr = Serializer::deSerialize<bufSize>(ptr, uType);
            type = static_cast<EntryType>(uType);
            switch (type) {
            case EntryType::POSITION_INCOMPLETE: h0.deSerialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART0:      h0.deSerialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART1:      h1.deSerialize<bufSize-su>(ptr); break;
            case EntryType::NODE_START:          se.deSerialize<bufSize-su>(ptr); break;
            case EntryType::NODE_END:            ee.deSerialize<bufSize-su>(ptr); break;
            }
        }
    };

    Entry entry;
    U8 entryBuffer[Entry::bufSize];
};

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
        entry.type = EntryType::NODE_START;
        entry.se.endIndex = -1;
        entry.se.parentIndex = parentIndex;
        entry.se.move = m.from() + (m.to() << 6) + (m.promoteTo() << 12);
        entry.se.alpha = alpha;
        entry.se.beta = beta;
        entry.se.ply = ply;
        entry.se.depth = depth;
        entry.serialize(entryBuffer);
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
        entry.type = EntryType::NODE_END;
        entry.ee.startIndex = startIndex;
        entry.ee.score = score;
        entry.ee.scoreType = scoreType;
        entry.ee.evalScore = evalScore;
        entry.ee.hashKey = hashKey;
        entry.serialize(entryBuffer);
        os.write((const char*)entryBuffer, sizeof(entryBuffer));
        return nextIndex++;
    }

private:
    void writeHeader(const Position& pos);
};

/** Dummy version of TreeLoggerWriter. */
class TreeLoggerWriterDummy {
public:
    TreeLoggerWriterDummy() { }
    void open(const std::string& filename, const Position& pos) { }
    void close() { }
    bool isOpened() const { return false; }
    U64 logNodeStart(U64 parentIndex, const Move& m, int alpha, int beta, int ply, int depth) { return 0; }
    U64 logNodeEnd(U64 startIndex, int score, int scoreType, int evalScore, U64 hashKey) { return 0; }
};

/**
 * Reader/analysis class for a search tree dumped to a file.
 */
class TreeLoggerReader : public TreeLoggerBase {
public:
    /** Constructor. */
    TreeLoggerReader(const std::string& filename)
        : fs(filename.c_str(), std::ios_base::out |
                               std::ios_base::in |
                               std::ios_base::binary),
          filePos(-1), fileLen(0), numEntries(0) {
        fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fs.seekg(0, std::ios_base::end);
        fileLen = fs.tellg();
        numEntries = (int) (fileLen / Entry::bufSize) - 2;
        computeForwardPointers();
    }

    void close() {
        fs.close();
    }

    /** Main loop of the interactive tree browser. */
    static void main(const std::string& filename);

private:
    static int indexToFileIndex(int index) {
        return index + 2;
    }

    /** Compute endIndex for all StartNode entries. */
    void computeForwardPointers();

    /** Write forward pointer data to disk. */
    void flushForwardPointerData(std::vector<std::pair<int,int>>& toWrite);

    /** Get root node position. */
    void getRootNodePosition(Position& pos);

    /** Read an entry. */
    void readEntry(int index, Entry& entry);

    /** Write an entry. */
    void writeEntry(int index, const Entry& entry);

    /** Read a start/end entry.
     * @return True if entry was a start entry, false if it was an end entry. */
    bool readEntry(int index, StartEntry& se, EndEntry& ee);

    /** Run the interactive analysis main loop. */
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


    std::fstream fs;
    S64 filePos;        // Current file read position (seekg)
    S64 fileLen;
    int numEntries;
};

#endif /* TREELOGGER_HPP_ */
