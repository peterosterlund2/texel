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
    struct Entry {
        int countSuccess;
        int countFail;
        mutable int score;
    };
    struct Entry ht[Piece::nPieceTypes][64];

public:
    History() {
        for (int p = 0; p < Piece::nPieceTypes; p++) {
            for (int sq = 0; sq < 64; sq++) {
                Entry& e = ht[p][sq];
                e.countSuccess = 0;
                e.countFail = 0;
                e.score = -1;
            }
        }
    }

    /** Record move as a success. */
    void addSuccess(const Position& pos, const Move& m, int depth) {
        int p = pos.getPiece(m.from());
        int cnt = depth;
        Entry& e = ht[p][m.to()];
        int val = e.countSuccess + cnt;
        if (val > 1000) {
            val /= 2;
            e.countFail /= 2;
        }
        e.countSuccess = val;
        e.score = -1;
    }

    /** Record move as a failure. */
    void addFail(const Position& pos, const Move& m, int depth) {
        int p = pos.getPiece(m.from());
        int cnt = depth;
        Entry& e = ht[p][m.to()];
        e.countFail += cnt;
        e.score = -1;
    }

    /** Get a score between 0 and 49, depending of the success/fail ratio of the move. */
    int getHistScore(const Position& pos, const Move& m) const {
        int p = pos.getPiece(m.from());
        const Entry& e = ht[p][m.to()];
        int ret = e.score;
        if (ret >= 0)
            return ret;
        int succ = e.countSuccess;
        int fail = e.countFail;
        if (succ + fail > 0) {
            ret = succ * 49 / (succ + fail);
        } else
            ret = 0;
        e.score = ret;
        return ret;
    }
};

#endif /* HISTORY_HPP_ */
