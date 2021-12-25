/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
#include <map>

/** Finds proof games for a list of positions in a text file. */
class ProofGameFilter {
    friend class ProofGameTest;
public:
    /** Constructor. */
    ProofGameFilter();

    /** Read a list of FENs from a stream and classify them as legal/illegal/unknown
     *  with regards to reachability from the starting position. */
    void filterFens(std::istream& is, std::ostream& os);

private:
    /** Type of information that can be attached to a position in a line in a text file. */
    enum Info {
        ILLEGAL,       // Position known to be illegal
        UNKNOWN,       // Not known whether position is legal or illegal
        LEGAL,         // Position known to be legal

        FORCED,        // For illegal positions, list of forced initial captures

        KERNEL,        // For unknown positions, proof kernel
        EXT_KERNEL,    // For unknown positions, extended proof kernel
        PATH,          // For unknown positions, legal chess moves corresponding to extended kernel
        STATUS,        // For unknown positions, status of attempts to find proof game
        FAIL,          // For unknown positions, algorithm failed to find a PATH or PROOF

        PROOF,         // Legal chess moves (proof game) leading to the goal position
    };

    struct Line {
        std::string fen;
        std::map<Info, std::vector<std::string>> data;

        /** Return true if there is data for a given token type. */
        bool hasToken(Info tokType) const;

        /** Get data for a given token type. */
        std::vector<std::string>& tokenData(Info tokType);

        /** Print text representation to "os". */
        void print(std::ostream& os);
    };

    /** Determine if position is illegal, unknown or legal, based on existence
     *  of an extended proof kernel. Prints result to "os". */
    void computeExtProofKernel(const Position& startPos, Line& line, std::ostream& os);

    /** Compute a sequence of moves corresponding to an extended proof kernel.
     *  This computation can fail even if a solution exists. Prints result to "os". */
    void computePath(const Position& startPos, Line& line, std::ostream& os);

    /** Try to compute a proof game starting with a given path and ending in a
     *  goal position, both specified by "line". The STATUS data in "line" keeps
     *  track of how much effort has been spent so far on the computation.
     *  This method has three possible outcomes:
     *  1. LEGAL, PROOF    : A proof game is found.
     *  2. UNKNOWN, FAIL   : A proof game is not found and more computation will not help.
     *  3. UNKNOWN, STATUS : A proof game is not found but searching more might help.
     *  Prints result to "os". */
    void computeProofGame(const Position& startPos, Line& line, std::ostream& os);

    /** Read a line of text and store in the Line object. */
    bool readLine(std::istream& is, Line& line);

    /** Data for conversion between Info enum and string representation. */
    struct InfoData {
        ProofGameFilter::Info info;
        std::string tokenStr;
    };
    static std::vector<InfoData> infoStrData;
    static void init();

    /** Convert between string and Info enum value. */
    static Info str2Info(const std::string& infoStr);
    static std::string info2Str(Info info);
};

inline bool
ProofGameFilter::Line::hasToken(Info tokType) const {
    return data.find(tokType) != data.end();
}

inline std::vector<std::string>&
ProofGameFilter::Line::tokenData(Info tokType) {
    return data[tokType];
}


#endif /* PROOFGAMEFILTER_HPP_ */
