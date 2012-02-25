#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "moveTest.hpp"

void
runSuite(const SuiteBase& suite) {
    cute::ide_listener lis;
    cute::makeRunner(lis)(suite.getSuite(), suite.getName().c_str());
}

int main(){
    runSuite(MoveTest());
    return 0;
}
