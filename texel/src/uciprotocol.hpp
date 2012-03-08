/*
 * uciprotocol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef UCIPROTOCOL_HPP_
#define UCIPROTOCOL_HPP_

#include "position.hpp"
#include "enginecontrol.hpp"

#include <vector>
#include <string>
#include <iosfwd>

/**
 * Handle the UCI protocol mode.
 */
class UCIProtocol {
private:
    // Data set by the "position" command.
    Position pos;
    std::vector<Move> moves;

    // Engine data
    std::shared_ptr<EngineControl> engine;

    // Set to true to break out of main loop
    bool quit;

public:
    static void main(bool autoStart);

    UCIProtocol();

    void mainLoop(std::istream& is, std::ostream& os, bool autoStart);

private:
    void handleCommand(const std::string& cmdLine, std::ostream& os);

    void initEngine(std::ostream& os);

    /** Convert a string to tokens by splitting at whitespace characters. */
    void tokenize(const std::string& cmdLine, std::vector<std::string>& tokens);
};


#endif /* UCIPROTOCOL_HPP_ */
