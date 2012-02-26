/*
 * history.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef HISTORY_HPP_
#define HISTORY_HPP_

#include "piece.hpp"
#include "position.hpp"

/**
 * Implements the relative history heuristic.
 */
class History {
private:
    int countSuccess[Piece::nPieceTypes][64];
    int countFail[Piece::nPieceTypes][64];
    mutable int score[Piece::nPieceTypes][64];

public:
    History() {
        for (int p = 0; p < Piece::nPieceTypes; p++) {
            for (int sq = 0; sq < 64; sq++) {
                countSuccess[p][sq] = 0;
                countFail[p][sq] = 0;
                score[p][sq] = -1;
            }
        }
    }

    /** Record move as a success. */
    void addSuccess(const Position& pos, const Move& m, int depth) {
        int p = pos.getPiece(m.from());
        int cnt = depth;
        int val = countSuccess[p][m.to()] + cnt;
        if (val > 1000) {
            val /= 2;
            countFail[p][m.to()] /= 2;
        }
        countSuccess[p][m.to()] = val;
        score[p][m.to()] = -1;
    }

    /** Record move as a failure. */
    void addFail(const Position& pos, const Move& m, int depth) {
        int p = pos.getPiece(m.from());
        int cnt = depth;
        countFail[p][m.to()] += cnt;
        score[p][m.to()] = -1;
    }

    /** Get a score between 0 and 49, depending of the success/fail ratio of the move. */
    int getHistScore(const Position& pos, const Move& m) const {
        int p = pos.getPiece(m.from());
        int ret = score[p][m.to()];
        if (ret >= 0)
            return ret;
        int succ = countSuccess[p][m.to()];
        int fail = countFail[p][m.to()];
        if (succ + fail > 0) {
            ret = succ * 49 / (succ + fail);
        } else
            ret = 0;
        score[p][m.to()] = ret;
        return ret;
    }
};

#endif /* HISTORY_HPP_ */
