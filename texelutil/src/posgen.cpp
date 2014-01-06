/*
 * posgen.cpp
 *
 *  Created on: Jan 6, 2014
 *      Author: petero
 */

#include "posgen.hpp"
#include "textio.hpp"

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
