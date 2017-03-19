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

    /** Get numbers of cores/threads for this node. */
    void getConcurrency(int& cores, int& threads);

    /** Get number of cores/threads for a child node and all its children recursively. */
    void getChildConcurrency(int childNo, int& cores, int& threads);

private:
    Cluster();

    /** Check that node 0 can perform IO. */
    void checkIO();

    /** Compute parent and child nodes. */
    void computeNeighbors();

    /** Compute number of cores/threads for this node and all child nodes. */
    void computeConcurrency();

    struct Concurrency {
        int cores = 1;
        int threads = 1;
    };

    /** Compute hardware concurrency for this node. */
    void computeThisConcurrency(Concurrency& concurrency);


    int rank = 0;
    int size = 1;

    int parent = -1;
    std::vector<int> children;

    Concurrency thisConcurrency;
    std::vector<Concurrency> childConcurrency;
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

inline void
Cluster::getConcurrency(int& cores, int& threads) {
    cores = thisConcurrency.cores;
    threads = thisConcurrency.threads;
}

inline void
Cluster::getChildConcurrency(int childNo, int& cores, int& threads) {
    cores = childConcurrency[childNo].cores;
    threads = childConcurrency[childNo].threads;
}

#endif /* CLUSTER_HPP_ */
