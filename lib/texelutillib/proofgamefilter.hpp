/*
    Texel - A UCI chess engine.
    Copyright (C) 2015-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * proofgamefilter.hpp
 *
 *  Created on: Dec 24, 2021
 *      Author: petero
 */

#ifndef PROOFGAMEFILTER_HPP_
#define PROOFGAMEFILTER_HPP_

#include "util/util.hpp"
#include "position.hpp"
#include "assignment.hpp"

#include <string>
#include <vector>

/** Finds proof games for a list of positions in a text file. */
class ProofGameFilter {
    friend class ProofGameTest;
public:
    /** Constructor. */
    ProofGameFilter();

    /** Read a list of FENs from a stream and classify them as legal/illegal/unknown
     *  with regards to reachability from the starting position. */
    void filterFens(std::istream& is, std::ostream& os);
};

#endif /* PROOFGAMEFILTER_HPP_ */
