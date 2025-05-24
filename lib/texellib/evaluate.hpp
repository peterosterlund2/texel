/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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

#include "piece.hpp"
#include "position.hpp"
#include "nneval.hpp"

#if _MSC_VER
#include <xmmintrin.h>
#endif

class EvaluateTest;

/** Position evaluation routines. */
class Evaluate {
    friend class EvaluateTest;
private:
    struct MaterialHashData {
        MaterialHashData();
        int id;
        int score;
        U8 endGame;
    };

    struct EvalHashData {
        EvalHashData();
        U64 data;    // 0-15: Score, 16-63 hash key
    };

public:
    struct EvalHashTables {
        EvalHashTables();
        std::vector<MaterialHashData> materialHash;

        using EvalHashType = std::array<EvalHashData,(1<<16)>;
        EvalHashType evalHash;

        std::shared_ptr<NNEvaluator> nnEval;
    private:
        const NetData& initNetData();
    };

    /** Constructor. */
    explicit Evaluate(EvalHashTables& et);

    static int pieceValueOrder[Piece::nPieceTypes];

    /** Get evaluation hash tables. */
    static std::unique_ptr<EvalHashTables> getEvalHashTables();

    /** Prefetch hash table cache lines. */
    void prefetch(U64 key);

    /** Connect a position to this evaluation object. */
    void connectPosition(const Position& pos);

    /**
     * Static evaluation of a position.
     * The current position in the connected Position object is evaluated.
     * @return The evaluation score, measured in centipawns.
     *         Positive values are good for the side to make the next move.
     */
    int evalPos();
    int evalPosPrint();

    void setWhiteContempt(int contempt);
    int getWhiteContempt() const;

    /** Compute "swindle" score corresponding to an evaluation score when
     * the position is a known TB draw.
     * @param distToWin For draws that would be a win if the 50-move rule
     *                  did not exist, this is the number of extra plies
     *                  past halfMoveClock = 100 that are needed to win.
     */
    static int swindleScore(int evalScore, int distToWin);

    /**
     * Interpolate between (x1,y1) and (x2,y2).
     * If x < x1, return y1, if x > x2 return y2. Otherwise, use linear interpolation.
     */
    static int interpolate(int x, int x1, int y1, int x2, int y2);

    static const int IPOLMAX = 1024;

private:
    template <bool print> int evalPos();

    EvalHashData& getEvalHashEntry(U64 key);

    /** Get material score */
    int materialScore(bool print);

    /** Compute material score. */
    void computeMaterialScore(MaterialHashData& mhd, bool print) const;

    /** Holds the position to evaluate. */
    const Position* posP = nullptr;

    std::vector<MaterialHashData>& materialHash;
    const MaterialHashData* mhd;

    EvalHashTables::EvalHashType& evalHash;

    NNEvaluator& nnEval;

    int whiteContempt; // Assume white is this many centipawns stronger than black
};


inline
Evaluate::MaterialHashData::MaterialHashData()
    : id(-1), score(0) {
}

inline
Evaluate::EvalHashData::EvalHashData()
    : data(0xffffffffffff0000ULL) {
}

inline void
Evaluate::prefetch(U64 key) {
#ifdef USE_PREFETCH
#if _MSC_VER
    _mm_prefetch((const char*)&getEvalHashEntry(key), 3);
#else
    __builtin_prefetch(&getEvalHashEntry(key));
#endif
#endif
}

inline void
Evaluate::setWhiteContempt(int contempt) {
    whiteContempt = contempt;
}

inline int
Evaluate::getWhiteContempt() const {
    return whiteContempt;
}

inline int
Evaluate::interpolate(int x, int x1, int y1, int x2, int y2) {
    if (x > x2) {
        return y2;
    } else if (x < x1) {
        return y1;
    } else {
        return (x - x1) * (y2 - y1) / (x2 - x1) + y1;
    }
}

inline int
Evaluate::materialScore(bool print) {
    int mId = posP->materialId();
    int key = (mId >> 16) * 40507 + mId;
    MaterialHashData& newMhd = materialHash[key & (materialHash.size() - 1)];
    if ((newMhd.id != mId) || print)
        computeMaterialScore(newMhd, print);
    mhd = &newMhd;
    return newMhd.score;
}

inline Evaluate::EvalHashData&
Evaluate::getEvalHashEntry(U64 key) {
    int e0 = (int)key & (evalHash.size() - 1);
    return evalHash[e0];
}

#endif /* EVALUATE_HPP_ */
