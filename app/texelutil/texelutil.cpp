/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * texelutil.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "chesstool.hpp"
#include "posgen.hpp"
#include "spsa.hpp"
#include "gsprt.hpp"
#include "matchrunner.hpp"
#include "bookbuild.hpp"
#include "proofgame.hpp"
#include "proofgamefilter.hpp"
#include "revmovegen.hpp"
#include "matchbookcreator.hpp"
#include "tbgen.hpp"
#include "parameters.hpp"
#include "chessError.hpp"
#include "computerPlayer.hpp"
#include "gametree.hpp"
#include "textio.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>

using namespace std::string_literals;

void
parseParValues(const std::string& fname, std::vector<ParamValue>& parValues) {
    Parameters& uciPars = Parameters::instance();
    std::vector<std::string> lines = ChessTool::readFile(fname);
    for (const std::string& line : lines) {
        std::vector<std::string> fields;
        splitString(line, fields);
        if (fields.size() < 2)
            throw ChessParseError("Invalid parameter specification:" + line);
        if (!uciPars.getParam(fields[0]))
            throw ChessParseError("No such parameter:" + fields[0]);
        double value;
        if (str2Num(fields[1], value)) {
            ParamValue pv;
            pv.name = fields[0];
            pv.value = (int)floor(value + 0.5);
            parValues.push_back(pv);
        }
    }
}

void
setInitialValues(const std::string& fname) {
    Parameters& uciPars = Parameters::instance();
    std::vector<ParamValue> parValues;
    parseParValues(fname, parValues);
    for (const ParamValue& pv : parValues)
        uciPars.set(pv.name, num2Str(pv.value));
}

void
usage() {
    std::cerr << "Usage: texelutil [-iv file] [-e] [-moveorder] cmd params\n";
    std::cerr << " -iv file : Set initial parameter values\n";
    std::cerr << " -j n : Use n worker threads\n";
    std::cerr << " -e : Use cross entropy error function\n";
    std::cerr << " -s : Use search score instead of game result\n";
    std::cerr << " -moveorder : Optimize static move ordering\n";
    std::cerr << "cmd is one of:\n";
    std::cerr << "\n";
    std::cerr << " p2f [n [us]] : Convert from PGN to FEN, using each position with probability\n";
    std::cerr << "                1/n. If us is 1, also include UnScored moves\n";
    std::cerr << " f2p      : Convert from FEN to PGN\n";
    std::cerr << " m2f      : For each line, convert sequence of moves to fen\n";
    std::cerr << " filter type pars : Keep positions that satisfy a condition\n";
    std::cerr << "        score scLimit prLimit : qScore and search score differ less than limits\n";
    std::cerr << "        mtrldiff [-m] dQ dR dB [dN] dP : material difference satisfies pattern\n";
    std::cerr << "                                     -m treat bishop and knight as same type\n";
    std::cerr << "        mtrl [-m] wQ wR wB [wN] wP bQ bR bB [bN] bP : material satisfies pattern\n";
    std::cerr << "                                     -m treat bishop and knight as same type\n";
    std::cerr << "\n";
#if !_MSC_VER
    std::cerr << " search script : Update search score in FEN file by running script on all lines\n";
#endif
    std::cerr << " qsearch : Update positions in FEN file to position at end of q-search\n";
    std::cerr << " searchfens time inc : Search all positions in FEN file\n";
    std::cerr << " fen2bin [-useResult] [-noincheck] [-prlimit lim] outFile\n";
    std::cerr << "                     : Convert FEN+score data to binary format\n";
    std::cerr << "\n";
    std::cerr << " outliers threshold  : Print positions with unexpected game result\n";
    std::cerr << " evaleffect evalfile : Print eval improvement when parameters are changed\n";
    std::cerr << " pawnadv  : Compute evaluation error for different pawn advantage\n";
    std::cerr << " score2prob : Compute table of expected score as function of centipawns\n";
    std::cerr << " parrange p a b c    : Compare evaluation error for different parameter values\n";
#ifdef USE_ARMADILLO
    std::cerr << " gnopt p1 p2 ...     : Optimize parameters using Gauss-Newton method\n";
#endif
    std::cerr << " localopt p1 p2 ...  : Optimize parameters using local search\n";
    std::cerr << " localopt2 p1 p2 ... : Optimize parameters using local search with big jumps\n";
    std::cerr << " printpar : Print evaluation tables and parameters\n";
    std::cerr << " patchpar srcdir : Update parameter values in parameters.[ch]pp\n";
    std::cerr << " evalstat p1 p2 ...  : Print parameter statistics\n";
    std::cerr << " residual xType inclNo : Print evaluation error as function of material\n";
    std::cerr << "                         xType is mtrlsum, mtrldiff, pawnsum, pawndiff or eval\n";
    std::cerr << "                         inclNo is 0/1 to exclude/include position/game numbers\n";
    std::cerr << " simplify z1 z2 ... : p1 p2 ... : Set zi to zero, adjust pi to approximate\n";
    std::cerr << "                                  original evaluation\n";
    std::cerr << "\n";
    std::cerr << " genfen qvsn : Generate all positions of a given type\n";
    std::cerr << " rndfen n [seed] : Generate about n random legal (and some illegal) positions\n";
    std::cerr << " rndtest [seed1 [seed2]] : Write random binary data to standard output\n";
    std::cerr << "\n";
    std::cerr << " tblist nPieces : Print all tablebase types\n";
    std::cerr << " dtmstat type1 [type2 ...] : Generate tablebase DTM statistics\n";
    std::cerr << " dtzstat type1 [type2 ...] : Generate tablebase DTZ statistics\n";
    std::cerr << " egstat type pieceType1 [pieceType2 ...] : Endgame WDL statistics\n";
    std::cerr << " wdltest type1 [type2 ...] : Compare RTB and GTB WDL tables\n";
    std::cerr << " dtztest type1 [type2 ...] : Compare RTB DTZ and GTB DTM tables\n";
    std::cerr << " dtz fen                   : Retrieve DTZ value for a position\n";
    std::cerr << " wdldump type1 [type2 ...] : Dump RTB WDL data to out.bin\n";
    std::cerr << "\n";
#ifdef USE_GSL
    std::cerr << " gamesim meanResult drawProb nGames nSimul : Simulate game results\n";
    std::cerr << " enginesim nGames p1 p2 ... : Simulate engine with parameters p1, p2, ...\n";
    std::cerr << " tourneysim nSimul nRounds elo1 elo2 ... : Simulate tournament\n";
    std::cerr << " spsasim nSimul nIter gamesPerIter a c param1 ... : Simulate SPSA optimization\n";
    std::cerr << " spsa spsafile.conf : Run SPSA optimization using the given configuration file\n";
    std::cerr << "\n";
#endif
    std::cerr << " tbgen wq wr wb wn bq br bb bn : Generate pawn-less tablebase in memory\n";
    std::cerr << " tbgentest type1 [type2 ...]   : Compare pawnless tablebase against GTB\n";
    std::cerr << "\n";
    std::cerr << " book improve bookFile searchTime nThreads \"startmoves\" [c1 c2 c3]\n";
    std::cerr << "                                            : Improve opening book\n";
    std::cerr << " book import bookFile pgnFile [maxPly]      : Import moves from PGN file\n";
    std::cerr << " book export bookFile polyglotFile maxErrSelf errOtherExpConst \\\n";
    std::cerr << "             [noleaf] [-e excludeFile.pgn]\n";
    std::cerr << "                                            : Export as polyglot book\n";
    std::cerr << " book query bookFile maxErrSelf errOtherExpConst : Interactive query mode\n";
    std::cerr << " book stats bookFile                        : Print book statistics\n";
    std::cerr << "\n";
    std::cerr << " creatematchbook depth searchTime : Analyze  positions in perft(depth)\n";
    std::cerr << " countuniq pgnFile : Count number of unique positions as function of depth\n";
    std::cerr << " pgnstat pgnFile [-p] : Print statistics for games in a PGN file\n";
    std::cerr << "           -p : Consider game pairs when computing standard deviation\n";
    std::cerr << "\n";
    std::cerr << " gsprt elo0 elo1 [-ab alpha beta] (w d l | n00 n05 n10 n15 n20)\n";
#if !_MSC_VER
    std::cerr << " match (-n nGames | -gsprt elo0 elo1 [-ab alpha beta])\n";
    std::cerr << "       engine1 tc1 engine2 tc2 script\n";
#endif
    std::cerr << "\n";
    std::cerr << " proofgame [-w a:b] [-d] [-m maxNodes] [-v] [-na] [-nokernel]\n";
    std::cerr << "           [-i \"initFen\"] [-ipgn \"initPgnFile\"] \"goalFen\"\n";
    std::cerr << " proofgame -f [-o outfile] [-retry] [-rnd seed] [-rndkernel]\n";
    std::cerr << " proofkernel [-i \"initFen\"] \"goalFen\"\n";
    std::cerr << " revmoves \"fen\"\n";
    std::cerr << std::flush;
    ::exit(2);
}

void
parseParamDomains(int argc, char* argv[], std::vector<ParamDomain>& params) {
    int i = 2;
    while (i + 3 < argc) {
        ParamDomain pd;
        pd.name = argv[i];
        if (!str2Num(argv[i+1], pd.minV) ||
            !str2Num(argv[i+2], pd.step) || (pd.step <= 0) ||
            !str2Num(argv[i+3], pd.maxV))
            usage();
        if (!Parameters::instance().getParam(pd.name))
            throw ChessParseError("No such parameter:" + pd.name);
        pd.value = Parameters::instance().getIntPar(pd.name);
        pd.value = (pd.value - pd.minV) / pd.step * pd.step + pd.minV;
        params.push_back(pd);
        i += 4;
    }
}

void
getParams(int argc, char* argv[], std::vector<ParamDomain>& params1,
          std::vector<ParamDomain>& params2) {
    Parameters& uciPars = Parameters::instance();
    bool firstSet = true;
    for (int i = 2; i < argc; i++) {
        std::string parName(argv[i]);
        if (parName == ":") {
            if (!firstSet)
                throw ChessParseError("Too many parameter sets");
            firstSet = false;
            continue;
        }
        std::vector<ParamDomain>& params = firstSet ? params1 : params2;
        ParamDomain pd;
        if (uciPars.getParam(parName)) {
            pd.name = parName;
            params.push_back(pd);
        } else if (uciPars.getParam(parName + "1")) {
            for (int n = 1; ; n++) {
                pd.name = parName + num2Str(n);
                if (!uciPars.getParam(pd.name))
                    break;
                params.push_back(pd);
            }
        } else
            throw ChessParseError("No such parameter:" + parName);
    }
    auto setParamInfo = [&uciPars](ParamDomain& pd) {
        std::shared_ptr<Parameters::ParamBase> p = uciPars.getParam(pd.name);
        const Parameters::SpinParam& sp = dynamic_cast<const Parameters::SpinParam&>(*p.get());
        pd.minV = sp.getMinValue();
        pd.step = 1;
        pd.maxV = sp.getMaxValue();
        pd.value = sp.getIntPar();
    };
    for (ParamDomain& pd : params1)
        setParamInfo(pd);
    for (ParamDomain& pd : params2)
        setParamInfo(pd);
}

void
getParams(int argc, char* argv[], std::vector<ParamDomain>& params) {
    std::vector<ParamDomain> params2;
    getParams(argc, argv, params, params2);
    if (!params2.empty())
        throw ChessParseError("Unexpected second set of parameters");
}

static void
doFilterCmd(int argc, char* argv[], ChessTool& chessTool) {
    if (argc < 3)
        usage();
    std::string type = argv[2];
    if (type == "score") {
        if (argc != 5)
            usage();
        int scLimit;
        double prLimit;
        if (!str2Num(argv[3], scLimit) || !str2Num(argv[4], prLimit))
            usage();
        chessTool.filterScore(std::cin, scLimit, prLimit);
    } else if (type == "mtrldiff") {
        if (argc != 8)
            usage();
        bool minorEqual = argv[3] == "-m"s;
        int first = minorEqual ? 4 : 3;
        std::vector<std::pair<bool,int>> mtrlPattern;
        for (int i = first; i < 8; i++) {
            if (argv[i] == "x"s)
                mtrlPattern.push_back(std::make_pair(false, 0));
            else {
                int d;
                if (!str2Num(argv[i], d))
                    usage();
                mtrlPattern.push_back(std::make_pair(true, d));
            }
        }
        chessTool.filterMtrlBalance(std::cin, minorEqual, mtrlPattern);
    } else if (type == "mtrl") {
        if (argc < 4)
            usage();
        bool minorEqual = argv[3] == "-m"s;
        if (argc != (minorEqual ? 12 : 13))
            usage();
        int first = minorEqual ? 4 : 3;
        std::vector<std::pair<bool,int>> mtrlPattern;
        for (int i = first; i < argc; i++) {
            if (argv[i] == "x"s)
                mtrlPattern.push_back(std::make_pair(false, 0));
            else {
                int d;
                if (!str2Num(argv[i], d))
                    usage();
                mtrlPattern.push_back(std::make_pair(true, d));
            }
        }
        chessTool.filterTotalMaterial(std::cin, minorEqual, mtrlPattern);
    } else
        usage();
}

static void
doFen2Bin(int argc, char* argv[], ChessTool& chessTool) {
    bool useResult = false;
    bool noInCheck = false;
    double prLimit = -1;

    argc -= 2;
    argv += 2;
    while (argc > 0) {
        std::string arg = argv[0];
        if (arg == "-useResult") {
            useResult = true;
            argc--;
            argv++;
        } else if (arg == "-noincheck") {
            noInCheck = true;
            argc--;
            argv++;
        } else if (argc > 1 && arg == "-prlimit") {
            if (!str2Num(argv[1], prLimit) || prLimit < 0.0)
                usage();
            argc -= 2;
            argv += 2;
        } else
            break;
    }
    if (argc != 1)
        usage();
    std::string outFile = argv[0];
    chessTool.fen2bin(std::cin, outFile, useResult, noInCheck, prLimit);
}

static void
doBookCmd(int argc, char* argv[]) {
    if (argc < 4)
        usage();
    std::string bookCmd = argv[2];
    std::string bookFile = argv[3];
    std::string logFile = bookFile + ".log";
    if (bookCmd == "improve") {
        ChessTool::setupTB();
        if ((argc < 6) || (argc > 10))
            usage();
        std::string startMoves;
        if (argc >= 7)
            startMoves = argv[6];
        int searchTime, numThreads;
        if (!str2Num(argv[4], searchTime) || (searchTime <= 0) ||
            !str2Num(argv[5], numThreads) || (numThreads <= 0))
            usage();
        std::shared_ptr<BookBuild::Book> book;
        if (argc == 10) {
            int bookDepthCost, ownPErrCost, otherPErrCost;
            if (!str2Num(argv[7], bookDepthCost) || (bookDepthCost <= 0) ||
                !str2Num(argv[8], ownPErrCost)   || (ownPErrCost   <= 0) ||
                !str2Num(argv[9], otherPErrCost) || (otherPErrCost <= 0))
                usage();
            book = std::make_shared<BookBuild::Book>(logFile, bookDepthCost,
                                                     ownPErrCost, otherPErrCost);
        } else {
            book = std::make_shared<BookBuild::Book>(logFile);
        }
        book->improve(bookFile, searchTime, numThreads, startMoves);
    } else if (bookCmd == "import") {
        if (argc < 5 || argc > 6)
            usage();
        std::string pgnFile = argv[4];
        int maxPly = INT_MAX;
        if ((argc > 5) && !str2Num(argv[5], maxPly))
            usage();
        BookBuild::Book book(logFile);
        book.importPGN(bookFile, pgnFile, maxPly);
    } else if (bookCmd == "export") {
        if (argc < 7)
            usage();
        std::string polyglotFile = argv[4];
        int maxErrSelf;
        double errOtherExpConst;
        if (!str2Num(argv[5], maxErrSelf) ||
            !str2Num(argv[6], errOtherExpConst))
            usage();
        bool includeLeafNodes = true;
        std::string excludeFile;
        for (int i = 7; i < argc; i++) {
            if (argv[i] == "noleaf"s) {
                includeLeafNodes = false;
            } else if (argv[i] == "-e"s && i + 1 < argc) {
                excludeFile = argv[i+1];
                i++;
            } else
                usage();
        }
        BookBuild::Book book("");
        book.exportPolyglot(bookFile, polyglotFile, maxErrSelf, errOtherExpConst,
                            includeLeafNodes, excludeFile);
    } else if (bookCmd == "query") {
        if (argc != 6)
            usage();
        int maxErrSelf;
        double errOtherExpConst;
        if (!str2Num(argv[4], maxErrSelf) ||
            !str2Num(argv[5], errOtherExpConst))
            usage();
        BookBuild::Book book("");
        book.interactiveQuery(bookFile, maxErrSelf, errOtherExpConst);
    } else if (bookCmd == "stats") {
        if (argc != 4)
            usage();
        BookBuild::Book book("");
        book.statistics(bookFile);
    } else {
        usage();
    }
}

static void
doGsprt(int argc, char* argv[]) {
    Gsprt::InParams pars;
    Gsprt::Sample sample;

    if (argc < 4 || !str2Num(argv[2], pars.elo0) || !str2Num(argv[3], pars.elo1))
        usage();

    int arg = 4;
    while (arg < argc) {
        if (arg + 2 < argc && argv[arg] == "-ab"s) {
            if (!str2Num(argv[arg+1], pars.alpha) ||
                !str2Num(argv[arg+2], pars.beta))
                usage();
            pars.useBounds = true;
            arg += 3;
        } else {
            break;
        }
    }

    int nGames = 0;
    if (arg + 3 == argc) {
        pars.usePentanomial = false;
        for (int i = 0; i < 3; i++) {
            if (!str2Num(argv[arg+i], sample.stats[i]))
                usage();
            nGames += sample.stats[i];
        }
    } else if (arg + 5 == argc) {
        pars.usePentanomial = true;
        for (int i = 0; i < 5; i++) {
            if (!str2Num(argv[arg+i], sample.stats[i]))
                usage();
            nGames += sample.stats[i];
        }
        nGames *= 2;
    } else {
        usage();
    }

    Gsprt::Result res;
    Gsprt gsprt(pars);
    gsprt.compute(sample, res);

    std::cout << "score0      : " << res.expectedScore0 << std::endl;
    std::cout << "score1      : " << res.expectedScore1 << std::endl;

    double mean = res.sampleScore;
    double sDev = res.sampleStdDev;
    std::cout << "Sample score: " << mean
              << " elo: " << Gsprt::score2Elo(res.sampleScore)
              << " nGames: " << nGames
              << " sd: " << sDev;
    if (sDev > 0) {
        std::stringstream ss;
        ss.precision(2);
        ss << std::fixed << (mean - 0.5) / sDev;
        std::cout << " c: " << ss.str();
    }
    std::cout << std::endl;

    if (pars.useBounds) {
        std::cout << "a   : " << res.a << std::endl;
        std::cout << "b   : " << res.b << std::endl;
    }
    std::cout << "LLR : " << res.llr << std::endl;
    if (pars.useBounds) {
        if (res.llr < res.a) {
            std::cout << "elo <= " << pars.elo0 << std::endl;
        } else if (res.llr > res.b) {
            std::cout << "elo >= " << pars.elo1 << std::endl;
        } else {
            std::cout << "elo unknown" << std::endl;
        }
    }
}

#if !_MSC_VER
static void
doMatch(int argc, char* argv[], int nWorkers) {
    int numGames = 0;
    bool fixedGames = false;
    Gsprt::InParams gsprtParams;
    gsprtParams.useBounds = true;
    gsprtParams.usePentanomial = true;
    bool gsprt = false;

    int arg = 2;
    while (arg < argc) {
        if (arg+1 < argc && argv[arg] == "-n"s) {
            if (!str2Num(argv[arg+1], numGames) || numGames <= 0)
                usage();
            fixedGames = true;
            arg += 2;
        } else if (arg+2 < argc && argv[arg] == "-gsprt"s) {
            if (!str2Num(argv[arg+1], gsprtParams.elo0) ||
                !str2Num(argv[arg+2], gsprtParams.elo1))
                usage();
            gsprt = true;
            arg += 3;
        } else if (arg+2 < argc && argv[arg] == "-ab"s) {
            if (!str2Num(argv[arg+1], gsprtParams.alpha) ||
                !str2Num(argv[arg+2], gsprtParams.beta))
                usage();
            arg += 3;
        } else {
            break;
        }
    }

    if (fixedGames == gsprt)
        usage();
    if (argc - arg != 5)
        usage();

    MatchRunner::EngineParams engine1;
    MatchRunner::EngineParams engine2;

    engine1.name = argv[arg+0];
    engine1.timeControl = argv[arg+1];
    engine2.name = argv[arg+2];
    engine2.timeControl = argv[arg+3];
    std::string script = argv[arg+4];

    MatchRunner mr(nWorkers, engine1, engine2);
    if (fixedGames)
        mr.runFixedNumGames(numGames, script);
    else
        mr.runGsprtGames(gsprtParams, script);
}
#endif

static void
doProofGameCmd(int argc, char* argv[], int nWorkers) {
    bool filter = false;

    // Options used in non-filtering mode
    std::string initFen = TextIO::startPosFEN;
    std::string initPgnFile;
    S64 maxNodes = -1;
    int a = 1, b = 1;
    bool dynamic = false;
    bool verbose = false;
    bool useNonAdmissible = false;
    bool useKernel = true;

    // Options used in filtering (-f) mode
    std::string outFile;
    bool retry = false;
    U64 rndSeed = 0;
    bool rndKernel = false;

    int arg = 2;
    while (arg < argc) {
        if (arg+1 < argc && argv[arg] == "-w"s) {
            std::string s(argv[arg+1]);
            size_t idx = s.find(':');
            if ((idx == std::string::npos) ||
                    !str2Num(s.substr(0, idx), a) ||
                    !str2Num(s.substr(idx+1), b))
                usage();
            arg += 2;
        } else  if (arg + 1 < argc && argv[arg] == "-i"s) {
            initFen = argv[arg+1];
            arg += 2;
        } else if (arg + 1 < argc && argv[arg] == "-ipgn"s) {
            initPgnFile = argv[arg+1];
            arg += 2;
        } else if (arg + 1 < argc && argv[arg] == "-m"s) {
            if (!str2Num(argv[arg+1], maxNodes))
                usage();
            arg += 2;
        } else if (argv[arg] == "-d"s) {
            dynamic = true;
            arg++;
        } else if (argv[arg] == "-v"s) {
            verbose = true;
            arg++;
        } else if (argv[arg] == "-nokernel"s) {
            useKernel = false;
            arg++;
        } else if (argv[arg] == "-f"s) {
            filter = true;
            arg++;
        } else if (arg + 1 < argc && argv[arg] == "-o"s) {
            outFile = argv[arg+1];
            arg += 2;
        } else if (argv[arg] == "-retry"s) {
            retry = true;
            arg++;
        } else if (argv[arg] == "-na"s) {
            useNonAdmissible = true;
            arg++;
        } else if (arg + 1 < argc && argv[arg] == "-rnd"s) {
            if (!str2Num(argv[arg+1], rndSeed))
                usage();
            arg += 2;
        } else if (argv[arg] == "-rndkernel"s) {
            rndKernel = true;
            arg++;
        } else {
            break;
        }
    }
    if (filter) {
        ProofGameFilter pgf(nWorkers, rndSeed, rndKernel);
        if (outFile.empty()) {
            pgf.filterFens(std::cin, std::cout, retry);
        } else {
            pgf.filterFensIterated(std::cin, outFile, retry);
        }
    } else {
        if (arg+1 != argc)
            usage();
        if (!outFile.empty())
            usage();
        std::string goalFen = argv[arg];

        std::vector<Move> initPath;
        if (!initPgnFile.empty()) {
            std::ifstream is(initPgnFile);
            if (!is.good())
                throw ChessParseError("Failed to open file");
            PgnReader reader(is);
            GameTree gt;
            while (reader.readPGN(gt))
                ;
            GameNode gn = gt.getRootNode();
            if (TextIO::toFEN(gn.getPos()) != initFen)
                throw ChessParseError("Incorrect PGN start position");
            while (gn.nChildren() > 0) {
                gn.goForward(0);
                initPath.push_back(gn.getMove());
            }
        }

        ProofGame ps(initFen, goalFen, useKernel, initPath);
        auto opts = ProofGame::Options()
            .setWeightA(a)
            .setWeightB(b)
            .setDynamic(dynamic)
            .setUseNonAdmissible(useNonAdmissible)
            .setMaxNodes(maxNodes)
            .setVerbose(verbose);
        ProofGame::Result result;
        ps.search(opts, result);
    }
}

static void
doProofKernelCmd(int argc, char* argv[], int nWorkers) {
    std::string initFen = TextIO::startPosFEN;
    int arg = 2;
    while (arg < argc) {
        if (arg + 1 < argc && argv[arg] == "-i"s) {
            initFen = argv[arg+1];
            arg += 2;
        } else {
            break;
        }
    }
    if (arg+1 != argc)
        usage();
    std::string goalFen = argv[arg];

    Position initPos = TextIO::readFEN(initFen);
    Position goalPos = TextIO::readFEN(goalFen);

    U64 blocked;
    ProofGame::computeBlocked(initPos, goalPos, blocked);
    ProofKernel pk(initPos, goalPos, blocked, std::cout);
    pk.findAll();
}

static void
doRevMoves(const std::string& fen) {
    Position pos = TextIO::readFEN(fen);
    std::string correctedFen = TextIO::toFEN(pos);
    if (correctedFen != fen)
        std::cout << "Corrected FEN: " << correctedFen << std::endl;

    std::vector<UnMove> revMoves;
    RevMoveGen::genMoves(pos, revMoves, true);

    for (const UnMove& um : revMoves) {
        std::string captP = TextIO::pieceToChar(um.ui.capturedPiece, true);
        if (captP.empty()) captP = "-";
        std::string castle = TextIO::castleMaskToString(um.ui.castleMask);
        Square epSq = um.ui.epSquare;
        std::string ep = !epSq.isValid() ? "-" : TextIO::squareToString(epSq);

        std::cout << um.move << " captP: " << captP << " castle: " << castle
                  << " ep: " << ep << std::endl;
    }
}

int
main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    int nWorkers = std::thread::hardware_concurrency();

    try {
        ComputerPlayer::initEngine();
        bool useEntropyErrorFunction = false;
        bool optimizeMoveOrdering = false;
        bool useSearchScore = false;
        while (true) {
            if ((argc >= 3) && (argv[1] == "-iv"s)) {
                setInitialValues(argv[2]);
                argc -= 2;
                argv += 2;
            } else if ((argc >= 3) && (argv[1] == "-j"s)) {
                if (!str2Num(argv[2], nWorkers) || nWorkers <= 0)
                    usage();
                argc -= 2;
                argv += 2;
            } else if ((argc >= 2) && (argv[1] == "-e"s)) {
                useEntropyErrorFunction = true;
                argc -= 1;
                argv += 1;
            } else if ((argc >= 2) && (argv[1] == "-s"s)) {
                useSearchScore = true;
                argc -= 1;
                argv += 1;
            } else if ((argc >= 2) && (argv[1] == "-moveorder"s)) {
                optimizeMoveOrdering = true;
                argc -= 1;
                argv += 1;
            } else
                break;
        }
        if (argc < 2)
            usage();

        std::string cmd = argv[1];
        ChessTool chessTool(useEntropyErrorFunction, optimizeMoveOrdering, useSearchScore,
                            nWorkers);
        if (cmd == "p2f") {
            int n = 1;
            if (argc > 4)
                usage();
            if (argc > 2)
                if (!str2Num(argv[2], n) || n < 1)
                    usage();
            bool includeUnScored = false;
            if (argc > 3)
                includeUnScored = argv[3] == "1"s;
            chessTool.pgnToFen(std::cin, n, includeUnScored);
        } else if (cmd == "f2p") {
            chessTool.fenToPgn(std::cin);
        } else if (cmd == "m2f") {
            chessTool.movesToFen(std::cin);
        } else if (cmd == "pawnadv") {
            chessTool.pawnAdvTable(std::cin);
        } else if (cmd == "filter") {
            doFilterCmd(argc, argv, chessTool);
#if !_MSC_VER
        } else if (cmd == "search") {
            if (argc != 3)
                usage();
            std::string script = argv[2];
            chessTool.computeSearchScores(std::cin, script);
#endif
        } else if (cmd == "qsearch") {
            if (argc != 2)
                usage();
            chessTool.computeQSearchPos(std::cin);
        } else if (cmd == "searchfens") {
            if (argc != 4)
                usage();
            int baseTime = 0, increment = 0;
            if (!str2Num(argv[2], baseTime) || (baseTime < 0) ||
                !str2Num(argv[3], increment) || (increment < 0))
                usage();
            chessTool.searchPositions(std::cin, baseTime, increment);
        } else if (cmd == "fen2bin") {
            doFen2Bin(argc, argv, chessTool);
        } else if (cmd == "outliers") {
            int threshold;
            if ((argc < 3) || !str2Num(argv[2], threshold))
                usage();
            chessTool.outliers(std::cin, threshold);
        } else if (cmd == "evaleffect") {
            if (argc != 3)
                usage();
            std::vector<ParamValue> parValues;
            parseParValues(argv[2], parValues);
            chessTool.evalEffect(std::cin, parValues);
        } else if (cmd == "parrange") {
            std::vector<ParamDomain> params;
            parseParamDomains(argc, argv, params);
            if (params.size() != 1)
                usage();
            chessTool.paramEvalRange(std::cin, params[0]);
#ifdef USE_ARMADILLO
        } else if (cmd == "gnopt") {
            if (useEntropyErrorFunction)
                usage();
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            chessTool.gnOptimize(std::cin, params);
#endif
        } else if (cmd == "localopt") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            chessTool.localOptimize(std::cin, params);
        } else if (cmd == "localopt2") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            chessTool.localOptimize2(std::cin, params);
        } else if (cmd == "simplify") {
            std::vector<ParamDomain> zeroParams, params;
            getParams(argc, argv, zeroParams, params);
            chessTool.simplify(std::cin, zeroParams, params);
        } else if (cmd == "printpar") {
            chessTool.printParams();
        } else if (cmd == "patchpar") {
            if (argc != 3)
                usage();
            std::string directory = argv[2];
            chessTool.patchParams(directory);
        } else if (cmd == "evalstat") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            chessTool.evalStat(std::cin, params);
        } else if (cmd == "residual") {
            if (argc != 4)
                usage();
            std::string xTypeStr = argv[2];
            bool includePosGameNr = (argv[3] != "0"s);
            chessTool.printResiduals(std::cin, xTypeStr, includePosGameNr);
        } else if (cmd == "genfen") {
            if ((argc < 3) || !PosGenerator::generate(argv[2]))
                usage();
        } else if (cmd == "rndfen") {
            int n = 0;
            U64 rndSeed = currentTimeMillis();
            if (argc < 3 || argc > 4 || !str2Num(argv[2], n) || n < 1)
                usage();
            if (argc == 4 && !str2Num(argv[3], rndSeed))
                usage();
            PosGenerator::randomLegal(n, rndSeed, nWorkers, std::cout);
        } else if (cmd == "rndtest") {
            U64 seed1 = currentTimeMillis();
            U64 seed2 = 0;
            if (argc > 2 && !str2Num(argv[2], seed1))
                usage();
            if (argc > 3 && !str2Num(argv[3], seed2))
                usage();
            if (argc > 4)
                usage();
            Random rnd(seed1, seed2);
            while (true) {
                U64 r = rnd.nextU64();
                std::cout.write((const char*)&r, sizeof(r));
            }
        } else if (cmd == "tblist") {
            int nPieces;
            if ((argc != 3) || !str2Num(argv[2], nPieces) || nPieces < 2)
                usage();
            PosGenerator::tbList(nPieces);
        } else if (cmd == "dtmstat") {
            if (argc < 3)
                usage();
            std::vector<std::string> tbTypes;
            for (int i = 2; i < argc; i++)
                tbTypes.push_back(argv[i]);
            PosGenerator::dtmStat(tbTypes);
        } else if (cmd == "dtzstat") {
            if (argc < 3)
                usage();
            std::vector<std::string> tbTypes;
            for (int i = 2; i < argc; i++)
                tbTypes.push_back(argv[i]);
            PosGenerator::dtzStat(tbTypes);
        } else if (cmd == "egstat") {
            if (argc < 4)
                usage();
            std::string tbType = argv[2];
            std::vector<std::string> pieceTypes;
            for (int i = 3; i < argc; i++)
                pieceTypes.push_back(argv[i]);
            PosGenerator::egStat(tbType, pieceTypes);
        } else if (cmd == "wdltest") {
            if (argc < 3)
                usage();
            std::vector<std::string> tbTypes;
            for (int i = 2; i < argc; i++)
                tbTypes.push_back(argv[i]);
            PosGenerator::wdlTest(tbTypes);
        } else if (cmd == "wdldump") {
            if (argc < 3)
                usage();
            std::vector<std::string> tbTypes;
            for (int i = 2; i < argc; i++)
                tbTypes.push_back(argv[i]);
            PosGenerator::wdlDump(tbTypes);
        } else if (cmd == "dtztest") {
            if (argc < 3)
                usage();
            std::vector<std::string> tbTypes;
            for (int i = 2; i < argc; i++)
                tbTypes.push_back(argv[i]);
            PosGenerator::dtzTest(tbTypes);
        } else if (cmd == "dtz") {
            if (argc < 3)
                usage();
            std::string fen = argv[2];
            ChessTool::probeDTZ(fen);
        } else if (cmd == "score2prob") {
            ScoreToProb sp;
            for (int i = -100; i <= 100; i++)
                std::cout << "i:" << i << " p:" << sp.getProb(i) << std::endl;
#ifdef USE_GSL
        } else if (cmd == "gamesim") {
            if (argc != 6)
                usage();
            double meanResult, drawProb;
            int nGames, nSimul;
            if (!str2Num(argv[2], meanResult) ||
                !str2Num(argv[3], drawProb) ||
                !str2Num(argv[4], nGames) ||
                !str2Num(argv[5], nSimul))
                usage();
            Spsa::gameSimulation(meanResult, drawProb, nGames, nSimul);
        } else if (cmd == "enginesim") {
            if (argc != 7)
                usage();
            int nGames;
            if (!str2Num(argv[2], nGames))
                usage();
            std::vector<double> params;
            for (int i = 3; i < argc; i++) {
                double tmp;
                if (!str2Num(argv[i], tmp))
                    usage();
                params.push_back(tmp);
            }
            Spsa::engineSimulation(nGames, params);
        } else if (cmd == "tourneysim") {
            if (argc < 6)
                usage();
            int nSimul, nRounds;
            if (!str2Num(argv[2], nSimul) || nSimul < 1 ||
                !str2Num(argv[3], nRounds) || nRounds < 1)
                usage();
            std::vector<double> elo;
            for (int i = 4; i < argc; i++) {
                double tmp;
                if (!str2Num(argv[i], tmp))
                    usage();
                elo.push_back(tmp);
            }
            Spsa::tourneySimulation(nSimul, nRounds, elo);
        } else if (cmd == "spsasim") {
            if (argc < 8)
                usage();
            int nSimul, nIter, gamesPerIter;
            double a, c;
            if (!str2Num(argv[2], nSimul) ||
                !str2Num(argv[3], nIter) ||
                !str2Num(argv[4], gamesPerIter) ||
                !str2Num(argv[5], a) ||
                !str2Num(argv[6], c))
                usage();

            std::vector<double> startParams;
            for (int i = 7; i < argc; i++) {
                double tmp;
                if (!str2Num(argv[i], tmp))
                    usage();
                startParams.push_back(tmp);
            }
            Spsa::spsaSimulation(nSimul, nIter, gamesPerIter, a, c, startParams);
        } else if (cmd == "spsa") {
            if (argc != 3)
                usage();
            std::string filename = argv[2];
            Spsa::spsa(filename);
#endif
        } else if (cmd == "tbgen") {
            if (argc != 10)
                usage();
            PieceCount pc;
            if (!str2Num(argv[2], pc.nwq) ||
                !str2Num(argv[3], pc.nwr) ||
                !str2Num(argv[4], pc.nwb) ||
                !str2Num(argv[5], pc.nwn) ||
                !str2Num(argv[6], pc.nbq) ||
                !str2Num(argv[7], pc.nbr) ||
                !str2Num(argv[8], pc.nbb) ||
                !str2Num(argv[9], pc.nbn))
                usage();
                VectorStorage vs;
                TBGenerator<VectorStorage> tbGen(vs, pc);
                RelaxedShared<S64> maxTimeMillis(-1);
                tbGen.generate(maxTimeMillis, true);
        } else if (cmd == "tbgentest") {
            if (argc < 3)
                usage();
            std::vector<std::string> tbTypes;
            for (int i = 2; i < argc; i++)
                tbTypes.push_back(argv[i]);
            PosGenerator::tbgenTest(tbTypes);
        } else if (cmd == "book") {
            doBookCmd(argc, argv);
        } else if (cmd == "creatematchbook") {
            if (argc != 4)
                usage();
            int depth, searchTime;
            if (!str2Num(argv[2], depth) || (depth < 0) ||
                !str2Num(argv[3], searchTime) || (searchTime <= 0))
                usage();
            MatchBookCreator mbc(nWorkers);
            mbc.createBook(depth, searchTime, std::cout);
        } else if (cmd == "countuniq") {
            if (argc != 3)
                usage();
            std::string pgnFile = argv[2];
            MatchBookCreator mbc(nWorkers);
            mbc.countUniq(pgnFile, std::cout);
        } else if (cmd == "pgnstat") {
            if (argc < 3 || argc > 4)
                usage();
            bool pairMode = false;
            if (argc == 4) {
                if (argv[3] == "-p"s)
                    pairMode = true;
                else
                    usage();
            }
            std::string pgnFile = argv[2];
            MatchBookCreator mbc(nWorkers);
            mbc.pgnStat(pgnFile, pairMode, std::cout);
        } else if (cmd == "gsprt") {
            doGsprt(argc, argv);
#if !_MSC_VER
        } else if (cmd == "match") {
            doMatch(argc, argv, nWorkers);
#endif
        } else if (cmd == "proofgame") {
            doProofGameCmd(argc, argv, nWorkers);
        } else if (cmd == "proofkernel") {
            doProofKernelCmd(argc, argv, nWorkers);
        } else if (cmd == "revmoves") {
            if (argc != 3)
                usage();
            std::string fen = argv[2];
            doRevMoves(fen);
        } else {
            usage();
        }
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
