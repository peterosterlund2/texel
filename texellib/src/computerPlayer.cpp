/*
 * computerPlayer.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "computerPlayer.hpp"
#include "textio.hpp"

#include <iostream>

std::string ComputerPlayer::engineName;


static StaticInitializer<ComputerPlayer> cpInit;

void
ComputerPlayer::staticInitialize() {
    std::string name = "Texel 1.00";
    if (false)
        name += " 32-bit";
    if (false)
        name += " 64-bit";
    engineName = name;
}


std::string
ComputerPlayer::getCommand(const Position& posIn, bool drawOffer, const std::vector<Position>& history) {
    // Create a search object
    std::vector<U64> posHashList(200 + history.size());
    int posHashListSize = 0;
    for (size_t i = 0; i < history.size(); i++)
        posHashList[posHashListSize++] = history[i].zobristHash();
    tt.nextGeneration();
    Position pos(posIn);
    Search sc(pos, posHashList, posHashListSize, tt);

    // Determine all legal moves
    MoveGen::MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    sc.scoreMoveList(moves, 0);

    // Test for "game over"
    if (moves.size == 0) {
        // Switch sides so that the human can decide what to do next.
        return "swap";
    }

    if (bookEnabled) {
        Move bookMove;
        book.getBookMove(pos, bookMove);
        if (!bookMove.isEmpty()) {
            std::cout << "Book moves: " << book.getAllBookMoves(pos) << std::endl;
            return TextIO::moveToString(pos, bookMove, false);
        }
    }

    // Find best move using iterative deepening
    currentSearch = &sc;
    sc.setListener(listener);
    Move bestM;
    if ((moves.size == 1) && (canClaimDraw(pos, posHashList, posHashListSize, moves.m[0]) == "")) {
        bestM = moves.m[0];
        bestM.setScore(0);
    } else {
        sc.timeLimit(minTimeMillis, maxTimeMillis);
        bestM = sc.iterativeDeepening(moves, maxDepth, maxNodes, verbose);
    }
    currentSearch = NULL;
    //        tt.printStats();
    std::string strMove = TextIO::moveToString(pos, bestM, false);

    // Claim draw if appropriate
    if (bestM.score() <= 0) {
        std::string drawClaim = canClaimDraw(pos, posHashList, posHashListSize, bestM);
        if (drawClaim != "")
            strMove = drawClaim;
    }
    return strMove;
}

std::string
ComputerPlayer::canClaimDraw(Position& pos, std::vector<U64>& posHashList,
                             int posHashListSize, const Move& move) {
    std::string drawStr;
    if (Search::canClaimDraw50(pos)) {
        drawStr = "draw 50";
    } else if (Search::canClaimDrawRep(pos, posHashList, posHashListSize, posHashListSize)) {
        drawStr = "draw rep";
    } else {
        std::string strMove = TextIO::moveToString(pos, move, false);
        posHashList[posHashListSize++] = pos.zobristHash();
        UndoInfo ui;
        pos.makeMove(move, ui);
        if (Search::canClaimDraw50(pos)) {
            drawStr = "draw 50 " + strMove;
        } else if (Search::canClaimDrawRep(pos, posHashList, posHashListSize, posHashListSize)) {
            drawStr = "draw rep " + strMove;
        }
        pos.unMakeMove(move, ui);
    }
    return drawStr;
}

std::pair<Move, std::string>
ComputerPlayer::searchPosition(Position& pos, int maxTimeMillis) {
    // Create a search object
    std::vector<U64> posHashList(200);
    tt.nextGeneration();
    Search sc(pos, posHashList, 0, tt);

    // Determine all legal moves
    MoveGen::MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    sc.scoreMoveList(moves, 0);

    // Find best move using iterative deepening
    sc.timeLimit(maxTimeMillis, maxTimeMillis);
    Move bestM = sc.iterativeDeepening(moves, -1, -1, false);

    // Extract PV
    std::string PV = TextIO::moveToString(pos, bestM, false) + " ";
    UndoInfo ui;
    pos.makeMove(bestM, ui);
    PV += tt.extractPV(pos);
    pos.unMakeMove(bestM, ui);

//    tt.printStats();

    // Return best move and PV
    return std::pair<Move, std::string>(bestM, PV);
}
