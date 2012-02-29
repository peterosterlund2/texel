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
#include "transpositionTable.hpp"
#include "textio.hpp"

#include <vector>
#include <fstream>

/**
 * Base class for logging a search tree to file.
 */
class TreeLoggerBase {
protected:
    char entryBuffer[16];

    int putInt(int idx, int nBytes, U64 value) {
        for (int i = 0; i < nBytes; i++) {
            entryBuffer[idx++] = value & 0xff;
            value >>= 8;
        }
        return idx;
    }

    U64 getInt(int idx, int nBytes) {
        U64 ret = 0;
        for (int i = 0; i < nBytes; i++)
            ret |= ((U64)entryBuffer[idx++]) << (i*8);
        return ret;
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

/**
 * Writer class for logging a search tree to file.
 */
class TreeLoggerWriter : public TreeLoggerBase {
private:
    std::ofstream os;
    U64 nextIndex;

public:
    /** Constructor. */
    TreeLoggerWriter(const std::string& filename, const Position& pos)
        : os(filename.c_str(), std::ios_base::out |
                               std::ios_base::binary |
                               std::ios_base::trunc),
          nextIndex(0)
    {
        writeHeader(pos);
        nextIndex = 0;
    }

    void close() {
        os.close();
    }

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
        int idx = 0;
        idx = putInt(idx, 4, -1);
        idx = putInt(idx, 4, parentIndex);
        idx = putInt(idx, 2, m.from() + (m.to() << 6) + (m.promoteTo() << 12));
        idx = putInt(idx, 2, alpha);
        idx = putInt(idx, 2, beta);
        idx = putInt(idx, 1, ply);
        idx = putInt(idx, 1, depth);
        os.write(entryBuffer, sizeof(entryBuffer));
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
        int idx = 0;
        idx = putInt(idx, 4, startIndex);
        idx = putInt(idx, 2, score);
        idx = putInt(idx, 2, scoreType);
        idx = putInt(idx, 2, evalScore);
        idx = putInt(idx, 6, hashKey);
        os.write(entryBuffer, sizeof(entryBuffer));
        return nextIndex++;
    }

private:
    void writeHeader(const Position& pos) {
        char header[128];
        for (size_t i = 0; i < COUNT_OF(header); i++)
            header[i] = 0;
        std::string fen = TextIO::toFEN(pos);
        for (size_t i = 0; i < fen.length(); i++)
            header[i] = fen[i];
        os.write(header, COUNT_OF(header));
    }
};

/**
 * Reader/analysis class for logging a search tree to file.
 */
class TreeLoggerReader : public TreeLoggerBase {
    std::fstream fs;
    int numEntries;

public:
    /** Constructor. */
    TreeLoggerReader(const std::string& filename)
        : fs(filename.c_str(), std::ios_base::out |
                               std::ios_base::in |
                               std::ios_base::binary),
          numEntries(0)
    {
        fs.seekg(0, std::ios_base::end);
        U64 len = fs.tellg();
        numEntries = (int) ((len - 128) / 16);
        computeForwardPointers();
    }

    void close() {
        fs.close();
    }

    /** Main loop of the interactive tree browser. */
    static void main(int argc, char* argv[]) {
        if (argc != 2) {
            printf("Usage: progname filename\n");
            exit(1);
        }
        TreeLoggerReader an(argv[1]);
        Position rootPos = TextIO::readFEN(an.getRootNodeFEN());
        an.mainLoop(rootPos);
        an.close();
    }

private:

    static int indexToFileOffs(int index) {
        return 128 + index * 16;
    }

    void writeInt(std::streamoff pos, int nBytes, U64 value) {
        fs.seekp(pos, std::ios_base::beg);
        putInt(0, nBytes, value);
        fs.write(entryBuffer, nBytes);
    }

    U64 readInt(std::streamoff pos, int nBytes) {
        fs.seekg(pos, std::ios_base::beg);
        fs.read(entryBuffer, nBytes);
        return getInt(0, nBytes);
    }

    /** Compute endIndex for all StartNode entries. */
    void computeForwardPointers() {
        U64 tmp = readInt(127, 1);
        if ((tmp & (1<<7)) != 0)
            return;
        printf("Computing forward pointers...\n");
        StartEntry se;
        EndEntry ee;
        for (int i = 0; i < numEntries; i++) {
            bool isStart = readEntry(i, se, ee);
            if (!isStart) {
                int offs = indexToFileOffs(ee.startIndex);
                writeInt(offs, 4, i);
            }
        }
        writeInt(127, 1, 1 << 7);
        printf("Computing forward pointers... done\n");
    }

    /** Get FEN string for root node position. */
    std::string getRootNodeFEN() {
        char buf[128];
        fs.seekg(0, std::ios_base::beg);
        fs.read(buf, sizeof(buf));
        int len = buf[0];
        std::string ret(&buf[1], len);
        return ret;
    }

    /** Read a start/end entry.
     * @return True if entry was a start entry, false if it was an end entry. */
    bool readEntry(int index, StartEntry& se, EndEntry& ee) {
        int offs = indexToFileOffs(index);
        fs.seekg(offs, std::ios_base::beg);
        fs.read(entryBuffer, 16);
        int otherIndex = getInt(0, 4);
        bool isStartEntry = (otherIndex == -1) || (otherIndex > index);
        if (isStartEntry) {
            se.endIndex = otherIndex;
            se.parentIndex = getInt(4, 4);
            int m = getInt(8, 4);
            se.move.setMove(m & 63, (m >> 6) & 63, (m >> 12) & 15, 0);
            se.alpha = getInt(10, 2);
            se.beta = getInt(12, 2);
            se.ply = getInt(14, 1);
            se.depth = getInt(15, 1);
        } else {
            ee.startIndex = otherIndex;
            ee.score = getInt(4, 2);
            ee.scoreType = getInt(6, 2);
            ee.evalScore = getInt(8, 2);
            ee.hashKey = getInt(10, 6);
        }
        return isStartEntry;
    }

private:
    void mainLoop(Position rootPos) {
        int currIndex = -1;
        std::string prevStr = "";

        bool doPrint = true;
        while (true) {
            if (doPrint) {
                std::vector<Move> moves;
                getMoveSequence(currIndex, moves);
                for (size_t i = 0; i < moves.size(); i++) {
                    const Move& m = moves[i];
                    printf(" %s", TextIO::moveToUCIString(m).c_str());
                }
                printf("\n");
                printNodeInfo(rootPos, currIndex);
                Position pos = getPosition(rootPos, currIndex);
                printf("%s", TextIO::asciiBoard(pos).c_str());
                printf("%s\n", TextIO::toFEN(pos).c_str());
                printf("%16lx\n", pos.historyHash());
                if (currIndex >= 0) {
                    std::vector<int> children;
                    findChildren(currIndex, children);
                    for (size_t i = 0; i < children.size(); i++)
                        printNodeInfo(rootPos, children[i]);
                }
            }
            doPrint = true;
            printf("Command:");
            std::string cmdStr;
            std::getline(std::cin, cmdStr);
            if (!std::cin)
                return;
            if (cmdStr.length() == 0)
                cmdStr = prevStr;
            if (startsWith(cmdStr, "q")) {
                return;
            } else if (startsWith(cmdStr, "?")) {
                printHelp();
                doPrint = false;
            } else if (isMove(cmdStr)) {
                std::vector<int> children;
                findChildren(currIndex, children);
                std::string m = cmdStr;
                StartEntry se;
                EndEntry ee;
                std::vector<int> found;
                for (size_t i = 0; i < children.size(); i++) {
                    readEntries(children[i], se, ee);
                    if (TextIO::moveToUCIString(se.move) == m)
                        found.push_back(children[i]);
                }
                if (found.empty()) {
                    printf("No such move\n");
                    doPrint = false;
                } else if (found.size() > 1) {
                    printf("Ambiguous move\n");
                    for (size_t i = 0; i < found.size(); i++)
                        printNodeInfo(rootPos, found[i]);
                    doPrint = false;
                } else {
                    currIndex = found[0];
                }
            } else if (startsWith(cmdStr, "u")) {
                int n = getArg(cmdStr, 1);
                for (int i = 0; i < n; i++)
                    currIndex = findParent(currIndex);
            } else if (startsWith(cmdStr, "l")) {
                std::vector<int> children;
                findChildren(currIndex, children);
                std::string m = getArgStr(cmdStr, "");
                for (size_t i = 0; i < children.size(); i++)
                    printNodeInfo(rootPos, children[i], m);
                doPrint = false;
            } else if (startsWith(cmdStr, "n")) {
                std::vector<int> nodes;
                getNodeSequence(currIndex, nodes);
                for (size_t i = 0; i < nodes.size(); i++)
                    printNodeInfo(rootPos, nodes[i]);
                doPrint = false;
            } else if (startsWith(cmdStr, "d")) {
                std::vector<int> nVec;
                getArgs(cmdStr, 0, nVec);
                for (size_t i = 0; i < nVec.size(); i++) {
                    int n = nVec[i];
                    std::vector<int> children;
                    findChildren(currIndex, children);
                    if ((n >= 0) && (n < (int)children.size())) {
                        currIndex = children[n];
                    } else
                        break;
                }
            } else if (startsWith(cmdStr, "p")) {
                std::vector<Move> moves;
                getMoveSequence(currIndex, moves);
                for (size_t i = 0; i < moves.size(); i++)
                    printf(" %s", TextIO::moveToUCIString(moves[i]).c_str());
                printf("\n");
                doPrint = false;
            } else if (startsWith(cmdStr, "h")) {
                U64 hashKey = getPosition(rootPos, currIndex).historyHash();
                hashKey = getHashKey(cmdStr, hashKey);
                std::vector<int> nodes;
                getNodeForHashKey(hashKey, nodes);
                for (size_t i = 0; i < nodes.size(); i++)
                    printNodeInfo(rootPos, nodes[i]);
                doPrint = false;
            } else {
                int i;
                if (str2Num(cmdStr, i))
                    if ((i >= -1) && (i < numEntries))
                        currIndex = i;
            }
            prevStr = cmdStr;
        }
    }

    bool isMove(std::string cmdStr) const {
        if (cmdStr.length() != 4)
            return false;
        cmdStr = toLowerCase(cmdStr);
        for (int i = 0; i < 4; i++) {
            int c = cmdStr[i];
            if ((i == 0) || (i == 2)) {
                if ((c < 'a') || (c > 'h'))
                    return false;
            } else {
                if ((c < '1') || (c > '8'))
                    return false;
            }
        }
        return true;
    }

    /** Return all nodes with a given hash key. */
    void getNodeForHashKey(U64 hashKey, std::vector<int>& nodes) {
        hashKey &= 0x0000ffffffffffffULL;
        StartEntry se;
        EndEntry ee;
        for (int index = 0; index < numEntries; index++) {
            bool isStart = readEntry(index, se, ee);
            if (!isStart) {
                if (ee.hashKey == hashKey) {
                    int sIdx = ee.startIndex;
                    nodes.push_back(sIdx);
                }
            }
        }
        std::sort(nodes.begin(), nodes.end());
    }

    /** Get hash key from an input string. */
    U64 getHashKey(std::string& s, U64 defKey) const {
        U64 key = defKey;
        size_t idx = s.find_first_of(' ');
        if (idx != s.npos) {
            s = s.substr(idx + 1);
            if (startsWith(s, "0x"))
                s = s.substr(2);
            hexStr2Num(s, key);
        }
        return key;
    }

    /** Get integer parameter from an input string. */
    static int getArg(const std::string& s, int defVal) {
        size_t idx = s.find_first_of(' ');
        if (idx != s.npos) {
            int tmp;
            if (str2Num(s.substr(idx+1), tmp))
                return tmp;
        }
        return defVal;
    }

    /** Get a list of integer parameters from an input string. */
    void getArgs(const std::string& s, int defVal, std::vector<int>& args) {
        std::vector<std::string> split;
        splitString(s, split);
        for (size_t i = 1; i < split.size(); i++) {
            int tmp;
            if (!str2Num(split[i], tmp)) {
                args.clear();
                break;
            }
            args.push_back(tmp);
        }
        if (args.empty())
            args.push_back(defVal);
    }

    /** Get a string parameter from an input string. */
    static std::string getArgStr(const std::string& s, const std::string& defVal) {
        size_t idx = s.find_first_of(' ');
        if (idx != s.npos)
            return s.substr(idx+1);
        return defVal;
    }

    void printHelp() {
        printf("  p              - Print move sequence\n");
        printf("  n              - Print node info corresponding to move sequence\n");
        printf("  l [move]       - List child nodes, optionally only for one move\n");
        printf("  d [n1 [n2...]] - Go to child \"n\"\n");
        printf("  move           - Go to child \"move\", if unique\n");
        printf("  u [levels]     - Move up\n");
        printf("  h [key]        - Find nodes with current (or given) hash key\n");
        printf("  num            - Go to node \"num\"\n");
        printf("  q              - Quit\n");
        printf("  ?              - Print this help\n");
    }

    /** Read start/end entries for a tree node. Return true if the end entry exists. */
    bool readEntries(int index, StartEntry& se, EndEntry& ee) {
        bool isStart = readEntry(index, se, ee);
        if (isStart) {
            int eIdx = se.endIndex;
            if (eIdx >= 0) {
                readEntry(eIdx, se, ee);
            } else {
                return false;
            }
        } else {
            int sIdx = ee.startIndex;
            readEntry(sIdx, se, ee);
        }
        return true;
    }

    /** Find the parent node to a node. */
    int findParent(int index) {
        if (index >= 0) {
            StartEntry se;
            EndEntry ee;
            readEntries(index, se, ee);
            index = se.parentIndex;
        }
        return index;
    }

    /** Find all children of a node. */
    void findChildren(int index, std::vector<int>& childs) {
        StartEntry se;
        EndEntry ee;
        int child = index + 1;
        while ((child >= 0) && (child < numEntries)) {
            bool haveEE = readEntries(child, se, ee);
            if (se.parentIndex == index)
                childs.push_back(child);
            if (!haveEE)
                break;
            if (child != ee.startIndex)
                break; // two end entries in a row, no more children
//            if (se.parentIndex != index)
//                break;
            child = se.endIndex + 1;
        }
    }

    /** Get node position in parents children list. */
    int getChildNo(int index) {
        std::vector<int> childs;
        findChildren(findParent(index), childs);
        for (size_t i = 0; i < childs.size(); i++)
            if (childs[i] == index)
                return i;
        return -1;
    }

    /** Get list of nodes from root position to a node. */
    void getNodeSequence(int index, std::vector<int>& nodes) {
        nodes.push_back(index);
        while (index >= 0) {
            index = findParent(index);
            nodes.push_back(index);
        }
        std::reverse(nodes.begin(), nodes.end());
    }

    /** Find list of moves from root node to a node. */
    void getMoveSequence(int index, std::vector<Move>& moves) {
        StartEntry se;
        EndEntry ee;
        while (index >= 0) {
            readEntries(index, se, ee);
            moves.push_back(se.move);
            index = findParent(index);
        }
        std::reverse(moves.begin(), moves.end());
    }

    /** Find the position corresponding to a node. */
    Position getPosition(const Position& rootPos, int index) {
        std::vector<Move> moves;
        getMoveSequence(index, moves);
        Position ret(rootPos);
        UndoInfo ui;
        for (size_t i = 0; i < moves.size(); i++)
            ret.makeMove(moves[i], ui);
        return ret;
    }

    void printNodeInfo(const Position& rootPos, int index) {
        printNodeInfo(rootPos, index, "");
    }

    void printNodeInfo(const Position& rootPos, int index, const std::string& filterMove) {
        if (index < 0) { // Root node
            printf("%8d entries:%d\n", index, numEntries);
        } else {
            StartEntry se;
            EndEntry ee;
            bool haveEE = readEntries(index, se, ee);
            std::string m = TextIO::moveToUCIString(se.move);
            if ((filterMove.length() > 0) && (m != filterMove))
                return;
            printf("%3d %8d %s a:%6d b:%6d p:%2d d:%2d", getChildNo(index), index,
                   m.c_str(), se.alpha, se.beta, se.ply, se.depth);
            if (haveEE) {
                int subTreeNodes = (se.endIndex - ee.startIndex - 1) / 2;
                std::string type;
                switch (ee.scoreType) {
                case TranspositionTable::TTEntry::T_EXACT: type = "= "; break;
                case TranspositionTable::TTEntry::T_GE   : type = ">="; break;
                case TranspositionTable::TTEntry::T_LE   : type = "<="; break;
                default                                  : type = "  "; break;
                }
                printf(" s:%s%6d e:%6d sub:%d", type.c_str(), ee.score, ee.evalScore, subTreeNodes);
            }
            printf("\n");
        }
    }
};

#endif /* TREELOGGER_HPP_ */
