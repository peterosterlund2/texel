/*
 * chesstool.cpp
 *
 *  Created on: Dec 24, 2013
 *      Author: petero
 */

#include "chesstool.hpp"
#include "search.hpp"
#include "textio.hpp"
#include "gametree.hpp"
#include "stloutput.hpp"

#include <queue>


ScoreToProb::ScoreToProb(double pawnAdvantage0)
    : pawnAdvantage(pawnAdvantage0) {
    for (int i = 0; i < MAXCACHE; i++)
        cache[i] = -1;
}

double
ScoreToProb::getProb(int score) {
    bool neg = false;
    if (score < 0) {
        score = -score;
        neg = true;
    }
    double ret = -1;
    if (score < MAXCACHE) {
        if (cache[score] < 0)
            cache[score] = computeProb(score);
        ret = cache[score];
    }
    if (ret < 0)
        ret = computeProb(score);
    if (neg)
        ret = 1 - ret;
    return ret;
}

// --------------------------------------------------------------------------------

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

void
ChessTool::pgnToFen(std::istream& is) {
    static std::vector<U64> nullHist(200);
    static TranspositionTable tt(19);
    static ParallelData pd(tt);
    static KillerTable kt;
    static History ht;
    static auto et = Evaluate::getEvalHashTables();
    static Search::SearchTables st(tt, kt, ht, *et);
    static TreeLogger treeLog;

    Position pos;
    const int mate0 = SearchConst::MATE0;
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    const int plyScale = SearchConst::plyScale;

    GameTree gt(is);
    while (gt.readPGN()) {
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
//            std::string move = TextIO::moveToUCIString(gn.getMove());
            std::string comment = gn.getComment();
            int commentScore;
            if (!getCommentScore(comment, commentScore))
                continue;

            sc.init(pos, nullHist, 0);
            int score = sc.negaScout(-mate0, mate0, 0, 0*plyScale, -1, MoveGen::inCheck(pos));
            if (!pos.getWhiteMove()) {
                score = -score;
                commentScore = -commentScore;
            }

            std::cout << fen << " : " << rScore << " : " << commentScore << " : " << score << std::endl;
        }
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

void
ChessTool::filterFEN(std::istream& is) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    ScoreToProb sp;
    Position pos;
    for (const PositionInfo& pi : positions) {
        double p1 = sp.getProb(pi.searchScore);
        double p2 = sp.getProb(pi.qScore);
        if ((std::abs(p1 - p2) < 0.05) && (std::abs(pi.searchScore - pi.qScore) < 200)) {
            pos.deSerialize(pi.posData);
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << pi.result << " : " << pi.searchScore << " : " << pi.qScore << std::endl;
        }
    }
}

void
ChessTool::paramEvalRange(std::istream& is, ParamDomain& pd) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    ScoreToProb sp;
    double bestVal = 1e100;
    for (int i = pd.minV; i <= pd.maxV; i += pd.step) {
        Parameters::instance().set(pd.name, num2Str(i));
        qEval(positions);
        double avgErr = computeAvgError(positions, sp);
        bool best = avgErr < bestVal;
        bestVal = std::min(bestVal, avgErr);
        std::stringstream ss;
        ss << "i:" << i << " err:" << std::setprecision(14) << avgErr << (best?" *":"");
        std::cout << ss.str() << std::endl;
    }
}

struct PrioParam {
    PrioParam(ParamDomain& pd0) : priority(1), pd(&pd0) {}
    double priority;
    ParamDomain* pd;
    bool operator<(const PrioParam& o) const {
        return priority < o.priority;
    }
};

void
ChessTool::localOptimize(std::istream& is, std::vector<ParamDomain>& pdVec) {
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    std::priority_queue<PrioParam> queue;
    for (ParamDomain& pd : pdVec)
        queue.push(PrioParam(pd));

    ScoreToProb sp;
    qEval(positions);
    double bestAvgErr = computeAvgError(positions, sp);
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
                double avgErr = computeAvgError(positions, sp);
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
}

template <int N>
static void
printTableNxN(const ParamTable<N*N>& pt, const std::string& name) {
    std::cout << name << ":" << std::endl;
    for (int y = 0; y < N; y++) {
        std::cout << "    " << ((y == 0) ? "{" : " ");
        for (int x = 0; x < N; x++) {
            std::cout << std::setw(4) << pt[y*N+x] << (((y==N-1) && (x == N-1)) ? " }," : ",");
        }
        std::cout << std::endl;
    }
}

template <int N>
static void
printTable(const ParamTable<N>& pt, const std::string& name) {
    std::cout << name << ":" << std::endl;
    std::cout << "    {";
    for (int i = 0; i < N; i++) {
        std::cout << std::setw(3) << pt[i] << ((i == N-1) ? " }," : ",");
    }
    std::cout << std::endl;
}

void
ChessTool::printParams() {
    printTableNxN<8>(kt1b, "kt1b");
    printTableNxN<8>(kt2b, "kt2b");
    printTableNxN<8>(pt1b, "pt1b");
    printTableNxN<8>(pt2b, "pt2b");
    printTableNxN<8>(nt1b, "nt1b");
    printTableNxN<8>(nt2b, "nt2b");
    printTableNxN<8>(bt1b, "bt1b");
    printTableNxN<8>(bt2b, "bt2b");
    printTableNxN<8>(qt1b, "qt1b");
    printTableNxN<8>(rt1b, "rt1b");
    printTableNxN<8>(knightOutpostBonus, "knightOutpostBonus");

    printTable(rookMobScore, "rookMobScore");
    printTable(bishMobScore, "bishMobScore");
    printTable(queenMobScore, "queenMobScore");

    printTableNxN<4>(majorPieceRedundancy, "majorPieceRedundancy");

    std::cout << "pV : " << pV << std::endl;
    std::cout << "nV : " << nV << std::endl;
    std::cout << "bV : " << bV << std::endl;
    std::cout << "rV : " << rV << std::endl;
    std::cout << "qV : " << qV << std::endl;

    std::cout << "pawnDoubledPenalty : " << pawnDoubledPenalty << std::endl;
    std::cout << "pawnIslandPenalty : " << pawnIslandPenalty << std::endl;
    std::cout << "pawnIsolatedPenalty : " << pawnIsolatedPenalty << std::endl;
    std::cout << "pawnBackwardPenalty : " << pawnBackwardPenalty << std::endl;
    std::cout << "pawnGuardedPassedBonus : " << pawnGuardedPassedBonus << std::endl;

    std::cout << "knightVsQueenBonus1 : " << knightVsQueenBonus1 << std::endl;
    std::cout << "knightVsQueenBonus2 : " << knightVsQueenBonus2 << std::endl;
    std::cout << "knightVsQueenBonus3 : " << knightVsQueenBonus3 << std::endl;

    std::cout << "pawnTradePenalty : " << pawnTradePenalty << std::endl;
    std::cout << "pieceTradeBonus : " << pieceTradeBonus << std::endl;

    std::cout << "rookHalfOpenBonus : " << rookHalfOpenBonus << std::endl;
    std::cout << "rookOpenBonus : " << rookOpenBonus << std::endl;
    std::cout << "rookDouble7thRowBonus : " << rookDouble7thRowBonus << std::endl;
    std::cout << "trappedRookPenalty : " << trappedRookPenalty << std::endl;

    std::cout << "bishopPairValue : " << bishopPairValue << std::endl;
    std::cout << "bishopPairPawnPenalty : " << bishopPairPawnPenalty << std::endl;

    std::cout << "kingAttackWeight : " << kingAttackWeight << std::endl;
    std::cout << "kingSafetyHalfOpenBCDEFG : " << kingSafetyHalfOpenBCDEFG << std::endl;
    std::cout << "kingSafetyHalfOpenAH : " << kingSafetyHalfOpenAH << std::endl;
    std::cout << "kingSafetyWeight : " << kingSafetyWeight << std::endl;
    std::cout << "pawnStormBonus : " << pawnStormBonus << std::endl;
}

bool
ChessTool::getCommentScore(const std::string& comment, int& score) {
    double fScore;
    if (!str2Num(comment, fScore))
        return false;
    score = (int)std::round(fScore * 100);
    return true;
}

void
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

void
ChessTool::readFENFile(std::istream& is, std::vector<PositionInfo>& data) {
    std::vector<std::string> lines = readStream(is);
    data.resize(lines.size());
    Position pos;
    PositionInfo pi;
    const int nLines = lines.size();
    std::atomic<bool> error(false);
#pragma omp parallel for default(none) shared(data,error,lines,std::cout) private(pos,pi)
    for (int i = 0; i < nLines; i++) {
        if (error)
            continue;
        const std::string& line = lines[i];
        std::vector<std::string> fields;
        splitString(line, " : ", fields);
        if (fields.size() != 4) {
#pragma omp critical
            if (!error) {
                std::cout << "line:" << line << std::endl;
                std::cout << "fields:" << fields << std::endl;
                error = true;
            }
        }
        pos = TextIO::readFEN(fields[0]);
        pos.serialize(pi.posData);
        if (!str2Num(fields[1], pi.result) ||
            !str2Num(fields[2], pi.searchScore) ||
            !str2Num(fields[3], pi.qScore)) {
            error = true;
        }
        data[i] = pi;
    }
    if (error)
        throw ChessParseError("Invalid file format");
}

void ChessTool::qEval(std::vector<PositionInfo>& positions) {
    TranspositionTable tt(19);
    ParallelData pd(tt);

    std::vector<U64> nullHist(200);
    KillerTable kt;
    History ht;
    std::shared_ptr<Evaluate::EvalHashTables> et;
    TreeLogger treeLog;
    Position pos;

    const int nPos = positions.size();
    const int chunkSize = 5000;

#pragma omp parallel for default(none) shared(positions,tt,pd) private(kt,ht,et,treeLog,pos) firstprivate(nullHist)
    for (int c = 0; c < nPos; c += chunkSize) {
        if (!et)
            et = Evaluate::getEvalHashTables();
        Search::SearchTables st(tt, kt, ht, *et);

        const int mate0 = SearchConst::MATE0;
        Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
        const int plyScale = SearchConst::plyScale;

        for (int i = 0; i < chunkSize; i++) {
            if (c + i >= nPos)
                break;
            PositionInfo& pi = positions[c + i];
            pos.deSerialize(pi.posData);
            sc.init(pos, nullHist, 0);
            int score = sc.negaScout(-mate0, mate0, 0, 0*plyScale, -1, MoveGen::inCheck(pos));
            if (!pos.getWhiteMove())
                score = -score;
            pi.qScore = score;
        }
    }
}

double
ChessTool::computeAvgError(const std::vector<PositionInfo>& positions, ScoreToProb& sp) {
    double errSum = 0;
    for (const PositionInfo& pi : positions) {
        double p = sp.getProb(pi.qScore);
        double err = p - pi.result;
        errSum += err * err;
    }
    double avgErr = sqrt(errSum / positions.size());
    return avgErr;
}
