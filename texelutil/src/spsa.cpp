/*
 * spsa.cpp
 *
 *  Created on: Feb 21, 2015
 *      Author: petero
 */

#include "spsa.hpp"
#include "util/timeUtil.hpp"
#include <iostream>
#include <limits>


template<class T>
std::ostream& operator<<(std::ostream& s, const std::vector<T>& v)
{
    s << "[";
    for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); ++i) {
        if (i != v.begin())
            s << ",";
        s << " " << (*i);
    }
    s << " ]";
    return s;
}


static Random seeder;

void
Spsa::gameSimulation(double meanResult, double drawProb,
                     int nGames, int nSimul) {
    ResultSimulation rs(meanResult, drawProb);
    double sum = 0;
    double sum2 = 0;
    for (int i = 0; i < nSimul; i++) {
        double score = rs.simulate(nGames);
        sum += score;
        sum2 += score * score;
//        std::cout << "score:" << score << std::endl;
    }
    double mean = sum / nSimul;
    double N = nSimul;
    double var = (sum2 - sum * sum / N) / (N - 1);
    double s = sqrt(var);
    std::cout << "mean:" << mean << " std:" << s << " meanElo: " << ResultSimulation::resultToElo(mean) << std::endl;
    for (int si = 1; si <= 5; si++) {
        std::cout << "i:" << si << ' '
                  << std::setw(10) << ResultSimulation::resultToElo(mean - s * si)
                  << std::setw(10) << ResultSimulation::resultToElo(mean + s * si)
                  << std::endl;
    }
}

void
Spsa::engineSimulation(int nGames, const std::vector<double>& params) {
    std::vector<double> params1, params2;
    params1.assign(params.begin(), params.begin() + (params.size() / 2));
    params2.assign(params.begin() + (params.size() / 2), params.end());
    SimulatedEnginePair se;
    se.setParams(params1, params2);
    double result = se.simulate(nGames);
    std::cout << "p0: " << params[0] << " p1: " << params[1]
              << "p2: " << params[2] << " p3: " << params[3]
              << " result: " << result << " elo: " << ResultSimulation::resultToElo(result)
              << std::endl;
}

void
Spsa::tourneySimulation(int nSimul, int nRounds, const std::vector<double>& elo) {
    const int N = elo.size();
//    std::cout << "N:" << N << std::endl;
    assert(N >= 2);
    assert(nSimul > 0);
    std::vector<int> nWins(N);
    int nTies = 0;
    std::vector<double> scores(N);

    ResultSimulation rs(0.5, 0.5);
    std::vector<std::vector<ResultSimulation>> rsM;

    std::vector<double> diffV { 0,   50,  100, 150, 200, 250,  300,  400 };
    std::vector<double> drawV { .64, .60, .52, .45, .38, .343, .282, .176 };
    for (int i = 0; i < N; i++) {
        std::vector<ResultSimulation> tmp;
        for (int j = 0; j < N; j++) {
            double eloDiff = abs(elo[i] - elo[j]);
#if 0
            double mean = ResultSimulation::eloToResult(elo[i] - elo[j]);
            double drawProb = std::min(mean, 1 - mean) * 0.8;
            for (int k = 1; k < (int)diffV.size(); k++) {
                if (eloDiff <= diffV[k]) {
                    drawProb = drawV[k-1] + (drawV[k] - drawV[k-1]) * (eloDiff - diffV[k-1]) / (diffV[k] - diffV[k-1]);
                    break;
                }
            }
#else
            auto f = [](double delta) -> double { return 1/(1+pow(10, delta / 400)); };
            double eloDraw = 200;
            double winP = f(-eloDiff + eloDraw);
            double lossP = f(+eloDiff + eloDraw);
            double drawP = 1 - winP - lossP;
//            std::cout << "winP:" << winP << " lossP:" << lossP << " drawP:" << drawP << std::endl;
            double mean = winP + drawP / 2;
            double drawProb = drawP;
#endif
//            std::cout << "i:" << i << " j:" << j << " m:" << mean << " d:" << drawProb << std::endl;
            tmp.emplace_back(mean, drawProb);
        }
        rsM.push_back(tmp);
    }

    for (int s = 0; s < nSimul; s++) {
        for (int i = 0; i < N; i++)
            scores[i] = 0.0;
        for (int i = 0; i < N; i++) {
            for (int j = i+1; j < N; j++) {
//                double mean = ResultSimulation::eloToResult(elo[i] - elo[j]);
//                double drawProb = std::min(mean, 1 - mean) * 0.8;
//                rs.setParams(mean, drawProb);
//                double score = (nRounds == 1) ? rs.simulateOneGame()
//                                              : rs.simulate(nRounds);
                double score = (nRounds == 1) ? rsM[i][j].simulateOneGame()
                                              : rsM[i][j].simulate(nRounds);
//                std::cout << "i:" << i << " j:" << j << " score:" << score << std::endl;
                scores[i] += score;
                scores[j] += 1 - score;
            }
        }
//        for (int i = 0; i < N; i++)
//            std::cout << "i:" << i << " score:" << scores[i] << std::endl;
        int bestI = 0;
        double bestScore = scores[bestI];
        int nBest = 1;
        int secondBestI = -1;
        for (int i = 1; i < N; i++) {
            if (rint(scores[i]*2) > rint(bestScore*2)) {
                bestI = i;
                bestScore = scores[i];
                nBest = 1;
//                std::cout << "findBest i:" << i << " best:" << bestScore << std::endl;
            } else if (rint(scores[i]*2) == rint(bestScore*2)) {
                secondBestI = i;
                nBest++;
//                std::cout << "findBest i:" << i << " tie, nBest:" << nBest << std::endl;
            }
        }
        if (nBest == 1) {
            nWins[bestI]++;
        } else if (nBest == 2) {
            double score = rsM[bestI][secondBestI].simulate(2);
            if (score > 0.5)
                nWins[bestI]++;
            else if (score < 0.5)
                nWins[secondBestI]++;
            else
                nTies++;
        } else {
            nTies++;
        }
    }

    for (int i = 0; i < N; i++) {
        std::stringstream ss;
        ss.precision(6);
        ss << std::fixed << nWins[i] / (double)nSimul;
        std::cout << std::setw(2) << i + 1 << "  " << std::setw(4) << elo[i] << "  "
                  << std::setw(8) << ss.str() << std::endl;
//        std::cout << "i:" << i << " nWin:" << nWins[i] << " " << nWins[i] / (double)nSimul << std::endl;
    }
    std::cout << "ties:" << nTies << " " << nTies / (double)nSimul << std::endl;
}

void
Spsa::spsaSimulation(int nSimul, int nIter, int gamesPerIter, double a, double c,
                     const std::vector<double>& startParams) {
    const int N = startParams.size();
    std::vector<double> params(startParams);
    std::vector<double> paramsNeg(startParams);
    std::vector<double> paramsPos(startParams);
    std::vector<double> signVec(N);
    std::vector<double> accum(N);
    Random rnd;
    rnd.setSeed(seeder.nextU64());
    SimulatedEnginePair enginePair;

    std::cout << "Initial elo: " << enginePair.getElo(startParams) << std::endl;

    std::vector<double> eloErr;
    for (int s = 0; s < nSimul; s++) {
        params = startParams;
        double A = nIter * 0.1;
        double alpha = 0.602;
        double gamma = 0.101;
        for (int k = 0; k < nIter; k++) {
            double ak = a / pow(A + k + 1, alpha);
            double ck = c / pow(k + 1, gamma);
#if 1
            for (int i = 0; i < N; i++) {
                signVec[i] = (rnd.nextU64() & 1) ? 1 : -1;
                paramsNeg[i] = params[i] - ck * signVec[i];
                paramsPos[i] = params[i] + ck * signVec[i];
            }
            enginePair.setParams(paramsPos, paramsNeg);
            double dy = -(enginePair.simulate(gamesPerIter) - 0.5);
            for (int i = 0; i < N; i++)
                params[i] = params[i] - ak * dy / (2 * ck * signVec[i]);
#else
            for (int i = 0; i < N; i++)
                accum[i] = 0;
            for (int g = 0; g < gamesPerIter; g++) {
                for (int i = 0; i < N; i++) {
                    signVec[i] = (rnd.nextU64() & 1) ? 1 : -1;
                    paramsNeg[i] = params[i] - ck * signVec[i];
                    paramsPos[i] = params[i] + ck * signVec[i];
                }
                enginePair.setParams(paramsPos, paramsNeg);
                double dy = -(enginePair.simulate(1) - 0.5);
                for (int i = 0; i < N; i++)
                    accum[i] += dy / (2 * ck * signVec[i]);
            }
            for (int i = 0; i < N; i++)
                params[i] = params[i] - ak * accum[i] / gamesPerIter;
#endif
            if (nSimul == 1) {
                if ((k == nIter - 1) || ((k % (std::max(nIter,20) / 20)) == 0))
                    std::cout << "k:" << k << " params: " << params
                              << " elo:" << enginePair.getElo(params) << std::endl;
            }
        }
        if (nSimul > 1) {
            if ((s == nSimul - 1) || ((s % (std::max(nSimul,20) / 20)) == 0))
                std::cout << "s:" << s << " params: " << params
                          << " elo:" << enginePair.getElo(params) << std::endl;
        }
        eloErr.push_back(enginePair.getElo(params));
//        std::cout << "eloErr: " << enginePair.getElo(params) << std::endl;
    }
    if (nSimul > 1) {
        double sum = 0;
        double sum2 = 0;
        double nElem = eloErr.size();
        for (int i = 0; i < nElem; i++) {
            double v = eloErr[i];
            sum += v;
            sum2 += v * v;
        }
        double mean = sum / nElem;
        double var = (sum2 - sum * sum / nElem) / (nElem - 1);
        double s = sqrt(var);
        std::cout << "eloAvg:" << mean << " eloStd:" << s << std::endl;
    }
}

ResultSimulation::ResultSimulation(double meanResult, double drawProb)
    : rng(gsl_rng_alloc(gsl_rng_mt19937),
          [](gsl_rng* rng) { gsl_rng_free(rng); })
{
    U64 s = seeder.nextU64();
//    std::cout << "seed:" << s << std::endl;
    gsl_rng_set(rng.get(), s);
    rnd.setSeed(seeder.nextU64());
    setParams(meanResult, drawProb);
}

void
ResultSimulation::setParams(double meanResult, double drawProb) {
    drawP = drawProb;
    winP = meanResult - drawP * 0.5;
    lossP = 1 - winP - drawP;
    assert(winP >= 0);
    assert(winP <= 1);
    assert(drawP >= 0);
    assert(drawP <= 1);
    assert(lossP >= 0);
    assert(lossP <= 1);
}

double
ResultSimulation::simulate(int nGames) {
    int nWin, nDraw, nLoss;
    simulate(nGames, nWin, nDraw, nLoss);
    return (nWin + nDraw * 0.5) / nGames;
}

void
ResultSimulation::simulate(int nGames, int& nWin, int& nDraw, int& nLoss) {
    double p[3];
    unsigned int n[3];
    p[0] = winP;
    p[1] = drawP;
    p[2] = lossP;

    gsl_ran_multinomial(rng.get(), 3, nGames, &p[0], &n[0]);
    nWin = n[0];
    nDraw = n[1];
    nLoss = n[2];
}

double
ResultSimulation::simulateOneGame() {
    double r = rnd.nextU64() / (double)std::numeric_limits<U64>::max();
    if (r < lossP)
        return 0;
    else if (r < lossP + drawP)
        return 0.5;
    else
        return 1;
}


SimulatedEnginePair::SimulatedEnginePair()
    : rs(0.5, 0.4) {
}

void
SimulatedEnginePair::setParams(const std::vector<double>& params1,
                               const std::vector<double>& params2) {
    double eloa = getElo(params1);
    double elob = getElo(params2);
    double elo = eloa - elob;
    double mean = ResultSimulation::eloToResult(elo);
    double drawProb = std::min(mean, 1 - mean) * 0.8;
    rs.setParams(mean, drawProb);
}

double
SimulatedEnginePair::simulate(int nGames) {
    if (nGames == 1)
        return rs.simulateOneGame();
    else
        return rs.simulate(nGames);
}

double
SimulatedEnginePair::getElo(const std::vector<double>& params) {
    auto sqr = [](double x) -> double { return x * x; };
#if 1
    assert(params.size() == 3);
    double p0 = params[0];
    double p1 = params[1];
    double p2 = params[2];
    double elo = 0 - sqr(p0 - 120) / 400 * 20
                   - sqr(p1 - 65) / 400 * 10;
    double px=std::max(p2*(1.0/10)-10,0.0);
    elo += 10*(px*px*exp(-px)-4*exp(-2));
    return elo / 10;
#else
    assert(params.size() == 10);
    double elo = 0;
    for (int i = 0; i < 10; i++) {
        double c = 110 + i * 2;
        elo -= sqr(params[i] - c) / 400 * 2;
    }
    return elo;
#endif
}
