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

private:
    struct PawnHashData {
        PawnHashData();
        U64 key;
        int score;            // Positive score means good for white
        short passedBonusW;
        short passedBonusB;
        U64 passedPawnsW;     // The most advanced passed pawns for each file
        U64 passedPawnsB;
    };
    static std::vector<PawnHashData> pawnHash;

    static const byte kpkTable[2*32*64*48/8];
    static const byte krkpTable[2*32*48*8];

    // King safety variables
    U64 wKingZone, bKingZone;       // Squares close to king that are worth attacking
    int wKingAttacks, bKingAttacks; // Number of attacks close to white/black king
    U64 wAttacksBB, bAttacksBB;
    U64 wPawnAttacks, bPawnAttacks; // Squares attacked by white/black pawns

public:
    /** Constructor. */
    Evaluate()
        : wKingZone(0), bKingZone(0),
          wKingAttacks(0), bKingAttacks(0),
          wAttacksBB(0), bAttacksBB(0),
          wPawnAttacks(0), bPawnAttacks(0) {
    }

    /**
     * Static evaluation of a position.
     * @param pos The position to evaluate.
     * @return The evaluation score, measured in centipawns.
     *         Positive values are good for the side to make the next move.
     */
    int evalPos(const Position& pos) {
        int score = pos.wMtrl - pos.bMtrl;

        wKingAttacks = bKingAttacks = 0;
        wKingZone = BitBoard::kingAttacks[pos.getKingSq(true)]; wKingZone |= wKingZone << 8;
        bKingZone = BitBoard::kingAttacks[pos.getKingSq(false)]; bKingZone |= bKingZone >> 8;
        wAttacksBB = bAttacksBB = 0L;

        U64 pawns = pos.pieceTypeBB[Piece::WPAWN];
        wPawnAttacks = ((pawns & BitBoard::maskBToHFiles) << 7) |
                       ((pawns & BitBoard::maskAToGFiles) << 9);
        pawns = pos.pieceTypeBB[Piece::BPAWN];
        bPawnAttacks = ((pawns & BitBoard::maskBToHFiles) >> 9) |
                       ((pawns & BitBoard::maskAToGFiles) >> 7);

        score += pieceSquareEval(pos);
        score += pawnBonus(pos);
        score += tradeBonus(pos);
        score += castleBonus(pos);

        score += rookBonus(pos);
        score += bishopEval(pos, score);
        score += threatBonus(pos);
        score += kingSafety(pos);
        score = endGameEval(pos, score);

        if (!pos.whiteMove)
            score = -score;
        return score;

        // FIXME! Test penalty if side to move has >1 hanging piece

        // FIXME! Test "tempo value"
    }

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

    /** Compute white_material - black_material. */
    static int material(const Position& pos) {
        return pos.wMtrl - pos.bMtrl;
    }

    static void staticInitialize();

private:
    /** Compute score based on piece square tables. Positive values are good for white. */
    int pieceSquareEval(const Position& pos) {
        int score = 0;
        const int wMtrl = pos.wMtrl;
        const int bMtrl = pos.bMtrl;
        const int wMtrlPawns = pos.wMtrlPawns;
        const int bMtrlPawns = pos.bMtrlPawns;

        // Kings
        {
            const int t1 = qV + 2 * rV + 2 * bV;
            const int t2 = rV;
            {
                const int k1 = pos.psScore1[Piece::WKING];
                const int k2 = pos.psScore2[Piece::WKING];
                const int t = bMtrl - bMtrlPawns;
                score += interpolate(t, t2, k2, t1, k1);
            }
            {
                const int k1 = pos.psScore1[Piece::BKING];
                const int k2 = pos.psScore2[Piece::BKING];
                const int t = wMtrl - wMtrlPawns;
                score -= interpolate(t, t2, k2, t1, k1);
            }
        }

        // Pawns
        {
            const int t1 = qV + 2 * rV + 2 * bV;
            const int t2 = rV;
            int wp1 = pos.psScore1[Piece::WPAWN];
            int wp2 = pos.psScore2[Piece::WPAWN];
            if ((wp1 != 0) || (wp2 != 0)) {
                const int tw = bMtrl - bMtrlPawns;
                score += interpolate(tw, t2, wp2, t1, wp1);
            }
            int bp1 = pos.psScore1[Piece::BPAWN];
            int bp2 = pos.psScore2[Piece::BPAWN];
            if ((bp1 != 0) || (bp2 != 0)) {
                const int tb = wMtrl - wMtrlPawns;
                score -= interpolate(tb, t2, bp2, t1, bp1);
            }
        }

        // Knights
        {
            const int t1 = qV + 2 * rV + 1 * bV + 1 * nV + 6 * pV;
            const int t2 = nV + 8 * pV;
            int n1 = pos.psScore1[Piece::WKNIGHT];
            int n2 = pos.psScore2[Piece::WKNIGHT];
            if ((n1 != 0) || (n2 != 0)) {
                score += interpolate(bMtrl, t2, n2, t1, n1);
            }
            n1 = pos.psScore1[Piece::BKNIGHT];
            n2 = pos.psScore2[Piece::BKNIGHT];
            if ((n1 != 0) || (n2 != 0)) {
                score -= interpolate(wMtrl, t2, n2, t1, n1);
            }
        }

        // Bishops
        {
            score += pos.psScore1[Piece::WBISHOP];
            score -= pos.psScore1[Piece::BBISHOP];
        }

        // Queens
        {
            const U64 occupied = pos.whiteBB | pos.blackBB;
            score += pos.psScore1[Piece::WQUEEN];
            U64 m = pos.pieceTypeBB[Piece::WQUEEN];
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
                wAttacksBB |= atk;
                score += queenMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB | bPawnAttacks))];
                bKingAttacks += BitBoard::bitCount(atk & bKingZone) * 2;
                m &= m-1;
            }
            score -= pos.psScore1[Piece::BQUEEN];
            m = pos.pieceTypeBB[Piece::BQUEEN];
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
                bAttacksBB |= atk;
                score -= queenMobScore[BitBoard::bitCount(atk & ~(pos.blackBB | wPawnAttacks))];
                wKingAttacks += BitBoard::bitCount(atk & wKingZone) * 2;
                m &= m-1;
            }
        }

        // Rooks
        {
            int r1 = pos.psScore1[Piece::WROOK];
            if (r1 != 0) {
                const int nP = bMtrlPawns / pV;
                const int s = r1 * std::min(nP, 6) / 6;
                score += s;
            }
            r1 = pos.psScore1[Piece::BROOK];
            if (r1 != 0) {
                const int nP = wMtrlPawns / pV;
                const int s = r1 * std::min(nP, 6) / 6;
                score -= s;
            }
        }

        return score;
    }

    /** Implement the "when ahead trade pieces, when behind trade pawns" rule. */
    int tradeBonus(const Position& pos) {
        const int wM = pos.wMtrl;
        const int bM = pos.bMtrl;
        const int wPawn = pos.wMtrlPawns;
        const int bPawn = pos.bMtrlPawns;
        const int deltaScore = wM - bM;

        int pBonus = 0;
        pBonus += interpolate((deltaScore > 0) ? wPawn : bPawn, 0, -30 * deltaScore / 100, 6 * pV, 0);
        pBonus += interpolate((deltaScore > 0) ? bM : wM, 0, 30 * deltaScore / 100, qV + 2 * rV + 2 * bV + 2 * nV, 0);

        return pBonus;
    }

    static int castleFactor[256];

    /** Score castling ability. */
    int castleBonus(const Position& pos) {
        if (pos.getCastleMask() == 0) return 0;

        const int k1 = kt1b[7*8+6] - kt1b[7*8+4];
        const int k2 = kt2b[7*8+6] - kt2b[7*8+4];
        const int t1 = qV + 2 * rV + 2 * bV;
        const int t2 = rV;
        const int t = pos.bMtrl - pos.bMtrlPawns;
        const int ks = interpolate(t, t2, k2, t1, k1);

        const int castleValue = ks + rt1b[7*8+5] - rt1b[7*8+7];
        if (castleValue <= 0)
            return 0;

        U64 occupied = pos.whiteBB | pos.blackBB;
        int tmp = (int) (occupied & 0x6E);
        if (pos.a1Castle()) tmp |= 1;
        if (pos.h1Castle()) tmp |= (1 << 7);
        const int wBonus = (castleValue * castleFactor[tmp]) >> 10;

        tmp = (int) ((occupied >> 56) & 0x6E);
        if (pos.a8Castle()) tmp |= 1;
        if (pos.h8Castle()) tmp |= (1 << 7);
        const int bBonus = (castleValue * castleFactor[tmp]) >> 10;

        return wBonus - bBonus;
    }

    int pawnBonus(const Position& pos) {
        U64 key = pos.pawnZobristHash();
        PawnHashData& phd = pawnHash[(int)key & (pawnHash.size() - 1)];
        if (phd.key != key)
            computePawnHashData(pos, phd);
        int score = phd.score;

        const int hiMtrl = qV + rV;
        score += interpolate(pos.bMtrl - pos.bMtrlPawns, 0, 2 * phd.passedBonusW, hiMtrl, phd.passedBonusW);
        score -= interpolate(pos.wMtrl - pos.wMtrlPawns, 0, 2 * phd.passedBonusB, hiMtrl, phd.passedBonusB);

        // Passed pawns are more dangerous if enemy king is far away
        int mtrlNoPawns;
        const int highMtrl = qV + rV;
        U64 m = phd.passedPawnsW;
        if (m != 0) {
            mtrlNoPawns = pos.bMtrl - pos.bMtrlPawns;
            if (mtrlNoPawns < highMtrl) {
                int kingPos = pos.getKingSq(false);
                int kingX = Position::getX(kingPos);
                int kingY = Position::getY(kingPos);
                while (m != 0) {
                    int sq = BitBoard::numberOfTrailingZeros(m);
                    int x = Position::getX(sq);
                    int y = Position::getY(sq);
                    int pawnDist = std::min(5, 7 - y);
                    int kingDistX = std::abs(kingX - x);
                    int kingDistY = std::abs(kingY - 7);
                    int kingDist = std::max(kingDistX, kingDistY);
                    int kScore = kingDist * 4;
                    if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                    score += interpolate(mtrlNoPawns, 0, kScore, highMtrl, 0);
                    if (!pos.whiteMove)
                        kingDist--;
                    if ((pawnDist < kingDist) && (mtrlNoPawns == 0))
                        score += 500; // King can't stop pawn
                    m &= m-1;
                }
            }
        }
        m = phd.passedPawnsB;
        if (m != 0) {
            mtrlNoPawns = pos.wMtrl - pos.wMtrlPawns;
            if (mtrlNoPawns < highMtrl) {
                int kingPos = pos.getKingSq(true);
                int kingX = Position::getX(kingPos);
                int kingY = Position::getY(kingPos);
                while (m != 0) {
                    int sq = BitBoard::numberOfTrailingZeros(m);
                    int x = Position::getX(sq);
                    int y = Position::getY(sq);
                    int pawnDist = std::min(5, y);
                    int kingDistX = std::abs(kingX - x);
                    int kingDistY = std::abs(kingY - 0);
                    int kingDist = std::max(kingDistX, kingDistY);
                    int kScore = kingDist * 4;
                    if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                    score -= interpolate(mtrlNoPawns, 0, kScore, highMtrl, 0);
                    if (pos.whiteMove)
                        kingDist--;
                    if ((pawnDist < kingDist) && (mtrlNoPawns == 0))
                        score -= 500; // King can't stop pawn
                    m &= m-1;
                }
            }
        }

        return score;
    }

    /** Compute pawn hash data for pos. */
    void computePawnHashData(const Position& pos, PawnHashData& ph) {
        int score = 0;

        // Evaluate double pawns and pawn islands
        U64 wPawns = pos.pieceTypeBB[Piece::WPAWN];
        U64 wPawnFiles = BitBoard::southFill(wPawns) & 0xff;
        int wDouble = BitBoard::bitCount(wPawns) - BitBoard::bitCount(wPawnFiles);
        int wIslands = BitBoard::bitCount(((~wPawnFiles) >> 1) & wPawnFiles);
        int wIsolated = BitBoard::bitCount(~(wPawnFiles<<1) & wPawnFiles & ~(wPawnFiles>>1));


        U64 bPawns = pos.pieceTypeBB[Piece::BPAWN];
        U64 bPawnFiles = BitBoard::southFill(bPawns) & 0xff;
        int bDouble = BitBoard::bitCount(bPawns) - BitBoard::bitCount(bPawnFiles);
        int bIslands = BitBoard::bitCount(((~bPawnFiles) >> 1) & bPawnFiles);
        int bIsolated = BitBoard::bitCount(~(bPawnFiles<<1) & bPawnFiles & ~(bPawnFiles>>1));

        score -= (wDouble - bDouble) * 25;
        score -= (wIslands - bIslands) * 15;
        score -= (wIsolated - bIsolated) * 15;

        // Evaluate backward pawns, defined as a pawn that guards a friendly pawn,
        // can't be guarded by friendly pawns, can advance, but can't advance without
        // being captured by an enemy pawn.
        U64 wPawnAttacks = (((wPawns & BitBoard::maskBToHFiles) << 7) |
                            ((wPawns & BitBoard::maskAToGFiles) << 9));
        U64 bPawnAttacks = (((bPawns & BitBoard::maskBToHFiles) >> 9) |
                            ((bPawns & BitBoard::maskAToGFiles) >> 7));
        U64 wBackward = wPawns & ~((wPawns | bPawns) >> 8) & (bPawnAttacks >> 8) &
                        ~BitBoard::northFill(wPawnAttacks);
        wBackward &= (((wPawns & BitBoard::maskBToHFiles) >> 9) |
                      ((wPawns & BitBoard::maskAToGFiles) >> 7));
        wBackward &= ~BitBoard::northFill(bPawnFiles);
        U64 bBackward = bPawns & ~((wPawns | bPawns) << 8) & (wPawnAttacks << 8) &
                        ~BitBoard::southFill(bPawnAttacks);
        bBackward &= (((bPawns & BitBoard::maskBToHFiles) << 7) |
                      ((bPawns & BitBoard::maskAToGFiles) << 9));
        bBackward &= ~BitBoard::northFill(wPawnFiles);
        score -= (BitBoard::bitCount(wBackward) - BitBoard::bitCount(bBackward)) * 15;

        // Evaluate passed pawn bonus, white
        U64 passedPawnsW = wPawns & ~BitBoard::southFill(bPawns | bPawnAttacks | (wPawns >> 8));
        static const int ppBonus[] = {-1,24,26,30,36,47,64,-1};
        int passedBonusW = 0;
        if (passedPawnsW != 0) {
            U64 guardedPassedW = passedPawnsW & (((wPawns & BitBoard::maskBToHFiles) << 7) |
                                                  ((wPawns & BitBoard::maskAToGFiles) << 9));
            passedBonusW += 15 * BitBoard::bitCount(guardedPassedW);
            U64 m = passedPawnsW;
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                int y = Position::getY(sq);
                passedBonusW += ppBonus[y];
                m &= m-1;
            }
        }

        // Evaluate passed pawn bonus, black
        U64 passedPawnsB = bPawns & ~BitBoard::northFill(wPawns | wPawnAttacks | (bPawns << 8));
        int passedBonusB = 0;
        if (passedPawnsB != 0) {
            U64 guardedPassedB = passedPawnsB & (((bPawns & BitBoard::maskBToHFiles) >> 9) |
                                                  ((bPawns & BitBoard::maskAToGFiles) >> 7));
            passedBonusB += 15 * BitBoard::bitCount(guardedPassedB);
            U64 m = passedPawnsB;
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                int y = Position::getY(sq);
                passedBonusB += ppBonus[7-y];
                m &= m-1;
            }
        }

        // Connected passed pawn bonus. Seems logical but doesn't help in tests
//        if (passedPawnsW != 0)
//            passedBonusW += 15 * BitBoard::bitCount(passedPawnsW & ((passedPawnsW & BitBoard::maskBToHFiles) >> 1));
//        if (passedPawnsB != 0)
//            passedBonusB += 15 * BitBoard::bitCount(passedPawnsB & ((passedPawnsB & BitBoard::maskBToHFiles) >> 1));

        ph.key = pos.pawnZobristHash();
        ph.score = score;
        ph.passedBonusW = (short)passedBonusW;
        ph.passedBonusB = (short)passedBonusB;
        ph.passedPawnsW = passedPawnsW;
        ph.passedPawnsB = passedPawnsB;
    }

    /** Compute rook bonus. Rook on open/half-open file. */
    int rookBonus(const Position& pos) {
        int score = 0;
        const U64 wPawns = pos.pieceTypeBB[Piece::WPAWN];
        const U64 bPawns = pos.pieceTypeBB[Piece::BPAWN];
        const U64 occupied = pos.whiteBB | pos.blackBB;
        U64 m = pos.pieceTypeBB[Piece::WROOK];
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            const int x = Position::getX(sq);
            if ((wPawns & BitBoard::maskFile[x]) == 0) { // At least half-open file
                score += (bPawns & BitBoard::maskFile[x]) == 0 ? 25 : 12;
            }
            U64 atk = BitBoard::rookAttacks(sq, occupied);
            wAttacksBB |= atk;
            score += rookMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB | bPawnAttacks))];
            if ((atk & bKingZone) != 0)
                bKingAttacks += BitBoard::bitCount(atk & bKingZone);
            m &= m-1;
        }
        U64 r7 = pos.pieceTypeBB[Piece::WROOK] & 0x00ff000000000000ULL;
        if (((r7 & (r7 - 1)) != 0) &&
            ((pos.pieceTypeBB[Piece::BKING] & 0xff00000000000000ULL) != 0))
            score += 30; // Two rooks on 7:th row
        m = pos.pieceTypeBB[Piece::BROOK];
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            const int x = Position::getX(sq);
            if ((bPawns & BitBoard::maskFile[x]) == 0) {
                score -= (wPawns & BitBoard::maskFile[x]) == 0 ? 25 : 12;
            }
            U64 atk = BitBoard::rookAttacks(sq, occupied);
            bAttacksBB |= atk;
            score -= rookMobScore[BitBoard::bitCount(atk & ~(pos.blackBB | wPawnAttacks))];
            if ((atk & wKingZone) != 0)
                wKingAttacks += BitBoard::bitCount(atk & wKingZone);
            m &= m-1;
        }
        r7 = pos.pieceTypeBB[Piece::BROOK] & 0xff00L;
        if (((r7 & (r7 - 1)) != 0) &&
            ((pos.pieceTypeBB[Piece::WKING] & 0xffL) != 0))
          score -= 30; // Two rooks on 2:nd row
        return score;
    }

    /** Compute bishop evaluation. */
    int bishopEval(const Position& pos, int oldScore) {
        int score = 0;
        const U64 occupied = pos.whiteBB | pos.blackBB;
        U64 wBishops = pos.pieceTypeBB[Piece::WBISHOP];
        U64 bBishops = pos.pieceTypeBB[Piece::BBISHOP];
        if ((wBishops | bBishops) == 0)
            return 0;
        U64 m = wBishops;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            U64 atk = BitBoard::bishopAttacks(sq, occupied);
            wAttacksBB |= atk;
            score += bishMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB | bPawnAttacks))];
            if ((atk & bKingZone) != 0)
                bKingAttacks += BitBoard::bitCount(atk & bKingZone);
            m &= m-1;
        }
        m = bBishops;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            U64 atk = BitBoard::bishopAttacks(sq, occupied);
            bAttacksBB |= atk;
            score -= bishMobScore[BitBoard::bitCount(atk & ~(pos.blackBB | wPawnAttacks))];
            if ((atk & wKingZone) != 0)
                wKingAttacks += BitBoard::bitCount(atk & wKingZone);
            m &= m-1;
        }

        bool whiteDark  = (pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq ) != 0;
        bool whiteLight = (pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskLightSq) != 0;
        bool blackDark  = (pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq ) != 0;
        bool blackLight = (pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskLightSq) != 0;
        int numWhite = (whiteDark ? 1 : 0) + (whiteLight ? 1 : 0);
        int numBlack = (blackDark ? 1 : 0) + (blackLight ? 1 : 0);

        // Bishop pair bonus
        if (numWhite == 2) {
            const int numPawns = pos.wMtrlPawns / pV;
            score += 28 + (8 - numPawns) * 3;
        }
        if (numBlack == 2) {
            const int numPawns = pos.bMtrlPawns / pV;
            score -= 28 + (8 - numPawns) * 3;
        }

        // FIXME!!! Bad bishop

        if ((numWhite == 1) && (numBlack == 1) && (whiteDark != blackDark) &&
            (pos.wMtrl - pos.wMtrlPawns == pos.bMtrl - pos.bMtrlPawns)) {
            const int penalty = (oldScore + score) / 2;
            const int loMtrl = 2 * bV;
            const int hiMtrl = 2 * (qV + rV + bV);
            int mtrl = pos.wMtrl + pos.bMtrl - pos.wMtrlPawns - pos.bMtrlPawns;
            score -= interpolate(mtrl, loMtrl, penalty, hiMtrl, 0);
        }

        // Penalty for bishop trapped behind pawn at a2/h2/a7/h7
        if (((wBishops | bBishops) & 0x0081000000008100L) != 0) {
            if ((pos.getPiece(48) == Piece::WBISHOP) && // a7
                (pos.getPiece(41) == Piece::BPAWN) &&
                (pos.getPiece(50) == Piece::BPAWN))
                score -= pV * 3 / 2;
            if ((pos.getPiece(55) == Piece::WBISHOP) && // h7
                (pos.getPiece(46) == Piece::BPAWN) &&
                (pos.getPiece(53) == Piece::BPAWN))
                score -= (pos.pieceTypeBB[Piece::WQUEEN] != 0) ? pV : pV * 3 / 2;
            if ((pos.getPiece(8)  == Piece::BBISHOP) &&  // a2
                (pos.getPiece(17) == Piece::WPAWN) &&
                (pos.getPiece(10) == Piece::WPAWN))
                score += pV * 3 / 2;
            if ((pos.getPiece(15) == Piece::BBISHOP) && // h2
                (pos.getPiece(22) == Piece::WPAWN) &&
                (pos.getPiece(13) == Piece::WPAWN))
                score += (pos.pieceTypeBB[Piece::BQUEEN] != 0) ? pV : pV * 3 / 2;
        }

        return score;
    }

    int threatBonus(const Position& pos) {
        int score = 0;

        // Sum values for all black pieces under attack
        U64 m = pos.pieceTypeBB[Piece::WKNIGHT];
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            wAttacksBB |= BitBoard::knightAttacks[sq];
            m &= m-1;
        }
        wAttacksBB &= (pos.pieceTypeBB[Piece::BKNIGHT] |
                       pos.pieceTypeBB[Piece::BBISHOP] |
                       pos.pieceTypeBB[Piece::BROOK] |
                       pos.pieceTypeBB[Piece::BQUEEN]);
        wAttacksBB |= wPawnAttacks;
        m = wAttacksBB & pos.blackBB & ~pos.pieceTypeBB[Piece::BKING];
        int tmp = 0;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            tmp += pieceValue[pos.getPiece(sq)];
            m &= m-1;
        }
        score += tmp + tmp * tmp / qV;

        // Sum values for all white pieces under attack
        m = pos.pieceTypeBB[Piece::BKNIGHT];
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            bAttacksBB |= BitBoard::knightAttacks[sq];
            m &= m-1;
        }
        bAttacksBB &= (pos.pieceTypeBB[Piece::WKNIGHT] |
                       pos.pieceTypeBB[Piece::WBISHOP] |
                       pos.pieceTypeBB[Piece::WROOK] |
                       pos.pieceTypeBB[Piece::WQUEEN]);
        bAttacksBB |= bPawnAttacks;
        m = bAttacksBB & pos.whiteBB & ~pos.pieceTypeBB[Piece::WKING];
        tmp = 0;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            tmp += pieceValue[pos.getPiece(sq)];
            m &= m-1;
        }
        score -= tmp + tmp * tmp / qV;
        return score / 64;
    }

    /** Compute king safety for both kings. */
    int kingSafety(const Position& pos) {
        const int minM = rV + bV;
        const int m = (pos.wMtrl - pos.wMtrlPawns + pos.bMtrl - pos.bMtrlPawns) / 2;
        if (m <= minM)
            return 0;
        const int maxM = qV + 2 * rV + 2 * bV + 2 * nV;
        int score = kingSafetyKPPart(pos);
        if (Position::getY(pos.wKingSq) == 0) {
            if (((pos.pieceTypeBB[Piece::WKING] & 0x60L) != 0) && // King on f1 or g1
                ((pos.pieceTypeBB[Piece::WROOK] & 0xC0L) != 0) && // Rook on g1 or h1
                ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[6]) != 0) &&
                ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[7]) != 0)) {
                score -= 6 * 15;
            } else
            if (((pos.pieceTypeBB[Piece::WKING] & 0x6L) != 0) && // King on b1 or c1
                ((pos.pieceTypeBB[Piece::WROOK] & 0x3L) != 0) && // Rook on a1 or b1
                ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[0]) != 0) &&
                ((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskFile[1]) != 0)) {
                score -= 6 * 15;
            }
        }
        if (Position::getY(pos.bKingSq) == 7) {
            if (((pos.pieceTypeBB[Piece::BKING] & 0x6000000000000000L) != 0) && // King on f8 or g8
                ((pos.pieceTypeBB[Piece::BROOK] & 0xC000000000000000L) != 0) && // Rook on g8 or h8
                ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[6]) != 0) &&
                ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[7]) != 0)) {
                score += 6 * 15;
            } else
            if (((pos.pieceTypeBB[Piece::BKING] & 0x600000000000000L) != 0) && // King on b8 or c8
                ((pos.pieceTypeBB[Piece::BROOK] & 0x300000000000000L) != 0) && // Rook on a8 or b8
                ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[0]) != 0) &&
                ((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskFile[1]) != 0)) {
                score += 6 * 15;
            }
        }
        score += (bKingAttacks - wKingAttacks) * 4;
        const int kSafety = interpolate(m, minM, 0, maxM, score);
        return kSafety;
    }

    struct KingSafetyHashData {
        KingSafetyHashData() : key((U64)-1), score(0) { }
        U64 key;
        int score;
    };
    static std::vector<KingSafetyHashData> kingSafetyHash;

    int kingSafetyKPPart(const Position& pos) {
        // FIXME!!! Try non-linear king safety
        const U64 key = pos.pawnZobristHash() ^ pos.kingZobristHash();
        KingSafetyHashData ksh = kingSafetyHash[(int)key & (kingSafetyHash.size() - 1)];
        if (ksh.key != key) {
            int score = 0;
            U64 wPawns = pos.pieceTypeBB[Piece::WPAWN];
            U64 bPawns = pos.pieceTypeBB[Piece::BPAWN];
            {
                int safety = 0;
                int halfOpenFiles = 0;
                if (Position::getY(pos.wKingSq) < 2) {
                    U64 shelter = 1ULL << Position::getX(pos.wKingSq);
                    shelter |= ((shelter & BitBoard::maskBToHFiles) >> 1) |
                               ((shelter & BitBoard::maskAToGFiles) << 1);
                    shelter <<= 8;
                    safety += 3 * BitBoard::bitCount(wPawns & shelter);
                    safety -= 2 * BitBoard::bitCount(bPawns & (shelter | (shelter << 8)));
                    shelter <<= 8;
                    safety += 2 * BitBoard::bitCount(wPawns & shelter);
                    shelter <<= 8;
                    safety -= BitBoard::bitCount(bPawns & shelter);

                    U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                    if (wOpen != 0) {
                        halfOpenFiles += 25 * BitBoard::bitCount(wOpen & 0xe7);
                        halfOpenFiles += 10 * BitBoard::bitCount(wOpen & 0x18);
                    }
                    U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                    if (bOpen != 0) {
                        halfOpenFiles += 25 * BitBoard::bitCount(bOpen & 0xe7);
                        halfOpenFiles += 10 * BitBoard::bitCount(bOpen & 0x18);
                    }
                    safety = std::min(safety, 8);
                }
                const int kSafety = (safety - 9) * 15 - halfOpenFiles;
                score += kSafety;
            }
            {
                int safety = 0;
                int halfOpenFiles = 0;
                if (Position::getY(pos.bKingSq) >= 6) {
                    U64 shelter = 1ULL << (56 + Position::getX(pos.bKingSq));
                    shelter |= ((shelter & BitBoard::maskBToHFiles) >> 1) |
                               ((shelter & BitBoard::maskAToGFiles) << 1);
                    shelter >>= 8;
                    safety += 3 * BitBoard::bitCount(bPawns & shelter);
                    safety -= 2 * BitBoard::bitCount(wPawns & (shelter | (shelter >> 8)));
                    shelter >>= 8;
                    safety += 2 * BitBoard::bitCount(bPawns & shelter);
                    shelter >>= 8;
                    safety -= BitBoard::bitCount(wPawns & shelter);

                    U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                    if (wOpen != 0) {
                        halfOpenFiles += 25 * BitBoard::bitCount(wOpen & 0xe7);
                        halfOpenFiles += 10 * BitBoard::bitCount(wOpen & 0x18);
                    }
                    U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                    if (bOpen != 0) {
                        halfOpenFiles += 25 * BitBoard::bitCount(bOpen & 0xe7);
                        halfOpenFiles += 10 * BitBoard::bitCount(bOpen & 0x18);
                    }
                    safety = std::min(safety, 8);
                }
                const int kSafety = (safety - 9) * 15 - halfOpenFiles;
                score -= kSafety;
            }
            ksh.key = key;
            ksh.score = score;
        }
        return ksh.score;
    }

    /** Implements special knowledge for some endgame situations. */
    int endGameEval(const Position& pos, int oldScore) {
        int score = oldScore;
        if (pos.wMtrl + pos.bMtrl > 6 * rV)
            return score;
        const int wMtrlPawns = pos.wMtrlPawns;
        const int bMtrlPawns = pos.bMtrlPawns;
        const int wMtrlNoPawns = pos.wMtrl - wMtrlPawns;
        const int bMtrlNoPawns = pos.bMtrl - bMtrlPawns;

        bool handled = false;
        if ((wMtrlPawns + bMtrlPawns == 0) && (wMtrlNoPawns < rV) && (bMtrlNoPawns < rV)) {
            // King + minor piece vs king + minor piece is a draw
            return 0;
        }
        if (!handled && (pos.wMtrl == qV) && (pos.bMtrl == pV) && (pos.pieceTypeBB[Piece::WQUEEN] != 0)) {
            int wk = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WKING]);
            int wq = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WQUEEN]);
            int bk = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BKING]);
            int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BPAWN]);
            score = evalKQKP(wk, wq, bk, bp);
            handled = true;
        }
        if (!handled && (pos.wMtrl == rV) && (pos.pieceTypeBB[Piece::WROOK] != 0)) {
            if (pos.bMtrl == pV) {
                int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BPAWN]);
                score = krkpEval(pos.getKingSq(true), pos.getKingSq(false),
                        bp, pos.whiteMove);
                handled = true;
            } else if ((pos.bMtrl == bV) && (pos.pieceTypeBB[Piece::BBISHOP] != 0)) {
                score /= 8;
                const int kSq = pos.getKingSq(false);
                const int x = Position::getX(kSq);
                const int y = Position::getY(kSq);
                if ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq) != 0) {
                    score += (7 - distToH1A8[7-y][7-x]) * 7;
                } else {
                    score += (7 - distToH1A8[7-y][x]) * 7;
                }
                handled = true;
            }
        }
        if (!handled && (pos.bMtrl == qV) && (pos.wMtrl == pV) && (pos.pieceTypeBB[Piece::BQUEEN] != 0)) {
            int bk = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BKING]);
            int bq = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BQUEEN]);
            int wk = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WKING]);
            int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WPAWN]);
            score = -evalKQKP(63-bk, 63-bq, 63-wk, 63-wp);
            handled = true;
        }
        if (!handled && (pos.bMtrl == rV) && (pos.pieceTypeBB[Piece::BROOK] != 0)) {
            if (pos.wMtrl == pV) {
                int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WPAWN]);
                score = -krkpEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                        63-wp, !pos.whiteMove);
                handled = true;
            } else if ((pos.wMtrl == bV) && (pos.pieceTypeBB[Piece::WBISHOP] != 0)) {
                score /= 8;
                const int kSq = pos.getKingSq(true);
                const int x = Position::getX(kSq);
                const int y = Position::getY(kSq);
                if ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq) != 0) {
                    score -= (7 - distToH1A8[7-y][7-x]) * 7;
                } else {
                    score -= (7 - distToH1A8[7-y][x]) * 7;
                }
                handled = true;
            }
        }
        if (!handled && (score > 0)) {
            if ((wMtrlPawns == 0) && (wMtrlNoPawns <= bMtrlNoPawns + bV)) {
                if (wMtrlNoPawns < rV) {
                    return -pos.bMtrl / 50;
                } else {
                    score /= 8;         // Too little excess material, probably draw
                    handled = true;
                }
            } else if ((pos.pieceTypeBB[Piece::WROOK] | pos.pieceTypeBB[Piece::WKNIGHT] |
                        pos.pieceTypeBB[Piece::WQUEEN]) == 0) {
                // Check for rook pawn + wrong color bishop
                if (((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskBToHFiles) == 0) &&
                    ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskLightSq) == 0) &&
                    ((pos.pieceTypeBB[Piece::BKING] & 0x0303000000000000ULL) != 0)) {
                    return 0;
                } else
                if (((pos.pieceTypeBB[Piece::WPAWN] & BitBoard::maskAToGFiles) == 0) &&
                    ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq) == 0) &&
                    ((pos.pieceTypeBB[Piece::BKING] & 0xC0C0000000000000ULL) != 0)) {
                    return 0;
                }
            }
        }
        if (!handled) {
            if (bMtrlPawns == 0) {
                if (wMtrlNoPawns - bMtrlNoPawns > bV) {
                    int wKnights = BitBoard::bitCount(pos.pieceTypeBB[Piece::WKNIGHT]);
                    int wBishops = BitBoard::bitCount(pos.pieceTypeBB[Piece::WBISHOP]);
                    if ((wKnights == 2) && (wMtrlNoPawns == 2 * nV) && (bMtrlNoPawns == 0)) {
                        score /= 50;    // KNNK is a draw
                    } else if ((wKnights == 1) && (wBishops == 1) && (wMtrlNoPawns == nV + bV) && (bMtrlNoPawns == 0)) {
                        score /= 10;
                        score += nV + bV + 300;
                        const int kSq = pos.getKingSq(false);
                        const int x = Position::getX(kSq);
                        const int y = Position::getY(kSq);
                        if ((pos.pieceTypeBB[Piece::WBISHOP] & BitBoard::maskDarkSq) != 0) {
                            score += (7 - distToH1A8[7-y][7-x]) * 10;
                        } else {
                            score += (7 - distToH1A8[7-y][x]) * 10;
                        }
                    } else {
                        score += 300;       // Enough excess material, should win
                    }
                    handled = true;
                } else if ((wMtrlNoPawns + bMtrlNoPawns == 0) && (wMtrlPawns == pV)) { // KPK
                    int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::WPAWN]);
                    score = kpkEval(pos.getKingSq(true), pos.getKingSq(false),
                                    wp, pos.whiteMove);
                    handled = true;
                }
            }
        }
        if (!handled && (score < 0)) {
            if ((bMtrlPawns == 0) && (bMtrlNoPawns <= wMtrlNoPawns + bV)) {
                if (bMtrlNoPawns < rV) {
                    return pos.wMtrl / 50;
                } else {
                    score /= 8;         // Too little excess material, probably draw
                    handled = true;
                }
            } else if ((pos.pieceTypeBB[Piece::BROOK] | pos.pieceTypeBB[Piece::BKNIGHT] |
                        pos.pieceTypeBB[Piece::BQUEEN]) == 0) {
                // Check for rook pawn + wrong color bishop
                if (((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskBToHFiles) == 0) &&
                    ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq) == 0) &&
                    ((pos.pieceTypeBB[Piece::WKING] & 0x0303ULL) != 0)) {
                    return 0;
                } else
                if (((pos.pieceTypeBB[Piece::BPAWN] & BitBoard::maskAToGFiles) == 0) &&
                    ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskLightSq) == 0) &&
                    ((pos.pieceTypeBB[Piece::WKING] & 0xC0C0ULL) != 0)) {
                    return 0;
                }
            }
        }
        if (!handled) {
            if (wMtrlPawns == 0) {
                if (bMtrlNoPawns - wMtrlNoPawns > bV) {
                    int bKnights = BitBoard::bitCount(pos.pieceTypeBB[Piece::BKNIGHT]);
                    int bBishops = BitBoard::bitCount(pos.pieceTypeBB[Piece::BBISHOP]);
                    if ((bKnights == 2) && (bMtrlNoPawns == 2 * nV) && (wMtrlNoPawns == 0)) {
                        score /= 50;    // KNNK is a draw
                    } else if ((bKnights == 1) && (bBishops == 1) && (bMtrlNoPawns == nV + bV) && (wMtrlNoPawns == 0)) {
                        score /= 10;
                        score -= nV + bV + 300;
                        const int kSq = pos.getKingSq(true);
                        const int x = Position::getX(kSq);
                        const int y = Position::getY(kSq);
                        if ((pos.pieceTypeBB[Piece::BBISHOP] & BitBoard::maskDarkSq) != 0) {
                            score -= (7 - distToH1A8[7-y][7-x]) * 10;
                        } else {
                            score -= (7 - distToH1A8[7-y][x]) * 10;
                        }
                    } else {
                        score -= 300;       // Enough excess material, should win
                    }
                    handled = true;
                } else if ((wMtrlNoPawns + bMtrlNoPawns == 0) && (bMtrlPawns == pV)) { // KPK
                    int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB[Piece::BPAWN]);
                    score = -kpkEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                                     63-bp, !pos.whiteMove);
                    handled = true;
                }
            }
        }
        return score;

        // FIXME! Add evaluation of KRPKR   : eg 8/8/8/5pk1/1r6/R7/8/4K3 w - - 0 74
        // FIXME! KRBKR is very hard to draw
    }

    static int evalKQKP(int wKing, int wQueen, int bKing, int bPawn) {
        bool canWin = false;
        if (((1ULL << bKing) & 0xFFFF) == 0) {
            canWin = true; // King doesn't support pawn
        } else if (std::abs(Position::getX(bPawn) - Position::getX(bKing)) > 2) {
            canWin = true; // King doesn't support pawn
        } else {
            switch (bPawn) {
            case 8:  // a2
                canWin = ((1ULL << wKing) & 0x0F1F1F1F1FULL) != 0;
                break;
            case 10: // c2
                canWin = ((1ULL << wKing) & 0x071F1F1FULL) != 0;
                break;
            case 13: // f2
                canWin = ((1ULL << wKing) & 0xE0F8F8F8ULL) != 0;
                break;
            case 15: // h2
                canWin = ((1ULL << wKing) & 0xF0F8F8F8F8ULL) != 0;
                break;
            default:
                canWin = true;
                break;
            }
        }

        const int dist = std::max(std::abs(Position::getX(wKing)-Position::getX(bPawn)),
                                  std::abs(Position::getY(wKing)-Position::getY(bPawn)));
        int score = qV - pV - 20 * dist;
        if (!canWin)
            score /= 50;
        return score;
    }

    static int kpkEval(int wKing, int bKing, int wPawn, bool whiteMove) {
        if (Position::getX(wKing) >= 4) { // Mirror X
            wKing ^= 7;
            bKing ^= 7;
            wPawn ^= 7;
        }
        int index = whiteMove ? 0 : 1;
        index = index * 32 + Position::getY(wKing)*4+Position::getX(wKing);
        index = index * 64 + bKing;
        index = index * 48 + wPawn - 8;

        int bytePos = index / 8;
        int bitPos = index % 8;
        bool draw = (((int)kpkTable[bytePos]) & (1 << bitPos)) == 0;
        if (draw)
            return 0;
        return qV - pV / 4 * (7-Position::getY(wPawn));
    }

    static int krkpEval(int wKing, int bKing, int bPawn, bool whiteMove) {
        if (Position::getX(bKing) >= 4) { // Mirror X
            wKing ^= 7;
            bKing ^= 7;
            bPawn ^= 7;
        }
        int index = whiteMove ? 0 : 1;
        index = index * 32 + Position::getY(bKing)*4+Position::getX(bKing);
        index = index * 48 + bPawn - 8;
        index = index * 8 + Position::getY(wKing);
        byte mask = krkpTable[index];
        bool canWin = (mask & (1 << Position::getX(wKing))) != 0;

        int score = rV - pV + Position::getY(bPawn) * pV / 4;
        if (canWin)
            score += 150;
        else
            score /= 50;
        return score;
    }
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
