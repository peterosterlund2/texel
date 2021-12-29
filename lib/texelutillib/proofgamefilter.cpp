/*
    Texel - A UCI chess engine.
    Copyright (C) 2015-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * proofgamefilter.cpp
 *
 *  Created on: Dec 24, 2021
 *      Author: petero
 */

#include "proofgamefilter.hpp"
#include "proofgame.hpp"
#include "textio.hpp"
#include "proofkernel.hpp"

#include <iostream>
#include <climits>


ProofGameFilter::ProofGameFilter() {
}

void ProofGameFilter::filterFens(std::istream& is, std::ostream& os) {
    const std::string startPosFEN = TextIO::startPosFEN;
    Position startPos = TextIO::readFEN(startPosFEN);
    while (true) {
        std::string line;
        std::getline(is, line);
        if (!is || is.eof())
            break;
        line = trim(line);

        std::string status;
        try {
            ProofGame pg(std::cerr, startPosFEN, line, 1, 1, false, true);
            std::vector<Move> movePath;
            int minCost = pg.search({}, movePath, ProofGame::Options().setMaxNodes(2));
            if (minCost == INT_MAX) {
                status = "illegal, other";
            } else if (minCost >= 0) {
                status = "legal, proof:";
                Position pos = startPos;
                UndoInfo ui;
                for (size_t i = 0; i < movePath.size(); i++) {
                    status += ' ';
                    status += TextIO::moveToString(pos, movePath[i], false);
                    pos.makeMove(movePath[i], ui);
                }
            } else {
                U64 blocked;
                if (!pg.computeBlocked(startPos, blocked))
                    blocked = 0xffffffffffffffffULL; // If goal not reachable, consider all pieces blocked
                ProofKernel pk(startPos, pg.getGoalPos(), blocked);
                std::vector<ProofKernel::PkMove> kernel;
                std::vector<ProofKernel::ExtPkMove> extKernel;
                auto res = pk.findProofKernel(kernel, extKernel);
                if (res == ProofKernel::FAIL) {
                    status = "illegal, no proof kernel";
                    if (!kernel.empty()) {
                        status += ", forced:";
                        for (const auto& m : kernel) {
                            status += ' ';
                            status += toString(m);
                        }
                    }
                } else if (res == ProofKernel::PROOF_KERNEL) {
                    status = "illegal, no extended proof kernel";
                } else {
                    status = "unknown, kernel:";
                    for (const auto& m : kernel) {
                        status += ' ';
                        status += toString(m);
                    }
                    status += " extKernel:";
                    for (const auto& m : extKernel) {
                        status += ' ';
                        status += toString(m);
                    }
                }
            }
        } catch (NotImplementedError& e) {
            status = std::string("unknown, ") + e.what();
        } catch (ChessError& e) {
            status = std::string("illegal, ") + e.what();
        }
        os << line << " " << status << std::endl;
    }
}
