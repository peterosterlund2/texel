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
 * threadpool.hpp
 *
 *  Created on: Oct 22, 2017
 *      Author: petero
 */

#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <exception>


/** A pool of threads to which tasks can be queued and results gotten back. */
template <typename Result>
class ThreadPool {
public:
    /** Constructor. Creates nThreads worker threads. */
    ThreadPool(int nThreads);
    /** Destructor. */
    ~ThreadPool();

    /** Add a task to be executed. The function "func" must have signature:
     *  Result func(int workerNo), where workerNo is between 0 and nThreads-1.
     *  Task execution starts in the same order as the tasks were added. */
    template <typename Func>
    void addTask(Func func);

    /** Wait for and retrieve a result. Return false if there is no task to wait for.
     *  The results are not necessarily returned in the same order as the tasks were added. */
    bool getResult(Result& result);

private:
    void workerLoop(int workerNo);

    std::mutex mutex;
    std::condition_variable taskCv;
    std::condition_variable completeCv;

    int nActive = 0;
    bool stopped = false;
    std::vector<std::thread> threads;
    std::deque<std::function<Result(int)>> tasks;
    std::deque<Result> results;
    std::deque<std::exception_ptr> exceptions;
};

template <typename Result>
ThreadPool<Result>::ThreadPool(int nThreads) {
    for (int i = 0; i < nThreads; i++)
        threads.emplace_back(std::thread([this,i]{ workerLoop(i); }));
}

template <typename Result>
ThreadPool<Result>::~ThreadPool() {
    {
        std::lock_guard<std::mutex> L(mutex);
        stopped = true;
    }
    taskCv.notify_all();
    for (auto& t : threads)
        t.join();
}

template <typename Result>
template <typename Func>
void ThreadPool<Result>::addTask(Func func) {
    {
        std::lock_guard<std::mutex> L(mutex);
        tasks.push_back(std::forward<Func>(func));
    }
    taskCv.notify_all();
}

template <typename Result>
bool ThreadPool<Result>::getResult(Result& result) {
    std::unique_lock<std::mutex> L(mutex);
    while (results.empty() && exceptions.empty()) {
        if (nActive == 0 && tasks.empty())
            return false;
        completeCv.wait(L);
    }
    if (!exceptions.empty()) {
        std::exception_ptr ex = exceptions.front();
        exceptions.pop_front();
        std::rethrow_exception(ex);
    }
    result = std::move(results.front());
    results.pop_front();
    return true;
}

template <typename Result>
void ThreadPool<Result>::workerLoop(int workerNo) {
    while (true) {
        std::function<Result(int)> task;
        {
            std::unique_lock<std::mutex> L(mutex);
            while (!stopped && tasks.empty())
                taskCv.wait(L);
            if (stopped)
                return;
            task = tasks.front();
            tasks.pop_front();
            nActive++;
        }
        try {
            Result result = task(workerNo);
            std::unique_lock<std::mutex> L(mutex);
            nActive--;
            bool empty = results.empty() && exceptions.empty();
            results.push_back(result);
            if (empty)
                completeCv.notify_all();
        } catch (...) {
            std::unique_lock<std::mutex> L(mutex);
            nActive--;
            bool empty = results.empty() && exceptions.empty();
            exceptions.push_back(std::current_exception());
            if (empty)
                completeCv.notify_all();
        }
    }
}

#endif
