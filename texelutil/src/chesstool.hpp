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
    std::string name;
    int minV, step, maxV;
    int value;
};

class ChessTool {
public:
    /** Read PGN files. For each position, print: "fen : gameResult : searchScore : qScore".
     * Skip positions where searchScore is a mate score. Also skip positions where corresponding
     * game score is unknown. All scores are from white's perspective. gameResult is 0.0, 0.5 or 1.0,
     * also from white's perspective. */
    static void pgnToFen(std::istream& is);

    /** Compute average evaluation error for different pawn advantage values. */
    static void pawnAdvTable(std::istream& is);

    /** Filter out positions where search score and q-search score differ too much. */
    static void filterFEN(std::istream& is);

    /** Use local search to find param values which minimize the average evaluation error. */
    static void paramEvalRange(std::istream& is, ParamDomain& pd);

private:
    /** Read score from a PGN comment, assuming cutechess-cli comment format.
     * Does not handle mate scores. */
    static bool getCommentScore(const std::string& comment, int& score);

    struct PositionInfo {
        Position::SerializeData posData;
        double result;   // Game result for white, 0, 0.5 or 1.0
        int searchScore; // Score reported by engine when game was played
        int qScore;      // q-search computed by this program
    };

    static void readFENFile(std::istream& is, std::vector<PositionInfo>& data);

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
