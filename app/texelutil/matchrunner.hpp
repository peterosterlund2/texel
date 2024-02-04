/*
    Texel - A UCI chess engine.
    Copyright (C) 2024  Peter Österlund, peterosterlund2@gmail.com

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
 * matchrunner.hpp
 *
 *  Created on: Feb 5, 2024
 *      Author: petero
 */

#ifndef MATCHRUNNER_HPP_
#define MATCHRUNNER_HPP_

#include "gsprt.hpp"

#include <string>


/** Runs a match between two chess engines. */
class MatchRunner {
public:
    struct EngineParams {
        std::string name;
        std::string timeControl;
    };

    /** Constructor. */
    MatchRunner(int nWorkers, const EngineParams& engine1, const EngineParams& engine2);

    /** Run a fixed number of games. */
    void runFixedNumGames(int numGames, const std::string& script) const;

    /** Run games until GSPRT determines which engine is better. */
    void runGsprtGames(const Gsprt::InParams& gsprtParams, const std::string& script) const;

private:
    /** Run "script" once and retrieve the result.
     *  @return True if script succeeded. */
    bool runOneBatch(const std::string& script, int workerNo, int (&stats)[5]) const;

    const int nWorkers;   // Number of worker threads to use
    EngineParams engine1Pars;
    EngineParams engine2Pars;
};


#endif /* MATCHRUNNER_HPP_ */
