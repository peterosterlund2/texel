/*
 * texelutil.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "chesstool.hpp"

#include <iostream>
#include <fstream>
#include <string>

/** Read a whole file into a string. */
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

void usage() {
    std::cerr << "Usage: texelutil [-p2f] [-pawnadv]" << std::endl;
    std::cerr << " -p2f     : Convert from PGN to FEN" << std::endl;
    std::cerr << " -pawnadv : Compute evaluation error for different pawn advantage" << std::endl;
    ::exit(2);
}

int main(int argc, char* argv[]) {
    if (argc < 2)
        usage();

    std::string cmd = argv[1];
    if (cmd == "-p2f") {
        ChessTool::pgnToFen(std::cin);
    } else if (cmd == "-pawnadv") {
        ChessTool::pawnAdvTable(std::cin);
    } else {
        ScoreToProb sp(300.0);
        for (int i = -100; i <= 100; i++)
            std::cout << "i:" << i << " p:" << sp.getProb(i) << std::endl;
    }
    return 0;
}
