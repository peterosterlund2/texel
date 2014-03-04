/*
 * chesstool.hpp
 *
 *  Created on: Dec 24, 2013
 *      Author: petero
 */

#ifndef CHESSTOOL_HPP_
#define CHESSTOOL_HPP_

#include "position.hpp"
#include <vector>
#include <iostream>

#include "armadillo"

/** Convert evaluation score to win probability using logistic model. */
class ScoreToProb {
public:
    /** @param pawnAdvantage Rating advantage corresponding to score 100. */
    ScoreToProb(double pawnAdvantage = 113);

    /** Return win probability corresponding to score. */
    double getProb(int score);

    /** Return log(getProb(score)). */
    double getLogProb(int score);

private:
    double computeProb(int score) const;

    double pawnAdvantage;
    static const int MAXCACHE = 1024;
    double cache[MAXCACHE];
    double logCacheP[MAXCACHE];
    double logCacheN[MAXCACHE];
};


class ParamDomain {
public:
    ParamDomain() : minV(-1), step(-1), maxV(-1), value(-1) {}
    std::string name;
    int minV, step, maxV;
    int value;
};

class ParamValue {
public:
    ParamValue() : value(-1) {}
    std::string name;
    int value;
};

class ChessTool {
public:
    /** Constructor. */
    ChessTool(bool useEntropyErrorFunction);

    /** Read a file into a string vector. */
    static std::vector<std::string> readFile(const std::string& fname);

    /** Read contents of a stream into a string vector. */
    static std::vector<std::string> readStream(std::istream& is);

    /** Read PGN files. For each position, print: "fen : gameResult : searchScore : qScore".
     * Skip positions where searchScore is a mate score. Also skip positions where corresponding
     * game score is unknown. All scores are from white's perspective. gameResult is 0.0, 0.5 or 1.0,
     * also from white's perspective. */
    void pgnToFen(std::istream& is);

    /** Read file with one FEN position per line. Output PGN file using "FEN" and "SetUp" tags. */
    void fenToPgn(std::istream& is);

    /** Compute average evaluation error for different pawn advantage values. */
    void pawnAdvTable(std::istream& is);

    /** Output positions where search score and q-search score differ less than limits. */
    void filterScore(std::istream& is, int scLimit, double prLimit);

    /** Output positions where material balance matches a pattern. */
    void filterMtrlBalance(std::istream& is, bool minorEqual,
                           const std::vector<std::pair<bool,int>>& mtrlPattern);

    /** Output positions where remaining material matches a pattern. */
    void filterTotalMaterial(std::istream& is, bool minorEqual,
                             const std::vector<std::pair<bool,int>>& mtrlPattern);

    /** Print positions where abs(qScore) >= threshold and game result != (1+sign(qScore))/2. */
    void outliers(std::istream& is, int threshold);

    /** Print how much position evaluation improves when parValues are applied to evaluation function.
     * Positions with no change are not printed. */
    void evalEffect(std::istream& is, const std::vector<ParamValue>& parValues);

    /** Compute average evaluation error for a range of parameter values. */
    void paramEvalRange(std::istream& is, ParamDomain& pd);

    /** Use local search (Gauss-Newton) to find param values which minimize the average evaluation error. */
    void gnOptimize(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Use local search to find param values which minimize the average evaluation error. */
    void localOptimize(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Use local search to find param values which minimize the average evaluation error.
     * Uses big jumps to speed up finding large changes and to possibly get to a better local minimum. */
    void localOptimize2(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Print evaluation parameters to cout. */
    void printParams();

    /** Update parameter values in directory/parameters.[ch]pp. */
    void patchParams(const std::string& directory);

    /** Print statistics about how each parameter affect the eval function. */
    void evalStat(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Print total material and evaluation error for each position. */
    void printResiduals(std::istream& is, const std::string& xTypeStr,
                        bool includePosGameNr);

private:
    /** Read score from a PGN comment, assuming cutechess-cli comment format.
     * Does not handle mate scores. */
    bool getCommentScore(const std::string& comment, int& score);

    struct PositionInfo {
        Position::SerializeData posData;
        double result;   // Game result for white, 0, 0.5 or 1.0
        int searchScore; // Score reported by engine when game was played
        int qScore;      // q-search computed by this program
        int gameNo;      // PGN game number this FEN came from

        double getErr(ScoreToProb& sp) const { return sp.getProb(qScore) - result; }
    };

    void readFENFile(std::istream& is, std::vector<PositionInfo>& data);

    /** Write PGN file to cout, with no moves and staring position given by pos. */
    void writePGN(const Position& pos);

    void accumulateATA(std::vector<PositionInfo>& positions, int beg, int end,
                       ScoreToProb& sp,
                       std::vector<ParamDomain>& pdVec,
                       arma::mat& aTa, arma::mat& aTb,
                       arma::mat& ePos, arma::mat& eNeg);

    /** Recompute all qScore values. */
    void qEval(std::vector<PositionInfo>& positions);
    /** Recompute all qScore values between indices beg and end. */
    void qEval(std::vector<PositionInfo>& positions, const int beg, const int end);

    /** Compute average evaluation corresponding to a set of parameter values. */
    double computeAvgError(std::vector<PositionInfo>& positions, ScoreToProb& sp,
                           const std::vector<ParamDomain>& pdVec, arma::mat& pdVal);

    /** Compute average evaluation error. */
    double computeAvgError(const std::vector<PositionInfo>& positions, ScoreToProb& sp);

    bool useEntropyErrorFunction;
};


inline
double ScoreToProb::computeProb(int score) const {
    return 1 / (1 + pow(10, -score * pawnAdvantage / 40000));
}

inline
ChessTool::ChessTool(bool useEntropyErr)
    : useEntropyErrorFunction(useEntropyErr)
{
}

#endif /* CHESSTOOL_HPP_ */
