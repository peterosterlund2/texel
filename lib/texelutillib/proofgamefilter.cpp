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
#include <mutex>


using PkMove = ProofKernel::PkMove;
using ExtPkMove = ProofKernel::ExtPkMove;

std::vector<ProofGameFilter::InfoData> ProofGameFilter::infoStrData;

void
ProofGameFilter::init() {
    infoStrData.push_back({ ILLEGAL,    "illegal"   });
    infoStrData.push_back({ UNKNOWN,    "unknown"   });
    infoStrData.push_back({ LEGAL,      "legal"     });

    infoStrData.push_back({ FORCED,     "forced"    });

    infoStrData.push_back({ KERNEL,     "kernel"    });
    infoStrData.push_back({ EXT_KERNEL, "extKernel" });
    infoStrData.push_back({ PATH,       "path"      });
    infoStrData.push_back({ STATUS,     "status"    });
    infoStrData.push_back({ FAIL,       "fail"      });

    infoStrData.push_back({ PROOF,      "proof"     });
}

ProofGameFilter::ProofGameFilter() {
    std::once_flag flag;
    call_once(flag, init);
}

void
ProofGameFilter::filterFens(std::istream& is, std::ostream& os) {
    Position startPos = TextIO::readFEN(TextIO::startPosFEN);
    while (true) {
        Line line;
        if (!readLine(is, line))
            break;

        if (!line.hasToken(ILLEGAL) && !line.hasToken(UNKNOWN) && !line.hasToken(LEGAL)) {
            computeExtProofKernel(startPos, line, os);
        } else if (line.hasToken(UNKNOWN) && !line.hasToken(FAIL)) {
            if (!line.hasToken(PATH)) {
                computePath(startPos, line, os);
            } else if (!line.hasToken(PROOF)) {
                computeProofGame(startPos, line, os);
            } else {
                line.print(os);
            }
        } else {
            line.print(os);
        }
    }
}

void
ProofGameFilter::computeExtProofKernel(const Position& startPos, Line& line, std::ostream& os) {
    auto setIllegal = [&line](const std::string& reason) {
        std::vector<std::string>& illegal = line.tokenData(ILLEGAL);
        illegal.clear();
        illegal.push_back(reason);
    };

    try {
        auto opts = ProofGame::Options().setSmallCache(true).setMaxNodes(2);
        ProofGame pg(TextIO::startPosFEN, line.fen, {}, std::cerr);
        std::vector<Move> movePath;
        int minCost = pg.search(movePath, opts);
        if (minCost == INT_MAX) {
            setIllegal("Other");
        } else if (minCost >= 0) {
            line.tokenData(LEGAL).clear();
            std::vector<std::string>& proof = line.tokenData(PROOF);
            proof.clear();
            Position pos = startPos;
            UndoInfo ui;
            for (size_t i = 0; i < movePath.size(); i++) {
                proof.push_back(TextIO::moveToString(pos, movePath[i], false));
                pos.makeMove(movePath[i], ui);
            }
        } else {
            U64 blocked;
            if (!pg.computeBlocked(startPos, blocked))
                blocked = 0xffffffffffffffffULL; // If goal not reachable, consider all pieces blocked
            ProofKernel pk(startPos, pg.getGoalPos(), blocked);
            std::vector<PkMove> kernel;
            std::vector<ExtPkMove> extKernel;
            auto res = pk.findProofKernel(kernel, extKernel);
            if (res == ProofKernel::FAIL) {
                setIllegal("No proof kernel");
                if (!kernel.empty()) {
                    std::vector<std::string>& forced = line.tokenData(FORCED);
                    for (const auto& m : kernel)
                        forced.push_back(toString(m));
                }
            } else if (res == ProofKernel::PROOF_KERNEL) {
                setIllegal("No extended proof kernel");
            } else {
                line.tokenData(UNKNOWN).clear();
                std::vector<std::string>& kernelInfo = line.tokenData(KERNEL);
                kernelInfo.clear();
                for (const auto& m : kernel)
                    kernelInfo.push_back(toString(m));
                std::vector<std::string>& extKernelInfo = line.tokenData(EXT_KERNEL);
                extKernelInfo.clear();
                for (const auto& m : extKernel)
                    extKernelInfo.push_back(toString(m));
            }
        }
    } catch (const NotImplementedError& e) {
        line.tokenData(UNKNOWN).clear();
        line.tokenData(UNKNOWN).push_back(e.what());
    } catch (const ChessError& e) {
        setIllegal(e.what());
    }
    line.print(os);
}

void
ProofGameFilter::computePath(const Position& startPos, Line& line, std::ostream& os) {
    line.print(os);
}

void
ProofGameFilter::computeProofGame(const Position& startPos, Line& line, std::ostream& os) {
    line.print(os);
}

// ----------------------------------------------------------------------------

bool
ProofGameFilter::readLine(std::istream& is, Line& line) {
    std::string lineStr;
    std::vector<std::string> arr;
    {
        std::getline(is, lineStr);
        if (!is || is.eof())
            return false;
        lineStr = trim(lineStr);

        splitString(lineStr, arr);
    }

    if (arr.size() < 6)
        return false;

    line.fen.clear();
    for (int i = 0; i < 6; i++) {
        if (i > 0)
            line.fen += ' ';
        line.fen += arr[i];
    }

    Info info = STATUS;
    bool infoValid = false;
    for (int i = 6; i < (int)arr.size(); i++) {
        const std::string& token = arr[i];
        if (endsWith(token, ":")) {
            info = str2Info(token.substr(0, token.length() - 1));
            line.data[info].clear();
            infoValid = true;
        } else {
            if (!infoValid)
                throw ChessParseError("Invalid line format: " + lineStr);
            line.data[info].push_back(token);
        }
    }

    return true;
}

ProofGameFilter::Info
ProofGameFilter::str2Info(const std::string& token) {
    for (auto& e : infoStrData)
        if (e.tokenStr == token)
            return e.info;
    throw ChessParseError("Invalid line format: " + token);
}

std::string
ProofGameFilter::info2Str(Info info) {
    for (auto& e : infoStrData)
        if (e.info == info)
            return e.tokenStr;
    assert(false);
    throw ChessError("internal error");
}

void
ProofGameFilter::Line::print(std::ostream& os) {
    os << fen;

    auto printTokenData = [&os,this](Info tokType) {
        if (hasToken(tokType)) {
            os << ' ' << info2Str(tokType) << ':';
            std::vector<std::string>& data = tokenData(tokType);
            for (const std::string& s : data)
                os << ' ' << s;
        }
    };

    if (hasToken(ILLEGAL)) {
        printTokenData(ILLEGAL);
        printTokenData(FORCED);
    } else if (hasToken(UNKNOWN)) {
        printTokenData(UNKNOWN);
        printTokenData(KERNEL);
        printTokenData(EXT_KERNEL);
        printTokenData(PATH);
        printTokenData(STATUS);
        printTokenData(FAIL);
    } else if (hasToken(LEGAL)) {
        printTokenData(LEGAL);
        printTokenData(PROOF);
    }

    os << std::endl;
}
