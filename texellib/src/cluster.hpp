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
 * cluster.hpp
 *
 *  Created on: Mar 12, 2017
 *      Author: petero
 */

#ifndef CLUSTER_HPP_
#define CLUSTER_HPP_

#include "parallel.hpp"
#ifdef CLUSTER
#include <mpi.h>
#endif

#include <vector>

#ifdef CLUSTER
class Cluster {
public:
    /** Get the singleton instance. */
    static Cluster& instance();

    /** Initialize cluster processes. */
    void init(int* argc, char*** argv);

    /** Terminate cluster processes. */
    void finalize();

    /** Return true if this is the master cluster node. (rank 0) */
    bool isMasterNode() const;

    /** Return true if there is more than one cluster node. */
    bool isEnabled() const;

    /** Create Communicator to communicate with cluster parent node. */
    std::unique_ptr<Communicator> createParentCommunicator() const;

    /** Create Communicators to communicate with cluster child nodes. */
    void createChildCommunicators(Communicator* mainThreadComm,
                                  std::vector<std::unique_ptr<Communicator>>& children) const;

    /** Assign numThreads threads to this node and child nodes so that
     *  available cores and hardware threads are utilized in a good way. */
    void assignThreads(int numThreads, int& threadsThisNode, std::vector<int>& threadsChildren);

private:
    Cluster();

    /** Return callers node number within the cluster. */
    int getNodeNumber() const;

    /** Return number of nodes in the cluster. */
    int getNumberOfNodes() const;

    /** Return the parent node number, or -1 if this is the root node. */
    int getParentNode() const;

    /** Return child node numbers. */
    const std::vector<int>& getChildNodes() const;

    /** Check that node 0 can perform IO. */
    void checkIO();

    /** Compute parent and child nodes. */
    void computeNeighbors();

    /** Compute number of cores/threads for this node and all child nodes. */
    void computeConcurrency();

    struct Concurrency {
        Concurrency(int c = 1, int t = 1) : cores(c), threads(t) {}
        int cores;    // Number of available cores
        int threads;  // Number of available hardware threads
    };

    /** Compute hardware concurrency for this node. */
    void computeThisConcurrency(Concurrency& concurrency) const;


    int rank = 0;
    int size = 1;

    int parent = -1;
    std::vector<int> children;

    Concurrency thisConcurrency;
    std::vector<std::vector<Concurrency>> childConcurrency;  // [childNo][level]
};

class MPICommunicator : public Communicator {
public:
    MPICommunicator(Communicator* parent, int myRank, int peerRank);

    void doSendInitSearch(const Position& pos,
                          const std::vector<U64>& posHashList, int posHashListSize,
                          bool clearHistory) override;
    void doSendStartSearch(int jobId, const SearchTreeInfo& sti,
                           int alpha, int beta, int depth) override;
    void doSendStopSearch() override;
    void doSendQuit() override;

    void doSendReportResult(int jobId, int score) override;
    void doSendReportStats(S64 nodesSearched, S64 tbHits) override;
    void retrieveStats(S64& nodesSearched, S64& tbHits) override;
    void doSendStopAck() override;
    void doSendQuitAck() override;

    void mpiSend();

    void doPoll() override;

    void notifyThread() override;

private:
    int myRank;
    int peerRank;
    static constexpr int MAX_BUF_SIZE = 4096;

    bool sendBusy = false;
    std::array<U8,MAX_BUF_SIZE> sendBuf;
    MPI_Request sendReq;

    bool recvBusy = false;
    std::array<U8,MAX_BUF_SIZE> recvBuf;
    MPI_Request recvReq;

    bool quitFlag = false;
};

inline bool
Cluster::isMasterNode() const {
    return getNodeNumber() == 0;
}

inline bool
Cluster::isEnabled() const {
    return getNumberOfNodes() > 1;
}

inline int
Cluster::getNodeNumber() const {
    return rank;
}

inline int
Cluster::getNumberOfNodes() const {
    return size;
}

inline int
Cluster::getParentNode() const {
    return parent;
}

inline const std::vector<int>&
Cluster::getChildNodes() const {
    return children;
}

#else
class Cluster {
public:
    static Cluster& instance();
    void init(int* argc, char*** argv) {}
    void finalize() {}
    bool isMasterNode() const { return true; }
    bool isEnabled() const { return false; }
    std::unique_ptr<Communicator> createParentCommunicator() const { return nullptr; }
    void createChildCommunicators(Communicator* mainThreadComm,
                                  std::vector<std::unique_ptr<Communicator>>& children) const {}
    void assignThreads(int numThreads, int& threadsThisNode, std::vector<int>& threadsChildren) {}
};
#endif


#endif /* CLUSTER_HPP_ */
