/*
 * treeLogger.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "treeLogger.hpp"
#include "transpositionTable.hpp"
#include "textio.hpp"
#include "position.hpp"


void
TreeLoggerWriter::writeHeader(const Position& pos) {
    char header[128];
    for (size_t i = 0; i < COUNT_OF(header); i++)
        header[i] = 0;
    std::string fen = TextIO::toFEN(pos);
    for (size_t i = 0; i < fen.length(); i++)
        header[i] = fen[i];
    os.write(header, COUNT_OF(header));
}


void
TreeLoggerReader::main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: progname filename\n");
        exit(1);
    }
    TreeLoggerReader an(argv[1]);
    Position rootPos = TextIO::readFEN(an.getRootNodeFEN());
    an.mainLoop(rootPos);
    an.close();
}

void
TreeLoggerReader::computeForwardPointers() {
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

std::string
TreeLoggerReader::getRootNodeFEN() {
    char buf[128];
    fs.seekg(0, std::ios_base::beg);
    fs.read(buf, sizeof(buf));
    int len = buf[0];
    std::string ret(&buf[1], len);
    return ret;
}

bool
TreeLoggerReader::readEntry(int index, StartEntry& se, EndEntry& ee) {
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

void
TreeLoggerReader::mainLoop(Position rootPos) {
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

bool
TreeLoggerReader::isMove(std::string cmdStr) const {
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

void
TreeLoggerReader::getNodeForHashKey(U64 hashKey, std::vector<int>& nodes) {
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

U64
TreeLoggerReader::getHashKey(std::string& s, U64 defKey) const {
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

int
TreeLoggerReader::getArg(const std::string& s, int defVal) {
    size_t idx = s.find_first_of(' ');
    if (idx != s.npos) {
        int tmp;
        if (str2Num(s.substr(idx+1), tmp))
            return tmp;
    }
    return defVal;
}

void
TreeLoggerReader::getArgs(const std::string& s, int defVal, std::vector<int>& args) {
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

std::string
TreeLoggerReader::getArgStr(const std::string& s, const std::string& defVal) {
    size_t idx = s.find_first_of(' ');
    if (idx != s.npos)
        return s.substr(idx+1);
    return defVal;
}

void
TreeLoggerReader::printHelp() {
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

bool
TreeLoggerReader::readEntries(int index, StartEntry& se, EndEntry& ee) {
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

int
TreeLoggerReader::findParent(int index) {
    if (index >= 0) {
        StartEntry se;
        EndEntry ee;
        readEntries(index, se, ee);
        index = se.parentIndex;
    }
    return index;
}

void
TreeLoggerReader::findChildren(int index, std::vector<int>& childs) {
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
//        if (se.parentIndex != index)
//            break;
        child = se.endIndex + 1;
    }
}

int
TreeLoggerReader::getChildNo(int index) {
    std::vector<int> childs;
    findChildren(findParent(index), childs);
    for (size_t i = 0; i < childs.size(); i++)
        if (childs[i] == index)
            return i;
    return -1;
}

void
TreeLoggerReader::getNodeSequence(int index, std::vector<int>& nodes) {
    nodes.push_back(index);
    while (index >= 0) {
        index = findParent(index);
        nodes.push_back(index);
    }
    std::reverse(nodes.begin(), nodes.end());
}

void
TreeLoggerReader::getMoveSequence(int index, std::vector<Move>& moves) {
    StartEntry se;
    EndEntry ee;
    while (index >= 0) {
        readEntries(index, se, ee);
        moves.push_back(se.move);
        index = findParent(index);
    }
    std::reverse(moves.begin(), moves.end());
}

Position
TreeLoggerReader::getPosition(const Position& rootPos, int index) {
    std::vector<Move> moves;
    getMoveSequence(index, moves);
    Position ret(rootPos);
    UndoInfo ui;
    for (size_t i = 0; i < moves.size(); i++)
        ret.makeMove(moves[i], ui);
    return ret;
}

void
TreeLoggerReader::printNodeInfo(const Position& rootPos, int index) {
    printNodeInfo(rootPos, index, "");
}

void
TreeLoggerReader::printNodeInfo(const Position& rootPos, int index, const std::string& filterMove) {
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
            case TType::T_EXACT: type = "= "; break;
            case TType::T_GE   : type = ">="; break;
            case TType::T_LE   : type = "<="; break;
            default                                  : type = "  "; break;
            }
            printf(" s:%s%6d e:%6d sub:%d", type.c_str(), ee.score, ee.evalScore, subTreeNodes);
        }
        printf("\n");
    }
}
