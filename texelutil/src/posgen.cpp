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
#include "parameters.hpp"
#include "computerPlayer.hpp"

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
                            pos.setPiece(Position::getSquare(i, 1), Piece::WPAWN);
                            pos.setPiece(Position::getSquare(i, 6), Piece::BPAWN);
                            pos.setPiece(Position::getSquare(i, 7), Piece::BKNIGHT);
                        }
                        pos.setPiece(Position::getSquare(bk, 7), Piece::BKING);
                        pos.setPiece(Position::getSquare(wk, 0), Piece::WKING);
                        pos.setPiece(Position::getSquare(q1, 0), Piece::WQUEEN);
                        pos.setPiece(Position::getSquare(q2, 0), Piece::WQUEEN);
                        pos.setPiece(Position::getSquare(q3, 0), Piece::WQUEEN);
                        writeFEN(pos);
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Position::getSquare(i, 6), Piece::EMPTY);
                            writeFEN(pos);
                            pos.setPiece(Position::getSquare(i, 6), Piece::BPAWN);
                        }
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Position::getSquare(i, 1), Piece::EMPTY);
                            writeFEN(pos);
                            pos.setPiece(Position::getSquare(i, 1), Piece::WPAWN);
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
    if (pos.getWhiteMove()) {
        U64 wPawnAttacks = ((wPawns & BitBoard::maskBToHFiles) << 7) |
                           ((wPawns & BitBoard::maskAToGFiles) << 9);
        return ((bPawns & BitBoard::maskRow5) << 8) & ~occupied & wPawnAttacks;
    } else {
        U64 bPawnAttacks = ((bPawns & BitBoard::maskBToHFiles) >> 9) |
                           ((bPawns & BitBoard::maskAToGFiles) >> 7);
        return ((wPawns & BitBoard::maskRow4) >> 8) & ~occupied & bPawnAttacks;
    }
}

void
PosGenerator::tbStat(const std::vector<std::string>& tbTypes) {
    UciParams::gtbPath->set("/home/petero/chess/gtb");
    UciParams::gtbCache->set("2047");
    ComputerPlayer::initEngine();
    for (std::string tbType : tbTypes) {
        std::vector<int> pieces;
        bool whitePawns, blackPawns;
        getPieces(tbType, pieces, whitePawns, blackPawns);
        const int nPieces = pieces.size();
        const bool anyPawns = whitePawns || blackPawns;
        const bool epPossible = whitePawns && blackPawns;

        int negScore = std::numeric_limits<int>::min(); // Largest negative score
        int posScore = std::numeric_limits<int>::max(); // Smallest positive score
        Position negPos, posPos;
        U64 nPos = 0;
        for (int wk = 0; wk < 64; wk++) {
            int x = Position::getX(wk);
            int y = Position::getY(wk);
            if (x >= 4)
                continue;
            if (!anyPawns)
                if (y >= 4 || y < x)
                    continue;
            for (int bk = 0; bk < 64; bk++) {
                int x2 = Position::getX(bk);
                int y2 = Position::getY(bk);
                if (std::abs(x2-x) < 2 && std::abs(y2-y) < 2)
                    continue;

                Position pos;
                pos.setPiece(wk, Piece::WKING);
                pos.setPiece(bk, Piece::BKING);
                std::vector<int> squares(nPieces, 0);
                int nPlaced = 0;

                while (true) {
                    // Place remaining pieces on first empty square
                    while (nPlaced < nPieces) {
                        int p = pieces[nPlaced];
                        for (int sq = 0; sq < 64; sq++) {
                            if (!squareValid(sq, p))
                                continue;
                            if (pos.getPiece(sq) == Piece::EMPTY) {
                                pos.setPiece(sq, p);
                                squares[nPlaced] = sq;
                                nPlaced++;
                                break;
                            }
                        }
                    }
                    nPos++;

                    // Update min/max score if position is valid
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
                        pos.setWhiteMove(white);

                        U64 epSquares = epPossible ? getEPSquares(pos) : 0;
                        while (true) {
                            if (epSquares) {
                                int epSq = BitBoard::numberOfTrailingZeros(epSquares);
                                pos.setEpSquare(epSq);
                                TextIO::fixupEPSquare(pos);
                                if (pos.getEpSquare() == -1) {
                                    epSquares &= epSquares - 1;
                                    continue;
                                }
                            } else {
                                pos.setEpSquare(-1);
                            }
                            int score;
                            if (!TBProbe::gtbProbeDTM(pos, 0, score))
                                throw ChessParseError("TB probe failed, pos:" + TextIO::toFEN(pos));
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
                            if (epSquares == 0)
                                break;
                            epSquares &= epSquares - 1;
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
        std::cout << tbType << " neg: " << negScore << " pos:" << posScore << " nPos:" << nPos << std::endl;
        std::cout << tbType << " negPos: " << TextIO::toFEN(negPos) << std::endl;
        std::cout << tbType << " posPos: " << TextIO::toFEN(posPos) << std::endl;
    }
}
