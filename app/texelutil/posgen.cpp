/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * posgen.cpp
 *
 *  Created on: Jan 6, 2014
 *      Author: petero
 */

#include "posgen.hpp"
#include "textio.hpp"
#include "moveGen.hpp"
#include "tbprobe.hpp"
#include "search.hpp"
#include "clustertt.hpp"
#include "history.hpp"
#include "killerTable.hpp"
#include "syzygy/rtb-probe.hpp"
#include "parameters.hpp"
#include "util/timeUtil.hpp"
#include "util/random.hpp"
#include "chesstool.hpp"
#include "tbgen.hpp"
#include "proofgame.hpp"
#include "proofkernel.hpp"
#include "threadpool.hpp"

#include <fstream>
#include <iomanip>

bool
PosGenerator::generate(const std::string& type) {
    if (type == "qvsn")
        genQvsN();
    else
        return false;
    return true;
}

static void
writeFEN(const Position& pos) {
    std::cout << TextIO::toFEN(pos) << '\n';
}

void
PosGenerator::genQvsN() {
    for (int bk = 0; bk < 8; bk++) {
        for (int wk = 0; wk < 8; wk++) {
            for (int q1 = 0; q1 < 8; q1++) {
                if (q1 == wk)
                    continue;
                for (int q2 = q1 + 1; q2 < 8; q2++) {
                    if (q2 == wk)
                        continue;
                    for (int q3 = q2 + 1; q3 < 8; q3++) {
                        if (q3 == wk)
                            continue;
                        Position pos;
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Square::getSquare(i, 1), Piece::WPAWN);
                            pos.setPiece(Square::getSquare(i, 6), Piece::BPAWN);
                            pos.setPiece(Square::getSquare(i, 7), Piece::BKNIGHT);
                        }
                        pos.setPiece(Square::getSquare(bk, 7), Piece::BKING);
                        pos.setPiece(Square::getSquare(wk, 0), Piece::WKING);
                        pos.setPiece(Square::getSquare(q1, 0), Piece::WQUEEN);
                        pos.setPiece(Square::getSquare(q2, 0), Piece::WQUEEN);
                        pos.setPiece(Square::getSquare(q3, 0), Piece::WQUEEN);
                        writeFEN(pos);
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Square::getSquare(i, 6), Piece::EMPTY);
                            writeFEN(pos);
                            pos.setPiece(Square::getSquare(i, 6), Piece::BPAWN);
                        }
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Square::getSquare(i, 1), Piece::EMPTY);
                            writeFEN(pos);
                            pos.setPiece(Square::getSquare(i, 1), Piece::WPAWN);
                        }
                    }
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------

static bool typeOk(const std::vector<int>& pieces) {
    int nQ = count(pieces.begin(), pieces.end(), 0);
    int nR = count(pieces.begin(), pieces.end(), 1);
    int nB = count(pieces.begin(), pieces.end(), 2);
    int nN = count(pieces.begin(), pieces.end(), 3);
    int nP = count(pieces.begin(), pieces.end(), 4);
    return nP <= 8 - std::max(0, (nQ-1)) -
                     std::max(0, (nR-2)) -
                     std::max(0, (nB-2)) -
                     std::max(0, (nN-2));
}

static char pTypes[] = { 'q', 'r', 'b', 'n', 'p' };
static const int nTypes = COUNT_OF(pTypes);

static void
getPieceCombos(int nPieces, std::vector<std::string>& out) {
    std::vector<int> pieces(nPieces, 0);
    while (true) {
        if (typeOk(pieces)) {
            std::string s;
            for (int i = 0; i < nPieces; i++)
                s += pTypes[pieces[i]];
            out.push_back(s);
        }

        bool done = true;
        for (int i = nPieces - 1; i >= 0; i--) {
            if (pieces[i] < nTypes-1) {
                pieces[i]++;
                for (int j = i + 1; j < nPieces; j++)
                    pieces[j] = pieces[i];
                done = false;
                break;
            }
        }
        if (done)
            break;
    }
}

static bool
wrongOrder(const std::string& w, const std::string& b) {
    std::vector<int> wOrder, bOrder;
    for (auto c : w)
        for (int i = 0; i < nTypes; i++)
            if (pTypes[i] == c) {
                wOrder.push_back(i);
                break;
            }
    for (auto c : b)
        for (int i = 0; i < nTypes; i++)
            if (pTypes[i] == c) {
                bOrder.push_back(i);
                break;
            }
    return wOrder > bOrder;
}

void
PosGenerator::tbList(int nPieces) {
    nPieces -= 2;
    for (int nWhite = nPieces; nWhite >= 0; nWhite--) {
        int nBlack = nPieces - nWhite;
        if (nBlack > nWhite)
            continue;
        std::vector<std::string> wCombos, bCombos;
        getPieceCombos(nWhite, wCombos);
        getPieceCombos(nBlack, bCombos);
        std::string s;
        s += 'k';
        for (auto w : wCombos)
            for (auto b : bCombos) {
                if (nWhite == nBlack && wrongOrder(w, b))
                    continue;
                std::cout << 'k' << w << 'k' << b << '\n';
            }
    }
}

// --------------------------------------------------------------------------------

static void
getPieces(const std::string& tbType, std::vector<int>& pieces,
          bool& whitePawns, bool& blackPawns) {
    if (tbType.length() < 1 || tbType[0] != 'k')
        throw ChessParseError("Invalid tbType: " + tbType);
    whitePawns = blackPawns = false;
    bool white = true;
    for (size_t i = 1; i < tbType.length(); i++) {
        switch (tbType[i]) {
            case 'k':
                if (!white)
                    throw ChessParseError("Invalid tbType: " + tbType);
                white = false;
                break;
            case 'q':
                pieces.push_back(white ? Piece::WQUEEN: Piece::BQUEEN);
                break;
            case 'r':
                pieces.push_back(white ? Piece::WROOK: Piece::BROOK);
                break;
            case 'b':
                pieces.push_back(white ? Piece::WBISHOP: Piece::BBISHOP);
                break;
            case 'n':
                pieces.push_back(white ? Piece::WKNIGHT: Piece::BKNIGHT);
                break;
            case 'p':
                pieces.push_back(white ? Piece::WPAWN: Piece::BPAWN);
                if (white)
                    whitePawns = true;
                else
                    blackPawns = true;
                break;
            default:
                throw ChessParseError("Invalid tbType: " + tbType);
        }
    }
    if (white)
        throw ChessParseError("Invalid tbType: " + tbType);
}

/** Return true if square is a valid square for piece. */
inline bool
squareValid(int square, int piece) {
    if (piece == Piece::WPAWN || piece == Piece::BPAWN) {
        return square >= 8 && square < 56;
    } else {
        return true;
    }
}

/** Return bitboard containing possible en passant squares. */
static U64
getEPSquares(const Position& pos) {
    U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
    U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);
    U64 occupied = pos.occupiedBB();
    if (pos.isWhiteMove()) {
        U64 wPawnAttacks = ((wPawns & BitBoard::maskBToHFiles) << 7) |
                           ((wPawns & BitBoard::maskAToGFiles) << 9);
        return ((bPawns & BitBoard::maskRow5) << 8) & ~occupied & wPawnAttacks;
    } else {
        U64 bPawnAttacks = ((bPawns & BitBoard::maskBToHFiles) >> 9) |
                           ((bPawns & BitBoard::maskAToGFiles) >> 7);
        return ((wPawns & BitBoard::maskRow4) >> 8) & ~occupied & bPawnAttacks;
    }
}

template <typename Func>
static void
iteratePositions(const std::string& tbType, bool skipSymmetric, Func func) {
    std::vector<int> pieces;
    bool whitePawns, blackPawns;
    getPieces(tbType, pieces, whitePawns, blackPawns);
    const int nPieces = pieces.size();
    const bool anyPawns = whitePawns || blackPawns;
    const bool epPossible = whitePawns && blackPawns;

    bool symTable = true; // True if white and black have the same pieces
    {
        int nPieces[Piece::nPieceTypes] = { 0 };
        for (int p : pieces)
            nPieces[p]++;
        for (int pt = Piece::WQUEEN; pt <= Piece::WPAWN; pt++)
            if (nPieces[pt] != nPieces[Piece::makeBlack(pt)])
                symTable = false;
    }

    for (int wk = 0; wk < 64; wk++) {
        int x = Square::getX(wk);
        int y = Square::getY(wk);
        if (skipSymmetric) {
            if (x >= 4)
                continue;
            if (!anyPawns)
                if (y >= 4 || y < x)
                    continue;
        }
        for (int bk = 0; bk < 64; bk++) {
            int x2 = Square::getX(bk);
            int y2 = Square::getY(bk);
            if (std::abs(x2-x) < 2 && std::abs(y2-y) < 2)
                continue;

            Position pos;
            pos.setPiece(wk, Piece::WKING);
            pos.setPiece(bk, Piece::BKING);
            std::vector<int> squares(nPieces, 0);
            int nPlaced = 0;

            while (true) {
                // Place remaining pieces on first empty square. Multiple equal
                // pieces are placed starting with the lowest square.
                while (nPlaced < nPieces) {
                    const int p = pieces[nPlaced];
                    int first = 0;
                    if (nPlaced > 0 && pieces[nPlaced-1] == p)
                        first = squares[nPlaced-1] + 1;
                    bool ok = false;
                    for (int sq = first; sq < 64; sq++) {
                        if (!squareValid(sq, p))
                            continue;
                        if (pos.getPiece(sq) == Piece::EMPTY) {
                            pos.setPiece(sq, p);
                            squares[nPlaced] = sq;
                            nPlaced++;
                            ok = true;
                            break;
                        }
                    }
                    if (!ok)
                        break;
                }

                if (nPlaced == nPieces) {
                    pos.setWhiteMove(true);
                    bool wKingAttacked = MoveGen::sqAttacked(pos, wk);
                    pos.setWhiteMove(false);
                    bool bKingAttacked = MoveGen::sqAttacked(pos, bk);
                    for (int side = 0; side < 2; side++) {
                        bool white = side == 0;
                        if (white) {
                            if (bKingAttacked)
                                continue;
                        } else {
                            if (wKingAttacked)
                                continue;
                        }
                        if (skipSymmetric && symTable && !white)
                            continue;
                        pos.setWhiteMove(white);

                        U64 epSquares = epPossible ? getEPSquares(pos) : 0;
                        while (true) {
                            if (epSquares) {
                                int epSq = BitBoard::firstSquare(epSquares);
                                pos.setEpSquare(epSq);
                                TextIO::fixupEPSquare(pos);
                                if (pos.getEpSquare() == -1) {
                                    epSquares &= epSquares - 1;
                                    continue;
                                }
                            } else {
                                pos.setEpSquare(-1);
                            }
                            func(pos);
                            if (epSquares == 0)
                                break;
                            epSquares &= epSquares - 1;
                        }
                    }
                }

                // Set up next position
                bool done = false;
                while (true) {
                    nPlaced--;
                    if (nPlaced < 0) {
                        done = true;
                        break;
                    }
                    int sq0 = squares[nPlaced];
                    int p = pos.getPiece(sq0);
                    pos.setPiece(sq0, Piece::EMPTY);
                    bool foundEmpty = false;
                    for (int sq = sq0 + 1; sq < 64; sq++) {
                        if (!squareValid(sq, p))
                            continue;
                        if (pos.getPiece(sq) == Piece::EMPTY) {
                            pos.setPiece(sq, p);
                            squares[nPlaced] = sq;
                            nPlaced++;
                            foundEmpty = true;
                            break;
                        }
                    }
                    if (foundEmpty)
                        break;
                }
                if (done)
                    break;
            }
        }
    }
}

/** Call func(pos) for all positions in a given tablebase.
 * func() must not modify pos. */
template <typename Func>
static void
iteratePositions(const std::string& tbType, Func func) {
    iteratePositions(tbType, true, func);
}

void
PosGenerator::dtmStat(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        int negScore = std::numeric_limits<int>::min(); // Largest negative score
        int posScore = std::numeric_limits<int>::max(); // Smallest positive score
        Position negPos, posPos;
        U64 nPos = 0;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int score;
            if (!TBProbe::gtbProbeDTM(pos, 0, score))
                throw ChessError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            if (score > 0) {
                if (score < posScore) {
                    posScore = score;
                    posPos = pos;
                }
            } else if (score < 0) {
                if (score > negScore) {
                    negScore = score;
                    negPos = pos;
                }
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " neg: " << negScore << " pos:" << posScore << " nPos:" << nPos
                  << " t:" << (t1-t0) << std::endl;
        std::cout << tbType << " negPos: " << TextIO::toFEN(negPos) << std::endl;
        std::cout << tbType << " posPos: " << TextIO::toFEN(posPos) << std::endl;
    }
}

void
PosGenerator::dtzStat(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        int negScore = std::numeric_limits<int>::max(); // Smallest negative score
        int posScore = std::numeric_limits<int>::min(); // Largest positive score
        Position negPos, posPos;
        U64 nPos = 0;
        int negReported = -1000;
        int posReported = 1000;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int success;
            int dtz = Syzygy::probe_dtz(pos, &success);
            if (!success)
                throw ChessError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            int wdl = Syzygy::probe_wdl(pos, &success);
            if (!success)
                throw ChessError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            if (dtz > 0) {
                if (wdl == 2) {
                    if (dtz > posScore) {
                        posScore = dtz;
                        posPos = pos;
                    }
                    if (dtz > 100 && dtz < posReported) {
                        posReported = dtz;
                        std::cout << "fen: " << TextIO::toFEN(pos) << " dtz:" << dtz << std::endl;
                    }
                }
            } else if (dtz < 0) {
                if (wdl == -2) {
                    if (dtz < negScore) {
                        negScore = dtz;
                        negPos = pos;
                    }
                    if (dtz < -100 && dtz > negReported) {
                        negReported = dtz;
                        std::cout << "fen: " << TextIO::toFEN(pos) << " dtz:" << dtz << std::endl;
                    }
                }
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " neg: " << negScore << " pos:" << posScore << " nPos:" << nPos
                  << " t:" << (t1-t0) << std::endl;
        std::cout << tbType << " negPos: " << TextIO::toFEN(negPos) << std::endl;
        std::cout << tbType << " posPos: " << TextIO::toFEN(posPos) << std::endl;
    }
}

/** Convert a 2-character string to a piece type. */
static Piece::Type
getPieceType(const std::string& s) {
    if (s.length() != 2)
        return Piece::EMPTY;
    bool white;
    if (s[0] == 'w')
        white = true;
    else if (s[0] == 'b')
        white = false;
    else
        return Piece::EMPTY;
    switch (s[1]) {
    case 'k': return white ? Piece::WKING : Piece::BKING;
    case 'q': return white ? Piece::WQUEEN : Piece::BQUEEN;
    case 'r': return white ? Piece::WROOK : Piece::BROOK;
    case 'b': return white ? Piece::WBISHOP : Piece::BBISHOP;
    case 'n': return white ? Piece::WKNIGHT : Piece::BKNIGHT;
    case 'p': return white ? Piece::WPAWN : Piece::BPAWN;
    default:
        return Piece::EMPTY;
    }
}

void
PosGenerator::egStat(const std::string& tbType, const std::vector<std::string>& pieceTypes) {
    ChessTool::setupTB();
    double t0 = currentTime();

    std::vector<Piece::Type> ptVec;
    for (const std::string& s : pieceTypes) {
        Piece::Type p = getPieceType(s);
        if (p == Piece::EMPTY)
            throw ChessParseError("Invalid piece type:" + s);
        ptVec.push_back(p);
    }

    TranspositionTable tt(512*1024);
    Notifier notifier;
    ThreadCommunicator comm(nullptr, tt, notifier, false);
    std::vector<U64> nullHist(SearchConst::MAX_SEARCH_DEPTH * 2);
    KillerTable kt;
    History ht;
    auto et = Evaluate::getEvalHashTables();
    Search::SearchTables st(comm.getCTT(), kt, ht, *et);
    TreeLogger treeLog;
    TranspositionTable::TTEntry ent;
    const int UNKNOWN_SCORE = -32767; // Represents unknown static eval score

    struct ScoreStat { U64 whiteWin = 0, draw = 0, blackWin = 0; };
    std::map<std::vector<int>, ScoreStat> stat; // sequence of squares -> wdl statistics
    std::vector<int> key;
    ScoreToProb s2p;
    U64 total = 0, rejected = 0, nextReport = 0;
    iteratePositions(tbType, false, [&](Position& pos) {
        total++;
        int evScore, qScore;
        {
            const int mate0 = SearchConst::MATE0;
            Search sc(pos, nullHist, 0, st, comm, treeLog);
            sc.init(pos, nullHist, 0);
            sc.q0Eval = UNKNOWN_SCORE;
            qScore = sc.quiesce(-mate0, mate0, 0, 0, MoveGen::inCheck(pos));
            Evaluate ev(*et);
            evScore = ev.evalPos(pos);
            if (std::abs(s2p.getProb(qScore) - s2p.getProb(evScore)) > 0.25) {
                rejected++;
                return;
            }
        }

        key.clear();
        for (auto pt : ptVec) {
            U64 m = pos.pieceTypeBB(pt);
            while (m) {
                int sq = BitBoard::extractSquare(m);
                key.push_back(sq);
            }
        }
        ScoreStat& ss = stat[key];

        int score;
        if (!TBProbe::rtbProbeWDL(pos, 0, score, ent))
            throw ChessError("RTB probe failed, pos:" + TextIO::toFEN(pos));
        if (!pos.isWhiteMove())
            score = -score;
        if (score > 0)
            ss.whiteWin++;
        else if (score < 0)
            ss.blackWin++;
        else
            ss.draw++;

        if (total >= nextReport) {
            nextReport += 4*1024*1024;
            std::cerr << "total:" << total << " rejected:" << rejected << std::endl;
        }
    });
    double t1 = currentTime();

    int nDigits = 1;
    {
        U64 maxVal = 0;
        for (const auto& s : stat)
            maxVal = std::max({maxVal, s.second.whiteWin, s.second.draw, s.second.blackWin});
        while (maxVal >= 10) {
            nDigits++;
            maxVal /= 10;
        }
    }

    for (const auto& p : stat) {
        const std::vector<int>& key = p.first;
        const ScoreStat& ss = p.second;
        for (size_t i = 0; i < key.size(); i++) {
            if (i > 0)
                std::cout << ' ';
            std::cout << std::setw(2) << key[i];
        }
        for (size_t i = 0; i < key.size(); i++)
            std::cout << ' ' << TextIO::squareToString(key[i]);

        std::cout << " :";
        int N = ss.whiteWin + ss.draw + ss.blackWin;
        double e = (N > 0) ? (ss.whiteWin + ss.draw * 0.5) / N : 0;
        std::cout << std::setw(nDigits+1) << ss.whiteWin
                  << std::setw(nDigits+1) << ss.draw
                  << std::setw(nDigits+1) << ss.blackWin
                  << ' ' << static_cast<int>(e * 1000 + 0.5) << '\n';
    }
    std::cout << " t:" << (t1-t0) << std::endl;
}

void
PosGenerator::wdlTest(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    TranspositionTable::TTEntry ent;
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        U64 nPos = 0, nDiff = 0, nDiff50 = 0;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int rtbScore, gtbScore;
            if (!TBProbe::rtbProbeWDL(pos, 0, rtbScore, ent))
                throw ChessError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::gtbProbeWDL(pos, 0, gtbScore))
                throw ChessError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            bool diff;
            if (rtbScore > 0) {
                diff = gtbScore <= 0;
            } else if (rtbScore < 0) {
                diff = gtbScore >= 0;
            } else {
                diff = gtbScore != 0;
                if (diff) {
                    int scoreDTM;
                    if (!TBProbe::gtbProbeDTM(pos, 0, scoreDTM))
                        throw ChessError("GTB probe failed, pos:" + TextIO::toFEN(pos));
                    if (std::abs(scoreDTM) < SearchConst::MATE0 - 100) {
                        diff = false;
                        nDiff50++;
                    }
                }
            }
            if (diff) {
                nDiff++;
                std::cout << tbType << " rtb:" << rtbScore << " gtb:" << gtbScore
                          << " pos:" << TextIO::toFEN(pos) << std::endl;
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " nPos:" << nPos << " nDiff:" << nDiff << " nDiff50:" << nDiff50
                  << " t:" << (t1-t0) << std::endl;
    }
}

void
PosGenerator::wdlDump(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    std::ofstream ofs("out.bin", std::ios::binary);
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        U64 nPos = 0;
        U64 cnt[5] = {0, 0, 0, 0, 0};
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int success;
            int wdl = Syzygy::probe_wdl(pos, &success);
            if (!success)
                throw ChessError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            if (!pos.isWhiteMove())
                wdl = -wdl;
            cnt[wdl+2]++;
            S8 c = wdl;
            ofs.write((const char*)&c, 1);
        });
        double t1 = currentTime();
        std::cout << tbType << " nPos:" << nPos << " t:" << (t1-t0) << std::endl;
        std::cout << cnt[0] << ' ' << cnt[1] << ' ' << cnt[2] << ' ' << cnt[3] << ' ' << cnt[4] << std::endl;
    }
}

void
PosGenerator::dtzTest(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    TranspositionTable::TTEntry ent;
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        U64 nPos = 0, nDiff = 0, nDiff50 = 0;
        int minSlack = std::numeric_limits<int>::max();
        int maxSlack = std::numeric_limits<int>::min();
        int minSlack2 = std::numeric_limits<int>::max();
        int maxSlack2 = std::numeric_limits<int>::min();
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int dtz, dtm, wdl;
            if (!TBProbe::rtbProbeDTZ(pos, 0, dtz, ent))
                throw ChessError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::gtbProbeDTM(pos, 0, dtm))
                throw ChessError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::rtbProbeWDL(pos, 0, wdl, ent))
                throw ChessError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            bool diff;
            int slack = 0;
            int slack2 = 0;
            if (dtz > 0) {
                slack = dtm - dtz;
                slack2 = dtz - wdl;
                diff = dtm <= 0 || slack < 0 || slack2 < 0;
            } else if (dtz < 0) {
                slack = -(dtm - dtz);
                slack2 = -(dtz - wdl);
                diff = dtm >= 0 || slack < 0 || slack2 < 0;
            } else {
                diff = dtm != 0;
                if (diff) {
                    if (std::abs(dtm) < SearchConst::MATE0 - 100) {
                        diff = false;
                        nDiff50++;
                    }
                }
            }
            minSlack = std::min(minSlack, slack);
            maxSlack = std::max(maxSlack, slack);
            minSlack2 = std::min(minSlack2, slack2);
            maxSlack2 = std::max(maxSlack2, slack2);
            if (diff) {
                nDiff++;
                std::cout << tbType << " dtz:" << dtz << " dtm:" << dtm
                          << " pos:" << TextIO::toFEN(pos) << std::endl;
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " nPos:" << nPos << " nDiff:" << nDiff << " nDiff50:" << nDiff50
                  << " t:" << (t1-t0) << std::endl;
        std::cout << tbType << " minSlack:" << minSlack << " maxSlack:" << maxSlack
                  << " minSlack2:" << minSlack2 << " maxSlack2:" << maxSlack2 << std::endl;
    }
}

void
PosGenerator::tbgenTest(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    for (std::string tbType : tbTypes) {
        std::vector<int> pieces;
        bool whitePawns, blackPawns;
        getPieces(tbType, pieces, whitePawns, blackPawns);
        if (whitePawns || blackPawns) {
            std::cout << "tbType: " << tbType << " pawns not supported" << std::endl;
            continue;
        }
        PieceCount pc;
        pc.nwq = pc.nwr = pc.nwb = pc.nwn = 0;
        pc.nbq = pc.nbr = pc.nbb = pc.nbn = 0;
        for (int p : pieces) {
            switch (p) {
            case Piece::WQUEEN:  pc.nwq++; break;
            case Piece::WROOK:   pc.nwr++; break;
            case Piece::WBISHOP: pc.nwb++; break;
            case Piece::WKNIGHT: pc.nwn++; break;
            case Piece::BQUEEN:  pc.nbq++; break;
            case Piece::BROOK:   pc.nbr++; break;
            case Piece::BBISHOP: pc.nbb++; break;
            case Piece::BKNIGHT: pc.nbn++; break;
            }
        }
        VectorStorage vs;
        TBGenerator<VectorStorage> tbGen(vs, pc);
        RelaxedShared<S64> maxTimeMillis(-1);
        tbGen.generate(maxTimeMillis, false);
        double t0 = currentTime();

        U64 nPos = 0;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int score, gtbScore;
            if (!tbGen.probeDTM(pos, 0, score))
                throw ChessError("tbGen probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::gtbProbeDTM(pos, 0, gtbScore))
                throw ChessError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            if (score != gtbScore) {
                std::cout << tbType << " i:" << nPos << " score:" << score << " gtbScore:" << gtbScore
                          << " pos:" << TextIO::toFEN(pos) << std::endl;
                throw ChessError("stop");
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " nPos:" << nPos << " compare time:" << (t1-t0) << std::endl;
    }
}

struct KingData {
    int wKing;
    int bKing;
    int castleMask;
};

const int a1C = 1 << Position::A1_CASTLE;
const int h1C = 1 << Position::H1_CASTLE;
const int a8C = 1 << Position::A8_CASTLE;
const int h8C = 1 << Position::H8_CASTLE;

static
void computeKingData(std::vector<KingData>& kingTable) {
    for (int k1 = 0; k1 < 64; k1++) {
        for (int k2 = 0; k2 < 64; k2++) {
            if (BitBoard::getKingDistance(k1, k2) <= 1)
                continue;
            for (int castleMask = 0; castleMask < 16; castleMask++) {
                bool a1 = castleMask & a1C;
                bool h1 = castleMask & h1C;
                bool a8 = castleMask & a8C;
                bool h8 = castleMask & h8C;
                if (k1 != E1 && (a1 || h1))
                    continue;
                if (k2 != E8 && (a8 || h8))
                    continue;
                kingTable.push_back({k1, k2, castleMask});
            }
        }
    }
}

const int pawn   = 0;
const int knight = 1;
const int bishop = 2;
const int rook   = 3;
const int queen  = 4;
const int king   = 5;

void
PosGenerator::randomLegal(int n, U64 rndSeed, int nWorkers, std::ostream& os) {
    std::vector<KingData> kingTable;
    computeKingData(kingTable);
    const int nKingCombs = 3969;
    assert(kingTable.size() == nKingCombs);

    Position startPos = TextIO::readFEN(TextIO::startPosFEN);
    std::mutex mutex;

    const double nPossible = 1.0 *
        2 *                 // Side to move
        nKingCombs *        // King arrangements, including castling flags
        (1ULL << 62) *      // Occupied squares for non-kings
        (1ULL << 30) *      // Piece color for non-kings
        std::pow(5, 30) *   // Non-king piece types
        6;                  // En passant possibilities
    const double nEstLegal = 4.8e44; // Estimated number of legal chess positions
    const U64 nTries = (U64)(nPossible / nEstLegal * n);
    {
        std::stringstream ss;
        ss.precision(10);
        ss << "multiplier: " << (nPossible / nTries);
        std::cerr << ss.str() << std::endl;
    }

    ThreadPool<int> pool(nWorkers);
    const U64 chunkSize = 100000000;
    for (U64 c = 0; c < nTries; c += chunkSize) {
        U64 seed = rndSeed + hashU64(c);
        U64 stop = std::min(chunkSize, nTries - c);
        auto func = [seed,&os,&kingTable,&startPos,&mutex,stop](int workerNo) {
            Position pos;
            int pieces[64] = {0}; // Piece/color for each square
            Random rand(seed);
            for (U64 i = 0; i < stop; i++) {
                U64 r = rand.nextU64();

                bool wtm = (r & 1) != 0;
                U64 occupied = r >> 2;                     // 62 occupied bits
                int nPieces = BitUtil::bitCount(occupied); // Not counting kings
                const int maxPieces = 30;  // Max number of pieces, excluding kings
                if (nPieces > maxPieces)
                    continue; // Too many pieces

                r = rand.nextU64() & ((1 << maxPieces) - 1);
                U64 whitePieces = r & ((1 << nPieces) - 1);
                if (whitePieces != r)
                    continue; // Non-existing pieces must have color 0

                int nWhite = BitUtil::bitCount(whitePieces);
                if (nWhite > 15 || nPieces - nWhite > 15)
                    continue; // Too many white/black pieces

                bool fail = false;
                for (int p = nPieces; p < maxPieces; p++)
                    if (rand.nextInt<5>() != pawn) {
                        fail = true;
                        break; // Non-existing pieces must have type pawn
                    }
                if (fail)
                    continue;

                const int kingCombIdx = rand.nextInt<nKingCombs>();
                const int wk = kingTable[kingCombIdx].wKing;
                const int bk = kingTable[kingCombIdx].bKing;
                const int k1 = std::min(wk, bk);
                const int k2 = std::max(wk, bk);

                U64 mask = occupied;
                occupied = 0; // Re-compute occupied to take king squares into account
                while (mask) {
                    int sq = BitBoard::extractSquare(mask);
                    if (sq >= k1) {
                        sq++;
                        if (sq >= k2)
                            sq++;
                    }
                    int p = rand.nextInt<5>();
                    if (p == pawn && (sq <= H1 || sq >= A8)) { // Pawn on first/last rank
                        fail = true;
                        break;
                    }
                    bool white = (whitePieces & 1) != 0;
                    whitePieces >>= 1;
                    pieces[sq] = (white << 3) | p;
                    occupied |= 1ULL << sq;
                }
                if (fail)
                    continue;

                const int castleMask = kingTable[kingCombIdx].castleMask;
                const int epCode = rand.nextInt<6>();
                randomLegalSlowPath(startPos, pos, pieces, wtm, occupied, wk, bk,
                                    castleMask, epCode, os, mutex);
            }
            return 0;
        };
        pool.addTask(func);
    }
    pool.getAllResults([](int){});
}

void
PosGenerator::randomLegalSlowPath(const Position& startPos, Position& pos,
                                  int pieces[64], bool wtm, U64 occupied,
                                  int wk, int bk, int castleMask, int epCode,
                                  std::ostream& os, std::mutex& mutex) {
    pieces[wk] = (1 << 3) | king;
    occupied |= 1ULL << wk;
    pieces[bk] = (0 << 3) | king;
    occupied |= 1ULL << bk;

    auto hasPiece = [&pieces,occupied](int x, int y, bool wtm, int pieceType) -> bool {
        int sq = Square::getSquare(x, y);
        int p = ((wtm ? 1 : 0) << 3) | pieceType;
        if ((occupied & (1ULL << sq)) == 0)
            return false;
        return pieces[sq] == p;
    };

    int epSquare = -1;
    if (epCode != 0) {
        const int y3 = wtm ? 4 : 3;
        const int y2 = wtm ? 5 : 2;
        const int y1 = wtm ? 6 : 1;
        for (int x = 0; x < 8; x++) {
            if (!hasPiece(x, y3, !wtm, pawn))
                continue;
            if (occupied & (1ULL << Square::getSquare(x, y2)))
                continue;
            if (occupied & (1ULL << Square::getSquare(x, y1)))
                continue;
            bool leftOk = x > 0 && hasPiece(x-1, y3, wtm, pawn);
            bool rightOk = x < 7 && hasPiece(x+1, y3, wtm, pawn);
            if (!leftOk && !rightOk)
                continue;
            if (--epCode == 0) {
                epSquare = 1ULL << Square::getSquare(x, y2);
                break;
            }
        }
        if (epSquare == -1)
            return;
    }

    if (castleMask != 0) {
        if ((castleMask & a1C) && !hasPiece(0, 0, true, rook))
            return;
        if ((castleMask & h1C) && !hasPiece(7, 0, true, rook))
            return;
        if ((castleMask & a8C) && !hasPiece(0, 7, false, rook))
            return;
        if ((castleMask & h8C) && !hasPiece(7, 7, false, rook))
            return;
    }

    for (int sq = 0; sq < 64; sq++) {
        if (occupied & (1ULL << sq)) {
            int p = pieces[sq];
            bool white = p >= 8;
            p &= 7;
            int pt;
            switch (p) {
            case pawn  : pt = Piece::WPAWN; break;
            case knight: pt = Piece::WKNIGHT; break;
            case bishop: pt = Piece::WBISHOP; break;
            case rook  : pt = Piece::WROOK; break;
            case queen : pt = Piece::WQUEEN; break;
            case king  : pt = Piece::WKING; break;
            default: assert(false); return;
            }
            if (!white)
                pt = Piece::makeBlack(pt);
            pos.setPiece(sq, pt);
        } else {
            pos.clearPiece(sq);
        }
    }
    pos.setWhiteMove(wtm);
    pos.setCastleMask(castleMask);
    pos.setEpSquare(epSquare);

    if (MoveGen::canTakeKing(pos))
        return; // King capture possible

    std::string fen = TextIO::toFEN(pos);
    try {
        std::stringstream ss;
        ProofGame pg(TextIO::startPosFEN, fen, false, {}, false, ss);
        auto opts = ProofGame::Options().setSmallCache(true).setMaxNodes(2);
        ProofGame::Result result;
        int minCost = pg.search(opts, result);
        if (minCost == INT_MAX)
            return;

        U64 blocked;
        if (!pg.computeBlocked(pos, blocked))
            return;
        ProofKernel pk(startPos, pos, blocked, ss);
        if (!pk.isGoalPossible())
            return;
    } catch (const ChessParseError& e) {
        return;
    }

    std::lock_guard<std::mutex> L(mutex);
    os << TextIO::toFEN(pos) << std::endl;
}
