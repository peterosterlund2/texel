/*
    Texel - A UCI chess engine.
    Copyright (C) 2020  Peter Österlund, peterosterlund2@gmail.com

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
 * threadorder.hpp
 *
 *  Created on: May 1, 2020
 *      Author: petero
 */

#ifndef THREADORDER_HPP_
#define THREADORDER_HPP_

#include "util/alignedAlloc.hpp"
#include "util/util.hpp"

#include <atomic>

#include <immintrin.h>

/**
 * The ThreadOrder class makes sure that a sequence of lock/unlock operations
 * from a set of threads are performed in a deterministic order. The order is:
 *
 *   thread 0 lock
 *   thread 0 unlock
 *   thread 1 lock
 *   thread 1 unlock
 *   ...
 *   thread N-1 lock
 *   thread N-1 unlock
 *   thread 0 lock
 *   thread 0 unlock
 *   ...
 *
 * If a thread calls the lock() function before it is its turn to run according
 * to the deterministic order, it will simply wait until other threads that
 * should run first are finished, even if the other threads have not yet called
 * the lock() function.
 *
 * To avoid deadlocks, all threads would have to lock/unlock the same number of
 * times. Since this is not realistic, there is also a quit() function that when
 * called from a thread causes that thread to no longer participate in the
 * round-robin scheduling order.
 *
 * This class can be used to implement a deterministic multithreaded search, but
 * the result will likely be much less efficient than a non-deterministic
 * multithreaded search.
 *
 * NOTE! This class uses busy wait loops, so it will be extremely inefficient if
 * there are more software threads than there are free hardware threads in the
 * system.
 */
class ThreadOrder {
private:
    class AtomicInt {
    public:
        AtomicInt(int val = 0) : a(val) {}
        AtomicInt(const AtomicInt& o)              { a.store(o.a.load()); }
        AtomicInt& operator=(const AtomicInt& o)   { a.store(o.a.load()); return *this; }
        bool load(std::memory_order mo) const      { return a.load(mo); }
        void store(bool val, std::memory_order mo) { a.store(val, mo); }
    private:
        std::atomic<int> a;
    };

public:
    ThreadOrder(int numThreads);
    ThreadOrder(const ThreadOrder& other) = delete;

    void lock(int threadNo);
    void unLock(int threadNo);
    void quit(int threadNo);

private:
    int nextThreadNo(int threadNo) const;
    AtomicInt& elem(int threadNo);

    vector_aligned<AtomicInt> flags;
    int numThreads;
    static const int stride = 16;
};

inline void
ThreadOrder::lock(int threadNo) {
    AtomicInt& flag = elem(threadNo);
    while (flag.load(std::memory_order_acquire) == 0) {
        _mm_pause();
    }
}

inline void
ThreadOrder::unLock(int threadNo) {
    elem(threadNo).store(0, std::memory_order_relaxed);
    for (int i = 0; i < numThreads; i++) {
        threadNo = nextThreadNo(threadNo);
        if (elem(threadNo).load(std::memory_order_relaxed) == 0) {
            elem(threadNo).store(1, std::memory_order_release);
            return;
        }
    }
}

inline void
ThreadOrder::quit(int threadNo) {
    lock(threadNo);

    elem(threadNo).store(2, std::memory_order_relaxed);

    for (int i = 0; i < numThreads; i++) {
        threadNo = nextThreadNo(threadNo);
        if (elem(threadNo).load(std::memory_order_relaxed) == 0) {
            elem(threadNo).store(1, std::memory_order_release);
            return;
        }
    }
}

inline int
ThreadOrder::nextThreadNo(int threadNo) const {
    threadNo++;
    if (threadNo == numThreads)
        threadNo = 0;
    return threadNo;
}

inline ThreadOrder::AtomicInt&
ThreadOrder::elem(int threadNo) {
    return flags[threadNo * stride];
}

#endif /* THREADORDER_HPP_ */
