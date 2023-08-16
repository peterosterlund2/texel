/*
    Texel - A UCI chess engine.
    Copyright (C) 2023  Peter Österlund, peterosterlund2@gmail.com

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
 * nnTest.hpp
 *
 *  Created on: Jul 25, 2023
 *      Author: petero
 */

#include "nnTest.hpp"

#include "nntypes.hpp"
#include "vectorop.hpp"
#include "textio.hpp"
#include "position.hpp"
#include "evaluate.hpp"

#include <vector>
#include <string>

#include "gtest/gtest.h"

TEST(NNTest, testMatMul) {
    NNTest::testMatMul();
}

void
NNTest::testMatMul() {
    {
        alignas(32) Matrix<S8,1,32> w;
        alignas(32) Vector<S8,32> in;
        alignas(32) Vector<S32,1> res;
        res(0) = 0;
        for (int i = 0; i < 32; i++) {
            w(0,i) = i+1;
            in(i) = i+2;
        }
        prepareMatMul(w);
        matMul(res, w, in);
        ASSERT_EQ(11968, res(0));
    }

    {
        alignas(32) Matrix<S8,1,32> w;
        alignas(32) Vector<S8,32> in;
        alignas(32) Vector<S32,1> res;
        res(0) = 0;
        for (int i = 0; i < 32; i++) {
            w(0,i) = (i & 1) ? -(i+1) : (i+1);
            in(i) = i+2;
        }
        prepareMatMul(w);
        matMul(res, w, in);
        ASSERT_EQ(-544, res(0));
    }

    {
        alignas(32) Matrix<S8,2,32> w;
        alignas(32) Vector<S8,32> in;
        alignas(32) Vector<S32,2> res;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 32; j++) {
                w(i,j) = i == 0 ? 127 : -127;
                in(j) = 127;
            }
            res(i) = 0;
        }
        prepareMatMul(w);
        matMul(res, w, in);
        ASSERT_EQ(127*127*32, res(0));
        ASSERT_EQ(-127*127*32, res(1));
    }

    {
        alignas(32) Matrix<S8,32,512> w;
        alignas(32) Vector<S8,512> in;
        alignas(32) Vector<S32,32> res;
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 512; j++)
                w(i,j) = (i + j * 7) % 64 - 32;
            res(i) = i;
        }
        for (int j = 0; j < 512; j++)
            in(j) = (j * j + 1) % 11;

        prepareMatMul(w);
        matMul(res, w, in);
        std::vector<int> expected = {
            -627, -820, -1077, -1270, -1271, -952, -889, -954,
            -1019, -1084, -1021, -702, -703, -896, -1153, -1410,
            -1603, -1604, -1285, -1222, -1287, -1352, -1417, -1354,
            -1035, -1036, -1229, -1486, -1679, -1872, -1873, -1554
        };
        for (int j = 0; j < 32; j++) {
            ASSERT_EQ(expected[j], res(j));
        }
    }
}

namespace {
    class Evaluator {
    public:
        Evaluator(bool incremental);
        void setPos(const std::string fen);
        void makeMove(const std::string& move);
        void makeNullMove();
        int getEval();
        void undo();
    private:
        bool incremental;
        Position pos;
        std::unique_ptr<Evaluate::EvalHashTables> et;
        Evaluate eval;
        std::vector<Move> moveStack;
        std::vector<UndoInfo> undoStack;
    };

    Evaluator::Evaluator(bool incremental)
        : incremental(incremental),
          et(Evaluate::getEvalHashTables()),
          eval(*et) {
        eval.connectPosition(pos);
    }

    void Evaluator::setPos(const std::string fen) {
        pos = TextIO::readFEN(fen);
        moveStack.clear();
        undoStack.clear();
    }

    void Evaluator::makeMove(const std::string& move) {
        if (!incremental)
            et->nnEval->forceFullEval();
        Position pos2(pos);
        Move m = TextIO::stringToMove(pos2, move);
        ASSERT_TRUE(!m.isEmpty());
        UndoInfo ui;
        pos.makeMove(m, ui);
        moveStack.push_back(m);
        undoStack.push_back(ui);
    }

    void Evaluator::makeNullMove() {
        pos.setWhiteMove(!pos.isWhiteMove());
        moveStack.clear();
        undoStack.clear();
    }

    int Evaluator::getEval() {
        if (!incremental)
            et->nnEval->forceFullEval();
        return eval.evalPos();
    }

    void Evaluator::undo() {
        if (!incremental)
            et->nnEval->forceFullEval();
        ASSERT_GT(moveStack.size(), 0);
        pos.unMakeMove(moveStack.back(), undoStack.back());
        moveStack.pop_back();
        undoStack.pop_back();
    }
}

TEST(NNTest, testIncremental) {
    NNTest::testIncremental();
}

void
NNTest::testIncremental() {
    auto test = [](const std::string& startFen, const std::vector<std::string>& cmdSeq) {
        Evaluator incrEval(true);  incrEval.setPos(startFen);
        Evaluator fullEval(false); fullEval.setPos(startFen);
        for (size_t i = 0; i < cmdSeq.size(); i++) {
            std::string cmd = cmdSeq[i];
            if (cmd == ":e") {
                int score1 = incrEval.getEval();
                int score2 = fullEval.getEval();
                std::string allCmds;
                for (size_t j = 0; j <= i; j++)
                    allCmds += ' ' + cmdSeq[j];
                ASSERT_EQ(score1, score2) << " fen: " << startFen << " i: " << i << " cmd:" << allCmds;
            } else if (cmd == ":u") {
                incrEval.undo();
                fullEval.undo();
            } else if (startsWith(cmd, ":set:")) {
                std::string newFen = cmd.substr(5);
                incrEval.setPos(newFen);
                fullEval.setPos(newFen);
            } else if (cmd == ":null") {
                incrEval.makeNullMove();
                fullEval.makeNullMove();
            } else {
                incrEval.makeMove(cmd);
                fullEval.makeMove(cmd);
            }
        }
    };

    test(TextIO::startPosFEN,
         {":e", "e4", ":e", "e5", ":e", ":u", "d5", ":e", ":u", ":e", ":u", ":e"});
    test("8/4k3/2b5/8/8/3BPN2/4K3/8 w - - 0 1",
         {"Kf2", "Bd5", "Bb5", "Bb7", "Ke2", ":e", ":u", ":u", ":u", ":u", ":u", ":e"});
    test(TextIO::startPosFEN,
         {":e", "e4", ":e", "e5", ":e", "Nf3", ":e", "Nc6", ":e", "Bb5", ":e", "a6", ":e",
          "Ba4", ":e", "Nf6", ":e", "O-O", ":e", "Be7", ":e",
          ":u", "Bc5", ":e",
          ":u", ":e", ":u", ":e", ":u", ":e", ":u", ":e",
          ":u", ":e", ":u", ":e", ":u", ":e", ":u", ":e",
          ":u", ":e", ":u", ":e"});
    test(TextIO::startPosFEN,
         {"e4", "e5", "Nf3", "Nc6", "Bb5", "a6", "Ba4", "Nf6", ":e",
          ":u", ":u", ":u", ":u", ":u", ":u", ":u", ":u", ":e"});
    test("2r1r3/1p1q2kp/p1nP1pp1/3B1b2/5P2/B1Q3P1/7P/R3R1K1 w - - 0 1",
         {"Qxf6+", ":e", ":u", "Qxc6", ":e", ":u", "Bxc6", ":e", ":u",
          "Bf7", ":e", "Qxf7", ":e", ":u", "Qxd6", ":e", ":u", ":u", ":e"});
    test(TextIO::startPosFEN,
         {"e4", ":e", "e5", ":e", "f4", ":e", "exf4", ":e", "g4", ":e", "fxg3", ":e",
          "Nf3", ":e", "g2", ":e", "Nc3", ":e", "gxh1Q", ":e",
          ":u", "g1R", ":e",
          ":u", ":e", ":u", ":e", ":u", ":e", ":u", ":e", ":u", ":e",
          ":u", ":e", ":u", ":e", ":u", ":e", ":u", ":e", ":u", ":e"});
    test(TextIO::startPosFEN,
         {"e4", "e5", "Nf3", "Nc6", ":e", ":u", ":u", ":u", ":u", "d4", ":e"});
    test("8/3k4/2r5/8/8/8/5B2/3N2K1 w - - 0 1",
         {":e", "Kg2", "Ke7", "Be3", "Rd6", "Kg1", "Kd7", ":e",
          ":u", ":u", ":u", ":u", ":u", ":u", ":e"});
    test(TextIO::startPosFEN,
         {"e4", ":e", "c5", ":e",
          ":set:8/3k4/2r5/8/6R1/8/8/3NK3 w - - 0 1", ":e",
          "Ne3", ":e", "Ke7", ":e", "Nc2", ":e",
          ":u", ":e", ":u", ":e", ":u", ":e"});
    test(TextIO::startPosFEN,
         {"Nf3", "Nc6", "Nh4", "Na5", "Nf3", ":e",
          ":u", ":u", ":u", ":u", ":u", ":e"});
    test(TextIO::startPosFEN, {":e", "Nf3", ":null", "Ng1", ":e"});
}
