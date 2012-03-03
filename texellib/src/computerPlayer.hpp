/*
 * computerPlayer.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef COMPUTERPLAYER_HPP_
#define COMPUTERPLAYER_HPP_

#include "player.hpp"
#include "transpositionTable.hpp"
#include "book.hpp"
#include "search.hpp"

#include <string>

/**
 * A computer algorithm player.
 */
class ComputerPlayer : public Player {
private:

    int minTimeMillis;
    int maxTimeMillis;
    int maxDepth;
    int maxNodes;
    TranspositionTable tt;
    Book book;
    bool bookEnabled;
    bool randomMode;
    Search* currentSearch;

    // Not implemented.
    ComputerPlayer(const ComputerPlayer& other);
    ComputerPlayer& operator=(const ComputerPlayer& other);

public:
    static std::string engineName;
    bool verbose;

    ComputerPlayer()
        : tt(15),
          book(verbose)
    {
        minTimeMillis = 10000;
        maxTimeMillis = 10000;
        maxDepth = 100;
        maxNodes = -1;
        verbose = true;
        bookEnabled = true;
        randomMode = false;
    }

#if 0
    void setTTLogSize(int logSize) {
        tt = TranspositionTable(logSize); // FIXME!! Resize
    }

    Search::Listener listener;
    public void setListener(Search::Listener listener) {
        this->listener = listener;
    }

    public std::string getCommand(const Position& pos, bool drawOffer, const std::vector<Position>& history) {
        // Create a search object
        U64[] posHashList = new U64[200 + history.size()];
        int posHashListSize = 0;
        for (size_t i = 0; i < history.size(); i++)
            posHashList[posHashListSize++] = history[i].zobristHash();
        tt.nextGeneration();
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
            Move bookMove = book.getBookMove(pos);
            if (!bookMove.isEmpty()) {
                printf("Book moves: %s\n", book.getAllBookMoves(pos));
                return TextIO::moveToString(pos, bookMove, false);
            }
        }

        // Find best move using iterative deepening
        currentSearch = &sc;
        sc.setListener(listener);
        Move bestM;
        if ((moves.size == 1) && (canClaimDraw(pos, posHashList, posHashListSize, moves.m[0]) == "")) {
            bestM = moves.m[0];
            bestM.score = 0;
        } else if (randomMode) {
            bestM = findSemiRandomMove(sc, moves);
        } else {
            sc.timeLimit(minTimeMillis, maxTimeMillis);
            bestM = sc.iterativeDeepening(moves, maxDepth, maxNodes, verbose);
        }
        currentSearch = NULL;
//        tt.printStats();
        std::string strMove = TextIO::moveToString(pos, bestM, false);

        // Claim draw if appropriate
        if (bestM.score <= 0) {
            std::string drawClaim = canClaimDraw(pos, posHashList, posHashListSize, bestM);
            if (drawClaim != "")
                strMove = drawClaim;
        }
        return strMove;
    }

    /** Check if a draw claim is allowed, possibly after playing "move".
     * @param move The move that may have to be made before claiming draw.
     * @return The draw string that claims the draw, or empty string if draw claim not valid.
     */
    private std::string canClaimDraw(const Position& pos, U64 posHashList[], int posHashListSize, Move move) {
        std::string drawStr = "";
        if (Search::canClaimDraw50(pos)) {
            drawStr = "draw 50";
        } else if (Search::canClaimDrawRep(pos, posHashList, posHashListSize, posHashListSize)) {
            drawStr = "draw rep";
        } else {
            std::string strMove = TextIO::moveToString(pos, move, false);
            posHashList[posHashListSize++] = pos.zobristHash();
            UndoInfo ui = new UndoInfo();
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

    public bool isHumanPlayer() {
        return false;
    }

    public void useBook(bool bookOn) {
        bookEnabled = bookOn;
    }

    public void timeLimit(int minTimeLimit, int maxTimeLimit, bool randomMode) {
        if (randomMode) {
            minTimeLimit = 0;
            maxTimeLimit = 0;
        }
        minTimeMillis = minTimeLimit;
        maxTimeMillis = maxTimeLimit;
        this->randomMode = randomMode;
        if (currentSearch != NULL)
            currentSearch.timeLimit(minTimeLimit, maxTimeLimit);
    }

    public void clearTT() {
        tt.clear();
    }

    /** Search a position and return the best move and score. Used for test suite processing. */
    public TwoReturnValues<Move, std::string> searchPosition(const Position& pos, int maxTimeMillis) {
        // Create a search object
        U64[] posHashList = new U64[200];
        tt.nextGeneration();
        Search sc = new Search(pos, posHashList, 0, tt);

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
        UndoInfo ui = new UndoInfo();
        pos.makeMove(bestM, ui);
        PV += tt.extractPV(pos);
        pos.unMakeMove(bestM, ui);

//        tt.printStats();

        // Return best move and PV
        return new TwoReturnValues<Move, std::string>(bestM, PV);
    }
#endif

    /** Initialize static data. */
    static void staticInitialize();

#if 0
private:
    private Move findSemiRandomMove(Search sc, MoveGen::MoveList moves) {
        sc.timeLimit(minTimeMillis, maxTimeMillis);
        Move bestM = sc.iterativeDeepening(moves, 1, maxNodes, verbose);
        int bestScore = bestM.score;

        Random rndGen = new SecureRandom();
        rndGen.setSeed(currentTimeMillis());

        int sum = 0;
        for (int mi = 0; mi < moves.size; mi++) {
            sum += moveProbWeight(moves.m[mi].score, bestScore);
        }
        int rnd = rndGen.nextInt(sum);
        for (int mi = 0; mi < moves.size; mi++) {
            int weight = moveProbWeight(moves.m[mi].score, bestScore);
            if (rnd < weight) {
                return moves.m[mi];
            }
            rnd -= weight;
        }
        assert(false);
        return null;
    }

    private final static int moveProbWeight(int moveScore, int bestScore) {
        double d = (bestScore - moveScore) / 100.0;
        double w = 100*Math.exp(-d*d/2);
        return (int)Math.ceil(w);
    }

    // FIXME!!! Test Botvinnik-Markoff extension
#endif
};


#endif /* COMPUTERPLAYER_HPP_ */
