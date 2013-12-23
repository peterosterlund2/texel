/*
 * texelutil.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "position.hpp"
#include "textio.hpp"
#include "gametree.hpp"

#include <iostream>
#include <fstream>
#include <string>

std::string readFile(const std::string& fname) {
    std::ifstream is(fname);
    std::string data;
    while (true) {
        std::string line;
        std::getline(is, line);
        if (!is || is.eof())
            break;
        data += line;
        data += '\n';
    }
    return data;
}

int main(int argc, char* argv[]) {
    std::string fname = "/home/petero/tmpfile.pgn";
    std::string pgnData = readFile(fname);

    GameTree gt;
    if (!gt.readPGN(pgnData))
        return false;

    GameNode gn = gt.getRootNode();
    while (true) {
        std::string fen = TextIO::toFEN(gn.getPos());
        if (gn.nChildren() == 0)
            break;
        gn.goForward(0);
        std::string move = TextIO::moveToUCIString(gn.getMove());
        std::string comment = gn.getComment();
        std::cout << fen << " : " << move << " : " << comment << std::endl;
    }
    return 0;
}
