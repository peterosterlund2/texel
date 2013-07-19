/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
#include "parallelTest.hpp"

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
    runSuite(ParallelTest());
    return 0;
}
