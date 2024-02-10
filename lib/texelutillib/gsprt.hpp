/*
    Texel - A UCI chess engine.
    Copyright (C) 2024  Peter Österlund, peterosterlund2@gmail.com

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
 * gsprt.hpp
 *
 *  Created on: Feb 2, 2024
 *      Author: petero
 */

#ifndef GSPRT_HPP_
#define GSPRT_HPP_

#include "dblvec.hpp"


/**
 * Calculates the log-likelihood ratio (llr) for two families of hypotheses:
 *   H0 : The elo is elo0
 *   H1 : The elo is elo1
 * given either win,draw,loss statistics (trinomial model) or
 * game pair statistics (pentanomial model), that is the number of
 * game pairs having result 0.0, 0.5, 1.0, 1.5, 2.0 points.
 *
 * Optionally also calculates the llr lower and upper stopping limits
 * from type 1 error frequency (alpha) and type 2 error frequency (beta).
 * A type 1 error means rejecting H0 when it is true.
 * A type 2 error means rejecting H1 when it is true.
 *
 * It might seem more natural to use the hypotheses H0' : "the elo is <= elo0",
 * and H1' : "the elo is >= elo1", since H0 and H1 are typically both false,
 * because it is extremely unlikely that the true elo has a particular exact
 * value (other than 0, which would happen if you test a program against an
 * exact copy of itself). However, H0/H1 has the advantage that the probability
 * of accepting the wrong hypothesis (i.e. the one with the elo farthest away
 * from the true elo) becomes much smaller than alpha/beta when the true elo is
 * far away from elo0/elo1. If H0'/H1' would be used, in the case of elo0=0,
 * elo1=1, true elo=100, the probability of making the wrong conclusion would
 * still be "beta".
 */
class Gsprt {
public:
    struct InParams {
        double elo0 = 0;
        double elo1 = 0;
        bool useBounds = false;
        double alpha = 0.05;
        double beta = 0.05;
        bool usePentanomial = false;
    };

    struct Sample {
        int stats[5] = {0, 0, 0, 0, 0}; // wdl or n00,n05,n10,n15,n20
    };

    struct Result {
        double expectedScore0 = 0; // Expected score in [0,1] corresponding to elo0
        double expectedScore1 = 0; // Expected score in [0,1] corresponding to elo1
        double sampleScore = 0; // Score of sample, in [0,1]
        double a = 0; // The lower stopping limit, computed from alpha, beta if useBounds==true
        double b = 0; // The upper stopping limit, computed from alpha, beta if useBounds==true

        double llr = 0; // The computed log-likelihood ratio
    };

    /** Constructor. */
    explicit Gsprt(const InParams& pars);

    /** Compute LLR and optionally a,b for a sample. */
    void compute(const Sample& sample, Result& res) const;

    /** Convert a relative elo value to an expected game score in [0,1]. */
    static double elo2Score(double elo) { return 1 / (1 + std::pow(10, -elo / 400)); }
    /** Convert an exepcted game score in [0,1] to a relative elo value. */
    static double score2Elo(double score) { return -400 * std::log10(1 / score - 1); }

private:
    /** Compute log-likelihood for a sample "f", given multinomial probabilities "p",
     * with multinomial values "a".
     * For WDL, a=[0,0.5,1]. For pentanomial, a=[0,0.25,0.5,0.75,1]. */
    double computeLL(const DblVec& a, const DblVec& f, const DblVec& p) const;

    /** Compute p[0] and p[n] so that sum(p[i]) = 1 and sum(a[i]*p[i]) = s. */
    void computeP0Pn(const DblVec& a, double s, DblVec& p) const;

    /** Find p that maximizes LL. Return maximum LL. */
    double computeBestLL(const DblVec& a, double s, const DblVec& f, DblVec& p) const;

    /** Compute LL gradient with respect to p[i] for 1<=i<n.
     *  Note that p[0] and p[n] depend on the other p[i], which affects the gradient. */
    void computeLLGrad(const DblVec& a, double s, const DblVec& f, const DblVec& p,
                       DblVec& grad) const;

    InParams pars;
};

#endif /* GSPRT_HPP_ */
