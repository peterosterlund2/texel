/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * ctgbook.cpp
 *
 *  Created on: Jul 15, 2016
 *      Author: petero
 */

#include "ctgbook.hpp"
#include "moveGen.hpp"
#include "textio.hpp"

#include <cassert>


Random CtgBook::rndGen;



namespace {

/** Read len bytes from offs in file f. */
void
readBytes(std::fstream& f, long offs, int len, std::vector<U8>& buf) {
    buf.resize(len);
    f.seekg(offs, std::ios_base::beg);
    f.read((char*)&buf[0], len);
    if (!f)
        for (int i = 0; i < len; i++)
            buf[i] = 0;
}

/** Convert len bytes starting at offs in buf to an integer. */
int
extractInt(const std::vector<U8>& buf, int offs, int len) {
    int val = 0;
    for (int i = 0; i < len; i++)
        val = (val << 8) + buf.at(offs + i);
    return val;
}

int
mirrorSquareColor(int sq) {
    int x = Position::getX(sq);
    int y = 7 - Position::getY(sq);
    return Position::getSquare(x, y);
}

int
mirrorPieceColor(int piece) {
    if (Piece::isWhite(piece)) {
        return Piece::makeBlack(piece);
    } else {
        return Piece::makeWhite(piece);
    }
}

void
mirrorMoveColor(Move& m) {
    if (!m.isEmpty())
        m.setMove(mirrorSquareColor(m.from()),
                  mirrorSquareColor(m.to()),
                  mirrorPieceColor(m.promoteTo()), 0);
}

int
mirrorSquareLeftRight(int sq) {
    int x = 7 - Position::getX(sq);
    int y = Position::getY(sq);
    return Position::getSquare(x, y);
}

void
mirrorPosLeftRight(Position& pos) {
    for (int sq = 0; sq < 64; sq++) {
        int mSq = mirrorSquareLeftRight(sq);
        if (sq < mSq) {
            int piece1 = pos.getPiece(sq);
            int piece2 = pos.getPiece(mSq);
            pos.setPiece(sq, piece2);
            pos.setPiece(mSq, piece1);
        }
    }
    int epSquare = pos.getEpSquare();
    if (epSquare >= 0) {
        int mEpSquare = mirrorSquareLeftRight(epSquare);
        pos.setEpSquare(mEpSquare);
    }
}

void
mirrorMoveLeftRight(Move& m) {
    if (!m.isEmpty())
        m.setMove(mirrorSquareLeftRight(m.from()),
                  mirrorSquareLeftRight(m.to()),
                  m.promoteTo(), 0);
}
void
mirrorPosColor(Position& pos) {
    for (int sq = 0; sq < 64; sq++) {
        int mSq = mirrorSquareColor(sq);
        if (sq < mSq) {
            int piece1 = pos.getPiece(sq);
            int piece2 = pos.getPiece(mSq);
            pos.setPiece(mSq, mirrorPieceColor(piece1));
            pos.setPiece(sq, mirrorPieceColor(piece2));
        }
    }
    pos.setWhiteMove(!pos.isWhiteMove());

    int castleMask = 0;
    if (pos.a1Castle()) castleMask |= (1 << Position::A8_CASTLE);
    if (pos.h1Castle()) castleMask |= (1 << Position::H8_CASTLE);
    if (pos.a8Castle()) castleMask |= (1 << Position::A1_CASTLE);
    if (pos.h8Castle()) castleMask |= (1 << Position::H1_CASTLE);
    pos.setCastleMask(castleMask);

    int epSquare = pos.getEpSquare();
    if (epSquare >= 0) {
        int mEpSquare = mirrorSquareColor(epSquare);
        pos.setEpSquare(mEpSquare);
    }
}

/** Converts a position to a byte array. */
void
positionToByteArray(Position& pos, std::vector<U8>& encodedPos) {
    BitVector bits;
    bits.addBits(0, 8); // Header byte
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int p = pos.getPiece(Position::getSquare(x, y));
            switch (p) {
            case Piece::EMPTY:   bits.addBits(0x00, 1); break;
            case Piece::WKING:   bits.addBits(0x20, 6); break;
            case Piece::WQUEEN:  bits.addBits(0x22, 6); break;
            case Piece::WROOK:   bits.addBits(0x16, 5); break;
            case Piece::WBISHOP: bits.addBits(0x14, 5); break;
            case Piece::WKNIGHT: bits.addBits(0x12, 5); break;
            case Piece::WPAWN:   bits.addBits(0x06, 3); break;
            case Piece::BKING:   bits.addBits(0x21, 6); break;
            case Piece::BQUEEN:  bits.addBits(0x23, 6); break;
            case Piece::BROOK:   bits.addBits(0x17, 5); break;
            case Piece::BBISHOP: bits.addBits(0x15, 5); break;
            case Piece::BKNIGHT: bits.addBits(0x13, 5); break;
            case Piece::BPAWN:   bits.addBits(0x07, 3); break;
            }
        }
    }

    TextIO::fixupEPSquare(pos);
    bool ep = pos.getEpSquare() != -1;
    bool cs = pos.getCastleMask() != 0;
    if (!ep && !cs)
        bits.addBit(false); // At least one pad bit

    int specialBits = (ep ? 3 : 0) + (cs ? 4 : 0);
    while (bits.padBits() != specialBits)
        bits.addBit(false);

    if (ep)
        bits.addBits(Position::getX(pos.getEpSquare()), 3);
    if (cs) {
        bits.addBit(pos.h8Castle());
        bits.addBit(pos.a8Castle());
        bits.addBit(pos.h1Castle());
        bits.addBit(pos.a1Castle());
    }

    assert((bits.getLength() & 7) == 0);
    int header = bits.getLength() / 8;
    if (ep) header |= 0x20;
    if (cs) header |= 0x40;

    encodedPos = bits.getBytes();
    encodedPos[0] = (U8)header;
}

}

void
BitVector::addBit(bool value) {
    int byteIdx = length / 8;
    int bitIdx = 7 - (length & 7);
    while ((int)buf.size() <= byteIdx)
        buf.push_back(0);
    if (value)
        buf[byteIdx] |= 1 << bitIdx;
    length++;
}

void
BitVector::addBits(int mask, int numBits) {
    for (int i = 0; i < numBits; i++) {
        int b = numBits - 1 - i;
        addBit((mask & (1 << b)) != 0);
    }
}

int
BitVector::padBits() {
    int bitIdx = length & 7;
    return (bitIdx == 0) ? 0 : 8 - bitIdx;
}

const std::vector<U8>&
BitVector::getBytes() const {
    return buf;
}

int
BitVector::getLength() const {
    return length;
}

// --------------------------------------------------------------------------------

PositionData::MoveInfo PositionData::moveInfo[256];
static StaticInitializer<PositionData> posDataInit;

void
PositionData::staticInitialize() {
    moveInfo[0x00] = { Piece::WPAWN  , 4, +1, +1 };
    moveInfo[0x01] = { Piece::WKNIGHT, 1, -2, -1 };
    moveInfo[0x03] = { Piece::WQUEEN , 1, +2, +0 };
    moveInfo[0x04] = { Piece::WPAWN  , 1, +0, +1 };
    moveInfo[0x05] = { Piece::WQUEEN , 0, +0, +1 };
    moveInfo[0x06] = { Piece::WPAWN  , 3, -1, +1 };
    moveInfo[0x08] = { Piece::WQUEEN , 1, +4, +0 };
    moveInfo[0x09] = { Piece::WBISHOP, 1, +6, +6 };
    moveInfo[0x0a] = { Piece::WKING  , 0, +0, -1 };
    moveInfo[0x0c] = { Piece::WPAWN  , 0, -1, +1 };
    moveInfo[0x0d] = { Piece::WBISHOP, 0, +3, +3 };
    moveInfo[0x0e] = { Piece::WROOK  , 1, +3, +0 };
    moveInfo[0x0f] = { Piece::WKNIGHT, 0, -2, -1 };
    moveInfo[0x12] = { Piece::WBISHOP, 0, +7, +7 };
    moveInfo[0x13] = { Piece::WKING  , 0, +0, +1 };
    moveInfo[0x14] = { Piece::WPAWN  , 7, +1, +1 };
    moveInfo[0x15] = { Piece::WBISHOP, 0, +5, +5 };
    moveInfo[0x18] = { Piece::WPAWN  , 6, +0, +1 };
    moveInfo[0x1a] = { Piece::WQUEEN , 1, +0, +6 };
    moveInfo[0x1b] = { Piece::WBISHOP, 0, -1, +1 };
    moveInfo[0x1d] = { Piece::WBISHOP, 1, +7, +7 };
    moveInfo[0x21] = { Piece::WROOK  , 1, +7, +0 };
    moveInfo[0x22] = { Piece::WBISHOP, 1, -2, +2 };
    moveInfo[0x23] = { Piece::WQUEEN , 1, +6, +6 };
    moveInfo[0x24] = { Piece::WPAWN  , 7, -1, +1 };
    moveInfo[0x26] = { Piece::WBISHOP, 0, -7, +7 };
    moveInfo[0x27] = { Piece::WPAWN  , 2, -1, +1 };
    moveInfo[0x28] = { Piece::WQUEEN , 0, +5, +5 };
    moveInfo[0x29] = { Piece::WQUEEN , 0, +6, +0 };
    moveInfo[0x2a] = { Piece::WKNIGHT, 1, +1, -2 };
    moveInfo[0x2d] = { Piece::WPAWN  , 5, +1, +1 };
    moveInfo[0x2e] = { Piece::WBISHOP, 0, +1, +1 };
    moveInfo[0x2f] = { Piece::WQUEEN , 0, +1, +0 };
    moveInfo[0x30] = { Piece::WKNIGHT, 1, -1, -2 };
    moveInfo[0x31] = { Piece::WQUEEN , 0, +3, +0 };
    moveInfo[0x32] = { Piece::WBISHOP, 1, +5, +5 };
    moveInfo[0x34] = { Piece::WKNIGHT, 0, +1, +2 };
    moveInfo[0x36] = { Piece::WKNIGHT, 0, +2, +1 };
    moveInfo[0x37] = { Piece::WQUEEN , 0, +0, +4 };
    moveInfo[0x38] = { Piece::WQUEEN , 1, -4, +4 };
    moveInfo[0x39] = { Piece::WQUEEN , 0, +5, +0 };
    moveInfo[0x3a] = { Piece::WBISHOP, 0, +6, +6 };
    moveInfo[0x3b] = { Piece::WQUEEN , 1, -5, +5 };
    moveInfo[0x3c] = { Piece::WBISHOP, 0, -5, +5 };
    moveInfo[0x41] = { Piece::WQUEEN , 1, +5, +5 };
    moveInfo[0x42] = { Piece::WQUEEN , 0, -7, +7 };
    moveInfo[0x44] = { Piece::WKING  , 0, +1, -1 };
    moveInfo[0x45] = { Piece::WQUEEN , 0, +3, +3 };
    moveInfo[0x4a] = { Piece::WPAWN  , 7, +0, +2 };
    moveInfo[0x4b] = { Piece::WQUEEN , 0, -5, +5 };
    moveInfo[0x4c] = { Piece::WKNIGHT, 1, +1, +2 };
    moveInfo[0x4d] = { Piece::WQUEEN , 1, +0, +1 };
    moveInfo[0x50] = { Piece::WROOK  , 0, +0, +6 };
    moveInfo[0x52] = { Piece::WROOK  , 0, +6, +0 };
    moveInfo[0x54] = { Piece::WBISHOP, 1, -1, +1 };
    moveInfo[0x55] = { Piece::WPAWN  , 2, +0, +1 };
    moveInfo[0x5c] = { Piece::WPAWN  , 6, +1, +1 };
    moveInfo[0x5f] = { Piece::WPAWN  , 4, +0, +2 };
    moveInfo[0x61] = { Piece::WQUEEN , 0, +6, +6 };
    moveInfo[0x62] = { Piece::WPAWN  , 1, +0, +2 };
    moveInfo[0x63] = { Piece::WQUEEN , 1, -7, +7 };
    moveInfo[0x66] = { Piece::WBISHOP, 0, -3, +3 };
    moveInfo[0x67] = { Piece::WKING  , 0, +1, +1 };
    moveInfo[0x69] = { Piece::WROOK  , 1, +0, +7 };
    moveInfo[0x6a] = { Piece::WBISHOP, 0, +4, +4 };
    moveInfo[0x6b] = { Piece::WKING  , 0, +2, +0 };
    moveInfo[0x6e] = { Piece::WROOK  , 0, +5, +0 };
    moveInfo[0x6f] = { Piece::WQUEEN , 1, +7, +7 };
    moveInfo[0x72] = { Piece::WBISHOP, 1, -7, +7 };
    moveInfo[0x74] = { Piece::WQUEEN , 0, +2, +0 };
    moveInfo[0x79] = { Piece::WBISHOP, 1, -6, +6 };
    moveInfo[0x7a] = { Piece::WROOK  , 0, +0, +3 };
    moveInfo[0x7b] = { Piece::WROOK  , 1, +0, +6 };
    moveInfo[0x7c] = { Piece::WPAWN  , 2, +1, +1 };
    moveInfo[0x7d] = { Piece::WROOK  , 1, +0, +1 };
    moveInfo[0x7e] = { Piece::WQUEEN , 0, -3, +3 };
    moveInfo[0x7f] = { Piece::WROOK  , 0, +1, +0 };
    moveInfo[0x80] = { Piece::WQUEEN , 0, -6, +6 };
    moveInfo[0x81] = { Piece::WROOK  , 0, +0, +1 };
    moveInfo[0x82] = { Piece::WPAWN  , 5, -1, +1 };
    moveInfo[0x85] = { Piece::WKNIGHT, 0, -1, +2 };
    moveInfo[0x86] = { Piece::WROOK  , 0, +7, +0 };
    moveInfo[0x87] = { Piece::WROOK  , 0, +0, +5 };
    moveInfo[0x8a] = { Piece::WKNIGHT, 0, +1, -2 };
    moveInfo[0x8b] = { Piece::WPAWN  , 0, +1, +1 };
    moveInfo[0x8c] = { Piece::WKING  , 0, -1, -1 };
    moveInfo[0x8e] = { Piece::WQUEEN , 1, -2, +2 };
    moveInfo[0x8f] = { Piece::WQUEEN , 0, +7, +0 };
    moveInfo[0x92] = { Piece::WQUEEN , 1, +1, +1 };
    moveInfo[0x94] = { Piece::WQUEEN , 0, +0, +3 };
    moveInfo[0x96] = { Piece::WPAWN  , 1, +1, +1 };
    moveInfo[0x97] = { Piece::WKING  , 0, -1, +0 };
    moveInfo[0x98] = { Piece::WROOK  , 0, +3, +0 };
    moveInfo[0x99] = { Piece::WROOK  , 0, +0, +4 };
    moveInfo[0x9a] = { Piece::WQUEEN , 0, +0, +6 };
    moveInfo[0x9b] = { Piece::WPAWN  , 2, +0, +2 };
    moveInfo[0x9d] = { Piece::WQUEEN , 0, +0, +2 };
    moveInfo[0x9f] = { Piece::WBISHOP, 1, -4, +4 };
    moveInfo[0xa0] = { Piece::WQUEEN , 1, +0, +3 };
    moveInfo[0xa2] = { Piece::WQUEEN , 0, +2, +2 };
    moveInfo[0xa3] = { Piece::WPAWN  , 7, +0, +1 };
    moveInfo[0xa5] = { Piece::WROOK  , 1, +0, +5 };
    moveInfo[0xa9] = { Piece::WROOK  , 1, +2, +0 };
    moveInfo[0xab] = { Piece::WQUEEN , 1, -6, +6 };
    moveInfo[0xad] = { Piece::WROOK  , 1, +4, +0 };
    moveInfo[0xae] = { Piece::WQUEEN , 1, +3, +3 };
    moveInfo[0xb0] = { Piece::WQUEEN , 1, +0, +4 };
    moveInfo[0xb1] = { Piece::WPAWN  , 5, +0, +2 };
    moveInfo[0xb2] = { Piece::WBISHOP, 0, -6, +6 };
    moveInfo[0xb5] = { Piece::WROOK  , 1, +5, +0 };
    moveInfo[0xb7] = { Piece::WQUEEN , 0, +0, +5 };
    moveInfo[0xb9] = { Piece::WBISHOP, 1, +3, +3 };
    moveInfo[0xbb] = { Piece::WPAWN  , 4, +0, +1 };
    moveInfo[0xbc] = { Piece::WQUEEN , 1, +5, +0 };
    moveInfo[0xbd] = { Piece::WQUEEN , 1, +0, +2 };
    moveInfo[0xbe] = { Piece::WKING  , 0, +1, +0 };
    moveInfo[0xc1] = { Piece::WBISHOP, 0, +2, +2 };
    moveInfo[0xc2] = { Piece::WBISHOP, 1, +2, +2 };
    moveInfo[0xc3] = { Piece::WBISHOP, 0, -2, +2 };
    moveInfo[0xc4] = { Piece::WROOK  , 1, +1, +0 };
    moveInfo[0xc5] = { Piece::WROOK  , 1, +0, +4 };
    moveInfo[0xc6] = { Piece::WQUEEN , 1, +0, +5 };
    moveInfo[0xc7] = { Piece::WPAWN  , 6, -1, +1 };
    moveInfo[0xc8] = { Piece::WPAWN  , 6, +0, +2 };
    moveInfo[0xc9] = { Piece::WQUEEN , 1, +0, +7 };
    moveInfo[0xca] = { Piece::WBISHOP, 1, -3, +3 };
    moveInfo[0xcb] = { Piece::WPAWN  , 5, +0, +1 };
    moveInfo[0xcc] = { Piece::WBISHOP, 1, -5, +5 };
    moveInfo[0xcd] = { Piece::WROOK  , 0, +2, +0 };
    moveInfo[0xcf] = { Piece::WPAWN  , 3, +0, +1 };
    moveInfo[0xd1] = { Piece::WPAWN  , 1, -1, +1 };
    moveInfo[0xd2] = { Piece::WKNIGHT, 1, +2, +1 };
    moveInfo[0xd3] = { Piece::WKNIGHT, 1, -2, +1 };
    moveInfo[0xd7] = { Piece::WQUEEN , 0, -1, +1 };
    moveInfo[0xd8] = { Piece::WROOK  , 1, +6, +0 };
    moveInfo[0xd9] = { Piece::WQUEEN , 0, -2, +2 };
    moveInfo[0xda] = { Piece::WKNIGHT, 0, -1, -2 };
    moveInfo[0xdb] = { Piece::WPAWN  , 0, +0, +2 };
    moveInfo[0xde] = { Piece::WPAWN  , 4, -1, +1 };
    moveInfo[0xdf] = { Piece::WKING  , 0, -1, +1 };
    moveInfo[0xe0] = { Piece::WKNIGHT, 1, +2, -1 };
    moveInfo[0xe1] = { Piece::WROOK  , 0, +0, +7 };
    moveInfo[0xe3] = { Piece::WROOK  , 1, +0, +3 };
    moveInfo[0xe5] = { Piece::WQUEEN , 0, +4, +0 };
    moveInfo[0xe6] = { Piece::WPAWN  , 3, +0, +2 };
    moveInfo[0xe7] = { Piece::WQUEEN , 0, +4, +4 };
    moveInfo[0xe8] = { Piece::WROOK  , 0, +0, +2 };
    moveInfo[0xe9] = { Piece::WKNIGHT, 0, +2, -1 };
    moveInfo[0xeb] = { Piece::WPAWN  , 3, +1, +1 };
    moveInfo[0xec] = { Piece::WPAWN  , 0, +0, +1 };
    moveInfo[0xed] = { Piece::WQUEEN , 0, +7, +7 };
    moveInfo[0xee] = { Piece::WQUEEN , 1, -1, +1 };
    moveInfo[0xef] = { Piece::WROOK  , 0, +4, +0 };
    moveInfo[0xf0] = { Piece::WQUEEN , 1, +7, +0 };
    moveInfo[0xf1] = { Piece::WQUEEN , 0, +1, +1 };
    moveInfo[0xf3] = { Piece::WKNIGHT, 1, -1, +2 };
    moveInfo[0xf4] = { Piece::WROOK  , 1, +0, +2 };
    moveInfo[0xf5] = { Piece::WBISHOP, 1, +1, +1 };
    moveInfo[0xf6] = { Piece::WKING  , 0, -2, +0 };
    moveInfo[0xf7] = { Piece::WKNIGHT, 0, -2, +1 };
    moveInfo[0xf8] = { Piece::WQUEEN , 1, +1, +0 };
    moveInfo[0xf9] = { Piece::WQUEEN , 1, +0, +6 };
    moveInfo[0xfa] = { Piece::WQUEEN , 1, +3, +0 };
    moveInfo[0xfb] = { Piece::WQUEEN , 1, +2, +2 };
    moveInfo[0xfd] = { Piece::WQUEEN , 0, +0, +7 };
    moveInfo[0xfe] = { Piece::WQUEEN , 1, -3, +3 };
}

void
PositionData::setFromPageBuf(const std::vector<U8>& pageBuf, int offs) {
    posLen = pageBuf.at(offs) & 0x1f;
    moveBytes = extractInt(pageBuf, offs + posLen, 1);
    int bufLen = posLen + moveBytes + posInfoBytes;
    buf.clear();
    for (int i = 0; i < bufLen; i++)
        buf.push_back(pageBuf.at(offs + i));
}

void
PositionData::getBookMoves(std::vector<BookEntry>& entries) {
    int nMoves = (moveBytes - 1) / 2;
    for (int mi = 0; mi < nMoves; mi++) {
        int move  = extractInt(buf, posLen + 1 + mi * 2, 1);
        int flags = extractInt(buf, posLen + 1 + mi * 2 + 1, 1);
        Move m = decodeMove(pos, move);
        if (m.isEmpty())
            continue;
//      System.out.printf("mi:%d m:%s flags:%d\n", mi, TextIO.moveToUCIString(m), flags);
        BookEntry ent;
        ent.move = m;
        switch (flags) {
        default:
        case 0x00: ent.weight = 1;       break; // No annotation
        case 0x01: ent.weight = 8;       break; // !
        case 0x02: ent.weight = 0;       break; // ?
        case 0x03: ent.weight = 32;      break; // !!
        case 0x04: ent.weight = 0;       break; // ??
        case 0x05: ent.weight = 0.5f;    break; // !?
        case 0x06: ent.weight = 0.125f;  break; // ?!
        case 0x08: ent.weight = 1000000; break; // Only move
        }
        entries.push_back(ent);
    }
}

int
PositionData::getOpponentScore() {
    int statStart = posLen + moveBytes;
//    int wins  = extractInt(buf, statStart + 3, 3);
    int loss  = extractInt(buf, statStart + 6, 3);
    int draws = extractInt(buf, statStart + 9, 3);
//    std::cout << "w:" << wins << " l:" << loss << " d:" << draws << std::endl;
    return loss * 2 + draws;
}

int
PositionData::getRecommendation() {
    int statStart = posLen + moveBytes;
    int recom = extractInt(buf, statStart + 30, 1);
    return recom;
}

int
PositionData::findPiece(const Position& pos, int piece, int pieceNo) {
    for (int x = 0; x < 8; x++)
        for (int y = 0; y < 8; y++) {
            int sq = Position::getSquare(x, y);
            if (pos.getPiece(sq) == piece)
                if (pieceNo-- == 0)
                    return sq;
        }
    return -1;
}

Move
PositionData::decodeMove(const Position& pos, int moveCode) {
    Move move;
    MoveInfo mi = moveInfo[moveCode];
    if (mi.piece == Piece::EMPTY)
        return move;
    int from = findPiece(pos, mi.piece, mi.pieceNo);
    if (from < 0)
        return move;
    int toX = (Position::getX(from) + mi.dx) & 7;
    int toY = (Position::getY(from) + mi.dy) & 7;
    int to = Position::getSquare(toX, toY);
    int promoteTo = Piece::EMPTY;
    if ((pos.getPiece(from) == Piece::WPAWN) && (toY == 7))
        promoteTo = Piece::WQUEEN;
    move.setMove(from, to, promoteTo, 0);
    return move;
}

// --------------------------------------------------------------------------------

CtbFile::CtbFile(std::fstream& f) {
    std::vector<U8> buf;
    readBytes(f, 4, 8, buf);
    lowerPageBound = extractInt(buf, 0, 4);
    upperPageBound = extractInt(buf, 4, 4);
}

// --------------------------------------------------------------------------------

CtoFile::CtoFile(std::fstream& f0)
    : f(f0) {
}

void
CtoFile::getHashIndices(const std::vector<U8>& encodedPos, const CtbFile& ctb,
                        std::vector<int>& indices) {
    int hash = getHashValue(encodedPos);
    for (int n = 0; n < 0x7fffffff; n = 2*n + 1) {
        int c = (hash & n) + n;
        if (c < ctb.lowerPageBound)
            continue;
        indices.push_back(c);
        if (c >= ctb.upperPageBound)
            break;
    }
}

int
CtoFile::getPage(int hashIndex) {
    std::vector<U8> buf;
    readBytes(f, 16 + 4 * hashIndex, 4, buf);
    int page = extractInt(buf, 0, 4);
    return page;
}

const int
CtoFile::tbl[] = {
    0x3100d2bf, 0x3118e3de, 0x34ab1372, 0x2807a847,
    0x1633f566, 0x2143b359, 0x26d56488, 0x3b9e6f59,
    0x37755656, 0x3089ca7b, 0x18e92d85, 0x0cd0e9d8,
    0x1a9e3b54, 0x3eaa902f, 0x0d9bfaae, 0x2f32b45b,
    0x31ed6102, 0x3d3c8398, 0x146660e3, 0x0f8d4b76,
    0x02c77a5f, 0x146c8799, 0x1c47f51f, 0x249f8f36,
    0x24772043, 0x1fbc1e4d, 0x1e86b3fa, 0x37df36a6,
    0x16ed30e4, 0x02c3148e, 0x216e5929, 0x0636b34e,
    0x317f9f56, 0x15f09d70, 0x131026fb, 0x38c784b1,
    0x29ac3305, 0x2b485dc5, 0x3c049ddc, 0x35a9fbcd,
    0x31d5373b, 0x2b246799, 0x0a2923d3, 0x08a96e9d,
    0x30031a9f, 0x08f525b5, 0x33611c06, 0x2409db98,
    0x0ca4feb2, 0x1000b71e, 0x30566e32, 0x39447d31,
    0x194e3752, 0x08233a95, 0x0f38fe36, 0x29c7cd57,
    0x0f7b3a39, 0x328e8a16, 0x1e7d1388, 0x0fba78f5,
    0x274c7e7c, 0x1e8be65c, 0x2fa0b0bb, 0x1eb6c371
};

int
CtoFile::getHashValue(const std::vector<U8>& encodedPos) {
    int hash = 0;
    int tmp = 0;
    for (int i = 0; i < (int)encodedPos.size(); i++) {
        U8 ch = encodedPos[i];
        tmp += ((0x0f - (ch & 0x0f)) << 2) + 1;
        hash += tbl[tmp & 0x3f];
        tmp += ((0xf0 - (ch & 0xf0)) >> 2) + 1;
        hash += tbl[tmp & 0x3f];
    }
    return hash;
}

// --------------------------------------------------------------------------------

CtgFile::CtgFile(std::fstream& f0, CtbFile& ctb0, CtoFile& cto0)
    : f(f0), ctb(ctb0), cto(cto0) {
}

bool
CtgFile::getPositionData(const Position& pos0, PositionData& pd) {
    Position pos(pos0);
    bool mirrorColor = !pos.isWhiteMove();
    if (mirrorColor)
        mirrorPosColor(pos);

    bool mirrorLeftRight = false;
    if ((pos.getCastleMask() == 0) && (Position::getX(pos.getKingSq(true)) < 4)) {
        mirrorPosLeftRight(pos);
        mirrorLeftRight = true;
    }

    std::vector<U8> encodedPos;
    positionToByteArray(pos, encodedPos);
    std::vector<int> hashIdxList;
    CtoFile::getHashIndices(encodedPos, ctb, hashIdxList);

    for (int i = 0; i < (int)hashIdxList.size(); i++) {
        int page = cto.getPage(hashIdxList[i]);
        if (page < 0)
            continue;
        if (findInPage(page, encodedPos, pd)) {
            pd.pos = pos;
            pd.mirrorColor = mirrorColor;
            pd.mirrorLeftRight = mirrorLeftRight;
            return true;
        }
    }
    return false;
}

bool
CtgFile::findInPage(int page, const std::vector<U8>& encodedPos, PositionData& pd) {
    std::vector<U8> pageBuf;
    readBytes(f, (page+1)*4096L, 4096, pageBuf);
    try {
        int nPos = extractInt(pageBuf, 0, 2);
        int nBytes = extractInt(pageBuf, 2, 2);
        for (int i = nBytes; i < 4096; i++)
            pageBuf.at(i) = 0; // Don't depend on trailing garbage
        int offs = 4;
        for (int p = 0; p < nPos; p++) {
            bool match = true;
            for (int i = 0; i < (int)encodedPos.size(); i++)
                if (encodedPos[i] != pageBuf.at(offs+i)) {
                    match = false;
                    break;
                }
            if (match) {
                pd.setFromPageBuf(pageBuf, offs);
                return true;
            }

            int posLen = pageBuf.at(offs) & 0x1f;
            offs += posLen;
            int moveBytes = extractInt(pageBuf, offs, 1);
            offs += moveBytes;
            offs += PositionData::posInfoBytes;
        }
        return false;
    } catch (const std::out_of_range& ex) {
        return false; // Ignore corrupt book file entries
    }
}

// --------------------------------------------------------------------------------

CtgBook::CtgBook(const std::string& fileName, bool tournament, bool preferMain)
    : tournamentMode(tournament), preferMainLines(preferMain) {
    int len = fileName.length();
    std::string ctgFile = fileName.substr(0, len-1) + "g";
    std::string ctbFile = fileName.substr(0, len-1) + "b";
    std::string ctoFile = fileName.substr(0, len-1) + "o";

    ctgF.open(ctgFile, std::ios_base::in | std::ios_base::binary);
    ctbF.open(ctbFile, std::ios_base::in | std::ios_base::binary);
    ctoF.open(ctoFile, std::ios_base::in | std::ios_base::binary);
}

bool
CtgBook::getBookMove(Position& pos, Move& out) {
    std::vector<BookEntry> bookMoves;
    getBookEntries(pos, bookMoves);
    if (bookMoves.empty())
        return false;

    MoveList legalMoves;
    MoveGen::pseudoLegalMoves(pos, legalMoves);
    MoveGen::removeIllegal(pos, legalMoves);

    float fSum = 0;
    for (const BookEntry& be : bookMoves) {
        bool contains = false;
        for (int mi = 0; mi < legalMoves.size; mi++)
            if (legalMoves[mi].equals(be.move)) {
                contains = true;
                break;
            }
        if (!contains)
            return false;
        fSum += be.weight;
//        std::cout << "move:" << TextIO::moveToUCIString(be.move) << " w:" << be.weight << std::endl;
    }
    if (fSum <= 0)
        return false;

    float scale = 1000000/fSum;
    int sum = 0;
    for (const BookEntry& be : bookMoves) {
        bool contains = false;
        for (int mi = 0; mi < legalMoves.size; mi++)
            if (legalMoves[mi].equals(be.move)) {
                contains = true;
                break;
            }
        if (!contains)
            return false;
        sum += (int)(be.weight * scale);
    }
//    std::cout << "sum:" << sum << std::endl;
    if (sum <= 0)
        return false;

    int rnd = rndGen.nextInt(sum);
    sum = 0;
    for (const BookEntry& be : bookMoves) {
        sum += (int)(be.weight * scale);
        if (rnd < sum) {
            out = be.move;
            return true;
        }
    }
    // Should never get here
    assert(false);
}

void
CtgBook::getBookEntries(const Position& pos, std::vector<BookEntry>& bookMoves) {
    CtbFile ctb(ctbF);
    CtoFile cto(ctoF);
    CtgFile ctg(ctgF, ctb, cto);

    PositionData pd, movePd;
    if (ctg.getPositionData(pos, pd)) {
        bool mirrorColor = pd.mirrorColor;
        bool mirrorLeftRight = pd.mirrorLeftRight;
        pd.getBookMoves(bookMoves);
        UndoInfo ui;
        for (BookEntry& be : bookMoves) {
            pd.pos.makeMove(be.move, ui);
            bool haveMovePd = ctg.getPositionData(pd.pos, movePd);
            pd.pos.unMakeMove(be.move, ui);
            float weight = be.weight;
            if (!haveMovePd) {
//              System.out.printf("%s : no pos\n", TextIO.moveToUCIString(be.move));
                weight = 0;
            } else {
                int recom = movePd.getRecommendation();
                if ((recom >= 64) && (recom < 128)) {
                    if (tournamentMode)
                        weight = 0;
                } else if (recom >= 128) {
                    if (preferMainLines)
                        weight *= 10;
                }
                float score = movePd.getOpponentScore() + 1e-4f;
//              double w0 = weight;
                weight *= score;
//              System.out.printf("%s : w0:%.3f rec:%d score:%d %.3f\n", TextIO.moveToUCIString(be.move),
//                                w0, recom, score, weight);
            }
            be.weight = weight;
        }
        if (mirrorLeftRight) {
            for (BookEntry& be : bookMoves)
                mirrorMoveLeftRight(be.move);
        }
        if (mirrorColor) {
            for (BookEntry& be : bookMoves)
                mirrorMoveColor(be.move);
        }
    }
}
