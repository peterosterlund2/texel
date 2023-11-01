/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * chesstool.cpp
 *
 *  Created on: Dec 24, 2013
 *      Author: petero
 */

#include "chesstool.hpp"
#include "search.hpp"
#include "clustertt.hpp"
#include "history.hpp"
#include "killerTable.hpp"
#include "textio.hpp"
#include "gametree.hpp"
#include "threadpool.hpp"
#include "syzygy/rtb-probe.hpp"
#include "tbprobe.hpp"
#include "tbpath.hpp"
#include "stloutput.hpp"
#include "timeUtil.hpp"
#include "logger.hpp"
#include "random.hpp"
#include "posutil.hpp"
#include "nnutil.hpp"

#include <queue>
#include <unordered_set>
#include <stdio.h>


// Static move ordering parameters
DECLARE_PARAM(moEvalWeight, 96, -128, 128, useUciParam || true);
DECLARE_PARAM(moHangPenalty1, -1, -128, 128, useUciParam || true);
DECLARE_PARAM(moHangPenalty2, -30, -128, 128, useUciParam || true);
DECLARE_PARAM(moSeeBonus, 104, -128, 128, useUciParam || true);

DEFINE_PARAM(moEvalWeight);
DEFINE_PARAM(moHangPenalty1);
DEFINE_PARAM(moHangPenalty2);
DEFINE_PARAM(moSeeBonus);

ScoreToProb::ScoreToProb(double pawnAdvantage0)
    : pawnAdvantage(pawnAdvantage0) {
    for (int i = 0; i < MAXCACHE; i++) {
        cache[i] = computeProb(i);
        logCacheP[i] = log(getProb(i));
        logCacheN[i] = log(getProb(-i));
    }
}

double
ScoreToProb::getProb(int score) const {
    bool neg = false;
    if (score < 0) {
        score = -score;
        neg = true;
    }
    double ret;
    if (score < MAXCACHE)
        ret = cache[score];
    else
        ret = computeProb(score);
    if (neg)
        ret = 1 - ret;
    return ret;
}

double
ScoreToProb::getLogProb(int score) const {
    if ((score >= 0) && (score < MAXCACHE))
        return logCacheP[score];
    if ((score < 0) && (score > -MAXCACHE))
        return logCacheN[-score];
    return log(getProb(score));
}

// --------------------------------------------------------------------------------

static bool strContains(const std::string& str, const std::string& sub) {
    return str.find(sub) != std::string::npos;
}

static int
findLine(const std::string& start, const std::string& contain, const std::vector<std::string>& lines) {
    for (int i = 0; i < (int)lines.size(); i++) {
        const std::string& line = lines[i];
        if (startsWith(line, start) && strContains(line, contain))
            return i;
    }
    return -1;
}

static std::vector<std::string> splitLines(const std::string& lines) {
    std::vector<std::string> ret;
    int start = 0;
    for (int i = 0; i < (int)lines.size(); i++) {
        if (lines[i] == '\n') {
            ret.push_back(lines.substr(start, i - start));
            start = i + 1;
        }
    }
    return ret;
}

static void
splitString(const std::string& line, const std::string& delim, std::vector<std::string>& fields) {
    size_t start = 0;
    while (true) {
        size_t n = line.find(delim, start);
        if (n == std::string::npos)
            break;
        fields.push_back(line.substr(start, n - start));
        start = n + delim.length();
    }
    if (start < line.length())
        fields.push_back(line.substr(start));
}

// --------------------------------------------------------------------------------

ChessTool::ChessTool(bool useEntropyErr, bool optMoveOrder, bool useSearchScore,
                     int nWorkers)
    : useEntropyErrorFunction(useEntropyErr),
      optimizeMoveOrdering(optMoveOrder),
      useSearchScore(useSearchScore),
      nWorkers(nWorkers) {
    moEvalWeight.registerParam("MoveOrderEvalWeight", Parameters::instance());
    moHangPenalty1.registerParam("MoveOrderHangPenalty1", Parameters::instance());
    moHangPenalty2.registerParam("MoveOrderHangPenalty2", Parameters::instance());
    moSeeBonus.registerParam("MoveOrderSeeBonus", Parameters::instance());
}

void
ChessTool::setupTB() {
    TBPath::setDefaultTBPaths();
}

std::vector<std::string>
ChessTool::readFile(const std::string& fname) {
    std::ifstream is(fname);
    return readStream(is);
}

std::vector<std::string>
ChessTool::readStream(std::istream& is) {
    std::vector<std::string> ret;
    while (true) {
        std::string line;
        std::getline(is, line);
        if (!is || is.eof())
            break;
        ret.push_back(line);
    }
    return ret;
}

static void writeFEN(std::ostream& os, const std::string& fen,
                     double result, int searchScore, int qScore, int gameNo,
                     const std::string& extra = "") {
    os << fen << " : "
       << result << " : "
       << searchScore << " : "
       << qScore << " : "
       << gameNo;
    if (!extra.empty())
        os << " : " << extra;
    os << '\n';
}

void
ChessTool::pgnToFen(std::istream& is, int everyNth, bool includeUnScored) {
    std::vector<U64> nullHist(SearchConst::MAX_SEARCH_DEPTH * 2);
    TranspositionTable tt(512*1024);
    Notifier notifier;
    ThreadCommunicator comm(nullptr, tt, notifier, false);
    KillerTable kt;
    History ht;
    auto et = Evaluate::getEvalHashTables();
    Search::SearchTables st(comm.getCTT(), kt, ht, *et);
    TreeLogger treeLog;
    Random rnd;

    Position pos;
    const int mate0 = SearchConst::MATE0;
    Search sc(pos, nullHist, 0, st, comm, treeLog);

    PgnReader reader(is);
    GameTree gt;
    int gameNo = 0;
    while (reader.readPGN(gt)) {
        gameNo++;
        GameTree::Result result = gt.getResult();
        if (result == GameTree::UNKNOWN)
            continue;
        double rScore = 0;
        switch (result) {
        case GameTree::WHITE_WIN: rScore = 1.0; break;
        case GameTree::BLACK_WIN: rScore = 0.0; break;
        case GameTree::DRAW:      rScore = 0.5; break;
        default: break;
        }
        GameNode gn = gt.getRootNode();
        while (true) {
            pos = gn.getPos();
            std::string fen = TextIO::toFEN(pos);
            if (gn.nChildren() == 0)
                break;
            gn.goForward(0);
            std::string move = TextIO::moveToUCIString(gn.getMove());
            std::string comment = gn.getComment();
            int commentScore = 0;
            if (!getCommentScore(comment, commentScore) && !includeUnScored)
                continue;

            if (everyNth > 1 && rnd.nextInt(everyNth) != 0)
                continue;

            sc.init(pos, nullHist, 0);
            int score = sc.quiesce(-mate0, mate0, 0, 0, MoveGen::inCheck(pos));
            if (!pos.isWhiteMove()) {
                score = -score;
                commentScore = -commentScore;
            }
            writeFEN(std::cout, fen, rScore, commentScore, score, gameNo, move);
        }
    }
    std::cout << std::flush;
}

void
ChessTool::fenToPgn(std::istream& is) {
    std::vector<std::string> lines = readStream(is);
    for (const std::string& line : lines) {
        Position pos(TextIO::readFEN(line));
        writePGN(pos);
    }
}

void
ChessTool::movesToFen(std::istream& is) {
    std::vector<std::string> lines = readStream(is);
    Position startPos(TextIO::readFEN(TextIO::startPosFEN));
    std::vector<std::string> words;
    for (const std::string& line : lines) {
        Position pos(startPos);
        UndoInfo ui;
        words.clear();
        splitString(line, words);
        bool inSequence = true;
        bool fenPrinted = false;
        for (const std::string& word : words) {
            if (inSequence) {
                Move move = TextIO::stringToMove(pos, word);
                if (move.isEmpty()) {
                    inSequence = false;
                    std::cout << TextIO::toFEN(pos);
                    fenPrinted = true;
                } else {
                    pos.makeMove(move, ui);
                }
            }
            if (!inSequence) {
                std::cout << ' ' << word;
            }
        }
        if (!fenPrinted)
            std::cout << TextIO::toFEN(pos);
        std::cout << std::endl;
    }
}

void
ChessTool::pawnAdvTable(std::istream& is) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);
    for (int pawnAdvantage = 1; pawnAdvantage <= 400; pawnAdvantage += 1) {
        ScoreToProb sp(pawnAdvantage);
        double avgErr = computeAvgError(positions, sp);
        std::stringstream ss;
        ss << "pa:" << pawnAdvantage << " err:" << std::setprecision(14) << avgErr;
        std::cout << ss.str() << std::endl;
    }
}

// --------------------------------------------------------------------------------

void
ChessTool::filterScore(std::istream& is, int scLimit, double prLimit) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    ScoreToProb sp;
    Position pos;
    for (const PositionInfo& pi : positions) {
        double p1 = sp.getProb(pi.searchScore);
        double p2 = sp.getProb(pi.qScore);
        if ((std::abs(p1 - p2) < prLimit) && (std::abs(pi.searchScore - pi.qScore) < scLimit)) {
            pos.deSerialize(pi.posData);
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << pi.result << " : " << pi.searchScore << " : " << pi.qScore
                      << " : " << pi.gameNo << '\n';
        }
    }
    std::cout << std::flush;
}

static int nPieces(const Position& pos, Piece::Type piece) {
    return BitBoard::bitCount(pos.pieceTypeBB(piece));
}

static bool isMatch(int v1, bool compare, int v2) {
    return !compare || (v1 == v2);
}

void
ChessTool::filterMtrlBalance(std::istream& is, bool minorEqual,
                             const std::vector<std::pair<bool,int>>& mtrlPattern) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    Position pos;
    int mtrlDiff[5];
    for (const PositionInfo& pi : positions) {
        pos.deSerialize(pi.posData);
        mtrlDiff[0] = nPieces(pos, Piece::WQUEEN)  - nPieces(pos, Piece::BQUEEN);
        mtrlDiff[1] = nPieces(pos, Piece::WROOK)   - nPieces(pos, Piece::BROOK);
        int nComp;
        if (minorEqual) {
            mtrlDiff[2] = nPieces(pos, Piece::WBISHOP) - nPieces(pos, Piece::BBISHOP) +
                          nPieces(pos, Piece::WKNIGHT) - nPieces(pos, Piece::BKNIGHT);
            mtrlDiff[3] = nPieces(pos, Piece::WPAWN)   - nPieces(pos, Piece::BPAWN);
            nComp = 4;
        } else {
            mtrlDiff[2] = nPieces(pos, Piece::WBISHOP) - nPieces(pos, Piece::BBISHOP);
            mtrlDiff[3] = nPieces(pos, Piece::WKNIGHT) - nPieces(pos, Piece::BKNIGHT);
            mtrlDiff[4] = nPieces(pos, Piece::WPAWN)   - nPieces(pos, Piece::BPAWN);
            nComp = 5;
        }
        bool inc1 = true, inc2 = true;
        for (int i = 0; i < nComp; i++) {
            if (!isMatch(mtrlDiff[i], mtrlPattern[i].first, mtrlPattern[i].second))
                inc1 = false;
            if (!isMatch(mtrlDiff[i], mtrlPattern[i].first, -mtrlPattern[i].second))
                inc2 = false;
        }
        int sign = 1;
        if (inc2 && !inc1) {
            pos = PosUtil::swapColors(pos);
            sign = -1;
        }
        if (inc1 || inc2) {
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << ((sign>0)?pi.result:(1-pi.result)) << " : "
                      << pi.searchScore * sign << " : " << pi.qScore * sign
                      << " : " << pi.gameNo << '\n';
        }
    }
    std::cout << std::flush;
}

void
ChessTool::filterTotalMaterial(std::istream& is, bool minorEqual,
                               const std::vector<std::pair<bool,int>>& mtrlPattern) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    Position pos;
    for (const PositionInfo& pi : positions) {
        pos.deSerialize(pi.posData);
        int wQ = nPieces(pos, Piece::WQUEEN);
        int wR = nPieces(pos, Piece::WROOK);
        int wB = nPieces(pos, Piece::WBISHOP);
        int wN = nPieces(pos, Piece::WKNIGHT);
        int wP = nPieces(pos, Piece::WPAWN);
        int bQ = nPieces(pos, Piece::BQUEEN);
        int bR = nPieces(pos, Piece::BROOK);
        int bB = nPieces(pos, Piece::BBISHOP);
        int bN = nPieces(pos, Piece::BKNIGHT);
        int bP = nPieces(pos, Piece::BPAWN);

        bool inc1, inc2;
        if (minorEqual) {
            inc1 = isMatch(wQ,    mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(wR,    mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(wB+wN, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(wP,    mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(bQ,    mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(bR,    mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(bB+bN, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(bP,    mtrlPattern[7].first, mtrlPattern[7].second);
            inc2 = isMatch(bQ,    mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(bR,    mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(bB+bN, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(bP,    mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(wQ,    mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(wR,    mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(wB+wN, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(wP,    mtrlPattern[7].first, mtrlPattern[7].second);
        } else {
            inc1 = isMatch(wQ, mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(wR, mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(wB, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(wN, mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(wP, mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(bQ, mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(bR, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(bB, mtrlPattern[7].first, mtrlPattern[7].second) &&
                   isMatch(bN, mtrlPattern[8].first, mtrlPattern[8].second) &&
                   isMatch(bP, mtrlPattern[9].first, mtrlPattern[9].second);
            inc2 = isMatch(bQ, mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(bR, mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(bB, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(bN, mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(bP, mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(wQ, mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(wR, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(wB, mtrlPattern[7].first, mtrlPattern[7].second) &&
                   isMatch(wN, mtrlPattern[8].first, mtrlPattern[8].second) &&
                   isMatch(wP, mtrlPattern[9].first, mtrlPattern[9].second);
        }
        int sign = 1;
        if (inc2 && !inc1) {
            pos = PosUtil::swapColors(pos);
            sign = -1;
        }
        if (inc1 || inc2) {
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << ((sign>0)?pi.result:(1-pi.result)) << " : "
                      << pi.searchScore * sign << " : " << pi.qScore * sign
                      << " : " << pi.gameNo << '\n';
        }
    }
    std::cout << std::flush;
}

void
ChessTool::outliers(std::istream& is, int threshold) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);
    Position pos;
    for (const PositionInfo& pi : positions) {
        if (((pi.qScore >=  threshold) && (pi.result < 1.0)) ||
            ((pi.qScore <= -threshold) && (pi.result > 0.0))) {
            pos.deSerialize(pi.posData);
            std::string fen = TextIO::toFEN(pos);
            writeFEN(std::cout, fen, pi.result, pi.searchScore, pi.qScore, pi.gameNo);
        }
    }
    std::cout << std::flush;
}

#if !_MSC_VER
void
ChessTool::computeSearchScores(std::istream& is, const std::string& script) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    int nPos = positions.size();

    std::atomic<bool> error(false);

    const int batchSize = 1000;
    struct Result {
        int beginIdx;
        int endIdx;
        std::array<int,batchSize> scores;
    };
    ThreadPool<Result> pool(nWorkers);
    for (int i = 0; i < nPos; i += batchSize) {
        Result r;
        r.beginIdx = i;
        r.endIdx = std::min(i + batchSize, nPos);
        auto func = [&script,&positions,r,&error](int workerNo) mutable {
            if (error)
                return r;
            std::string cmdLine = "\"" + script + "\"";
            cmdLine += " " + num2Str(workerNo);
            Position pos;
            for (int i = r.beginIdx; i < r.endIdx; i++) {
                pos.deSerialize(positions[i].posData);
                std::string fen = TextIO::toFEN(pos);
                cmdLine += " \"" + fen + "\"";
            }
            std::shared_ptr<FILE> f(popen(cmdLine.c_str(), "r"),
                                    [](FILE* f) { pclose(f); });
            std::string s;
            char buf[256];
            while (fgets(buf, sizeof(buf), f.get()))
                s += buf;
            std::vector<std::string> lines = splitLines(s);
            int nLines = lines.size();
            if (nLines != r.endIdx - r.beginIdx) {
                error.store(true);
                std::cerr << "Script failed: " << s << std::endl;
                return r;
            }
            for (int i = 0; i < nLines; i++) {
                if (!str2Num(lines[i], r.scores[i])) {
                    error.store(true);
                    std::cerr << "Not a number: " << lines[i] << std::endl;
                    return r;
                }
            }
            return r;
        };
        pool.addTask(func);
    }

    const int nBatches = (nPos - 1) / batchSize + 1;
    std::vector<bool> finished(nBatches, false);
    Position pos;
    for (int b = 0; b < nBatches; b++) {
        if (finished[b]) {
            int i0 = b * batchSize;
            int i1 = std::min((b + 1) * batchSize, nPos);
            for (int i = i0; i < i1; i++) {
                const PositionInfo& pi = positions[i];
                pos.deSerialize(pi.posData);
                std::string fen = TextIO::toFEN(pos);
                writeFEN(std::cout, fen, pi.result, pi.searchScore, pi.qScore, pi.gameNo);
            }
            std::cout << std::flush;
        } else {
            Result r;
            if (!pool.getResult(r)) {
                std::cerr << "No result available" << std::endl;
                exit(2);
            }
            if (error)
                break;
            int nScores = r.endIdx - r.beginIdx;

            for (int i = 0; i < nScores; i++) {
                pos.deSerialize(positions[r.beginIdx + i].posData);
                int c = pos.isWhiteMove() ? 1 : -1;
                positions[r.beginIdx + i].searchScore = r.scores[i] * c;
            }
            finished[r.beginIdx / batchSize] = true;
            b--;
        }
    }
}
#endif

void
ChessTool::computeQSearchPos(std::istream& is) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    int nPos = positions.size();

    TranspositionTable tt(512*1024);
    Notifier notifier;
    ThreadCommunicator comm(nullptr, tt, notifier, false);

    struct ThreadData {
        std::vector<U64> nullHist = std::vector<U64>(SearchConst::MAX_SEARCH_DEPTH * 2);
        KillerTable kt;
        History ht;
        std::shared_ptr<Evaluate::EvalHashTables> et;
        TreeLogger treeLog;
        Position pos;
    };
    std::vector<ThreadData> tdVec(nWorkers);

    const int batchSize = 5000;
    ThreadPool<int> pool(nWorkers);
    for (int i = 0; i < nPos; i += batchSize) {
        int beginIdx = i;
        int endIdx = std::min(i + batchSize, nPos);
        auto func = [&positions,&comm,&tdVec,beginIdx,endIdx](int workerNo) mutable {
            ThreadData& td = tdVec[workerNo];
            if (!td.et)
                td.et = Evaluate::getEvalHashTables();
            Search::SearchTables st(comm.getCTT(), td.kt, td.ht, *td.et);

            const int mate0 = SearchConst::MATE0;
            Position& pos = td.pos;
            Search sc(pos, td.nullHist, 0, st, comm, td.treeLog);

            for (int i = beginIdx; i < endIdx; i++) {
                PositionInfo& pi = positions[i];
                pos.deSerialize(pi.posData);
                sc.init(pos, td.nullHist, 0);
                auto ret = sc.quiescePos(-mate0, mate0, 0, 0, MoveGen::inCheck(pos));
                int score = ret.first;
                if (!pos.isWhiteMove())
                    score = -score;
                pi.qScore = score;
                pi.posData = ret.second;
                pi.searchScore = 0; // Original search score was for a possibly different position
            }
            return 0;
        };
        pool.addTask(func);
    }
    pool.getAllResults([](int){});

    Position pos;
    for (int i = 0; i < nPos; i++) {
        const PositionInfo& pi = positions[i];
        pos.deSerialize(pi.posData);
        std::string fen = TextIO::toFEN(pos);
        writeFEN(std::cout, fen, pi.result, pi.searchScore, pi.qScore, pi.gameNo);
    }
}

void
ChessTool::searchPositions(std::istream& is, int baseTime, int increment) {
    std::mutex mutex;
    std::atomic<bool> abort;
    ThreadPool<int> pool(nWorkers);
    int nLines = 0;
    for (int i = 0; i < nWorkers; i++) {
        auto func = [&is, baseTime, increment, &mutex, &abort, &nLines](int workerNo) mutable {
            TranspositionTable tt(8*1024*1024);
            Notifier notifier;
            ThreadCommunicator comm(nullptr, tt, notifier, false);
            std::vector<U64> nullHist(SearchConst::MAX_SEARCH_DEPTH * 2);
            KillerTable kt;
            History ht;
            Evaluate::EvalHashTables et;
            TreeLogger treeLog;
            Search::SearchTables st(comm.getCTT(), kt, ht, et);
            Position pos;
            int minProbeDepth = UciParams::minProbeDepth->getIntPar();

            while (true) {
                if (abort)
                    break;
                { // Read one line from "is"
                    std::string line;
                    {
                        std::lock_guard<std::mutex> L(mutex);
                        std::getline(is, line);
                        if (!is || is.eof())
                            break;
                        nLines++;
                        if (nLines % 1000 == 0)
                            std::cout << "nLines: " << nLines << std::endl;
                    }
                    std::vector<std::string> fields;
                    splitString(line, " : ", fields);
                    try {
                        pos = TextIO::readFEN(fields[0]);
                    } catch (ChessParseError&) {
                        abort = true;
                        throw;
                    }
                }

                MoveList moves;
                MoveGen::pseudoLegalMoves(pos, moves);
                MoveGen::removeIllegal(pos, moves);
                if (moves.size == 0)
                    continue;

                tt.nextGeneration();
                ht.init();

                const int movesToGo = 35;
                int timeLeft = baseTime;
                int searchTime;
                int i = 0;
                do {
                    searchTime = (timeLeft + increment * (movesToGo - 1)) / movesToGo;
                    timeLeft += increment - searchTime;
                } while (++i < pos.getFullMoveCounter());

                U64 seed = hashU64(currentTimeMillis() + hashU64(workerNo));

                Search sc(pos, nullHist, 0, st, comm, treeLog);
                sc.init(pos, nullHist, 0);
                sc.setStrength(1000, seed, 0);
                sc.timeLimit(searchTime, 3*searchTime, -1);
                sc.iterativeDeepening(moves, -1, -1, 1, false, minProbeDepth);
            }
            return 0;
        };
        pool.addTask(func);
    }
    pool.getAllResults([](int){});
}

void
ChessTool::fen2bin(std::istream& is, const std::string& outFile, bool useResult,
                   bool noInCheck, double prLimit) {
    using Record = NNUtil::Record;
    std::ofstream os;
    os.open(outFile.c_str(), std::ios_base::out | std::ios_base::binary);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    ScoreToProb sp;
    Position pos;
    Record r;
    std::string line;
    while (true) {
        std::getline(is, line);
        if (!is || is.eof())
            break;

        std::vector<std::string> fields;
        splitString(line, " : ", fields);

        pos = TextIO::readFEN(fields[0]);

        if (noInCheck && MoveGen::inCheck(pos))
            continue;
        if (prLimit >= 0) {
            int searchScore;
            int qScore;
            if (!str2Num(fields[2], searchScore) || !str2Num(fields[3], qScore))
                throw ChessParseError("Invalid score: " + line);
            double p1 = sp.getProb(searchScore);
            double p2 = sp.getProb(qScore);
            if (std::abs(p1 - p2) > prLimit)
                continue;
        }

        int score;
        if (!useResult) {
            int searchScore;
            if (!str2Num(fields[2], searchScore))
                throw ChessParseError("Invalid score: " + line);
            score = searchScore;
        } else {
            double gameResult;
            if (!str2Num(fields[1], gameResult))
                gameResult = -1;
            if (gameResult == 0.0) {
                score = -10000;
            } else if (gameResult == 0.5) {
                score = 0;
            } else if (gameResult == 1.0) {
                score = 10000;
            } else {
                throw ChessParseError("Invalid game result: " + line);
            }
        }
        NNUtil::posToRecord(pos, score, r);
        os.write((const char*)&r, sizeof(Record));
    }
}

void
ChessTool::evalEffect(std::istream& is, const std::vector<ParamValue>& parValues) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);

    for (PositionInfo& pi : positions)
        pi.searchScore = pi.qScore;

    Parameters& uciPars = Parameters::instance();
    for (const ParamValue& pv : parValues)
        uciPars.set(pv.name, num2Str(pv.value));

    qEval(positions);
    ScoreToProb sp;
    Position pos;
    for (const PositionInfo& pi : positions) {
        if (pi.qScore == pi.searchScore)
            continue;

        double evErr0 = std::abs(sp.getProb(pi.searchScore) - pi.result);
        double evErr1 = std::abs(sp.getProb(pi.qScore)      - pi.result);
        double improvement = evErr0 - evErr1;

        std::stringstream ss;
        ss.precision(6);
        ss << std::fixed << improvement;

        pos.deSerialize(pi.posData);
        std::string fen = TextIO::toFEN(pos);
        writeFEN(std::cout, fen, pi.result, pi.searchScore, pi.qScore, pi.gameNo, ss.str());
    }
    std::cout << std::flush;
}

// --------------------------------------------------------------------------------

void
ChessTool::paramEvalRange(std::istream& is, ParamDomain& pd) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    ScoreToProb sp;
    double bestVal = 1e100;
    for (int i = pd.minV; i <= pd.maxV; i += pd.step) {
        Parameters::instance().set(pd.name, num2Str(i));
        double avgErr = computeObjective(positions, sp);
        bool best = avgErr < bestVal;
        bestVal = std::min(bestVal, avgErr);
        std::stringstream ss;
        ss << "i:" << i << " err:" << std::setprecision(14) << avgErr << (best?" *":"");
        std::cout << ss.str() << std::endl;
    }
}

struct PrioParam {
    explicit PrioParam(ParamDomain& pd0) : priority(1), pd(&pd0) {}
    double priority;
    ParamDomain* pd;
    bool operator<(const PrioParam& o) const {
        return priority < o.priority;
    }
};

// --------------------------------------------------------------------------------

#ifdef USE_ARMADILLO
void
ChessTool::accumulateATA(std::vector<PositionInfo>& positions, int beg, int end,
                         const ScoreToProb& sp,
                         std::vector<ParamDomain>& pdVec,
                         arma::mat& aTa, arma::mat& aTb,
                         arma::mat& ePos, arma::mat& eNeg) {
    Parameters& uciPars = Parameters::instance();
    const int M = end - beg;
    const int N = pdVec.size();
    const double w = 1.0 / positions.size();

    arma::mat b(M, 1);
    qEval(positions, beg, end);
    for (int i = beg; i < end; i++)
        b.at(i-beg,0) = positions[i].getErr(sp) * w;

    arma::mat A(M, N);
    for (int j = 0; j < N; j++) {
        ParamDomain& pd = pdVec[j];
        std::cout << "j:" << j << " beg:" << beg << " name:" << pd.name << std::endl;
        const int v0 = pd.value;
        const int vPos = std::min(pd.maxV, pd.value + 1);
        const int vNeg = std::max(pd.minV, pd.value - 1);
        assert(vPos > vNeg);

        uciPars.set(pd.name, num2Str(vPos));
        qEval(positions, beg, end);
        double EPos = 0;
        for (int i = beg; i < end; i++) {
            const double err = positions[i].getErr(sp);
            A.at(i-beg,j) = err;
            EPos += err * err;
        }
        ePos.at(j, 0) += sqrt(EPos * w);

        uciPars.set(pd.name, num2Str(vNeg));
        qEval(positions, beg, end);
        double ENeg = 0;
        for (int i = beg; i < end; i++) {
            const double err = positions[i].getErr(sp);
            A.at(i-beg,j) = (A.at(i-beg,j) - err) / (vPos - vNeg) * w;
            ENeg += err * err;
        }
        eNeg.at(j, 0) += sqrt(ENeg * w);

        uciPars.set(pd.name, num2Str(v0));
    }

    aTa += A.t() * A;
    aTb += A.t() * b;
}

void
ChessTool::gnOptimize(std::istream& is, std::vector<ParamDomain>& pdVec) {
    double t0 = currentTime();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    const int nPos = positions.size();

    const int N = pdVec.size();
    arma::mat bestP(N, 1);
    for (int i = 0; i < N; i++)
        bestP.at(i, 0) = pdVec[i].value;
    ScoreToProb sp;
    double bestAvgErr = computeAvgError(positions, sp, pdVec, bestP);
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    const int chunkSize = 250000000 / N;

    while (true) {
        arma::mat aTa(N, N);  aTa.fill(0.0);
        arma::mat aTb(N, 1);  aTb.fill(0.0);
        arma::mat ePos(N, 1); ePos.fill(0.0);
        arma::mat eNeg(N, 1); eNeg.fill(0.0);

        for (int i = 0; i < nPos; i += chunkSize) {
            const int end = std::min(nPos, i + chunkSize);
            accumulateATA(positions, i, end, sp, pdVec, aTa, aTb, ePos, eNeg);
        }

        arma::mat delta = pinv(aTa) * aTb;
        bool improved = false;
        for (double alpha = 1.0; alpha >= 0.25; alpha /= 2) {
            arma::mat newP = bestP - delta * alpha;
            for (int i = 0; i < N; i++)
                newP.at(i, 0) = clamp((int)round(newP.at(i, 0)), pdVec[i].minV, pdVec[i].maxV);
            double avgErr = computeAvgError(positions, sp, pdVec, newP);
            for (int i = 0; i < N; i++) {
                ParamDomain& pd = pdVec[i];
                std::stringstream ss;
                ss << pd.name << ' ' << newP.at(i, 0) << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                std::cout << ss.str() << std::endl;
            }
            if (avgErr < bestAvgErr) {
                bestP = newP;
                bestAvgErr = avgErr;
                improved = true;
                break;
            }
        }
        if (!improved)
            break;
    }
    double t1 = currentTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

double
ChessTool::computeAvgError(std::vector<PositionInfo>& positions, const ScoreToProb& sp,
                           const std::vector<ParamDomain>& pdVec, arma::mat& pdVal) {
    assert(pdVal.n_rows == pdVec.size());
    assert(pdVal.n_cols == 1);

    Parameters& uciPars = Parameters::instance();
    for (int i = 0; i < (int)pdVal.n_rows; i++)
        uciPars.set(pdVec[i].name, num2Str(pdVal.at(i, 0)));
    qEval(positions);
    return computeAvgError(positions, sp);
}
#endif

// --------------------------------------------------------------------------------

void
ChessTool::localOptimize(std::istream& is, std::vector<ParamDomain>& pdVec) {
    double t0 = currentTime();
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    std::priority_queue<PrioParam> queue;
    for (ParamDomain& pd : pdVec)
        queue.push(PrioParam(pd));

    ScoreToProb sp;
    double bestAvgErr = computeObjective(positions, sp);
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    std::vector<PrioParam> tried;
    while (!queue.empty()) {
        PrioParam pp = queue.top(); queue.pop();
        ParamDomain& pd = *pp.pd;
        std::cout << pd.name << " prio:" << pp.priority << " q:" << queue.size()
                  << " min:" << pd.minV << " max:" << pd.maxV << " val:" << pd.value << std::endl;
        double oldBest = bestAvgErr;
        bool improved = false;
        for (int d = 0; d < 2; d++) {
            while (true) {
                const int newValue = pd.value + (d ? -1 : 1);
                if ((newValue < pd.minV) || (newValue > pd.maxV))
                    break;

                uciPars.set(pd.name, num2Str(newValue));
                double avgErr = computeObjective(positions, sp);
                uciPars.set(pd.name, num2Str(pd.value));

                std::stringstream ss;
                ss << pd.name << ' ' << newValue << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                std::cout << ss.str() << std::endl;

                if (avgErr >= bestAvgErr)
                    break;
                bestAvgErr = avgErr;
                pd.value = newValue;
                uciPars.set(pd.name, num2Str(pd.value));
                improved = true;
            }
            if (improved)
                break;
        }
        double improvement = oldBest - bestAvgErr;
        std::cout << pd.name << " improvement:" << improvement << std::endl;
        pp.priority = pp.priority * 0.1 + improvement * 0.9;
        if (improved) {
            for (PrioParam& pp2 : tried)
                queue.push(pp2);
            tried.clear();
        }
        tried.push_back(pp);
    }

    double t1 = currentTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

static void
updateMinMax(std::map<int,double>& funcValues, int bestV, int& minV, int& maxV) {
    auto it = funcValues.find(bestV);
    assert(it != funcValues.end());
    if (it != funcValues.begin()) {
        auto it2 = it; --it2;
        int nextMinV = it2->first;
        minV = std::max(minV, nextMinV);
    }
    auto it2 = it; ++it2;
    if (it2 != funcValues.end()) {
        int nextMaxV = it2->first;
        maxV = std::min(maxV, nextMaxV);
    }
}

static int
estimateMin(std::map<int,double>& funcValues, int bestV, int minV, int maxV) {
    return (minV + maxV) / 2;
}

void
ChessTool::localOptimize2(std::istream& is, std::vector<ParamDomain>& pdVec) {
    double t0 = currentTime();
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    std::priority_queue<PrioParam> queue;
    for (ParamDomain& pd : pdVec)
        queue.push(PrioParam(pd));

    ScoreToProb sp;
    double bestAvgErr = computeObjective(positions, sp);
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    std::vector<PrioParam> tried;
    while (!queue.empty()) {
        PrioParam pp = queue.top(); queue.pop();
        ParamDomain& pd = *pp.pd;
        std::cout << pd.name << " prio:" << pp.priority << " q:" << queue.size()
                  << " min:" << pd.minV << " max:" << pd.maxV << " val:" << pd.value << std::endl;
        double oldBest = bestAvgErr;

        std::map<int, double> funcValues;
        funcValues[pd.value] = bestAvgErr;
        int minV = pd.minV;
        int maxV = pd.maxV;
        while (true) {
            bool improved = false;
            for (int d = 0; d < 2; d++) {
                const int newValue = pd.value + (d ? -1 : 1);
                if ((newValue < minV) || (newValue > maxV))
                    continue;
                if (funcValues.count(newValue) == 0) {
                    uciPars.set(pd.name, num2Str(newValue));
                    double avgErr = computeObjective(positions, sp);
                    funcValues[newValue] = avgErr;
                    uciPars.set(pd.name, num2Str(pd.value));
                    std::stringstream ss;
                    ss << pd.name << ' ' << newValue << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                    std::cout << ss.str() << std::endl;
                }
                if (funcValues[newValue] < bestAvgErr) {
                    bestAvgErr = funcValues[newValue];
                    pd.value = newValue;
                    uciPars.set(pd.name, num2Str(pd.value));
                    updateMinMax(funcValues, pd.value, minV, maxV);
                    improved = true;

                    const int estimatedMinValue = estimateMin(funcValues, pd.value, minV, maxV);
                    if ((estimatedMinValue >= minV) && (estimatedMinValue <= maxV) &&
                        (funcValues.count(estimatedMinValue) == 0)) {
                        uciPars.set(pd.name, num2Str(estimatedMinValue));
                        double avgErr = computeObjective(positions, sp);
                        funcValues[estimatedMinValue] = avgErr;
                        uciPars.set(pd.name, num2Str(pd.value));
                        std::stringstream ss;
                        ss << pd.name << ' ' << estimatedMinValue << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                        std::cout << ss.str() << std::endl;

                        if (avgErr < bestAvgErr) {
                            bestAvgErr = avgErr;
                            pd.value = estimatedMinValue;
                            uciPars.set(pd.name, num2Str(pd.value));
                            updateMinMax(funcValues, pd.value, minV, maxV);
                            break;
                        }
                    }
                }
            }
            if (!improved)
                break;
        }
        double improvement = oldBest - bestAvgErr;
        std::cout << pd.name << " improvement:" << improvement << std::endl;
        pp.priority = pp.priority * 0.1 + improvement * 0.9;
        if (improvement > 0) {
            for (PrioParam& pp2 : tried)
                queue.push(pp2);
            tried.clear();
        }
        tried.push_back(pp);
    }

    double t1 = currentTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

// --------------------------------------------------------------------------------

void
ChessTool::simplify(std::istream& is, std::vector<ParamDomain>& zeroPars,
                    std::vector<ParamDomain>& approxPars) {
    double t0 = currentTime();
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    qEval(positions);
    for (PositionInfo& pi : positions)
        pi.searchScore = pi.qScore; // Use searchScore to store original evaluation

    for (ParamDomain& pd : zeroPars)
        uciPars.set(pd.name, "0");

    std::priority_queue<PrioParam> queue;
    for (ParamDomain& pd : approxPars)
        queue.push(PrioParam(pd));

    ScoreToProb sp;
    auto computeAvgErr = [&positions,&sp]() -> double {
        double errSum = 0;
        for (const PositionInfo& pi : positions) {
            double p0 = sp.getProb(pi.searchScore);
            double p1 = sp.getProb(pi.qScore);
            double err = p1 - p0;
            errSum += err * err;
        }
        return sqrt(errSum / positions.size());
    };

    qEval(positions);
    double bestAvgErr = computeAvgErr();
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    std::vector<PrioParam> tried;
    while (!queue.empty()) {
        PrioParam pp = queue.top(); queue.pop();
        ParamDomain& pd = *pp.pd;
        std::cout << pd.name << " prio:" << pp.priority << " q:" << queue.size()
                  << " min:" << pd.minV << " max:" << pd.maxV << " val:" << pd.value << std::endl;
        double oldBest = bestAvgErr;
        bool improved = false;
        for (int d = 0; d < 2; d++) {
            while (true) {
                const int newValue = pd.value + (d ? -1 : 1);
                if ((newValue < pd.minV) || (newValue > pd.maxV))
                    break;

                uciPars.set(pd.name, num2Str(newValue));
                qEval(positions);
                double avgErr = computeAvgErr();
                uciPars.set(pd.name, num2Str(pd.value));

                std::stringstream ss;
                ss << pd.name << ' ' << newValue << ' ' << std::setprecision(14) << avgErr
                   << ((avgErr < bestAvgErr) ? " *" : "");
                std::cout << ss.str() << std::endl;

                if (avgErr >= bestAvgErr)
                    break;
                bestAvgErr = avgErr;
                pd.value = newValue;
                uciPars.set(pd.name, num2Str(pd.value));
                improved = true;
            }
            if (improved)
                break;
        }
        double improvement = oldBest - bestAvgErr;
        std::cout << pd.name << " improvement:" << improvement << std::endl;
        pp.priority = pp.priority * 0.1 + improvement * 0.9;
        if (improved) {
            for (PrioParam& pp2 : tried)
                queue.push(pp2);
            tried.clear();
        }
        tried.push_back(pp);
    }

    double t1 = currentTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

// --------------------------------------------------------------------------------

template <int N>
static void
printTableNxN(const ParamTable<N*N>& pt, const std::string& name,
              std::ostream& os) {
    os << name << ":" << std::endl;
    for (int y = 0; y < N; y++) {
        os << "    " << ((y == 0) ? "{" : " ");
        for (int x = 0; x < N; x++) {
            os << std::setw(4) << pt[y*N+x] << (((y==N-1) && (x == N-1)) ? " }," : ",");
        }
        os << std::endl;
    }
}

template <int N>
static void
printTable(const ParamTable<N>& pt, const std::string& name, std::ostream& os) {
    os << name << ":" << std::endl;
    os << "    {";
    for (int i = 0; i < N; i++)
        os << std::setw(3) << pt[i] << ((i == N-1) ? " }," : ",");
    os << std::endl;
}

void
ChessTool::printParams() {
    std::ostream& os = std::cout;

    printTable(halfMoveFactor, "halfMoveFactor", os);
    printTable(stalePawnFactor, "stalePawnFactor", os);

    os << "pV : " << pV << std::endl;
    os << "nV : " << nV << std::endl;
    os << "bV : " << bV << std::endl;
    os << "rV : " << rV << std::endl;
    os << "qV : " << qV << std::endl;

    os << "knightVsQueenBonus1 : " << knightVsQueenBonus1 << std::endl;
    os << "knightVsQueenBonus2 : " << knightVsQueenBonus2 << std::endl;
    os << "knightVsQueenBonus3 : " << knightVsQueenBonus3 << std::endl;
    os << "krkpBonus           : " << krkpBonus << std::endl;
    os << "krpkbBonus           : " << krpkbBonus << std::endl;
    os << "krpkbPenalty         : " << krpkbPenalty << std::endl;
    os << "krpknBonus           : " << krpknBonus << std::endl;

    os << "moEvalWeight   : " << moEvalWeight << std::endl;
    os << "moHangPenalty1 : " << moHangPenalty1 << std::endl;
    os << "moHangPenalty2 : " << moHangPenalty2 << std::endl;
    os << "moSeeBonus     : " << moSeeBonus << std::endl;
}

template <int N>
static void
replaceTableNxN(const ParamTable<N*N>& pt, const std::string& name,
             std::vector<std::string>& cppFile) {
    int lineNo = findLine("ParamTable", " " + name + " ", cppFile);
    if (lineNo < 0)
        throw ChessParseError(name + " not found");
    if (lineNo + N >= (int)cppFile.size())
        throw ChessParseError("unexpected end of file");

    std::stringstream ss;
    printTableNxN<N>(pt, name, ss);
    std::vector<std::string> replaceLines = splitLines(ss.str());
    if (replaceLines.size() != N + 1)
        throw ChessParseError("Wrong number of replacement lines");
    for (int i = 1; i <= N; i++)
        cppFile[lineNo + i] = replaceLines[i];
}

template <int N>
static void
replaceTable(const ParamTable<N>& pt, const std::string& name,
           std::vector<std::string>& cppFile) {
    int lineNo = findLine("ParamTable", " " + name + " ", cppFile);
    if (lineNo < 0)
        throw ChessParseError(name + " not found");
    if (lineNo + 1 >= (int)cppFile.size())
        throw ChessParseError("unexpected end of file");

    std::stringstream ss;
    printTable<N>(pt, name, ss);
    std::vector<std::string> replaceLines = splitLines(ss.str());
    if (replaceLines.size() != 2)
        throw ChessParseError("Wrong number of replacement lines");
    cppFile[lineNo + 1] = replaceLines[1];
}

template <typename ParType>
static void replaceValue(const ParType& par, const std::string& name,
                         std::vector<std::string>& hppFile) {
    int lineNo = findLine("DECLARE_PARAM", "(" + name + ", ", hppFile);
    if (lineNo < 0)
        throw ChessParseError(name + " not found");

    const std::string& line = hppFile[lineNo];
    const int len = line.length();
    for (int i = 0; i < len; i++) {
        if (line[i] == ',') {
            for (int j = i + 1; j < len; j++) {
                if (line[j] != ' ') {
                    int p1 = j;
                    for (int k = p1 + 1; k < len; k++) {
                        if (line[k] == ',') {
                            int p2 = k;
                            int val = par;
                            hppFile[lineNo] = line.substr(0, p1) +
                                              num2Str(val) +
                                              line.substr(p2);
                            return;
                        }
                    }
                    goto error;
                }
            }
            goto error;
        }
    }
 error:
    throw ChessParseError("Failed to patch name : " + name);
}

void
ChessTool::patchParams(const std::string& directory) {
    std::vector<std::string> cppFile = readFile(directory + "/parameters.cpp");
    std::vector<std::string> hppFile = readFile(directory + "/parameters.hpp");

    replaceTable(halfMoveFactor, "halfMoveFactor", cppFile);
    replaceTable(stalePawnFactor, "stalePawnFactor", cppFile);

    replaceValue(pV, "pV", hppFile);
    replaceValue(nV, "nV", hppFile);
    replaceValue(bV, "bV", hppFile);
    replaceValue(rV, "rV", hppFile);
    replaceValue(qV, "qV", hppFile);

    replaceValue(knightVsQueenBonus1, "knightVsQueenBonus1", hppFile);
    replaceValue(knightVsQueenBonus2, "knightVsQueenBonus2", hppFile);
    replaceValue(knightVsQueenBonus3, "knightVsQueenBonus3", hppFile);
    replaceValue(krkpBonus, "krkpBonus", hppFile);
    replaceValue(krpkbBonus,   "krpkbBonus", hppFile);
    replaceValue(krpkbPenalty, "krpkbPenalty", hppFile);
    replaceValue(krpknBonus,   "krpknBonus", hppFile);

//    replaceValue(moEvalWeight, "moEvalWeight", hppFile);
//    replaceValue(moHangPenalty1, "moHangPenalty1", hppFile);
//    replaceValue(moHangPenalty2, "moHangPenalty2", hppFile);
//    replaceValue(moSeeBonus, "moSeeBonus", hppFile);

    std::ofstream osc(directory + "/parameters.cpp");
    for (const std::string& line : cppFile)
        osc << line << std::endl;

    std::ofstream osh(directory + "/parameters.hpp");
    for (const std::string& line : hppFile)
        osh << line << std::endl;
}

// --------------------------------------------------------------------------------

void
ChessTool::evalStat(std::istream& is, std::vector<ParamDomain>& pdVec) {
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    const int nPos = positions.size();

    ScoreToProb sp;
    const double avgErr0 = computeObjective(positions, sp);
    std::vector<int> qScores0;
    for (const PositionInfo& pi : positions)
        qScores0.push_back(pi.qScore);

    for (ParamDomain& pd : pdVec) {
        int newVal1 = (pd.value - pd.minV) > (pd.maxV - pd.value) ? pd.minV : pd.maxV;
        uciPars.set(pd.name, num2Str(newVal1));
        double avgErr = computeObjective(positions, sp);
        uciPars.set(pd.name, num2Str(pd.value));

        double nChanged = 0;
        std::unordered_set<int> games, changedGames;
        for (int i = 0; i < nPos; i++) {
            int gameNo = positions[i].gameNo;
            games.insert(gameNo);
            if (positions[i].qScore - qScores0[i]) {
                nChanged++;
                changedGames.insert(gameNo);
            }
        }
        double errChange1 = avgErr - avgErr0;
        double nChangedGames = changedGames.size();
        double nGames = games.size();

        double errChange2;
        int newVal2 = clamp(0, pd.minV, pd.maxV);
        if (newVal2 != newVal1) {
            uciPars.set(pd.name, num2Str(newVal2));
            double avgErr2 = computeObjective(positions, sp);
            uciPars.set(pd.name, num2Str(pd.value));
            errChange2 = avgErr2 - avgErr0;
        } else {
            errChange2 = errChange1;
        }

        std::cout << pd.name << " nMod:" << (nChanged / nPos)
                  << " nModG:" << (nChangedGames / nGames)
                  << " err1:" << errChange1 << " err2:" << errChange2 << std::endl;
    }
}

void
ChessTool::printResiduals(std::istream& is, const std::string& xTypeStr, bool includePosGameNr) {
    enum XType {
        MTRL_SUM,
        MTRL_DIFF,
        PAWN_SUM,
        PAWN_DIFF,
        EVAL
    };
    XType xType;
    if (xTypeStr == "mtrlsum") {
        xType = MTRL_SUM;
    } else if (xTypeStr == "mtrldiff") {
        xType = MTRL_DIFF;
    } else if (xTypeStr == "pawnsum") {
        xType = PAWN_SUM;
    } else if (xTypeStr == "pawndiff") {
        xType = PAWN_DIFF;
    } else if (xTypeStr == "eval") {
        xType = EVAL;
    } else {
        throw ChessParseError("Invalid X axis type");
    }

    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);
    const int nPos = positions.size();
    Position pos;
    ScoreToProb sp;
    for (int i = 0; i < nPos; i++) {
        const PositionInfo& pi = positions[i];
        pos.deSerialize(pi.posData);
        int x;
        switch (xType) {
        case MTRL_SUM:
            x = pos.wMtrl() + pos.bMtrl();
            break;
        case MTRL_DIFF:
            x = pos.wMtrl() - pos.bMtrl();
            break;
        case PAWN_SUM:
            x = pos.wMtrlPawns() + pos.bMtrlPawns();
            break;
        case PAWN_DIFF:
            x = pos.wMtrlPawns() - pos.bMtrlPawns();
            break;
        case EVAL:
            x = pi.qScore;
            break;
        }
        double r = pi.result - sp.getProb(pi.qScore);
        if (includePosGameNr)
            std::cout << i << ' ' << pi.gameNo << ' ';
        std::cout << x << ' ' << r << '\n';
    }
    std::cout << std::flush;
}

bool
ChessTool::getCommentScore(const std::string& comment, int& score) {
    double fScore;
    if (str2Num(comment, fScore)) {
        score = (int)round(fScore * 100);
        return true;
    }
    if (startsWith(comment, "+M")) {
        score = 10000;
        return true;
    }
    if (startsWith(comment, "-M")) {
        score = -10000;
        return true;
    }
    return false;
}

void
ChessTool::readFENFile(std::istream& is, std::vector<PositionInfo>& data) {
    std::vector<std::string> lines = readStream(is);
    data.resize(lines.size());
    const int nLines = lines.size();
    std::atomic<bool> error(false);

    const int batchSize = std::max(1000, nLines / (nWorkers * 10));
    ThreadPool<int> pool(nWorkers);
    for (int i = 0; i < nLines; i += batchSize) {
        int beginIndex = i;
        int endIndex = std::min(i + batchSize, nLines);
        auto func = [beginIndex,endIndex,&lines,&data,&error](int workerNo) mutable {
            Position pos;
            PositionInfo pi;
            for (int i = beginIndex; i < endIndex; i++) {
                if (error)
                    continue;
                const std::string& line = lines[i];
                std::vector<std::string> fields;
                splitString(line, " : ", fields);
                bool localError = false;
                if ((fields.size() < 4) || (fields.size() > 6))
                    localError = true;
                if (!localError) {
                    try {
                        pos = TextIO::readFEN(fields[0]);
                    } catch (ChessParseError&) {
                        localError = true;
                    }
                }
                if (!localError) {
                    pos.serialize(pi.posData);
                    if (!str2Num(fields[1], pi.result) ||
                        !str2Num(fields[2], pi.searchScore) ||
                        !str2Num(fields[3], pi.qScore)) {
                        localError = true;
                    }
                }
                if (!localError) {
                    pi.gameNo = -1;
                    if (fields.size() >= 5)
                        if (!str2Num(fields[4], pi.gameNo))
                            localError = true;
                }
                if (!localError) {
                    pi.cMove = Move().getCompressedMove();
                    if (fields.size() >= 6)
                        pi.cMove = TextIO::uciStringToMove(fields[5]).getCompressedMove();
                }
                if (!localError)
                    data[i] = pi;

                if (localError && !error.exchange(true)) {
                    std::cerr << "line:" << line << std::endl;
                    std::cerr << "fields:" << fields << std::endl;
                }
            }
            return 0;
        };
        pool.addTask(func);
    }
    pool.getAllResults([](int){});

    if (error)
        throw ChessParseError("Invalid file format");

    if (optimizeMoveOrdering) {
        std::cout << "positions before: " << data.size() << std::endl;
        // Only include positions where non-capture moves were played
        auto remove = [](const PositionInfo& pi) -> bool {
            Position pos;
            pos.deSerialize(pi.posData);
            Move m;
            m.setFromCompressed(pi.cMove);
            return m.isEmpty() || pos.getPiece(m.to()) != Piece::EMPTY;
        };
        data.erase(std::remove_if(data.begin(), data.end(), remove), data.end());
        std::cout << "positions after: " << data.size() << std::endl;
    }
}

void
ChessTool::writePGN(const Position& pos) {
    std::cout << "[Event \"?\"]" << std::endl;
    std::cout << "[Site \"?\"]" << std::endl;
    std::cout << "[Date \"????.??.??\"]" << std::endl;
    std::cout << "[Round \"?\"]" << std::endl;
    std::cout << "[White \"?\"]" << std::endl;
    std::cout << "[Black \"?\"]" << std::endl;
    std::cout << "[Result \"*\"]" << std::endl;
    std::cout << "[FEN \"" << TextIO::toFEN(pos) << "\"]" << std::endl;
    std::cout << "[SetUp \"1\"]" << std::endl;
    std::cout << "*" << std::endl;
}

// --------------------------------------------------------------------------------

double
ChessTool::computeObjective(std::vector<PositionInfo>& positions, const ScoreToProb& sp) {
    if (optimizeMoveOrdering) {
        return computeMoveOrderObjective(positions, sp);
    } else {
        qEval(positions);
        return computeAvgError(positions, sp);
    }
}

void
ChessTool::qEval(std::vector<PositionInfo>& positions) {
    qEval(positions, 0, positions.size());
}

void
ChessTool::qEval(std::vector<PositionInfo>& positions, const int beg, const int end) {
    TranspositionTable tt(512*1024);
    Notifier notifier;
    ThreadCommunicator comm(nullptr, tt, notifier, false);

    struct ThreadData {
        std::vector<U64> nullHist = std::vector<U64>(SearchConst::MAX_SEARCH_DEPTH * 2);
        KillerTable kt;
        History ht;
        std::shared_ptr<Evaluate::EvalHashTables> et;
        TreeLogger treeLog;
        Position pos;
    };
    std::vector<ThreadData> tdVec(nWorkers);

    const int chunkSize = 5000;
    ThreadPool<int> pool(nWorkers);
    for (int c = beg; c < end; c += chunkSize) {
        int beginIndex = c;
        int endIndex = std::min(c + chunkSize, end);
        auto func = [&comm,&positions,&tdVec,beginIndex,endIndex](int workerNo) mutable {
            ThreadData& td = tdVec[workerNo];
            if (!td.et)
                td.et = Evaluate::getEvalHashTables();
            Search::SearchTables st(comm.getCTT(), td.kt, td.ht, *td.et);

            const int mate0 = SearchConst::MATE0;
            Position& pos = td.pos;
            Search sc(pos, td.nullHist, 0, st, comm, td.treeLog);

            for (int i = beginIndex; i < endIndex; i++) {
                PositionInfo& pi = positions[i];
                pos.deSerialize(pi.posData);
                sc.init(pos, td.nullHist, 0);
                int score = sc.quiesce(-mate0, mate0, 0, 0, MoveGen::inCheck(pos));
                if (!pos.isWhiteMove())
                    score = -score;
                pi.qScore = score;
            }
            return 0;
        };
        pool.addTask(func);
    }
    pool.getAllResults([](int){});
}

double
ChessTool::computeAvgError(const std::vector<PositionInfo>& positions, const ScoreToProb& sp) {
    double errSum = 0;
    if (useEntropyErrorFunction) {
        for (const PositionInfo& pi : positions) {
            double err = -(pi.result * sp.getLogProb(pi.qScore) + (1 - pi.result) * sp.getLogProb(-pi.qScore));
            errSum += err;
        }
        return errSum / positions.size();
    } else if (useSearchScore) {
        for (const PositionInfo& pi : positions) {
            double err = sp.getProb(pi.qScore) - sp.getProb(pi.searchScore);
            errSum += err * err;
        }
        return sqrt(errSum / positions.size());
    } else {
        for (const PositionInfo& pi : positions) {
            double p = sp.getProb(pi.qScore);
            double err = p - pi.result;
            errSum += err * err;
        }
        return sqrt(errSum / positions.size());
    }
}

double
ChessTool::computeMoveOrderObjective(std::vector<PositionInfo>& positions, const ScoreToProb& sp) {
    const int beg = 0;
    const int end = positions.size();

    struct ThreadData {
        std::shared_ptr<Evaluate::EvalHashTables> et;
        Position pos;
    };
    std::vector<ThreadData> tdVec(nWorkers);

    const int chunkSize = 5000;

    ThreadPool<int> pool(nWorkers);
    for (int c = beg; c < end; c += chunkSize) {
        int beginIndex = c;
        int endIndex = std::min(c + chunkSize, end);
        auto func = [&positions,&sp,&tdVec,beginIndex,endIndex](int workerNo) mutable {
            ThreadData& td = tdVec[workerNo];
            if (!td.et)
                td.et = Evaluate::getEvalHashTables();
            Evaluate eval(*td.et);
            Position& pos = td.pos;

            for (int i = beginIndex; i < endIndex; i++) {
                PositionInfo& pi = positions[i];
                pos.deSerialize(pi.posData);

                MoveList moves;
                MoveGen::pseudoLegalMoves(pos, moves);
                MoveGen::removeIllegal(pos, moves);

                staticScoreMoveListQuiet(pos, eval, moves);

                double probSum = 0;
                for (int mi = 0; mi < moves.size; mi++) {
                    Move& m = moves[mi];
                    if (pos.getPiece(m.to()) != Piece::EMPTY)
                        continue;
                    probSum += sp.getProb(m.score());
                }
                double probFactor = probSum <= 0 ? 1 : 1 / probSum;
                double errSum = 0;
                int errCnt = 0;
                for (int mi = 0; mi < moves.size; mi++) {
                    Move& m = moves[mi];
                    if (pos.getPiece(m.to()) != Piece::EMPTY)
                        continue;
                    double p = sp.getProb(m.score()) * probFactor;
                    double expectedP = m.getCompressedMove() == pi.cMove ? 1 : 0;
                    double err = p - expectedP;
                    errSum += err * err;
                    errCnt++;
                }
                pi.result = errCnt > 0 ? errSum / errCnt : -1;
            }
            return 0;
        };
        pool.addTask(func);
    }
    pool.getAllResults([](int){});

    double errSum = 0;
    int errCnt = 0;
    for (int i = beg; i < end; i++) {
        PositionInfo& pi = positions[i];
        if (pi.result >= 0) {
            errSum += pi.result;
            errCnt++;
        }
    }

    return errCnt > 0 ? sqrt(errSum / errCnt) : 0;
}

void
ChessTool::staticScoreMoveListQuiet(Position& pos, Evaluate& eval, MoveList& moves) {
    eval.connectPosition(pos);
    int score0 = eval.evalPos();
    bool wtm = pos.isWhiteMove();
    UndoInfo ui;
    for (int i = 0; i < moves.size; i++) {
        Move& m = moves[i];
        int score = 0;

        int pVal = ::pieceValue[pos.getPiece(m.from())];
        int prevHang = 0;
        if (pVal > ::pV) {
            if (wtm) {
                if (BitBoard::wPawnAttacks(m.from()) & pos.pieceTypeBB(Piece::BPAWN))
                    prevHang = pVal;
            } else {
                if (BitBoard::bPawnAttacks(m.from()) & pos.pieceTypeBB(Piece::WPAWN))
                    prevHang = pVal;
            }
        }
        score += prevHang * moHangPenalty1 / 32;

        const int mate0 = SearchConst::MATE0;
        int seeScore = Search::SEE(pos, m, -mate0, mate0);
        score += seeScore * moSeeBonus / 32;

        pos.makeMove(m, ui);
        int score1 = -eval.evalPos();
        score += (score1 - score0) * moEvalWeight / 32;

        int currHang = 0;
        if (pVal > ::pV) {
            if (wtm) {
                if (BitBoard::wPawnAttacks(m.to()) & pos.pieceTypeBB(Piece::BPAWN))
                    currHang = pVal;
            } else {
                if (BitBoard::bPawnAttacks(m.to()) & pos.pieceTypeBB(Piece::WPAWN))
                    currHang = pVal;
            }
        }

        score -= currHang * moHangPenalty2 / 32;

        m.setScore(score);
        pos.unMakeMove(m, ui);
    }
}

// --------------------------------------------------------------------------------

void
ChessTool::probeDTZ(const std::string& fen) {
    setupTB();
    Position pos = TextIO::readFEN(fen);
    int success;
    int dtz = Syzygy::probe_dtz(pos, &success);
    std::cout << fen << " raw:";
    if (success)
        std::cout << dtz;
    else
        std::cout << "---";

    auto printScore = [](bool ok, const TranspositionTable::TTEntry& ent, int score) {
        if (ok) {
            std::cout << score;
            if (score == 0) {
                std::cout << " (" << ent.getEvalScore() << ")";
            } else {
                using namespace SearchConst;
                if (isWinScore(score)) {
                    std::cout << " (M" << (MATE0 - score) / 2 << ")";
                } else if (isLoseScore(score)) {
                    std::cout << " (-M" << ((MATE0 + score - 1) / 2) << ")";
                }
            }
        } else {
            std::cout << "---";
        }
    };

    int score = 0;
    TranspositionTable::TTEntry ent;
    bool ok = TBProbe::rtbProbeDTZ(pos, 0, score, ent);
    std::cout << " dtz:";
    printScore(ok, ent, score);

    ok = TBProbe::rtbProbeWDL(pos, 0, score, ent);
    std::cout << " wdl:";
    printScore(ok, ent, score);

    ok = TBProbe::gtbProbeDTM(pos, 0, score);
    std::cout << " dtm:";
    printScore(ok, ent, score);
    std::cout << std::endl;
}
