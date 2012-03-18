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
 * treeLogger.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "treeLogger.hpp"
#include "transpositionTable.hpp"
#include "textio.hpp"
#include "position.hpp"

#include <iostream>
#include <iomanip>

// FIXME!! Searching for hash keys is much faster in java version.

#ifdef TREELOG
void
TreeLoggerWriter::writeHeader(const Position& pos) {
    char header[128];
    for (size_t i = 0; i < COUNT_OF(header); i++)
        header[i] = 0;
    std::string fen = TextIO::toFEN(pos);
    header[0] = fen.length();
    for (size_t i = 0; i < fen.length(); i++)
        header[i+1] = fen[i];
    os.write(header, COUNT_OF(header));
}
#endif

void
TreeLoggerReader::main(const std::string& filename) {
    TreeLoggerReader an(filename);
    Position rootPos = TextIO::readFEN(an.getRootNodeFEN());
    an.mainLoop(rootPos);
    an.close();
}

void
TreeLoggerReader::computeForwardPointers() {
    U64 tmp = readInt(127, 1);
    if ((tmp & (1<<7)) != 0)
        return;
    std::cout << "Computing forward pointers..." << std::endl;
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
    std::cout << "Computing forward pointers... done" << std::endl;
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
    fs.read((char*)entryBuffer, 16);
    int otherIndex = getInt(0, 4);
    bool isStartEntry = (otherIndex == -1) || (otherIndex > index);
    if (isStartEntry) {
        se.endIndex = otherIndex;
        se.parentIndex = getInt(4, 4);
        int m = getInt(8, 2);
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
        ee.hashKey = getLong(10, 6);
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
                std::cout << " " << TextIO::moveToUCIString(m);
            }
            std::cout << std::endl;
            printNodeInfo(rootPos, currIndex);
            Position pos = getPosition(rootPos, currIndex);
            std::cout << TextIO::asciiBoard(pos);
            std::cout << TextIO::toFEN(pos) << std::endl;
            std::stringstream ss;
            ss << std::hex << std::setw(16) << std::setfill('0') << pos.historyHash();
            std::cout << ss.str() << std::endl;
            if (currIndex >= 0) {
                std::vector<int> children;
                findChildren(currIndex, children);
                for (size_t i = 0; i < children.size(); i++)
                    printNodeInfo(rootPos, children[i]);
            }
        }
        doPrint = true;
        std::cout << "Command:" << std::flush;
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
                std::cout << "No such move" << std::endl;
                doPrint = false;
            } else if (found.size() > 1) {
                std::cout << "Ambiguous move" << std::endl;
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
                std::cout << ' ' << TextIO::moveToUCIString(moves[i]);
            std::cout << std::endl;
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
    std::cout << "  p              - Print move sequence" << std::endl;
    std::cout << "  n              - Print node info corresponding to move sequence" << std::endl;
    std::cout << "  l [move]       - List child nodes, optionally only for one move" << std::endl;
    std::cout << "  d [n1 [n2...]] - Go to child \"n\"" << std::endl;
    std::cout << "  move           - Go to child \"move\", if unique" << std::endl;
    std::cout << "  u [levels]     - Move up" << std::endl;
    std::cout << "  h [key]        - Find nodes with current (or given) hash key" << std::endl;
    std::cout << "  num            - Go to node \"num\"" << std::endl;
    std::cout << "  q              - Quit" << std::endl;
    std::cout << "  ?              - Print this help" << std::endl;
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
    for (int i = 0; i < (int)childs.size(); i++)
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
        std::cout << std::setw(8) << index << " entries:" << numEntries << std::endl;
    } else {
        StartEntry se;
        EndEntry ee;
        bool haveEE = readEntries(index, se, ee);
        std::string m = TextIO::moveToUCIString(se.move);
        if ((filterMove.length() > 0) && (m != filterMove))
            return;
        std::cout << std::setw(3) << getChildNo(index)
                  << ' '   << std::setw(8) << index << ' ' << m
                  << " a:" << std::setw(6) << se.alpha
                  << " b:" << std::setw(6) << se.beta
                  << " p:" << std::setw(2) << (int)se.ply
                  << " d:" << std::setw(2) << (int)se.depth;
        if (haveEE) {
            int subTreeNodes = (se.endIndex - ee.startIndex - 1) / 2;
            std::string type;
            switch (ee.scoreType) {
            case TType::T_EXACT: type = "= "; break;
            case TType::T_GE   : type = ">="; break;
            case TType::T_LE   : type = "<="; break;
            default            : type = "  "; break;
            }
            std::cout << " s:" << type << std::setw(6) << ee.score
                      << " e:" << std::setw(6) << ee.evalScore
                      << " sub:" << subTreeNodes;
        }
        std::cout << std::endl;
    }
}
