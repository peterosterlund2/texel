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
    int nCores = thisConcurrency.cores;
    int nThreads = thisConcurrency.threads;
    std::cout << "rank:" << rank << " nCores:" << nCores << " nThreads:" << nThreads << std::endl;

    const int nChild = children.size();
    childConcurrency.resize(nChild);
    for (int i = 0; i < nChild; i++) {
        int buf[2];
        MPI_Status status;
        MPI_Recv(buf, 2, MPI_INT, children[i], 0, MPI_COMM_WORLD, &status);
        childConcurrency[i].cores = buf[0];
        childConcurrency[i].threads = buf[1];
        nCores += buf[0];
        nThreads += buf[1];
    }
    std::cout << "rank:" << rank << " nCoresT:" << nCores << " nThreadsT:" << nThreads << std::endl;
    if (parent != -1) {
        int buf[2];
        buf[0] = nCores;
        buf[1] = nThreads;
        MPI_Send(buf, 2, MPI_INT, parent, 0, MPI_COMM_WORLD);
    }
#endif
}

void
Cluster::computeThisConcurrency(Concurrency& concurrency) {
    int nodes;
    Numa::instance().getConcurrency(nodes, concurrency.cores, concurrency.threads);
}
