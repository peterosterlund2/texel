#include "computerPlayer.hpp"

#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"
#include "bookBuildTest.hpp"
#include "proofgameTest.hpp"
#include "gameTreeTest.hpp"


static void
runTests() {
    auto runSuite = [](const UtilSuiteBase& suite) {
        cute::ide_listener<> lis;
        cute::makeRunner(lis)(suite.getSuite(), suite.getName().c_str());
    };

    ComputerPlayer::initEngine();
    runSuite(BookBuildTest());
    runSuite(ProofGameTest());
    runSuite(GameTreeTest());
}


int main() {
    runTests();
}
