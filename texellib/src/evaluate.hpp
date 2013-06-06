/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
 * evaluate.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef EVALUATE_HPP_
#define EVALUATE_HPP_

#include "parameters.hpp"
#include "piece.hpp"
#include "position.hpp"
#include "alignedAlloc.hpp"

/**
 * Position evaluation routines.
 *
 */
class Evaluate {
public:
    static const int pV =   92; // + Parameters::instance().getIntPar("pV");
    static const int nV =  385; // + Parameters::instance().getIntPar("nV");
    static const int bV =  385; // + Parameters::instance().getIntPar("bV");
    static const int rV =  593; // + Parameters::instance().getIntPar("rV");
    static const int qV = 1244; // + Parameters::instance().getIntPar("qV");
    static const int kV = 9900; // Used by SEE algorithm, but not included in board material sums

    static int pieceValue[Piece::nPieceTypes];
    static int pieceValueOrder[Piece::nPieceTypes];

    /** Piece/square table for king during middle game. */
    static const int kt1b[64];

    /** Piece/square table for king during end game. */
    static const int kt2b[64];

    /** Piece/square table for pawns during middle game. */
    static const int pt1b[64];

    /** Piece/square table for pawns during end game. */
    static const int pt2b[64];

    /** Piece/square table for knights during middle game. */
    static const int nt1b[64];

    /** Piece/square table for knights during end game. */
    static const int nt2b[64];

    /** Piece/square table for bishops during middle game. */
    static const int bt1b[64];

    /** Piece/square table for bishops during middle game. */
    static const int bt2b[64];

    /** Piece/square table for queens during middle game. */
    static const int qt1b[64];

    /** Piece/square table for rooks during middle game. */
    static const int rt1b[64];

    static int kt1w[64], qt1w[64], rt1w[64], bt1w[64], nt1w[64], pt1w[64];
    static int kt2w[64], bt2w[64], nt2w[64], pt2w[64];

public:
    static const int* psTab1[Piece::nPieceTypes];
    static const int* psTab2[Piece::nPieceTypes];

    static const int distToH1A8[8][8];

    static const int rookMobScore[];
    static const int bishMobScore[];
    static const int queenMobScore[];
    static int knightMobScore[64][9];

private:
    struct PawnHashData {
        PawnHashData();
        U64 key;
        int score;            // Positive score means good for white
        short passedBonusW;
        short passedBonusB;
        U64 passedPawnsW;     // The most advanced passed pawns for each file
        U64 passedPawnsB;
        U64 outPostsW;        // Possible outpost squares for white
        U64 outPostsB;
    };
    static std::vector<PawnHashData> pawnHash;
    const PawnHashData* phd;

    static const ubyte kpkTable[2*32*64*48/8];
    static const ubyte krkpTable[2*32*48*8];
    static const U64 krpkrTable[2*24*64];

    // King safety variables
    U64 wKingZone, bKingZone;       // Squares close to king that are worth attacking
    int wKingAttacks, bKingAttacks; // Number of attacks close to white/black king
    U64 wAttacksBB, bAttacksBB;
    U64 wPawnAttacks, bPawnAttacks; // Squares attacked by white/black pawns

public:
    /** Constructor. */
    Evaluate();

    /**
     * Static evaluation of a position.
     * @param pos The position to evaluate.
     * @return The evaluation score, measured in centipawns.
     *         Positive values are good for the side to make the next move.
     */
    int evalPos(const Position& pos);

    /**
     * Interpolate between (x1,y1) and (x2,y2).
     * If x < x1, return y1, if x > x2 return y2. Otherwise, use linear interpolation.
     */
    static int interpolate(int x, int x1, int y1, int x2, int y2) {
        if (x > x2) {
            return y2;
        } else if (x < x1) {
            return y1;
        } else {
            return (x - x1) * (y2 - y1) / (x2 - x1) + y1;
        }
    }

    static const int IPOLMAX = 1024;

    /** Compute v1 + (v2-v1)*k/IPOLMAX */
    static int interpolate(int v1, int v2, int k) {
        return v1 + (v2 - v1) * k / IPOLMAX;
    }

    /** Compute white_material - black_material. */
    static int material(const Position& pos) {
        return pos.wMtrl - pos.bMtrl;
    }

    static void staticInitialize();

private:
    /** Compute score based on piece square tables. Positive values are good for white. */
    int pieceSquareEval(const Position& pos);

    struct MaterialHashData {
        MaterialHashData() : id(-1), score(0) { }
        int id;
        int score;
        short wPawnIPF, bPawnIPF;
        short wKnightIPF, bKnightIPF;
        short castleIPF;
        short wPassedPawnIPF, bPassedPawnIPF;
        short kingSafetyIPF;
        short diffColorBishopIPF;
        short wKnightOutPostIPF, bKnightOutPostIPF;
    };
    static std::vector<MaterialHashData> materialHash;
    const MaterialHashData* mhd;

    /** Get material score */
    int materialScore(const Position& pos) {
        int mId = pos.materialId();
        int key = (mId >> 16) * 40507 + mId;
        MaterialHashData& newMhd = materialHash[key & (materialHash.size() - 1)];
        if (newMhd.id != mId)
            computeMaterialScore(pos, newMhd);
        mhd = &newMhd;
        return newMhd.score;
    }

    /** Compute material score. */
    void computeMaterialScore(const Position& pos, MaterialHashData& mhd) const;

    /** Implement the "when ahead trade pieces, when behind trade pawns" rule. */
    int tradeBonus(const Position& pos) const;

    static int castleFactor[256];

    /** Score castling ability. */
    int castleBonus(const Position& pos);

    int pawnBonus(const Position& pos);

    /** Compute pawn hash data for pos. */
    void computePawnHashData(const Position& pos, PawnHashData& ph);

    /** Compute rook bonus. Rook on open/half-open file. */
    int rookBonus(const Position& pos);

    /** Compute bishop evaluation. */
    int bishopEval(const Position& pos, int oldScore);

    /** Compute knight evaluation. */
    int knightEval(const Position& pos);

    int threatBonus(const Position& pos);

    /** Compute king safety for both kings. */
    int kingSafety(const Position& pos);

    struct KingSafetyHashData {
        KingSafetyHashData() : key((U64)-1), score(0) { }
        U64 key;
        int score;
    };
    static vector_aligned<KingSafetyHashData> kingSafetyHash;

    int kingSafetyKPPart(const Position& pos);

    /** Implements special knowledge for some endgame situations. */
    int endGameEval(const Position& pos, int oldScore);

    static int evalKQKP(int wKing, int wQueen, int bKing, int bPawn, bool whiteMove);

    static int kpkEval(int wKing, int bKing, int wPawn, bool whiteMove);

    static int krkpEval(int wKing, int bKing, int bPawn, bool whiteMove);

    static int krpkrEval(int wKing, int bKing, int wPawn, int wRook, int bRook, bool whiteMove);
};

inline
Evaluate::PawnHashData::PawnHashData()
    : key((U64)-1), // Non-zero to avoid collision for positions with no pawns
      score(0),
      passedBonusW(0),
      passedBonusB(0),
      passedPawnsW(0),
      passedPawnsB(0) {
}

#endif /* EVALUATE_HPP_ */
