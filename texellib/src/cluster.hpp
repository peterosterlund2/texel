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

#include <vector>

class Cluster {
public:
    /** Get the singleton instance. */
    static Cluster& instance();

    /** Initialize cluster processes. */
    void init(int* argc, char*** argv);

    /** Terminate cluster processes. */
    void finalize();

    /** Return callers node number within the cluster. */
    int getNodeNumber() const;

    /** Return number of nodes in the cluster. */
    int getNumberOfNodes() const;

    /** Return the parent node number, or -1 if this is the root node. */
    int getParentNode() const;

    /** Return child node numbers. */
    const std::vector<int>& getChildNodes() const;

    /** Assign numThreads threads to this node and child nodes so that
     *  available cores and hardware threads are utilized in a good way. */
    void assignThreads(int numThreads, int& threadsThisNode, std::vector<int>& threadsChildren);

private:
    Cluster();

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
    void computeThisConcurrency(Concurrency& concurrency);


    int rank = 0;
    int size = 1;

    int parent = -1;
    std::vector<int> children;

    Concurrency thisConcurrency;
    std::vector<std::vector<Concurrency>> childConcurrency;  // [childNo][level]
};

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

#endif /* CLUSTER_HPP_ */
