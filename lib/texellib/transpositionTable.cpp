/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * transpositionTable.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "transpositionTable.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "largePageAlloc.hpp"
#include "alignedAlloc.hpp"
#include "threadpool.hpp"
#include "numa.hpp"

#include <iostream>
#include <iomanip>
#include <cstring>


TranspositionTable::TranspositionTable(U64 numEntries)
    : table(nullptr), ttStorage(*this) {
    reSize(numEntries);
}

void
TranspositionTable::reSize(U64 numEntries) {
    if (numEntries < 4)
        numEntries = 4;
    numEntries &= ~3;

    if (numEntries == tableSize)
        return;

    tableP.reset();
    table = nullptr;
    tableSize = 0;

    using TTE = TTEntryStorage;
    tableP = LargePageAlloc::allocate<TTE>(numEntries);
    if (!tableP)
        tableP = std::shared_ptr<TTE>(AlignedAllocator<TTE>().allocate(numEntries),
                                      [numEntries](TTE* p) {
                                          AlignedAllocator<TTE>().deallocate(p, numEntries);
                                      });
    table = tableP.get();
    tableSize = numEntries;

    generation = 0;
    clear();
}

void TranspositionTable::setUsedSize(U64 s) {
    usedSize = s;
    usedSizeShift = 0;
    U64 topBits = usedSize;
    while (topBits >= 256) {
        topBits /= 2;
        usedSizeShift++;
    }
    usedSizeTopBits = (int)topBits;
    usedSizeMask = ((1ULL << usedSizeShift) - 1) & ~3ULL;
}

void
TranspositionTable::clear() {
    setUsedSize(tableSize);
    tbGen.reset();
    notUsedCnt = 0;

    if (tableSize > 1024*1024 && (tableSize % 1024) == 0) {
        int nThreads = 4;
        int nChunks = 4;
        ThreadPool<int> pool(nThreads);
        U64 chunkSize = tableSize / nChunks;
        for (U64 i = 0; i < tableSize; i += chunkSize) {
            auto task = [this,chunkSize,i](int workerNo) {
                Numa::instance().bindThread(0);
                U64 len = std::min(chunkSize, tableSize - i);
                std::memset((void*)&table[i], 0, len * sizeof(TTEntryStorage));
                return 0;
            };
            pool.addTask(task);
        }
        pool.getAllResults([](int){});
    } else {
        std::memset((void*)&table[0], 0, tableSize * sizeof(TTEntryStorage));
    }
}

void
TranspositionTable::setWhiteContempt(int contempt) {
    if (contempt > 0) {
        contemptHash = 0x9E3779B97DE88147ULL * (U64)contempt;
    } else if (contempt < 0) {
        contemptHash = ~(0x9E3779B97DE88147ULL * ((U64)-contempt));
    } else
        contemptHash = 0;
}

void
TranspositionTable::insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore,
                           bool busy) {
    key ^= contemptHash;
    if (depth < 0) depth = 0;
    size_t idx0 = getIndex(key);
    TTEntry ent, tmp;
    size_t idx = idx0;
    for (int i = 0; i < 4; i++) {
        size_t idx1 = idx0 + i;
        tmp.load(table[idx1]);
        if (tmp.getKey() == key) {
            ent = tmp;
            idx = idx1;
            break;
        } else if (i == 0) {
            ent = tmp;
            idx = idx1;
        } else if (ent.betterThan(tmp, generation)) {
            ent = tmp;
            idx = idx1;
        }
    }
    bool doStore = true;
    if (!busy) {
        if ((ent.getKey() == key) && (ent.getDepth() > depth) && (ent.getType() == type)) {
            if (type == TType::T_EXACT)
                doStore = false;
            else if ((type == TType::T_GE) && (sm.score() <= ent.getScore(ply)))
                doStore = false;
            else if ((type == TType::T_LE) && (sm.score() >= ent.getScore(ply)))
                doStore = false;
        }
    }
    if (doStore) {
        if ((ent.getKey() != key) || (sm.from() != sm.to()))
            ent.setMove(sm);
        ent.setKey(key);
        ent.setScore(sm.score(), ply);
        ent.setDepth(depth);
        ent.setBusy(busy);
        ent.setGeneration((S8)generation);
        ent.setType(type);
        ent.setEvalScore(evalScore);
        ent.store(table[idx]);
    }
}

void
TranspositionTable::setBusy(const TTEntry& ent, int ply) {
    U64 key = ent.getKey();
    int type = ent.getType();
    int depth = ent.getDepth();
    int evalScore = ent.getEvalScore();
    Move sm;
    ent.getMove(sm);
    sm.setScore(ent.getScore(ply));
    insert(key, sm, type, ply, depth, evalScore, true);
}

void
TranspositionTable::extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv) {
    Position pos(rootPos);
    Move m(mFirst);
    UndoInfo ui;
    std::vector<U64> hashHistory;
    while (true) {
        pv.push_back(m);
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            break;
        hashHistory.push_back(pos.zobristHash());
        TTEntry ent;
        probe(pos.historyHash(), ent);
        if (ent.getType() == TType::T_EMPTY)
            break;
        ent.getMove(m);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool contains = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi] == m) {
                contains = true;
                break;
            }
        if  (!contains)
            break;
    }
}

std::string
TranspositionTable::extractPV(const Position& posIn) {
    std::string ret;
    Position pos(posIn);
    bool first = true;
    TTEntry ent;
    probe(pos.historyHash(), ent);
    UndoInfo ui;
    std::vector<U64> hashHistory;
    bool repetition = false;
    while (ent.getType() != TType::T_EMPTY) {
        Move m;
        ent.getMove(m);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool valid = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi] == m) {
                valid = true;
                break;
            }
        if  (!valid)
            break;
        if (repetition)
            break;
        if (!first)
            ret += ' ';
        if (ent.getType() == TType::T_LE)
            ret += '<';
        else if (ent.getType() == TType::T_GE)
            ret += '>';
        std::string moveStr = TextIO::moveToString(pos, m, false);
        ret += moveStr;
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            repetition = true;
        hashHistory.push_back(pos.zobristHash());
        probe(pos.historyHash(), ent);
        first = false;
    }
    return ret;
}

void
TranspositionTable::printStats(int rootDepth) const {
    int unused = 0;
    int thisGen = 0;
    std::vector<int> depHist;
    for (size_t i = 0; i < tableSize; i++) {
        TTEntry ent;
        ent.load(table[i]);
        if (ent.getType() == TType::T_EMPTY) {
            unused++;
        } else {
            if (ent.getGeneration() == generation)
                thisGen++;
            int d = ent.getDepth();
            while ((int)depHist.size() <= d)
                depHist.push_back(0);
            depHist[d]++;
        }
    }
    double w = 100.0 / tableSize;
    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << "hstat: d:" << rootDepth << " size:" << tableSize
       << " unused:" << unused << " (" << (unused*w) << "%)"
       << " thisGen:" << thisGen << " (" << (thisGen*w) << "%)" << std::endl;
    std::cout << ss.str();
    for (size_t i = 0; i < depHist.size(); i++) {
        int c = depHist[i];
        if (c > 0) {
            std::stringstream ss;
            ss.precision(2);
            ss << std::setw(4) << i
               << ' ' << std::setw(8) << c
               << " " << std::setw(6) << std::fixed << (c*w);
            std::cout << "hstat:" << ss.str() << std::endl;
        }
    }
}

int
TranspositionTable::getHashFull() const {
    if (tableSize < 1000)
        return 0;
    int hashFull = 0;
    for (int i = 0; i < 1000; i++) {
        TTEntry ent;
        ent.load(table[i]);
        if ((ent.getType() != TType::T_EMPTY) &&
            (ent.getGeneration() == generation))
            hashFull++;
    }
    return hashFull;
}

// --------------------------------------------------------------------------------

bool
TranspositionTable::updateTB(const Position& pos, RelaxedShared<S64>& maxTimeMillis) {
    if (BitBoard::bitCount(pos.occupiedBB()) > 4 ||
        pos.pieceTypeBB(Piece::WPAWN, Piece::BPAWN)) { // pos not suitable for TB generation
        if (tbGen && notUsedCnt++ > 3) {
            tbGen.reset();
            setUsedSize(tableSize);
            notUsedCnt = 0;
        }
        return tbGen != nullptr;
    }

    int score;
    if (tbGen && tbGen->probeDTM(pos, 0, score)) {
        notUsedCnt = 0;
        return true; // pos already in TB
    }

    static S64 requiredTime = 3000;
    if (maxTimeMillis >= 0 && maxTimeMillis < requiredTime)
        return false; // Not enough time to generate TB

    U64 ttSize = tableSize * sizeof(TTEntryStorage);
    const int tbSize = 5 * 1024 * 1024; // Max TB size, 10*64^3*2
    if (ttSize < tbSize + 2 * 1024 * 1024)
        return false;

    PieceCount pc;
    pc.nwq = BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN));
    pc.nwr = BitBoard::bitCount(pos.pieceTypeBB(Piece::WROOK));
    pc.nwb = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP));
    pc.nwn = BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT));
    pc.nbq = BitBoard::bitCount(pos.pieceTypeBB(Piece::BQUEEN));
    pc.nbr = BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK));
    pc.nbb = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP));
    pc.nbn = BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT));

    tbGen = make_unique<TBGenerator<TTStorage>>(ttStorage, pc);
    if (!tbGen->generate(maxTimeMillis, false)) {
        // Increase requiredTime unless computation was aborted
        S64 maxT = maxTimeMillis;
        if (maxT != 0)
            requiredTime = std::max(maxT, requiredTime) * 2;
        return false;
    }
    setUsedSize(tableSize - tbSize / sizeof(TTEntryStorage));
    notUsedCnt = 0;
    return true;
}

bool
TranspositionTable::probeDTM(const Position& pos, int ply, int& score) const {
    return tbGen && tbGen->probeDTM(pos, ply, score);
}
