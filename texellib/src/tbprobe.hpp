/*
    Texel - A UCI chess engine.
    Copyright (C) 2014  Peter Österlund, peterosterlund2@gmail.com

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
 * tbprobe.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: petero
 */

#ifndef TBPROBE_HPP_
#define TBPROBE_HPP_

#include "transpositionTable.hpp"

#include <string>


class Position;

/**
 * Handle tablebase probing.
 */
class TBProbe {
public:
    /** Initialize external libraries. */
    static void initialize(const std::string& path, int cacheMB);

    /** Probe one or more tablebases to get an exact score or a usable bound. */
    static bool tbProbe(const Position& pos, int ply, int alpha, int beta,
                        TranspositionTable::TTEntry& ent);

    /** Probe gaviota DTM tablebases.
     * @param pos  The position to probe.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The table base score. Only modified for tablebase hits.
     * @return True if pos was found in the tablebases.
     */
    static bool gtbProbeDTM(const Position& pos, int ply, int& score);

    /**
     * Probe gaviota WDL tablebases.
     * @param pos  The position to probe.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The table base score. Only modified for tablebase hits.
     */
    static bool gtbProbeWDL(const Position& pos, int ply, int& score);

private:
    /** Initialize */
    static void gtbInitialize(const std::string& path, int cacheMB);

    struct GtbProbeData {
        unsigned int stm, epsq, castles;
        static const int MAXLEN = 17;
        unsigned int  wSq[MAXLEN];
        unsigned int  bSq[MAXLEN];
        unsigned char wP[MAXLEN];
        unsigned char bP[MAXLEN];
    };

    /** Convert position to GTB probe format. */
    static void getGTBProbeData(const Position& pos, GtbProbeData& gtbData);

    static bool gtbProbeDTM(const GtbProbeData& gtbData, int ply, int& score);

    static bool gtbProbeWDL(const GtbProbeData& gtbData, int ply, int& score);
};


#endif /* TBPROBE_HPP_ */
