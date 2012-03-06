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
#include "searchparams.hpp"

#include <vector>
#include <iostream>
#include <string>

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
    static void main(bool autoStart) {
        UCIProtocol uciProt;
        uciProt.mainLoop(std::cin, std::cout, autoStart);
    }
    UCIProtocol()
        : quit(false)
    { }

    void mainLoop(std::istream& is, std::ostream& os, bool autoStart) {
        if (autoStart)
            handleCommand("uci", os);
        std::string line;
        while (getline(is, line) >= 0) {
            handleCommand(line, os);
            if (quit)
                break;
        }
    }

private:
    void handleCommand(const std::string& cmdLine, std::ostream& os) {
#if 0
        std::vector<std::string> tokens;
        tokenize(cmdLine, tokens);
        try {
            std::string cmd = tokens[0];
            if (cmd == "uci") {
                os << "id name " << ComputerPlayer::engineName << std::endl;
                os << "id author Peter Osterlund" << std::endl;
                EngineControl::printOptions(os);
                os << "uciok" << std::endl;
            } else if (cmd == "isready") {
                initEngine(os);
                os << "readyok" << std::endl;
            } else if (cmd == "setoption") {
                initEngine(os);
                std::string optionName;
                std::string optionValue;
                if (tokens[1] == "name") {
                    int idx = 2;
                    while ((idx < (int)tokens.size()) && (tokens[idx] != "value")) {
                        optionName += toLowerCase(tokens[idx++]);
                        optionName += ' ';
                    }
                    if ((idx < (int)tokens.size()) && (tokens[idx++] == "value")) {
                        while ((idx < (int)tokens.size())) {
                            optionValue += toLowerCase(tokens[idx++]);
                            optionValue += ' ';
                        }
                    }
                    engine.setOption(optionName.trim(), optionValue.trim());
                }
            } else if (cmd == "ucinewgame") {
                if (engine)
                    engine.newGame();
            } else if (cmd ==  "position") {
                std::string fen;
                int idx = 1;
                if (tokens[idx] == "startpos") {
                    idx++;
                    fen = TextIO::startPosFEN;
                } else if (tokens[idx] == "fen") {
                    idx++;
                    std::string sb;
                    while ((idx < (int)tokens.size()) && (tokens[idx] != "moves")) {
                        sb += tokens[idx++];
                        sb += ' ';
                    }
                    fen = sb.trim();
                }
                if (fen.length() > 0) {
                    pos = TextIO::readFEN(fen);
                    moves.clear();
                    if ((idx < (int)tokens.size()) && (tokens[idx++] == "moves")) {
                        for (size_t i = idx; i < tokens.size(); i++) {
                            Move m = TextIO::uciStringToMove(tokens[i]);
                            if (m.isEmpty())
                                break;
                            moves.push_back(m);
                        }
                    }
                }
            } else if (cmd == "go") {
                if (pos == null)
                    pos = TextIO::readFEN(TextIO::startPosFEN);
                initEngine(os);
                int idx = 1;
                SearchParams sPar;
                bool ponder = false;
                while (idx < (int)tokens.size()) {
                    std::string subCmd = tokens[idx++];
                    if (subCmd == "searchmoves") {
                        while (idx < (int)tokens.size()) {
                            Move m = TextIO::uciStringToMove(tokens[idx]);
                            if (m.isEmpty())
                                break;
                            sPar.searchMoves.push_back(m);
                            idx++;
                        }
                    } else if (subCmd == "ponder") {
                        ponder = true;
                    } else if (subCmd == "wtime") {
                        str2Num(tokens[idx++], sPar.wTime);
                    } else if (subCmd == "btime") {
                        str2Num(tokens[idx++], sPar.bTime);
                    } else if (subCmd == "winc") {
                        str2Num(tokens[idx++], sPar.wInc);
                    } else if (subCmd == "binc") {
                        str2Num(tokens[idx++], sPar.bInc);
                    } else if (subCmd == "movestogo") {
                        str2Num(tokens[idx++], sPar.movesToGo);
                    } else if (subCmd == "depth") {
                        str2Num(tokens[idx++], sPar.depth);
                    } else if (subCmd == "nodes") {
                        str2Num(tokens[idx++], sPar.nodes);
                    } else if (subCmd == "mate") {
                        str2Num(tokens[idx++], sPar.mate);
                    } else if (subCmd == "movetime") {
                        str2Num(tokens[idx++], sPar.moveTime);
                    } else if (subCmd == "infinite") {
                        sPar.infinite = true;
                    }
                }
                if (ponder) {
                    engine.startPonder(pos, moves, sPar);
                } else {
                    engine.startSearch(pos, moves, sPar);
                }
            } else if (cmd == "stop") {
                engine.stopSearch();
            } else if (cmd == "ponderhit") {
                engine.ponderHit();
            } else if (cmd == "quit") {
                if (engine)
                    engine.stopSearch();
                quit = true;
            }
        } catch (const ChessParseError& ex) {
        } catch (ArrayIndexOutOfBoundsException e) {
        }
#endif
    }

    void initEngine(std::ostream& os) {
#if 0
        if (engine)
            engine.reset(new EngineControl(os));
#endif
    }

#if 0
    /** Convert a string to tokens by splitting at whitespace characters. */
    void tokenize(const std::string& cmdLine, std::vector<std::string>& tokens) {
        cmdLine = cmdLine.trim();
        return cmdLine.split("\\s+");
    }
#endif
};


#endif /* UCIPROTOCOL_HPP_ */
