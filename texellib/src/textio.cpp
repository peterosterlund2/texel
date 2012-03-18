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

/*
 * textio.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "textio.hpp"
#include "moveGen.hpp"
#include "util.hpp"
#include <assert.h>

const std::string TextIO::startPosFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


Position
TextIO::readFEN(const std::string& fen) {
    Position pos;
    std::vector<std::string> words;
    splitString(fen, words);
    if (words.size() < 2)
        throw ChessParseError("Too few spaces");

    // Piece placement
    int row = 7;
    int col = 0;
    for (size_t i = 0; i < words[0].length(); i++) {
        char c = words[0][i];
        switch (c) {
            case '1': col += 1; break;
            case '2': col += 2; break;
            case '3': col += 3; break;
            case '4': col += 4; break;
            case '5': col += 5; break;
            case '6': col += 6; break;
            case '7': col += 7; break;
            case '8': col += 8; break;
            case '/': row--; col = 0; break;
            case 'P': safeSetPiece(pos, col, row, Piece::WPAWN);   col++; break;
            case 'N': safeSetPiece(pos, col, row, Piece::WKNIGHT); col++; break;
            case 'B': safeSetPiece(pos, col, row, Piece::WBISHOP); col++; break;
            case 'R': safeSetPiece(pos, col, row, Piece::WROOK);   col++; break;
            case 'Q': safeSetPiece(pos, col, row, Piece::WQUEEN);  col++; break;
            case 'K': safeSetPiece(pos, col, row, Piece::WKING);   col++; break;
            case 'p': safeSetPiece(pos, col, row, Piece::BPAWN);   col++; break;
            case 'n': safeSetPiece(pos, col, row, Piece::BKNIGHT); col++; break;
            case 'b': safeSetPiece(pos, col, row, Piece::BBISHOP); col++; break;
            case 'r': safeSetPiece(pos, col, row, Piece::BROOK);   col++; break;
            case 'q': safeSetPiece(pos, col, row, Piece::BQUEEN);  col++; break;
            case 'k': safeSetPiece(pos, col, row, Piece::BKING);   col++; break;
            default: throw ChessParseError("Invalid piece");
        }
    }
    if (words[1].length() == 0)
        throw ChessParseError("Invalid side");
    pos.setWhiteMove(words[1][0] == 'w');

    // Castling rights
    int castleMask = 0;
    if (words.size() > 2) {
        for (size_t i = 0; i < words[2].length(); i++) {
            char c = words[2][i];
            switch (c) {
                case 'K': castleMask |= (1 << Position::H1_CASTLE); break;
                case 'Q': castleMask |= (1 << Position::A1_CASTLE); break;
                case 'k': castleMask |= (1 << Position::H8_CASTLE); break;
                case 'q': castleMask |= (1 << Position::A8_CASTLE); break;
                case '-': break;
                default: throw ChessParseError("Invalid castling flags");
            }
        }
    }
    pos.setCastleMask(castleMask);

    if (words.size() > 3) {
        // En passant target square
        const std::string& epString = words[3];
        if (epString != "-") {
            if (epString.length() < 2)
                throw ChessParseError("Invalid en passant square");
            pos.setEpSquare(getSquare(epString));
        }
    }

    if (words.size() > 4)
        str2Num(words[4], pos.halfMoveClock);
    if (words.size() > 5)
        str2Num(words[5], pos.fullMoveCounter);

    // Each side must have exactly one king
    int wKings = 0;
    int bKings = 0;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int p = pos.getPiece(Position::getSquare(x, y));
            if (p == Piece::WKING)
                wKings++;
            else if (p == Piece::BKING)
                bKings++;
        }
    }
    if (wKings != 1)
        throw ChessParseError("White must have exactly one king");
    if (bKings != 1)
        throw ChessParseError("Black must have exactly one king");

    // Make sure king can not be captured
    Position pos2(pos);
    pos2.setWhiteMove(!pos.whiteMove);
    if (MoveGen::inCheck(pos2))
        throw ChessParseError("King capture possible");

    fixupEPSquare(pos);
    return pos;
}


void
TextIO::fixupEPSquare(Position& pos) {
    int epSquare = pos.getEpSquare();
    if (epSquare >= 0) {
        MoveGen::MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool epValid = false;
        for (int mi = 0; mi < moves.size; mi++) {
            const Move& m = moves[mi];
            if (m.to() == epSquare) {
                if (pos.getPiece(m.from()) == (pos.whiteMove ? Piece::WPAWN : Piece::BPAWN)) {
                    epValid = true;
                    break;
                }
            }
        }
        if (!epValid)
            pos.setEpSquare(-1);
    }
}


std::string
TextIO::toFEN(const Position& pos) {
    std::string ret;
    // Piece placement
    for (int r = 7; r >=0; r--) {
        int numEmpty = 0;
        for (int c = 0; c < 8; c++) {
            int p = pos.getPiece(Position::getSquare(c, r));
            if (p == Piece::EMPTY) {
                numEmpty++;
            } else {
                if (numEmpty > 0) {
                    ret += (char)('0' + numEmpty);
                    numEmpty = 0;
                }
                switch (p) {
                    case Piece::WKING:   ret += 'K'; break;
                    case Piece::WQUEEN:  ret += 'Q'; break;
                    case Piece::WROOK:   ret += 'R'; break;
                    case Piece::WBISHOP: ret += 'B'; break;
                    case Piece::WKNIGHT: ret += 'N'; break;
                    case Piece::WPAWN:   ret += 'P'; break;
                    case Piece::BKING:   ret += 'k'; break;
                    case Piece::BQUEEN:  ret += 'q'; break;
                    case Piece::BROOK:   ret += 'r'; break;
                    case Piece::BBISHOP: ret += 'b'; break;
                    case Piece::BKNIGHT: ret += 'n'; break;
                    case Piece::BPAWN:   ret += 'p'; break;
                    default: assert(false); break;
                }
            }
        }
        if (numEmpty > 0)
            ret += (char)('0' + numEmpty);
        if (r > 0)
            ret += '/';
    }
    ret += (pos.whiteMove ? " w " : " b ");

    // Castling rights
    bool anyCastle = false;
    if (pos.h1Castle()) {
        ret += 'K';
        anyCastle = true;
    }
    if (pos.a1Castle()) {
        ret += 'Q';
        anyCastle = true;
    }
    if (pos.h8Castle()) {
        ret += 'k';
        anyCastle = true;
    }
    if (pos.a8Castle()) {
        ret += 'q';
        anyCastle = true;
    }
    if (!anyCastle) {
        ret += '-';
    }

    // En passant target square
    {
        ret += ' ';
        if (pos.getEpSquare() >= 0) {
            int x = Position::getX(pos.getEpSquare());
            int y = Position::getY(pos.getEpSquare());
            ret += ((char)(x + 'a'));
            ret += ((char)(y + '1'));
        } else {
            ret += '-';
        }
    }

    // Move counters
    ret += ' ';
    ret += num2Str(pos.halfMoveClock);
    ret += ' ';
    ret += num2Str(pos.fullMoveCounter);

    return ret;
}

std::string
TextIO::moveToUCIString(const Move& m) {
    std::string ret = squareToString(m.from());
    ret += squareToString(m.to());
    switch (m.promoteTo()) {
        case Piece::WQUEEN:
        case Piece::BQUEEN:
            ret += "q";
            break;
        case Piece::WROOK:
        case Piece::BROOK:
            ret += "r";
            break;
        case Piece::WBISHOP:
        case Piece::BBISHOP:
            ret += "b";
            break;
        case Piece::WKNIGHT:
        case Piece::BKNIGHT:
            ret += "n";
            break;
        default:
            break;
    }
    return ret;
}

Move
TextIO::uciStringToMove(const std::string& move) {
    Move m;
    if ((move.length() < 4) || (move.length() > 5))
        return m;
    int fromSq = TextIO::getSquare(move.substr(0, 2));
    int toSq   = TextIO::getSquare(move.substr(2, 2));
    if ((fromSq < 0) || (toSq < 0)) {
        return m;
    }
    char prom = ' ';
    bool white = true;
    if (move.length() == 5) {
        prom = move[4];
        if (Position::getY(toSq) == 7) {
            white = true;
        } else if (Position::getY(toSq) == 0) {
            white = false;
        } else {
            return m;
        }
    }
    int promoteTo;
    switch (prom) {
        case ' ':
            promoteTo = Piece::EMPTY;
            break;
        case 'q':
            promoteTo = white ? Piece::WQUEEN : Piece::BQUEEN;
            break;
        case 'r':
            promoteTo = white ? Piece::WROOK : Piece::BROOK;
            break;
        case 'b':
            promoteTo = white ? Piece::WBISHOP : Piece::BBISHOP;
            break;
        case 'n':
            promoteTo = white ? Piece::WKNIGHT : Piece::BKNIGHT;
            break;
        default:
            return m;
    }
    return Move(fromSq, toSq, promoteTo);
}

static bool
isCapture(const Position& pos, const Move& move) {
    if (pos.getPiece(move.to()) != Piece::EMPTY)
        return true;
    int p = pos.getPiece(move.from());
    return (p == (pos.whiteMove ? Piece::WPAWN : Piece::BPAWN)) &&
           (move.to() == pos.getEpSquare());
}

static std::string
pieceToChar(int p) {
    switch (p) {
        case Piece::WQUEEN:  case Piece::BQUEEN:  return "Q";
        case Piece::WROOK:   case Piece::BROOK:   return "R";
        case Piece::WBISHOP: case Piece::BBISHOP: return "B";
        case Piece::WKNIGHT: case Piece::BKNIGHT: return "N";
        case Piece::WKING:   case Piece::BKING:   return "K";
    }
    return "";
}

static std::string
moveToString(Position& pos, const Move& move, bool longForm, const MoveGen::MoveList& moves) {
    std::string ret;
    int wKingOrigPos = Position::getSquare(4, 0);
    int bKingOrigPos = Position::getSquare(4, 7);
    if (move.from() == wKingOrigPos && pos.getPiece(wKingOrigPos) == Piece::WKING) {
        // Check white castle
        if (move.to() == Position::getSquare(6, 0))
            ret += "O-O";
        else if (move.to() == Position::getSquare(2, 0))
            ret += "O-O-O";
    } else if (move.from() == bKingOrigPos && pos.getPiece(bKingOrigPos) == Piece::BKING) {
        // Check black castle
        if (move.to() == Position::getSquare(6, 7))
            ret += "O-O";
        else if (move.to() == Position::getSquare(2, 7))
            ret += "O-O-O";
    }
    if (ret.length() == 0) {
        int p = pos.getPiece(move.from());
        ret += pieceToChar(p);
        int x1 = Position::getX(move.from());
        int y1 = Position::getY(move.from());
        int x2 = Position::getX(move.to());
        int y2 = Position::getY(move.to());
        if (longForm) {
            ret += (char)(x1 + 'a');
            ret += (char)(y1 + '1');
            ret += isCapture(pos, move) ? 'x' : '-';
        } else {
            if (p == (pos.whiteMove ? Piece::WPAWN : Piece::BPAWN)) {
                if (isCapture(pos, move))
                    ret += (char)(x1 + 'a');
            } else {
                int numSameTarget = 0;
                int numSameFile = 0;
                int numSameRow = 0;
                for (int mi = 0; mi < moves.size; mi++) {
                    const Move& m = moves[mi];
                    if (m.isEmpty())
                        break;
                    if ((pos.getPiece(m.from()) == p) && (m.to() == move.to())) {
                        numSameTarget++;
                        if (Position::getX(m.from()) == x1)
                            numSameFile++;
                        if (Position::getY(m.from()) == y1)
                            numSameRow++;
                    }
                }
                if (numSameTarget < 2) {
                    // No file/row info needed
                } else if (numSameFile < 2) {
                    ret += (char)(x1 + 'a');   // Only file info needed
                } else if (numSameRow < 2) {
                    ret += (char)(y1 + '1');   // Only row info needed
                } else {
                    ret += (char) (x1 + 'a');   // File and row info needed
                    ret += (char) (y1 + '1');
                }
            }
            if (isCapture(pos, move))
                ret += 'x';
        }
        ret += (char)(x2 + 'a');
        ret += (char)(y2 + '1');
        if (move.promoteTo() != Piece::EMPTY)
            ret += pieceToChar(move.promoteTo());
    }
    UndoInfo ui;
    if (MoveGen::givesCheck(pos, move)) {
        pos.makeMove(move, ui);
        MoveGen::MoveList nextMoves;
        MoveGen::pseudoLegalMoves(pos, nextMoves);
        MoveGen::removeIllegal(pos, nextMoves);
        if (nextMoves.size == 0)
            ret += '#';
        else
            ret += '+';
        pos.unMakeMove(move, ui);
    }

    return ret;
}

std::string
TextIO::moveToString(const Position& pos, const Move& move, bool longForm) {
    MoveGen::MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    Position tmpPos(pos);
    MoveGen::removeIllegal(tmpPos, moves);
    return ::moveToString(tmpPos, move, longForm, moves);
}

Move
TextIO::stringToMove(Position& pos, const std::string& strMoveIn) {
    std::string strMove;
    for (size_t i = 0; i < strMoveIn.length(); i++)
        if (strMoveIn[i] != '=')
            strMove += strMoveIn[i];
    Move move;
    if (strMove.length() == 0)
        return move;
    MoveGen::MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    Position tmpPos(pos);
    MoveGen::removeIllegal(tmpPos, moves);
    {
        char lastChar = strMove[strMove.length() - 1];
        if ((lastChar == '#') || (lastChar == '+')) {
            MoveGen::MoveList subMoves;
            int len = 0;
            for (int mi = 0; mi < moves.size; mi++) {
                const Move& m = moves[mi];
                std::string str1 = ::moveToString(pos, m, true, moves);
                if (str1[str1.length() - 1] == lastChar) {
                    subMoves[len++] = m;
                }
            }
            subMoves.size = len;
            moves = subMoves;
            strMove = normalizeMoveString(strMove);
        }
    }

    for (int i = 0; i < 2; i++) {
        // Search for full match
        for (int mi = 0; mi < moves.size; mi++) {
            const Move& m = moves[mi];
            std::string str1 = normalizeMoveString(::moveToString(pos, m, true, moves));
            std::string str2 = normalizeMoveString(::moveToString(pos, m, false, moves));
            if (i == 0) {
                if ((strMove == str1) || (strMove == str2))
                    return m;
            } else {
                if ((toLowerCase(strMove) == toLowerCase(str1)) ||
                    (toLowerCase(strMove) == toLowerCase(str2)))
                    return m;
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        // Search for unique substring match
        for (int mi = 0; mi < moves.size; mi++) {
            const Move& m = moves[mi];
            std::string str1 = normalizeMoveString(TextIO::moveToString(pos, m, true));
            std::string str2 = normalizeMoveString(TextIO::moveToString(pos, m, false));
            bool match;
            if (i == 0) {
                match = startsWith(str1, strMove) || startsWith(str2, strMove);
            } else {
                match = startsWith(toLowerCase(str1), toLowerCase(strMove)) ||
                        startsWith(toLowerCase(str2), toLowerCase(strMove));
            }
            if (match) {
                if (!move.isEmpty()) {
                    return Move(); // More than one match, not ok
                } else {
                    move = m;
                }
            }
        }
        if (!move.isEmpty())
            return move;
    }
    return move;
}

std::string
TextIO::asciiBoard(const Position& pos) {
    std::string ret;
    ret += "    +----+----+----+----+----+----+----+----+\n";
    for (int y = 7; y >= 0; y--) {
        ret += "    |";
        for (int x = 0; x < 8; x++) {
            ret += ' ';
            int p = pos.getPiece(Position::getSquare(x, y));
            if (p == Piece::EMPTY) {
                bool dark = Position::darkSquare(x, y);
                ret.append(dark ? ".. |" : "   |");
            } else {
                ret += Piece::isWhite(p) ? ' ' : '*';
                std::string pieceName = pieceToChar(p);
                if (pieceName.length() == 0)
                    pieceName = "P";

                ret += pieceName;
                ret += " |";
            }
        }

        ret += ("\n    +----+----+----+----+----+----+----+----+\n");
    }

    return ret;
}

