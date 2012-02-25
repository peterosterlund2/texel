#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "moveTest.hpp"

void runSuite(const std::string& suiteName, const cute::suite& s) {
    cute::ide_listener lis;
    cute::makeRunner(lis)(s, suiteName.c_str());
}

int main(){
    runSuite("moveTest", make_suite_moveTest());
    return 0;
}
