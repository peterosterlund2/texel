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
 * matchbookcreator.cpp
 *
 *  Created on: May 15, 2016
 *      Author: petero
 */

#include "matchbookcreator.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "search.hpp"
#include "textio.hpp"
#include "gametree.hpp"
#include <unordered_set>

MatchBookCreator::MatchBookCreator() {

}

void
MatchBookCreator::createBook(int depth, int searchTime, std::ostream& os) {
    createBookLines(depth);

    std::vector<BookLine> lines;
    for (const auto& bl : bookLines)
        lines.push_back(bl.second);
    std::random_shuffle(lines.begin(), lines.end());
    evaluateBookLines(lines, searchTime, os);
}

void
MatchBookCreator::createBookLines(int depth) {
    std::vector<Move> moveList;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    createBookLines(pos, moveList, depth);

#if 0
    int i = 0;
    for (const auto& bl : bookLines) {
        std::cout << std::setw(5) << i;
        for (Move m : bl.second.moves)
            std::cout << ' ' << TextIO::moveToUCIString(m);
        std::cout << std::endl;
        i++;
    }
#endif
}

void
MatchBookCreator::createBookLines(Position& pos, std::vector<Move>& moveList, int depth) {
    if (depth <= 0) {
        if (bookLines.find(pos.historyHash()) == bookLines.end())
            bookLines[pos.historyHash()] = BookLine(moveList);
        return;
    }
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    UndoInfo ui;
    for (int mi = 0; mi < moves.size; mi++) {
        const Move& m = moves[mi];
        pos.makeMove(m, ui);
        moveList.push_back(m);
        createBookLines(pos, moveList, depth - 1);
        moveList.pop_back();
        pos.unMakeMove(m, ui);
    }
}

void
MatchBookCreator::evaluateBookLines(std::vector<BookLine>& lines, int searchTime,
                                    std::ostream& os) {
    const int nLines = lines.size();
    TranspositionTable tt(28);
    ParallelData pd(tt);
    std::shared_ptr<Evaluate::EvalHashTables> et;

#pragma omp parallel for schedule(dynamic) default(none) shared(lines,tt,pd,searchTime,os) private(et)
    for (int i = 0; i < nLines; i++) {
        BookLine& bl = lines[i];

        KillerTable kt;
        History ht;
        TreeLogger treeLog;
        if (!et)
            et = Evaluate::getEvalHashTables();

        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        UndoInfo ui;
        std::vector<U64> posHashList(200 + bl.moves.size());
        int posHashListSize = 0;
        for (const Move& m : bl.moves) {
            posHashList[posHashListSize++] = pos.zobristHash();
            pos.makeMove(m, ui);
            if (pos.getHalfMoveClock() == 0)
                posHashListSize = 0;
        }

        MoveList legalMoves;
        MoveGen::pseudoLegalMoves(pos, legalMoves);
        MoveGen::removeIllegal(pos, legalMoves);

        Search::SearchTables st(tt, kt, ht, *et);
        Search sc(pos, posHashList, posHashListSize, st, pd, nullptr, treeLog);
        sc.timeLimit(searchTime, searchTime);

        int maxDepth = -1;
        S64 maxNodes = -1;
        bool verbose = false;
        int maxPV = 1;
        bool onlyExact = true;
        int minProbeDepth = 1;
        Move bestMove = sc.iterativeDeepening(legalMoves, maxDepth, maxNodes, verbose, maxPV,
                                              onlyExact, minProbeDepth);
        int score = bestMove.score();
        if (!pos.isWhiteMove())
            score = -score;
        bl.score = score;
#pragma omp critical
        {
            os << std::setw(5) << i << ' ' << std::setw(6) << score;
            for (Move m : bl.moves)
                os << ' ' << TextIO::moveToUCIString(m);
            os << std::endl;
        }
    }
}

void
MatchBookCreator::countUniq(const std::string& pgnFile, std::ostream& os) {
    std::ifstream is(pgnFile);
    PgnReader reader(is);
    std::vector<std::unordered_set<U64>> uniqPositions;
    GameTree gt;
    int nGames = 0;
    try {
        std::cerr << "Reading games" << std::endl;
        while (reader.readPGN(gt)) {
            nGames++;
            GameNode gn = gt.getRootNode();
            int ply = 0;
            while (true) {
                while ((int)uniqPositions.size() <= ply)
                    uniqPositions.push_back(std::unordered_set<U64>());
                uniqPositions[ply].insert(gn.getPos().zobristHash());
                if (gn.nChildren() == 0)
                    break;
                gn.goForward(0);
                ply++;
            }
        }
        std::cerr << "Counting uniq" << std::endl;

        std::unordered_set<U64> uniq;
        if (uniqPositions.size() > 0)
            uniq.insert(uniqPositions[0].begin(), uniqPositions[0].end());
        for (size_t i = 1; i < uniqPositions.size(); i++) {
            int u0 = uniq.size();
            uniq.insert(uniqPositions[i].begin(), uniqPositions[i].end());
            int u1 = uniq.size();
            os << std::setw(3) << i << ' ' << u1 - u0 << std::endl;
        }
    } catch (...) {
        std::cerr << "Error parsing game " << nGames << std::endl;
        throw;
    }
}
