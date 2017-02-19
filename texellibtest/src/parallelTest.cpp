/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * parallelTest.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: petero
 */

#define _GLIBCXX_USE_NANOSLEEP

#include "parallelTest.hpp"
#include "parallel.hpp"
#include "position.hpp"
#include "textio.hpp"
#include "searchUtil.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <chrono>

#include "cute.h"

using namespace SearchConst;

class NotifyCounter {
public:
    NotifyCounter(Notifier& notifier);
    ~NotifyCounter();

    int getCount() const { return cnt; }

private:
    void mainLoop();

    std::unique_ptr<std::thread> thread;
    Notifier& notifier;
    std::atomic<int> cnt { 0 };
    std::atomic<bool> quit { false };
};

NotifyCounter::NotifyCounter(Notifier& notifier)
    : notifier(notifier) {
    thread = make_unique<std::thread>([this](){ mainLoop(); });
}

NotifyCounter::~NotifyCounter() {
    quit = true;
    notifier.notify();
    thread->join();
}

void
NotifyCounter::mainLoop() {
    while (!quit) {
        notifier.wait();
        cnt++;
    }
}

static int
getCount(NotifyCounter& nc, int expected) {
    int c = -1;
    for (int i = 0; i < 50; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        c = nc.getCount();
        if (c == expected)
            break;
    }
    return c;
}

void
ParallelTest::testCommunicator() {
    Notifier notifier0;
    NotifyCounter c0(notifier0);
    ThreadCommunicator root(nullptr, notifier0);

    Notifier notifier1;
    NotifyCounter c1(notifier1);
    ThreadCommunicator child1(&root, notifier1);

    Notifier notifier2;
    NotifyCounter c2(notifier2);
    ThreadCommunicator child2(&root, notifier2);

    Notifier notifier3;
    NotifyCounter c3(notifier3);
    ThreadCommunicator child3(&child2, notifier3);

    ASSERT_EQUAL(0, c0.getCount());
    ASSERT_EQUAL(0, c1.getCount());
    ASSERT_EQUAL(0, c2.getCount());
    ASSERT_EQUAL(0, c3.getCount());

    Position pos;
    SearchTreeInfo sti;
    std::vector<U64> posHashList(200);
    int posHashListSize = 0;
    root.sendInitSearch(pos, sti, posHashList, posHashListSize);
    ASSERT_EQUAL(0, getCount(c0, 0));
    ASSERT_EQUAL(1, getCount(c1, 1));
    ASSERT_EQUAL(1, getCount(c2, 1));
    ASSERT_EQUAL(0, getCount(c3, 0));

    class Handler : public Communicator::CommandHandler {
    public:
        Handler(Communicator& comm) : comm(comm) {}

        void initSearch(const Position& pos, const SearchTreeInfo& sti,
                        const std::vector<U64>& posHashList, int posHashListSize) override {
            comm.sendInitSearch(pos, sti, posHashList, posHashListSize);
            nInit++;
        }
        void startSearch(int jobId, int alpha, int beta, int depth) override {
            comm.sendStartSearch(jobId, alpha, beta, depth);
            nStart++;
        }
        void stopSearch() override {
            comm.sendStopSearch();
            nStop++;
        }
        void reportResult(int jobId, int score) override {
            comm.sendReportResult(jobId, score);
            nReport++;
        }

        int getNInit() const { return nInit; }
        int getNStart() const { return nStart; }
        int getNStop() const { return nStop; }
        int getNReport() const { return nReport; };

    private:
        Communicator& comm;
        int nInit = 0;
        int nStart = 0;
        int nStop = 0;
        int nReport = 0;
    };

    Handler h0(root);
    Handler h1(child1);
    Handler h2(child2);

    child1.poll(h1);
    ASSERT_EQUAL(0, getCount(c0, 0));
    ASSERT_EQUAL(1, getCount(c1, 1));
    ASSERT_EQUAL(1, getCount(c2, 1));
    ASSERT_EQUAL(0, getCount(c3, 0));
    ASSERT_EQUAL(1, h1.getNInit());
    ASSERT_EQUAL(0, h2.getNInit());

    child2.poll(h2);
    ASSERT_EQUAL(0, getCount(c0, 0));
    ASSERT_EQUAL(1, getCount(c1, 1));
    ASSERT_EQUAL(1, getCount(c2, 1));
    ASSERT_EQUAL(1, getCount(c3, 1));
    ASSERT_EQUAL(1, h1.getNInit());
    ASSERT_EQUAL(1, h2.getNInit());

    int jobId = 1;
    root.sendStartSearch(jobId, -100, 100, 3);
    ASSERT_EQUAL(2, getCount(c1, 2));
    ASSERT_EQUAL(2, getCount(c2, 2));
    child2.poll(h2);
    ASSERT_EQUAL(1, h2.getNStart());
    ASSERT_EQUAL(2, getCount(c3, 2));

    child3.sendReportResult(jobId, 17);
    ASSERT_EQUAL(3, getCount(c2, 3));
    ASSERT_EQUAL(0, getCount(c0, 0));

    child2.poll(h2);
    ASSERT_EQUAL(1, h2.getNReport());
    ASSERT_EQUAL(1, getCount(c0, 1));
    root.poll(h0);
    ASSERT_EQUAL(1, h0.getNReport());

    ASSERT_EQUAL(2, getCount(c1, 2));
    ASSERT_EQUAL(0, h1.getNReport());

    // Node counters
    ASSERT_EQUAL(0, root.getNumSearchedNodes());
    ASSERT_EQUAL(0, child1.getNumSearchedNodes());
    ASSERT_EQUAL(0, child2.getNumSearchedNodes());
    ASSERT_EQUAL(0, child3.getNumSearchedNodes());
    ASSERT_EQUAL(0, root.getTbHits());
    ASSERT_EQUAL(0, child1.getTbHits());
    ASSERT_EQUAL(0, child2.getTbHits());
    ASSERT_EQUAL(0, child3.getTbHits());

    child3.sendReportStats(100, 10);
    ASSERT_EQUAL(0, root.getNumSearchedNodes());
    ASSERT_EQUAL(100, child2.getNumSearchedNodes());
    ASSERT_EQUAL(0, child3.getNumSearchedNodes());
    ASSERT_EQUAL(0, root.getTbHits());
    ASSERT_EQUAL(10, child2.getTbHits());
    ASSERT_EQUAL(0, child3.getTbHits());
    ASSERT_EQUAL(3, getCount(c2, 3));
    ASSERT_EQUAL(2, getCount(c3, 2));

    child2.poll(h2);
    ASSERT_EQUAL(0, root.getNumSearchedNodes());
    ASSERT_EQUAL(0, child3.getNumSearchedNodes());
    ASSERT_EQUAL(100, child2.getNumSearchedNodes());
    ASSERT_EQUAL(0, root.getTbHits());
    ASSERT_EQUAL(0, child3.getTbHits());
    ASSERT_EQUAL(10, child2.getTbHits());
    ASSERT_EQUAL(3, getCount(c2, 3));
    ASSERT_EQUAL(2, getCount(c3, 2));

    child2.sendReportStats(200, 30);
    ASSERT_EQUAL(300, root.getNumSearchedNodes());
    ASSERT_EQUAL(0, child3.getNumSearchedNodes());
    ASSERT_EQUAL(0, child2.getNumSearchedNodes());
    ASSERT_EQUAL(40, root.getTbHits());
    ASSERT_EQUAL(0, child3.getTbHits());
    ASSERT_EQUAL(0, child2.getTbHits());
    ASSERT_EQUAL(3, getCount(c2, 3));
    ASSERT_EQUAL(2, getCount(c3, 2));
}

cute::suite
ParallelTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testCommunicator));
    return s;
}
