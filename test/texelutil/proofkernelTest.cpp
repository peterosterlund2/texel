/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * proofkernelTest.cpp
 *
 *  Created on: Oct 23, 2021
 *      Author: petero
 */

#include "proofkernelTest.hpp"
#include "proofkernel.hpp"
#include "position.hpp"

#include "gtest/gtest.h"


TEST(ProofKernelTest, testPawnColumn) {
    ProofKernelTest::testPawnColumn();
}

void
ProofKernelTest::testPawnColumn() {
    using PawnColumn = ProofKernel::PawnColumn;
    auto WHITE = ProofKernel::WHITE;
    auto BLACK = ProofKernel::BLACK;

    PawnColumn col;
    ASSERT_EQ(0, col.nPawns());

    col.addPawn(0, WHITE);
    ASSERT_EQ(1, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));

    col.addPawn(1, BLACK);
    ASSERT_EQ(2, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(BLACK, col.getPawn(1));

    col.addPawn(0, BLACK);
    ASSERT_EQ(3, col.nPawns());
    ASSERT_EQ(BLACK, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));

    col.addPawn(1, WHITE);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(BLACK, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(WHITE, col.getPawn(2));
    ASSERT_EQ(BLACK, col.getPawn(3));

    col.removePawn(0);
    ASSERT_EQ(3, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));

    col.addPawn(3, WHITE);
    col.addPawn(4, BLACK);
    ASSERT_EQ(5, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(WHITE, col.getPawn(3));
    ASSERT_EQ(BLACK, col.getPawn(4));

    col.removePawn(3);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(BLACK, col.getPawn(3));

    col.setPawn(3, WHITE);
    ASSERT_EQ(4, col.nPawns());
    ASSERT_EQ(WHITE, col.getPawn(0));
    ASSERT_EQ(WHITE, col.getPawn(1));
    ASSERT_EQ(BLACK, col.getPawn(2));
    ASSERT_EQ(WHITE, col.getPawn(3));

    for (int i = 0; i < 2; i++) {
        col.setPawn(1, BLACK);
        ASSERT_EQ(4, col.nPawns());
        ASSERT_EQ(WHITE, col.getPawn(0));
        ASSERT_EQ(BLACK, col.getPawn(1));
        ASSERT_EQ(BLACK, col.getPawn(2));
        ASSERT_EQ(WHITE, col.getPawn(3));
    }
}

TEST(ProofKernelTest, testPawnColPromotion) {
    ProofKernelTest::testPawnColPromotion();
}

void
ProofKernelTest::testPawnColPromotion() {
    using PawnColumn = ProofKernel::PawnColumn;
    auto WHITE = ProofKernel::WHITE;
    auto BLACK = ProofKernel::BLACK;

    auto setPawns = [](PawnColumn& col,
                       const std::vector<ProofKernel::PieceColor>& v) {
        while (col.nPawns() > 0)
            col.removePawn(0);
        for (auto c : v)
            col.addPawn(col.nPawns(), c);
    };

    PawnColumn col;
    setPawns(col, {WHITE, BLACK});
    ASSERT_EQ(0, col.nPromotions(WHITE));
    ASSERT_EQ(0, col.nPromotions(BLACK));

    setPawns(col, {BLACK, WHITE});
    ASSERT_EQ(1, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {WHITE});
    ASSERT_EQ(1, col.nPromotions(WHITE));
    ASSERT_EQ(0, col.nPromotions(BLACK));

    setPawns(col, {BLACK});
    ASSERT_EQ(0, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {BLACK, WHITE, WHITE});
    ASSERT_EQ(2, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {BLACK, WHITE, BLACK, WHITE, WHITE});
    ASSERT_EQ(2, col.nPromotions(WHITE));
    ASSERT_EQ(1, col.nPromotions(BLACK));

    setPawns(col, {WHITE, WHITE});
    ASSERT_EQ(2, col.nPromotions(WHITE));
    ASSERT_EQ(0, col.nPromotions(BLACK));

    setPawns(col, {BLACK, BLACK});
    ASSERT_EQ(0, col.nPromotions(WHITE));
    ASSERT_EQ(2, col.nPromotions(BLACK));
}
