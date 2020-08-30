/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * ctgbook.hpp
 *
 *  Created on: Jul 15, 2016
 *      Author: petero
 */

#ifndef CTGBOOK_HPP_
#define CTGBOOK_HPP_

#include "util/util.hpp"
#include "util/random.hpp"
#include "position.hpp"

#include <vector>
#include <fstream>


struct BookEntry;

class CtgBook {
public:
    /** Constructor. */
    CtgBook(const std::string& fileName, bool tournament, bool preferMain);

    /** Get a random book move for a position.
     * @return true if a book move was found, false otherwise. */
    bool getBookMove(Position& pos, Move& out);

private:
    void getBookEntries(const Position& pos, std::vector<BookEntry>& bookMoves);

    std::fstream ctgF;
    std::fstream ctbF;
    std::fstream ctoF;

    bool tournamentMode;
    bool preferMainLines;

    static Random rndGen;
};


#endif /* CTGBOOK_HPP_ */
