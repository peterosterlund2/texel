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
 * matchrunner.cpp
 *
 *  Created on: Feb 5, 2024
 *      Author: petero
 */

#include "matchrunner.hpp"
#include "gsprt.hpp"
#include "threadpool.hpp"
#include "util.hpp"
#include "timeUtil.hpp"

#include <atomic>
#include <cmath>
#include <iostream>


MatchRunner::MatchRunner(int nWorkers, const EngineParams& engine1, const EngineParams& engine2)
    : nWorkers(nWorkers), engine1Pars(engine1), engine2Pars(engine2) {
}

bool
MatchRunner::runOneBatch(const std::string& script, int workerNo, int (&stats)[5]) const {
    std::string cmdLine = "\"" + script + "\"";
    cmdLine += " " + num2Str(workerNo);
    cmdLine += " " + engine1Pars.name + " " + engine1Pars.timeControl;
    cmdLine += " " + engine2Pars.name + " " + engine2Pars.timeControl;

    std::shared_ptr<FILE> f(popen(cmdLine.c_str(), "r"),
                            [](FILE* f) { pclose(f); });
    std::string s;
    char buf[256];
    while (fgets(buf, sizeof(buf), f.get()))
        s += buf;
    std::vector<std::string> lines = splitLines(s);
    if (lines.size() != 1) {
        std::cerr << "Script failed: " << s << std::endl;
        return false;
    }

    std::vector<std::string> statStrs;
    splitString(lines[0], statStrs);
    bool ok = statStrs.size() == 5;
    if (ok) {
        for (int i = 0; i < 5; i++) {
            if (!str2Num(statStrs[i], stats[i])) {
                ok = false;
                break;
            }
        }
    }
    if (!ok)
        std::cerr << "Script failed: " << s << std::endl;
    return ok;
}

static void
computeStats(const int (&batchStats)[5], int (&totStats)[5], int& nPlayed, double& score, double& elo) {
    nPlayed = 0;
    score = 0;
    for (int i = 0; i < 5; i++) {
        totStats[i] += batchStats[i];
        nPlayed += totStats[i] * 2;
        score += totStats[i] * i * 0.5;
    }
    score /= nPlayed;
    elo = Gsprt::score2Elo(score);

    score = std::rint(score * 1000) / 1000;
    elo = std::rint(elo * 10) / 10;
}

void
MatchRunner::runFixedNumGames(int numGames, const std::string& script) const {
    std::atomic<bool> error(false);
    const int batchSize = 100;

    struct Result {
        int workerNo;
        int stats[5];
    };
    ThreadPool<Result> pool(nWorkers);
    for (int i = 0; i < numGames; i += batchSize) {
        auto func = [this,&script,&error](int workerNo) {
            Result r;
            r.workerNo = workerNo;
            if (error)
                return r;
            if (!runOneBatch(script, workerNo, r.stats))
                error.store(true);
            return r;
        };
        pool.addTask(func);
    }

    double t0 = currentTime();
    int stats[5] = {0, 0, 0, 0, 0};

    pool.getAllResults([&](const Result& r) {
        double t1 = currentTime();

        int nPlayed;
        double score, elo;
        computeStats(r.stats, stats, nPlayed, score, elo);

        std::cout << "c:" << r.workerNo
                  << " n:" << nPlayed
                  << " t:" << (int)(t1 - t0)
                  << " s:" << score << " elo:" << elo
                  << " :";
        for (int i = 0; i < 5; i++)
            std::cout << " " << r.stats[i];
        std::cout << " :";
        for (int i = 0; i < 5; i++)
            std::cout << " " << stats[i];
        std::cout << std::endl;
    });
}

void
MatchRunner::runGsprtGames(const Gsprt::InParams& gsprtParams, const std::string& script) const {
    Gsprt gsprt(gsprtParams);
    Gsprt::Sample sample;
    Gsprt::Result gsprtRes;

    gsprt.compute(sample, gsprtRes); // Compute with 0 games to check parameters

    std::atomic<bool> error(false);
    struct Result {
        int workerNo;
        int stats[5];
    };
    ThreadPool<Result> pool(nWorkers);

    auto task = [this,&script,&error](int workerNo) {
        Result r;
        r.workerNo = workerNo;
        if (error)
            return r;
        if (!runOneBatch(script, workerNo, r.stats))
            error.store(true);
        return r;
    };

    for (int i = 0; i < nWorkers; i++)
        pool.addTask(task);

    bool done = false;
    double t0 = currentTime();

    while (true) {
        Result r;
        if (!pool.getResult(r))
            break;

        double t1 = currentTime();

        int nPlayed;
        double score;
        double elo;
        computeStats(r.stats, sample.stats, nPlayed, score, elo);

        gsprt.compute(sample, gsprtRes);

        double llr = gsprtRes.llr;
        double a = gsprtRes.a;
        double b = gsprtRes.b;
        double relLlr = (2*llr - a - b) / (b - a);
        relLlr = rint(relLlr * 1000) / 1000;

        if (relLlr < -1 || relLlr > 1)
            done = true;

        if (!done)
            pool.addTask(task);

        std::cout << "c:" << r.workerNo
                  << " n:" << nPlayed
                  << " t:" << (int)(t1 - t0)
                  << " llr: " << relLlr
                  << " s:" << score << " elo:" << elo
                  << " :";
        for (int i = 0; i < 5; i++)
            std::cout << " " << r.stats[i];
        std::cout << " :";
        for (int i = 0; i < 5; i++)
            std::cout << " " << sample.stats[i];
        std::cout << std::endl;
    }
}
