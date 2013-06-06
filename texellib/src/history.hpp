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
        init();
    }

    void init();

    void reScale();

    /** Record move as a success. */
    void addSuccess(const Position& pos, const Move& m, int depth) {
        int p = pos.getPiece(m.from());
        int cnt = depth;
        Entry& e = ht[p][m.to()];
        int val = e.countSuccess + cnt;
        if (val + e.countFail > 1300) {
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
        int val = e.countFail + cnt;
        if (val + e.countSuccess > 1300) {
            val /= 2;
            e.countSuccess /= 2;
        }
        e.countFail = val;
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
