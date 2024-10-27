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
 * gsprt.cpp
 *
 *  Created on: Feb 2, 2024
 *      Author: petero
 */

#include "gsprt.hpp"
#include "chessError.hpp"
#include "stloutput.hpp"


Gsprt::Gsprt(const InParams& pars)
    : pars(pars) {
}

double
Gsprt::computeLL(const DblVec& a, const DblVec& f, const DblVec& p) const {
    int n = a.size() - 1;
    double ll = 0;
    for (int i = 0; i <= n; i++)
        if (f[i] > 0) // Avoid log(0) = -inf when f[i] = p[i] = 0
            ll += f[i] * log(p[i]);
    return ll;
}

void
Gsprt::computeP0Pn(const DblVec& a, double s, DblVec& p) const {
    int n = a.size() - 1;

    double sum1 = 0;
    for (int i = 1; i < n; i++)
        sum1 += a[i] * p[i];
    p[n] = s - sum1;

    double sum2 = 0;
    for (int i = 1; i <= n; i++)
        sum2 += p[i];
    p[0] = 1 - sum2;
}

void
Gsprt::computeLLGrad(const DblVec& a, double s, const DblVec& f, const DblVec& p,
                     DblVec& grad) const {
    // ll  = f_0*log(1-s-sum(1<=i<n, (1-a_i)*p_i)) + sum(1<=i<n, f_i*log(p_i)) + f_n*log(s-sum(1<=i<n, a_i*p_i))
    // d(ll)/d(p_j) = f_0*(a_j-1)/(1-s - sum(1<=i<n, (1-a_i)*p_i)) + f_j/p_j - f_n*a_j/(s - sum(1<=i<n, a_i*p_i))

    int n = a.size() - 1;
    DblVec infGrad(n + 1);
    bool gradIsInf = false;

    for (int j = 1; j < n; j++) {
        double g = 0;

        if (f[0] > 0) {
            double sum = 0;
            for (int i = 1; i < n; i++)
                sum += (1 - a[i]) * p[i];

            double q = f[0] * (a[j] - 1);
            double d = 1 - s - sum;
            if (d <= 0) {
                infGrad[j] += q;
                gradIsInf = true;
            }
            g += q / d;
        }

        if (f[j] > 0) {
            double q = f[j];
            double d = p[j];
            if (d <= 0) {
                infGrad[j] += q;
                gradIsInf = true;
            }
            g += q / d;
        }

        if (f[n] > 0) {
            double sum = 0;
            for (int i = 1; i < n; i++)
                sum += a[i] * p[i];
            double q = -f[n] * a[j];
            double d = s - sum;
            if (d <= 0) {
                infGrad[j] += q;
                gradIsInf = true;
            }
            g += q / d;
        }

        grad[j] = (f[j] == 0 && p[j] == 0) ? std::max(0.0, g) : g;
    }

    if (gradIsInf)
        grad = infGrad;
}

double
Gsprt::computeBestLL(const DblVec& a, double s, const DblVec& f, DblVec& p) const {
    // This is basically gradient ascent with safeguards to stay within the
    // valid domain and rudimentary step size adjustments.
    // If f[0]*f[n] > 0 a faster algorithm is possible, but has not been implemented.

    int n = a.size() - 1;

    p *= 0;
    computeP0Pn(a, s, p);

    double bestLL = computeLL(a, f, p);
//    std::cout << "bestLL: " << bestLL << " p: " << p << std::endl;

    double stepSize = 0.1;
    const int nTries = 3;
    DblVec grad(n+1);
    std::vector<DblVec> tmpP;
    for (int k = 0; k < nTries; k++)
        tmpP.push_back(DblVec(n+1));
    int maxSteps = 10000;

    auto computeNextP = [&](double ss, DblVec& nextP) {
        nextP = p + ss * grad;
        computeP0Pn(a, s, nextP);
    };

    auto pValid = [&](const DblVec& pVec) {
        for (int i = 0; i <= n; i++)
            if (pVec[i] < 0 || pVec[i] > 1)
                return false;
        return true;
    };

    bool converged = false;
    for (int step = 0; step < maxSteps; step++) {
        computeLLGrad(a, s, f, p, grad);
//        std::cout << "grad: " << grad << std::endl;

        for (int t = 0; t < 20; t++) { // Find valid step
            computeNextP(stepSize, tmpP[0]);
            if (pValid(tmpP[0]))
                break;
            stepSize *= 0.5;
//            std::cout << "stepSize: " << stepSize << std::endl;
        }
        if (!pValid(tmpP[0])) {
            std::cout << "Cannot find valid step size, step: " << step << std::endl;
            break;
        }

        for (int t = 0; t < 20; t++) { // Find improving step
            double ll = computeLL(a, f, tmpP[0]);
//            std::cout << "stepSize: " << stepSize << " dll: " << (ll - bestLL) << std::endl;
            if (ll > bestLL)
                break;
            stepSize *= 0.5;
            computeNextP(stepSize, tmpP[0]);
        }

        // Try smaller/larger stepSize
//      double tmpStepSize[nTries] = { stepSize, stepSize * 0.5, stepSize * 0.5, stepSize * 1.5, stepSize * 1.5 };
        double tmpStepSize[nTries] = { stepSize, stepSize * 0.5, stepSize * 1.5 };
        for (int k = 1; k < nTries; k++)
            computeNextP(tmpStepSize[k], tmpP[k]);

        double ll[nTries];
        for (int k = 0; k < nTries; k++) {
            ll[k] = computeLL(a, f, tmpP[k]);
//            std::cout << "k: " << k << " step: " << tmpStepSize[k]
//                      << " ll: " << ll[k] << " dll: " << (ll[k] - bestLL) << std::endl;
        }

        bool newBest = false;
        for (int k = 0; k < nTries; k++) {
            if (pValid(tmpP[k]) && ll[k] > bestLL) {
                bestLL = ll[k];
                p = tmpP[k];
                stepSize = tmpStepSize[k];
                newBest = true;
            }
        }
        if (newBest) {
//            std::cout << "bestLL: " << bestLL << " step: " << step
//                      << " stepSize: " << stepSize << " p: " << p << std::endl;
        } else {
//            std::cout << "Converged at iteration: " << step << std::endl;
            converged = true;
            break;
        }
    }
    if (!converged)
        std::cout << "Max number of iterations reached" << std::endl;
//    std::cout << "p: " << p << std::endl;

    return bestLL;
}

void
Gsprt::compute(const Sample& sample, Result& res) const {
    if (pars.elo0 >= pars.elo1)
        throw ChessError("elo0 must be < elo1");
    if (pars.alpha > 0.5 || pars.beta > 0.5)
        throw ChessError("alpha and beta must be <= 0.5");
    if (pars.alpha <= 0 || pars.beta <= 0)
        throw ChessError("alpha and beta must be > 0");

    res.expectedScore0 = elo2Score(pars.elo0);
    res.expectedScore1 = elo2Score(pars.elo1);

    if (pars.useBounds) {
        res.a = log(pars.beta / (1 - pars.alpha));
        res.b = log((1 - pars.beta) / pars.alpha);
    }

    double N = 0;
    int nProbs = pars.usePentanomial ? 5 : 3;
    for (int i = 0; i < nProbs; i++)
        N += sample.stats[i];
    if (N == 0) {
        res.llr = 0;
        return;
    }

    DblVec a(nProbs); // Value of each possible outcome
    DblVec f(nProbs); // Sample frequency
    for (int i = 0; i < nProbs; i++)
        a[i] = i / (double)(nProbs - 1);
    if (pars.usePentanomial) {
        for (int i = 0; i < nProbs; i++)
            f[i] = sample.stats[i];
    } else {
        for (int i = 0; i < nProbs; i++)
            f[i] = sample.stats[2-i];
    }

    double eps = 1e-3;
    if (f[0] == 0) {
        double d = std::min(1.0, eps * N);
        f[0] = d;
        N += d;
    }
    int n = nProbs - 1;
    if (f[n] == 0) {
        double d = std::min(1.0, eps * N);
        f[n] = d;
        N += d;
    }

    for (int i = 0; i < nProbs; i++)
        f[i] /= N;

    double avg = a.dot(f);
    double sum2 = 0;
    for (int i = 0; i < nProbs; i++)
        sum2 += (a[i]-avg)*(a[i]-avg)*f[i];
    res.sampleScore = avg;
    res.sampleStdDev = sqrt(sum2 / (N-1));

    DblVec p(nProbs);
    double ll0 = N * computeBestLL(a, res.expectedScore0, f, p);
//    std::cout << "ll0: " << ll0 << std::endl;

    double ll1 = N * computeBestLL(a, res.expectedScore1, f, p);
//    std::cout << "ll1: " << ll1 << std::endl;

    res.llr = ll1 - ll0;
}
