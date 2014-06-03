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

#include <string>


class Position;

/**
 * Handle tablebase probing.
 */
class TBProbe {
public:
    /** Initialize external libraries. */
    static void initialize(const std::string& path);

    /** Probe gaviota tablebases.
     * @param pos  The position to probe.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The table base score. Only modified for tablebase hits.
     * @return True if pos was found in the tablebases.
     */
    static bool gtbProbe(const Position& pos, int ply, int& score);
};


#endif /* TBPROBE_HPP_ */
