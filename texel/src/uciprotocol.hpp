/*
 * uciprotocol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef UCIPROTOCOL_HPP_
#define UCIPROTOCOL_HPP_

#if 0
/**
 * Handle the UCI protocol mode.
 * @author petero
 */
public class UCIProtocol {
    // Data set by the "position" command.
    Position pos;
    ArrayList<Move> moves;

    // Engine data
    EngineControl engine;

    // Set to true to break out of main loop
    boolean quit;


    public static void main(boolean autoStart) {
        UCIProtocol uciProt = new UCIProtocol();
        uciProt.mainLoop(System.in, System.out, autoStart);
    }

    public UCIProtocol() {
        pos = null;
        moves = new ArrayList<Move>();
        quit = false;
    }

    final public void mainLoop(InputStream is, PrintStream os, boolean autoStart) {
        try {
            if (autoStart) {
                handleCommand("uci", os);
            }
            InputStreamReader inStrRead = new InputStreamReader(is);
            BufferedReader inBuf = new BufferedReader(inStrRead);
            String line;
            while ((line = inBuf.readLine()) != null) {
                handleCommand(line, os);
                if (quit) {
                    break;
                }
            }
        } catch (IOException ex) {
            // If stream closed or other I/O error, terminate program
        }
    }

    final void handleCommand(String cmdLine, PrintStream os) {
        String[] tokens = tokenize(cmdLine);
        try {
            String cmd = tokens[0];
            if (cmd.equals("uci")) {
                os.printf("id name %s%n", ComputerPlayer.engineName);
                os.printf("id author Peter Osterlund%n");
                EngineControl.printOptions(os);
                os.printf("uciok%n");
            } else if (cmd.equals("isready")) {
                initEngine(os);
                os.printf("readyok%n");
            } else if (cmd.equals("setoption")) {
                initEngine(os);
                StringBuilder optionName = new StringBuilder();
                StringBuilder optionValue = new StringBuilder();
                if (tokens[1].endsWith("name")) {
                    int idx = 2;
                    while ((idx < tokens.length) && !tokens[idx].equals("value")) {
                        optionName.append(tokens[idx++].toLowerCase());
                        optionName.append(' ');
                    }
                    if ((idx < tokens.length) && tokens[idx++].equals("value")) {
                        while ((idx < tokens.length)) {
                            optionValue.append(tokens[idx++].toLowerCase());
                            optionValue.append(' ');
                        }
                    }
                    engine.setOption(optionName.toString().trim(), optionValue.toString().trim());
                }
            } else if (cmd.equals("ucinewgame")) {
                if (engine != null) {
                    engine.newGame();
                }
            } else if (cmd.equals("position")) {
                String fen = null;
                int idx = 1;
                if (tokens[idx].equals("startpos")) {
                    idx++;
                    fen = TextIO.startPosFEN;
                } else if (tokens[idx].equals("fen")) {
                    idx++;
                    StringBuilder sb = new StringBuilder();
                    while ((idx < tokens.length) && !tokens[idx].equals("moves")) {
                        sb.append(tokens[idx++]);
                        sb.append(' ');
                    }
                    fen = sb.toString().trim();
                }
                if (fen != null) {
                    pos = TextIO.readFEN(fen);
                    moves.clear();
                    if ((idx < tokens.length) && tokens[idx++].equals("moves")) {
                        for (int i = idx; i < tokens.length; i++) {
                            Move m = TextIO.uciStringToMove(tokens[i]);
                            if (m != null) {
                                moves.add(m);
                            } else {
                                break;
                            }
                        }
                    }
                }
            } else if (cmd.equals("go")) {
                if (pos == null) {
                    try {
                        pos = TextIO.readFEN(TextIO.startPosFEN);
                    } catch (ChessParseError ex) {
                        throw new RuntimeException();
                    }
                }
                initEngine(os);
                int idx = 1;
                SearchParams sPar = new SearchParams();
                boolean ponder = false;
                while (idx < tokens.length) {
                    String subCmd = tokens[idx++];
                    if (subCmd.equals("searchmoves")) {
                        while (idx < tokens.length) {
                            Move m = TextIO.uciStringToMove(tokens[idx]);
                            if (m != null) {
                                sPar.searchMoves.add(m);
                                idx++;
                            } else {
                                break;
                            }
                        }
                    } else if (subCmd.equals("ponder")) {
                        ponder = true;
                    } else if (subCmd.equals("wtime")) {
                        sPar.wTime = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("btime")) {
                        sPar.bTime = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("winc")) {
                        sPar.wInc = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("binc")) {
                        sPar.bInc = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("movestogo")) {
                        sPar.movesToGo = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("depth")) {
                        sPar.depth = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("nodes")) {
                        sPar.nodes = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("mate")) {
                        sPar.mate = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("movetime")) {
                        sPar.moveTime = Integer.parseInt(tokens[idx++]);
                    } else if (subCmd.equals("infinite")) {
                        sPar.infinite = true;
                    }
                }
                if (ponder) {
                    engine.startPonder(pos, moves, sPar);
                } else {
                    engine.startSearch(pos, moves, sPar);
                }
            } else if (cmd.equals("stop")) {
                engine.stopSearch();
            } else if (cmd.equals("ponderhit")) {
                engine.ponderHit();
            } else if (cmd.equals("quit")) {
                if (engine != null) {
                    engine.stopSearch();
                }
                quit = true;
            }
        } catch (ChessParseError ex) {
        } catch (ArrayIndexOutOfBoundsException e) {
        } catch (NumberFormatException nfe) {
        }
    }

    final private void initEngine(PrintStream os) {
        if (engine == null) {
            engine = new EngineControl(os);
        }
    }

    /** Convert a string to tokens by splitting at whitespace characters. */
    final String[] tokenize(String cmdLine) {
        cmdLine = cmdLine.trim();
        return cmdLine.split("\\s+");
    }
}
#endif


#endif /* UCIPROTOCOL_HPP_ */
