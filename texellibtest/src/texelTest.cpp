#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "bitBoardTest.hpp"
#include "bookTest.hpp"
#include "computerPlayerTest.hpp"
#include "evaluateTest.hpp"
#include "gameTest.hpp"
#include "historyTest.hpp"
#include "killerTableTest.hpp"
#include "moveGenTest.hpp"
#include "moveTest.hpp"
#include "pieceTest.hpp"
#include "positionTest.hpp"
#include "searchTest.hpp"
#include "textioTest.hpp"
#include "transpositionTableTest.hpp"

void
runSuite(const SuiteBase& suite) {
    cute::ide_listener lis;
    cute::makeRunner(lis)(suite.getSuite(), suite.getName().c_str());
}

int main() {
    runSuite(BitBoardTest());
    runSuite(BookTest());
    runSuite(ComputerPlayerTest());
    runSuite(EvaluateTest());
    runSuite(GameTest());
    runSuite(HistoryTest());
    runSuite(KillerTableTest());
    runSuite(MoveGenTest());
    runSuite(MoveTest());
    runSuite(PieceTest());
    runSuite(PositionTest());
    runSuite(SearchTest());
    runSuite(TextIOTest());
    runSuite(TranspositionTableTest());
    return 0;
}
