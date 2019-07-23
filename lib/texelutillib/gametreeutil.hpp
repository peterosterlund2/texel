/*
    Texel - A UCI chess engine.
    Copyright (C) 2019  Peter Österlund, peterosterlund2@gmail.com

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
 * gametreeutil.hpp
 *
 *  Created on: Jul 23, 2019
 *      Author: petero
 */

#ifndef GAMETREEUTIL_HPP_
#define GAMETREEUTIL_HPP_

#include "gametree.hpp"
#include <functional>

class GameTreeUtil {
public:
    /**
     * The PGN is traversed in depth first order. For each move,
     * func(const Position& parentPos, const GameNode& node) is called.
     */
    template <typename Func>
    static void iteratePgn(PgnReader& reader, Func func);
};

template <typename Func>
void GameTreeUtil::iteratePgn(PgnReader& reader, Func func) {
    GameTree gt;
    while (reader.readPGN(gt)) {
        GameNode gn = gt.getRootNode();
        std::function<void()> iterateTree = [&]() {
            Position parentPos = gn.getPos();
            for (int i = 0; i < gn.nChildren(); i++) {
                gn.goForward(i);
                func(parentPos, gn);
                iterateTree();
                gn.goBack();
            }
        };
        iterateTree();
    }
}

#endif /* GAMETREEUTIL_HPP_ */
