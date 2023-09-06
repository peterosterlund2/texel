/*
    Texel - A UCI chess engine.
    Copyright (C) 2023  Peter Österlund, peterosterlund2@gmail.com

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
 * featureperm.cpp
 *
 *  Created on: Aug 26, 2023
 *      Author: petero
 */

#include "featureperm.hpp"
#include "random.hpp"
#include "randperm.hpp"
#include "timeUtil.hpp"

#include <iostream>
#include <cassert>


const int maxGrpSize = 4;

FeaturePerm::FeaturePerm(NetData& net)
    : net(net) {
}

void
FeaturePerm::permute(const std::vector<BitSet>& featureActivations, int nPos,
                     bool useLocalSearch, U64 rndSeed) {
    std::vector<int> permutation;
    std::vector<int> groupCount;
    computeGreedyPerm(featureActivations, nPos, permutation, groupCount);
    if (useLocalSearch)
        localOptimize(featureActivations, nPos, rndSeed, permutation, groupCount);
    permuteNet(permutation);
}

void
FeaturePerm::computeGreedyPerm(const std::vector<BitSet>& featureActivations,
                               int nPos, std::vector<int>& permutation,
                               std::vector<int>& groupCount) {
    std::vector<int> remainingF;
    for (int f = 0; f < NetData::n1; f++)
        remainingF.push_back(f);

    std::vector<BitSet> tmpSets(2);
    BitSet& currAct = tmpSets[0];
    BitSet& tmpSet = tmpSets[1];
    int grpSize = 0;

    std::cout << "Computing greedy permutation..." << std::endl;
    permutation.clear();
    int iter = 0;
    int oldTot = 0;
    double numNonZero = 0;
    while (!remainingF.empty()) {
        if (grpSize == maxGrpSize) {
            currAct.clear();
            grpSize = 0;
            oldTot = 0;
            std::cout << "---" << std::endl;
        }

        int bestI = -1;
        int bestCnt = 0;
        for (int i = 0; i < (int)remainingF.size(); i++) {
            int f = remainingF[i];
            tmpSet = currAct;
            tmpSet |= featureActivations[f];
            int totCnt = tmpSet.bitCount();
            if (bestI == -1 || totCnt < bestCnt) {
                bestI = i;
                bestCnt = totCnt;
            }
        }
        int bestF = remainingF[bestI];
        int newCnt = featureActivations[bestF].bitCount();
        currAct |= featureActivations[bestF];
        int totCnt = currAct.bitCount();

        remainingF[bestI] = remainingF[(int)remainingF.size()-1];
        remainingF.pop_back();

        std::cout << "i: " << std::setw(3) << iter
                  << " f: " << std::setw(3) << bestF
                  << " new: " << std::setw(8) << newCnt
                  << " inc: " << std::setw(8) << (totCnt - oldTot)
                  << " tot: " << std::setw(8) << totCnt
                  << " p: " << ((double)totCnt / (2*nPos))
                  << std::endl;

        if (grpSize == maxGrpSize - 1) {
            numNonZero += (double)totCnt / (2*nPos);
            groupCount.push_back(totCnt);
        }

        permutation.push_back(bestF);
        oldTot = totCnt;
        grpSize++;
        iter++;
    }
    std::cout << "non-zero prob: " << (numNonZero / (iter / maxGrpSize)) << std::endl;
}

void
FeaturePerm::localOptimize(const std::vector<BitSet>& featureActivations,
                           int nPos, U64 rndSeed, std::vector<int>& permutation,
                           std::vector<int>& groupCount) {
    const int nFeats = NetData::n1;
    assert(nFeats == featureActivations.size());
    assert(nFeats % maxGrpSize == 0);
    int nGroups = nFeats / maxGrpSize;

    auto activationProb = [nPos,nGroups](S64 totCnt) {
        return (double)totCnt / (2*nPos) / nGroups;
    };

    S64 totCnt = 0; // Total number of times a group is non-zero
    for (int g = 0; g < nGroups; g++)
        totCnt += groupCount[g];
    const double initProb = activationProb(totCnt);

    auto getGroupCount = [&featureActivations,&permutation](int g) -> int {
        int f0 = g * maxGrpSize;
        BitSet bs(featureActivations[permutation[f0]]);
        for (int i = 1; i < maxGrpSize; i++)
            bs |= featureActivations[permutation[f0+i]];
        return bs.bitCount();
    };

    if (rndSeed == 0) {
        rndSeed = currentTimeMillis();
        std::cout << "Random seed: " << rndSeed << std::endl;
    }
    Random rnd(rndSeed);

    std::vector<U8> activeVec(nFeats*nFeats, true);
    auto active = [&activeVec,nFeats](int f1, int f2) -> U8& {
        if (f1 > f2)
            std::swap(f1, f2);
        return activeVec[f1*nFeats+f2];
    };

    int iter = 0;
    bool improved;
    do {
        improved = false;
        const int N2 = nFeats * nFeats;
        RandPerm rp(N2, rnd.nextU64());
        for (int i = 0; i < N2; i++) {
            int p = rp.perm(i);
            int f1 = p / nFeats;
            int f2 = p % nFeats;
            if (f1 / maxGrpSize == f2 / maxGrpSize)
                continue; // Same group
            if (!active(f1,f2))
                continue;
            active(f1,f2) = false;
            iter++;

            int g1 = f1 / maxGrpSize;
            int g2 = f2 / maxGrpSize;

            int oldG1Cnt = groupCount[g1];
            int oldG2Cnt = groupCount[g2];

            std::swap(permutation[f1], permutation[f2]);

            int g1Cnt = getGroupCount(g1);
            int g2Cnt = getGroupCount(g2);

            int delta = g1Cnt + g2Cnt - (oldG1Cnt + oldG2Cnt);
            if (delta < 0) {
                groupCount[g1] = g1Cnt;
                groupCount[g2] = g2Cnt;
                totCnt += delta;
                double actProb = activationProb(totCnt);
                std::cout << "i: " << iter << " f1: " << f1 << " f2: " << f2 << " delta: " << delta
                          << " prob: " << actProb << " (" << (actProb/initProb) << ")"
                          << std::endl;
                improved = true;
                auto activateGroup = [&active,nFeats](int g) {
                    for (int f1 = g*maxGrpSize; f1 < (g+1)*maxGrpSize; f1++)
                        for (int f2 = 0; f2 < nFeats; f2++)
                            active(f1,f2) = true;
                };
                activateGroup(g1);
                activateGroup(g2);
                break;
            } else {
                std::swap(permutation[f1], permutation[f2]);
            }
        }
    } while (improved);
}

void
FeaturePerm::permuteNet(std::vector<int>& permutation) {
    int n1 = NetData::n1;
    for (int newF = 0; newF < n1; newF++) {
        int oldF = permutation[newF];
        for (int i = 0; i < NetData::inFeatures; i++)
            std::swap(net.weight1(i, newF), net.weight1(i, oldF));
        std::swap(net.bias1(newF), net.bias1(oldF));
        for (int i = 0; i < NetData::n2; i++) {
            std::swap(net.lin2.weight(i, newF+n1*0), net.lin2.weight(i, oldF+n1*0));
            std::swap(net.lin2.weight(i, newF+n1*1), net.lin2.weight(i, oldF+n1*1));
        }
        permutation[newF] = newF;
        for (int x = newF+1; x < n1; x++) {
            if (permutation[x] == newF) {
                permutation[x] = oldF;
                break;
            }
        }
    }
}
