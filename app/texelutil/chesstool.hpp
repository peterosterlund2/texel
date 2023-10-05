/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2016  Peter Österlund, peterosterlund2@gmail.com

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
#include <cmath>

#ifdef USE_ARMADILLO
#include "armadillo"
#endif

class Evaluate;
class MoveList;

/** Convert evaluation score to win probability using logistic model. */
class ScoreToProb {
public:
    /** @param pawnAdvantage Rating advantage corresponding to score 100. */
    explicit ScoreToProb(double pawnAdvantage = 113);

    /** Return win probability corresponding to score. */
    double getProb(int score) const;

    /** Return log(getProb(score)). */
    double getLogProb(int score) const;

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
    /** Constructor.
     * @param useEntropyErrorFunction  Use entropy error function instead of LSQ
     *                                 in optimization objective function.
     * @param optmizeMoveOrdering  If true, optimize static move ordering parameters
     *                             instead of evaluation function parameters.
     * @param useSearchScore       If true, use the search score instead of
     *                             the game result when optimizing.
     * @param nWorkers             Number of worker threads to use. */
    ChessTool(bool useEntropyErrorFunction, bool optimizeMoveOrdering,
              bool useSearchScore, int nWorkers);

    /** Setup tablebase directory paths. */
    static void setupTB();

    /** Read a file into a string vector. */
    static std::vector<std::string> readFile(const std::string& fname);

    /** Read contents of a stream into a string vector. */
    static std::vector<std::string> readStream(std::istream& is);

    /** Read PGN files. For each position, print: "fen : gameResult : searchScore : qScore".
     * Skip positions where searchScore is a mate score. Also skip positions where corresponding
     * game score is unknown. All scores are from white's perspective. gameResult is 0.0, 0.5 or 1.0,
     * also from white's perspective.
     * If everyNth is larger than one, each position is printed with probability 1/everyNth.
     * If includeUnScored is true, also moves without a score in the PGN comment (such as book moves)
     * are included. */
    void pgnToFen(std::istream& is, int everyNth, bool includeUnScored);

    /** Read file with one FEN position per line. Output PGN file using "FEN" and "SetUp" tags. */
    void fenToPgn(std::istream& is);

    /** Read lines from is and for each line, replace a sequence of moves with the resulting FEN
     * after executing those moves from the initial position. Any remaining words on the line
     * are copied unmodified to standard output. */
    void movesToFen(std::istream& is);

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

#if !_MSC_VER
    /** In a FEN file, update the search score in each line by running a script to get the new score. */
    void computeSearchScores(std::istream& is, const std::string& script);
#endif

    /** In a FEN file, update the positions to the position at the end of q-search. */
    void computeQSearchPos(std::istream& is);

    /** Search all positions in a FEN file. The time to search each position is determined
     *  from the move number and the time control parameters. Note that this function does
     *  not generate any output, but it is still useful if the SearchTreeSampler is enabled. */
    void searchPositions(std::istream& is, int baseTime, int increment);

    /** Convert FEN+score data to binary format.
     *  If "useResult" is true, use the game result instead of the search score.
     *  If "noInCheck" is true, ignore positions where side to move is in check. */
    void fen2bin(std::istream& is, const std::string& outFile, bool useResult, bool noInCheck);

    /** Print how much position evaluation improves when parValues are applied to evaluation function.
     * Positions with no change are not printed. */
    void evalEffect(std::istream& is, const std::vector<ParamValue>& parValues);

    /** Compute average evaluation error for a range of parameter values. */
    void paramEvalRange(std::istream& is, ParamDomain& pd);

#ifdef USE_ARMADILLO
    /** Use local search (Gauss-Newton) to find param values which minimize the average evaluation error. */
    void gnOptimize(std::istream& is, std::vector<ParamDomain>& pdVec);
#endif

    /** Use local search to find param values which minimize the average evaluation error. */
    void localOptimize(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Use local search to find param values which minimize the average evaluation error.
     * Uses big jumps to speed up finding large changes and to possibly get to a better local minimum. */
    void localOptimize2(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Set all zeroPars parameters to 0, then adjust approxPars so that the difference compared to
     * the original parameter values is minimized. */
    void simplify(std::istream& is, std::vector<ParamDomain>& zeroPars,
                  std::vector<ParamDomain>& approxPars);

    /** Print evaluation parameters to cout. */
    void printParams();

    /** Update parameter values in directory/parameters.[ch]pp. */
    void patchParams(const std::string& directory);

    /** Print statistics about how each parameter affect the eval function. */
    void evalStat(std::istream& is, std::vector<ParamDomain>& pdVec);

    /** Print total material and evaluation error for each position. */
    void printResiduals(std::istream& is, const std::string& xTypeStr,
                        bool includePosGameNr);

    /** Retrieve and print DTZ value from syzygy tablebases for a position. */
    static void probeDTZ(const std::string& fen);

private:
    /** Read score from a PGN comment, assuming cutechess-cli comment format. */
    static bool getCommentScore(const std::string& comment, int& score);

    struct PositionInfo {
        Position::SerializeData posData;
        double result;   // Game result for white, 0, 0.5 or 1.0
        int searchScore; // Score reported by engine when game was played
        int qScore;      // q-search score computed by this program
        int gameNo;      // PGN game number this FEN came from
        U16 cMove;       // Next move in this position

        double getErr(const ScoreToProb& sp) const { return sp.getProb(qScore) - result; }
    };

    void readFENFile(std::istream& is, std::vector<PositionInfo>& data);

    /** Write PGN file to cout, with no moves and staring position given by pos. */
    void writePGN(const Position& pos);

#ifdef USE_ARMADILLO
    void accumulateATA(std::vector<PositionInfo>& positions, int beg, int end,
                       const ScoreToProb& sp,
                       std::vector<ParamDomain>& pdVec,
                       arma::mat& aTa, arma::mat& aTb,
                       arma::mat& ePos, arma::mat& eNeg);

    /** Compute average evaluation corresponding to a set of parameter values. */
    double computeAvgError(std::vector<PositionInfo>& positions, const ScoreToProb& sp,
                           const std::vector<ParamDomain>& pdVec, arma::mat& pdVal);
#endif

    /** Compute the optimization objective function. */
    double computeObjective(std::vector<PositionInfo>& positions, const ScoreToProb& sp);

    /** Recompute all qScore values. */
    void qEval(std::vector<PositionInfo>& positions);
    /** Recompute all qScore values between indices beg and end. */
    void qEval(std::vector<PositionInfo>& positions, const int beg, const int end);

    /** Compute average evaluation error. */
    double computeAvgError(const std::vector<PositionInfo>& positions, const ScoreToProb& sp);

    /** Compute objective function value for move ordering optimization. */
    double computeMoveOrderObjective(std::vector<PositionInfo>& positions, const ScoreToProb& sp);

    /** Score moves (for move ordering) based on static rules. */
    static void staticScoreMoveListQuiet(Position& pos, Evaluate& eval, MoveList& moves);

    const bool useEntropyErrorFunction;
    const bool optimizeMoveOrdering;
    const bool useSearchScore;
    const int nWorkers;
};


inline
double ScoreToProb::computeProb(int score) const {
    return 1 / (1 + pow(10, -score * pawnAdvantage / 40000));
}

#endif /* CHESSTOOL_HPP_ */
