/*
 * enginecontrol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef ENGINECONTROL_HPP_
#define ENGINECONTROL_HPP_

#include "search.hpp"
#include "transpositionTable.hpp"
#include "moveGen.hpp"
#include "position.hpp"
#include "move.hpp"
#include "parameters.hpp"
#include "random.hpp"
#include "searchparams.hpp"
#include "computerPlayer.hpp"
#include "textio.hpp"

#include <vector>
#include <iostream>
#include <memory>

/**
 * Control the search thread.
 */
class EngineControl {
private:
    std::ostream& os;

#if 0
    Thread engineThread;
    Object threadMutex;
#endif
    std::shared_ptr<Search> sc;
    TranspositionTable tt;

    Position pos;
    std::vector<U64> posHashList;
    int posHashListSize;
    bool ponder;     // True if currently doing pondering
    bool onePossibleMove;
    bool infinite;

    int minTimeLimit;
    int maxTimeLimit;
    int maxDepth;
    int maxNodes;
    std::vector<Move> searchMoves;

    // Options
    int hashSizeMB;
    bool ownBook;
    bool analyseMode;
    bool ponderMode;

    // Reduced strength variables
    int strength;
    U64 randomSeed;

    /**
     * This class is responsible for sending "info" strings during search.
     */
    class SearchListener : public Search::Listener {
        std::ostream& os;

    public:
        SearchListener(std::ostream& os0) : os(os0) { }

        void notifyDepth(int depth) {
            os << "info depth " << depth << std::endl;
        }

        void notifyCurrMove(const Move& m, int moveNr) {
            os << "info currmove " << moveToString(m) << " currmovenumber " << moveNr << std::endl;
        }

        void notifyPV(int depth, int score, int time, long nodes, int nps, bool isMate,
                bool upperBound, bool lowerBound, const std::vector<Move>& pv) {
            std::string pvBuf;
            for (Move m : pv) {
                pvBuf += ' ';
                pvBuf += moveToString(m);
            }
            std::string bound;
            if (upperBound) {
                bound = " upperbound";
            } else if (lowerBound) {
                bound = " lowerbound";
            }
            os << "info depth " << depth << " score " << (isMate ? "mate " : "cp ")
               << score << bound << " time " << time << " nodes " << nodes
               << " nps " << nps << " pv" << pvBuf << std::endl;
        }

        void notifyStats(U64 nodes, int nps, int time) {
            os << "info nodes " << nodes << " nps " << nps << " time " << time << std::endl;
        }
    };

public:
    EngineControl(std::ostream& o)
        : os(o),
          tt(8),
          hashSizeMB(16),
          ownBook(false),
          analyseMode(false),
          ponderMode(true),
          strength(1000),
          randomSeed(0) {
        setupTT();
    }

    void startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
        setupPosition(pos, moves);
        computeTimeLimit(sPar);
        ponder = false;
        infinite = (maxTimeLimit < 0) && (maxDepth < 0) && (maxNodes < 0);
        searchMoves = sPar.searchMoves;
        startThread(minTimeLimit, maxTimeLimit, maxDepth, maxNodes);
    }

    void startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
        setupPosition(pos, moves);
        computeTimeLimit(sPar);
        ponder = true;
        infinite = false;
        startThread(-1, -1, -1, -1);
    }

    void ponderHit() {
        std::shared_ptr<Search> mySearch;
#if 0
        synchronized (threadMutex) {
#endif
            mySearch = sc;
#if 0
        }
#endif
        if (mySearch) {
            if (onePossibleMove) {
                if (minTimeLimit > 1) minTimeLimit = 1;
                if (maxTimeLimit > 1) maxTimeLimit = 1;
            }
            mySearch->timeLimit(minTimeLimit, maxTimeLimit);
        }
        infinite = (maxTimeLimit < 0) && (maxDepth < 0) && (maxNodes < 0);
        ponder = false;
    }

    void stopSearch() {
        stopThread();
    }

    void newGame() {
        randomSeed = Random().nextU64();
        tt.clear();
    }

    /**
     * Compute thinking time for current search.
     */
    void computeTimeLimit(const SearchParams& sPar) {
        minTimeLimit = -1;
        maxTimeLimit = -1;
        maxDepth = -1;
        maxNodes = -1;
        if (sPar.infinite) {
            minTimeLimit = -1;
            maxTimeLimit = -1;
            maxDepth = -1;
        } else if (sPar.depth > 0) {
            maxDepth = sPar.depth;
        } else if (sPar.mate > 0) {
            maxDepth = sPar.mate * 2 - 1;
        } else if (sPar.moveTime > 0) {
            minTimeLimit = maxTimeLimit = sPar.moveTime;
        } else if (sPar.nodes > 0) {
            maxNodes = sPar.nodes;
        } else {
            int moves = sPar.movesToGo;
            if (moves == 0) {
                moves = 999;
            }
            moves = std::min(moves, 45); // Assume 45 more moves until end of game
            if (ponderMode) {
                const double ponderHitRate = 0.35;
                moves = (int)ceil(moves * (1 - ponderHitRate));
            }
            bool white = pos.whiteMove;
            int time = white ? sPar.wTime : sPar.bTime;
            int inc  = white ? sPar.wInc : sPar.bInc;
            const int margin = std::min(1000, time * 9 / 10);
            int timeLimit = (time + inc * (moves - 1) - margin) / moves;
            minTimeLimit = (int)(timeLimit * 0.85);
            maxTimeLimit = (int)(minTimeLimit * (std::max(2.5, std::min(4.0, moves / 2.0))));

            // Leave at least 1s on the clock, but can't use negative time
            minTimeLimit = clamp(minTimeLimit, 1, time - margin);
            maxTimeLimit = clamp(maxTimeLimit, 1, time - margin);
        }
    }

private:
    static int clamp(int val, int min, int max) {
        if (val < min)
            return min;
        else if (val > max)
            return max;
        else
            return val;
    }

    void startThread(int minTimeLimit, int maxTimeLimit, int maxDepth, int maxNodes) {
#if 0
        synchronized (threadMutex) {} // Must not start new search until old search is finished
#endif
        sc.reset(new Search(pos, posHashList, posHashListSize, tt));
        sc->timeLimit(minTimeLimit, maxTimeLimit);
        sc->setListener(new SearchListener(os));
        sc->setStrength(strength, randomSeed);
        MoveGen::MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        if (searchMoves.size() > 0)
            moves.filter(searchMoves);
        MoveGen::MoveList srchMoves = moves;
        onePossibleMove = false;
        if ((srchMoves.size < 2) && !infinite) {
            onePossibleMove = true;
            if (!ponder)
                if ((maxDepth < 0) || (maxDepth > 2))
                    maxDepth = 2;
        }
        tt.nextGeneration();
#if 0
        const int srchmaxDepth = maxDepth;
        engineThread = new Thread(new Runnable() {
            public void run() {
                Move m;
                if (ownBook && !analyseMode) {
                    Book book(false);
                    m = book.getBookMove(pos);
                }
                if (m.isEmpty())
                    m = sc.iterativeDeepening(srchMoves, srchmaxDepth, maxNodes, false);
                while (ponder || infinite) {
                    // We should not respond until told to do so. Just wait until
                    // we are allowed to respond.
                    try {
                        Thread.sleep(10);
                    } catch (InterruptedException ex) {
                        break;
                    }
                }
                Move ponderMove = getPonderMove(pos, m);
                synchronized (threadMutex) {
                    os << "bestmove " << moveToString(m);
                    if (ponderMove != null)
                        os << " ponder " << moveToString(ponderMove);
                    os << std::endl;
                    engineThread = null;
                    sc = null;
                }
            }
        });
        engineThread.start();
#endif
    }

    void stopThread() {
#if 0
        std::shared_ptr<Thread> myThread;
        std::shared_ptr<Search> mySearch;
        synchronized (threadMutex) {
            myThread = engineThread;
            mySearch = sc;
        }
        if (myThread) {
            mySearch->timeLimit(0, 0);
            infinite = false;
            ponder = false;
            myThread->join();
        }
#endif
    }

    void setupTT() {
        int nEntries = hashSizeMB > 0 ? hashSizeMB * (1 << 20) / 24 : 1024;
        int logSize = (int) floor(log(nEntries) / log(2.0));
        tt.reSize(logSize);
    }

    void setupPosition(Position pos, const std::vector<Move>& moves) {
        UndoInfo ui;
        posHashList.resize(200 + moves.size());
        posHashListSize = 0;
        for (size_t i = 0; i < moves.size(); i++) {
            const Move& m = moves[i];
            posHashList[posHashListSize++] = pos.zobristHash();
            pos.makeMove(m, ui);
        }
        this->pos = pos;
    }

    /**
     * Try to find a move to ponder from the transposition table.
     */
    Move getPonderMove(Position pos, const Move& m) {
        if (m.isEmpty())
            return Move();
        Move ret;
        UndoInfo ui;
        pos.makeMove(m, ui);
        TranspositionTable::TTEntry ent;
        tt.probe(pos.historyHash(), ent);
        if (ent.type != TType::T_EMPTY) {
            ret = Move();
            ent.getMove(ret);
            MoveGen::MoveList moves;
            MoveGen::pseudoLegalMoves(pos, moves);
            MoveGen::removeIllegal(pos, moves);
            bool contains = false;
            for (int mi = 0; mi < moves.size; mi++)
                if (moves.m[mi].equals(ret)) {
                    contains = true;
                    break;
                }
            if  (!contains)
                ret = Move();
        }
        pos.unMakeMove(m, ui);
        return ret;
    }

    static std::string moveToString(const Move& m) {
        if (m.isEmpty())
            return "0000";
        std::string ret = TextIO::squareToString(m.from());
        ret += TextIO::squareToString(m.to());
        switch (m.promoteTo()) {
            case Piece::WQUEEN:
            case Piece::BQUEEN:
                ret += 'q';
                break;
            case Piece::WROOK:
            case Piece::BROOK:
                ret += 'r';
                break;
            case Piece::WBISHOP:
            case Piece::BBISHOP:
                ret += 'b';
                break;
            case Piece::WKNIGHT:
            case Piece::BKNIGHT:
                ret += 'n';
                break;
            default:
                break;
        }
        return ret;
    }

    static void printOptions(std::ostream& os) {
        os << "option name Hash type spin default 16 min 1 max 2048" << std::endl;
        os << "option name OwnBook type check default false" << std::endl;
        os << "option name Ponder type check default true" << std::endl;
        os << "option name UCI_AnalyseMode type check default false" << std::endl;
        os << "option name UCI_EngineAbout type string default " << ComputerPlayer::engineName
           << " by Peter Osterlund, see http://web.comhem.se/petero2home/javachess/index.html" << std::endl;
        os << "option name Strength type spin default 1000 min 0 max 1000" << std::endl;


        std::vector<std::string> parNames;
        Parameters::instance().getParamNames(parNames);
        for (size_t i = 0; i < parNames.size(); i++) {
            std::string pName = parNames[i];
            std::shared_ptr<Parameters::ParamBase> p = Parameters::instance().getParam(pName);
            switch (p->type) {
            case Parameters::CHECK: {
                const Parameters::CheckParam& cp = static_cast<const Parameters::CheckParam&>(*p.get());
                os << "optionn name " << cp.name << " type check default "
                   << (cp.defaultValue?"true":"false") << std::endl;
                break;
            }
            case Parameters::SPIN: {
                const Parameters::SpinParam& sp = static_cast<const Parameters::SpinParam&>(*p.get());
                os << "option name " << sp.name << " type spin default "
                   << sp.defaultValue << " min " << sp.minValue
                   << " max " << sp.maxValue << std::endl;
                break;
            }
            case Parameters::COMBO: {
                const Parameters::ComboParam& cp = static_cast<const Parameters::ComboParam&>(*p.get());
                os << "option name " << cp.name << " type combo default " << cp.defaultValue;
                for (size_t i = 0; i < cp.allowedValues.size(); i++)
                    os << " var " << cp.allowedValues[i];
                os << std::endl;
                break;
            }
            case Parameters::BUTTON:
                os << "option name " << p->name << " type button" << std::endl;
                break;
            case Parameters::STRING: {
                const Parameters::StringParam& sp = static_cast<const Parameters::StringParam&>(*p.get());
                os << "option name " << sp.name << " type string default "
                   << sp.defaultValue << std::endl;
                break;
            }
            }
        }
    }

    void setOption(const std::string& optionName, const std::string& optionValue) {
        if (optionName == "hash") {
            str2Num(optionValue, hashSizeMB);
            setupTT();
        } else if (optionName == "ownbook") {
            ownBook = (toLowerCase(optionValue) == "true");
        } else if (optionName == "ponder") {
            ponderMode = (toLowerCase(optionValue) == "true");
        } else if (optionName == "uci_analysemode") {
            analyseMode = (toLowerCase(optionValue) == "true");
        } else if (optionName == "strength") {
            str2Num(optionValue, strength);
        } else {
            Parameters::instance().set(optionName, optionValue);
        }
    }
};


#endif /* ENGINECONTROL_HPP_ */
