/*
    Texel - A UCI chess engine.
    Copyright (C) 2017  Peter Österlund, peterosterlund2@gmail.com

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
 * cluster.cpp
 *
 *  Created on: Mar 12, 2017
 *      Author: petero
 */

#include "cluster.hpp"
#include "numa.hpp"
#ifdef CLUSTER
#include <mpi.h>
#endif
#include <thread>
#include <iostream>


Cluster&
Cluster::instance() {
    static Cluster inst;
    return inst;
}

Cluster::Cluster() {
}

void
Cluster::init(int* argc, char*** argv) {
#ifdef CLUSTER
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED)
        return;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    checkIO();
    computeNeighbors();
    computeConcurrency();
#endif
}

void
Cluster::finalize() {
#ifdef CLUSTER
    MPI_Finalize();
#endif
}

void
Cluster::checkIO() {
#ifdef CLUSTER
    int* ioPtr;
    int flag;
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_IO, &ioPtr, &flag);
    int ioRank;
    if (flag == 0) {
        ioRank = -1;
    } else if (*ioPtr == MPI_ANY_SOURCE) {
        ioRank = 0;
    } else if (*ioPtr == MPI_PROC_NULL) {
        ioRank = -1;
    } else {
        ioRank = *ioPtr;
    }
    int ioMinRank;
    MPI_Allreduce(&ioRank, &ioMinRank, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    if (ioMinRank != 0) {
        if (rank == 0)
            std::cerr << "Node 0 does not support standard IO" << std::endl;
        exit(2);
    }
#endif
}

void
Cluster::computeNeighbors() {
    const int maxChildren = 4;
    int n = getNodeNumber();
    parent = n > 0 ? (n - 1) / maxChildren : -1;

    for (int i = 0; i < maxChildren; i++) {
        int c = n * maxChildren + i + 1;
        if (c < getNumberOfNodes())
            children.push_back(c);
    }
}

void
Cluster::computeConcurrency() {
#ifdef CLUSTER
    computeThisConcurrency(thisConcurrency);

    const int nChild = children.size();
    int nChildLevels = 0;
    for (int c = 0; c < nChild; c++) {
        MPI_Status status;
        MPI_Probe(children[c], 0, MPI_COMM_WORLD, &status);
        int count;
        MPI_Get_count(&status, MPI_INT, &count);
        std::vector<int> buf(count);
        MPI_Recv(&buf[0], count, MPI_INT, children[c], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::vector<Concurrency> childConcur;
        int nLev = count / 2;
        for (int i = 0; i < nLev; i++)
            childConcur.emplace_back(buf[2*i], buf[2*i+1]);
        childConcurrency.push_back(std::move(childConcur));
        nChildLevels = std::max(nChildLevels, nLev);
    }
    if (parent != -1) {
        int count = (nChildLevels + 1) * 2;
        std::vector<int> buf(count);
        buf[0] = thisConcurrency.cores;
        buf[1] = thisConcurrency.threads;
        for (int lev = 0; lev < nChildLevels; lev++) {
            int nc = 0, nt = 0;
            for (int c = 0; c < nChild; c++) {
                if (lev < (int)childConcurrency[c].size()) {
                    nc += childConcurrency[c][lev].cores;
                    nt += childConcurrency[c][lev].threads;
                }
            }
            buf[2*lev+2] = nc;
            buf[2*lev+3] = nt;
        }
        MPI_Send(&buf[0], count, MPI_INT, parent, 0, MPI_COMM_WORLD);
    }
#endif
}

void
Cluster::computeThisConcurrency(Concurrency& concurrency) {
    int nodes;
    Numa::instance().getConcurrency(nodes, concurrency.cores, concurrency.threads);
}

void
Cluster::assignThreads(int numThreads, int& threadsThisNode, std::vector<int>& threadsChildren) {
#ifdef CLUSTER
    int nTotalCores = thisConcurrency.cores;
    int nTotalThreads = thisConcurrency.threads;
    const int nChild = childConcurrency.size();
    threadsThisNode = 0;
    threadsChildren.assign(nChild, 0);
    int numChildLevels = 0;
    std::vector<int> htChildren(nChild, 0);
    for (int c = 0; c < nChild; c++) {
        const std::vector<Concurrency>& childConcur = childConcurrency[c];
        int nLev = childConcur.size();
        for (int lev = 0; lev < nLev; lev++) {
            int nc = childConcur[lev].cores;
            int nt = childConcur[lev].threads;
            nTotalCores += nc;
            nTotalThreads += nt;
            htChildren[c] += nt - nc;
        }
        numChildLevels = std::max(numChildLevels, nLev);
    }

    const int nOverCommit = numThreads / nTotalThreads;
    numThreads %= nTotalThreads;

    // Assign threads to cores using breadth first
    if (numThreads > 0) {
        int t = std::min(thisConcurrency.cores, numThreads);
        threadsThisNode += t;
        numThreads -= t;
    }
    for (int lev = 0; lev < numChildLevels && numThreads > 0; lev++) {
        for (int c = 0; c < nChild && numThreads > 0; c++) {
            if (lev < (int)childConcurrency[c].size()) {
                int t = std::min(childConcurrency[c][lev].cores, numThreads);
                threadsChildren[c] += t;
                numThreads -= t;
            }
        }
    }

    // Assign threads to hardware threads proportionally
    int htThisNode = thisConcurrency.threads - thisConcurrency.cores;
    int htRemain = htThisNode;
    for (int c = 0; c < nChild; c++)
        htRemain += htChildren[c];

    if (numThreads > 0) {
        int t = (numThreads * htThisNode + htRemain - 1) / htRemain;
        threadsThisNode += t;
        numThreads -= t;
        htRemain -= htThisNode;
    }
    for (int c = 0; c < nChild && numThreads > 0; c++) {
        int t = (numThreads * htChildren[c] + htRemain - 1) / htRemain;
        threadsChildren[c] += t;
        numThreads -= t;
        htRemain -= htChildren[c];
    }

    // Assign over-committed threads
    threadsThisNode += nOverCommit * thisConcurrency.threads;
    for (int c = 0; c < nChild; c++) {
        int concur = 0;
        for (auto& e : childConcurrency[c])
            concur += e.threads;
        threadsChildren[c] += nOverCommit * concur;
    }
#endif
}
