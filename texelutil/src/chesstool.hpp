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

/** Convert evaluation score to win probability using logistic model. */
class ScoreToProb {
public:
    /** @param pawnAdvantage Rating advantage corresponding to score 100. */
    ScoreToProb(double pawnAdvantage = 113);

    /** Return win probability corresponding to score. */
    double getProb(int score);

private:
    double computeProb(int score) const;

    double pawnAdvantage;
    static const int MAXCACHE = 1024;
    double cache[MAXCACHE];
};


class ParamDomain {
public:
    ParamDomain() : minV(-1), step(-1), maxV(-1), value(-1) {}
    std::string name;
    int minV, step, maxV;
    int value;
};

class ChessTool {
public:
    /** Read a file into a string vector. */
    static std::vector<std::string> readFile(const std::string& fname);

    /** Read contents of a stream into a string vector. */
    static std::vector<std::string> readStream(std::istream& is);

    /** Read PGN files. For each position, print: "fen : gameResult : searchScore : qScore".
     * Skip positions where searchScore is a mate score. Also skip positions where corresponding
     * game score is unknown. All scores are from white's perspective. gameResult is 0.0, 0.5 or 1.0,
     * also from white's perspective. */
    static void pgnToFen(std::istream& is);

    /** Read file with one FEN position per line. Output PGN file using "FEN" and "SetUp" tags. */
    static void fenToPgn(std::istream& is);

    /** Compute average evaluation error for different pawn advantage values. */
    static void pawnAdvTable(std::istream& is);

    /** Filter out positions where search score and q-search score differ too much. */
    static void filterFEN(std::istream& is);

    /** Print positions where abs(qScore) >= threshold and game result != (1+sign(qScore))/2. */
    static void outliers(std::istream& is, int threshold);

    /** Compute average evaluation error for a range of parameter values. */
    static void paramEvalRange(std::istream& is, ParamDomain& pd);

    /** Use local search to find param values which minimize the average evaluation error. */
    static void localOptimize(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Use local search to find param values which minimize the average evaluation error.
     * Uses big jumps to speed up finding large changes and to possibly get to a better local minimum. */
    static void localOptimize2(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Print evaluation parameters to cout. */
    static void printParams();

    /** Print statistics about how each parameter affect the eval function. */
    static void evalStat(std::istream& is, std::vector<ParamDomain>& pdVec);

private:
    /** Read score from a PGN comment, assuming cutechess-cli comment format.
     * Does not handle mate scores. */
    static bool getCommentScore(const std::string& comment, int& score);

    struct PositionInfo {
        Position::SerializeData posData;
        double result;   // Game result for white, 0, 0.5 or 1.0
        int searchScore; // Score reported by engine when game was played
        int qScore;      // q-search computed by this program
        int gameNo;      // PGN game number this FEN came from
    };

    static void readFENFile(std::istream& is, std::vector<PositionInfo>& data);

    /** Write PGN file to cout, with no moves and staring position given by pos. */
    static void writePGN(const Position& pos);

    /** Recompute all qScore values. */
    static void qEval(std::vector<PositionInfo>& positions);

    /** Compute average evaluation error. */
    static double computeAvgError(const std::vector<PositionInfo>& positions, ScoreToProb& sp);
};


inline
double ScoreToProb::computeProb(int score) const {
    return 1 / (1 + pow(10, -score * pawnAdvantage / 40000));
}


#endif /* CHESSTOOL_HPP_ */
