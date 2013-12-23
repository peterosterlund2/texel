/*
 * texelutil.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "position.hpp"
#include "textio.hpp"
#include "search.hpp"
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

bool getCommentScore(const std::string& comment, int& score) {
    double fScore;
    if (!str2Num(comment, fScore))
        return false;
    score = (int)std::round(fScore * 100);
    return true;
}

class ChessTool {
public:
    static bool main(const std::string& fname);
};

bool ChessTool::main(const std::string& fname) {
    static std::vector<U64> nullHist(200);
    static TranspositionTable tt(19);
    static ParallelData pd(tt);
    static KillerTable kt;
    static History ht;
    static auto et = Evaluate::getEvalHashTables();
    static Search::SearchTables st(tt, kt, ht, *et);
    static TreeLogger treeLog;

    Position pos;
    const int mate0 = SearchConst::MATE0;
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    const int plyScale = SearchConst::plyScale;

    std::ifstream is(fname);
    GameTree gt(is);
    while (gt.readPGN()) {
        GameTree::Result result = gt.getResult();
        if (result == GameTree::UNKNOWN)
            continue;
        double rScore = 0;
        switch (result) {
        case GameTree::WHITE_WIN: rScore = 1.0; break;
        case GameTree::BLACK_WIN: rScore = 0.0; break;
        case GameTree::DRAW:      rScore = 0.5; break;
        default: break;
        }
        GameNode gn = gt.getRootNode();
        while (true) {
            pos = gn.getPos();
            std::string fen = TextIO::toFEN(pos);
            if (gn.nChildren() == 0)
                break;
            gn.goForward(0);
//            std::string move = TextIO::moveToUCIString(gn.getMove());
            std::string comment = gn.getComment();
            int commentScore;
            if (!getCommentScore(comment, commentScore))
                continue;

            sc.init(pos, nullHist, 0);
            int score = sc.negaScout(-mate0, mate0, 0, 0*plyScale, -1, MoveGen::inCheck(pos));
            if (!pos.getWhiteMove()) {
                score = -score;
                commentScore = -commentScore;
            }

            std::cout << fen << " : " << rScore << " : " << commentScore << " : " << score << std::endl;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::string fname = "/home/petero/tmpfile.pgn";
    if (argc > 1)
        fname = argv[1];
    ChessTool::main(fname);
    return 0;
}
