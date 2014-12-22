/*
 * texelutil.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "chesstool.hpp"
#include "posgen.hpp"
#include "parameters.hpp"
#include "chessParseError.hpp"
#include "computerPlayer.hpp"

#include <iostream>
#include <fstream>
#include <string>

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
        int value;
        if (str2Num(fields[1], value)) {
            ParamValue pv;
            pv.name = fields[0];
            pv.value = value;
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
    std::cerr << "Usage: texelutil [-iv file] [-e] cmd params\n";
    std::cerr << " -iv file : Set initial parameter values\n";
    std::cerr << " -e : Use cross entropy error function\n";
    std::cerr << "cmd is one of:\n";
    std::cerr << " p2f      : Convert from PGN to FEN\n";
    std::cerr << " f2p      : Convert from FEN to PGN\n";
    std::cerr << " filter type pars : Keep positions that satisfy a condition\n";
    std::cerr << "        score scLimit prLimit : qScore and search score differ less than limits\n";
    std::cerr << "        mtrldiff [-m] dQ dR dB [dN] dP : material difference satisfies pattern\n";
    std::cerr << "                                     -m treat bishop and knight as same type\n";
    std::cerr << "        mtrl [-m] wQ wR wB [wN] wP bQ bR bB [bN] bP : material satisfies pattern\n";
    std::cerr << "                                     -m treat bishop and knight as same type\n";
    std::cerr << " outliers threshold  : Print positions with unexpected game result\n";
    std::cerr << " evaleffect evalfile : Print eval improvement when parameters are changed\n";
    std::cerr << " pawnadv  : Compute evaluation error for different pawn advantage\n";
    std::cerr << " score2prob : Compute table of expected score as function of centipawns\n";
    std::cerr << " parrange p a b c    : Compare evaluation error for different parameter values\n";
    std::cerr << " gnopt p1 p2 ...     : Optimize parameters using Gauss-Newton method\n";
    std::cerr << " localopt p1 p2 ...  : Optimize parameters using local search\n";
    std::cerr << " localopt2 p1 p2 ... : Optimize parameters using local search with big jumps\n";
    std::cerr << " printpar : Print evaluation tables and parameters\n";
    std::cerr << " patchpar srcdir : Update parameter values in parameters.[ch]pp\n";
    std::cerr << " evalstat p1 p2 ...  : Print parameter statistics\n";
    std::cerr << " residual xType inclNo : Print evaluation error as function of material\n";
    std::cerr << "                         xType is mtrlsum, mtrldiff, pawnsum, pawndiff or eval\n";
    std::cerr << "                         inclNo is 0/1 to exclude/include position/game numbers\n";
    std::cerr << " genfen qvsn : Generate all positions of a given type\n";
    std::cerr << " tblist nPieces : Print all tablebase types\n";
    std::cerr << " dtmstat type1 [type2 ...] : Generate tablebase DTM statistics\n";
    std::cerr << " dtzstat type1 [type2 ...] : Generate tablebase DTZ statistics\n";
    std::cerr << " egstat type pieceType1 [pieceType2 ...] : Endgame WDL statistics\n";
    std::cerr << " wdltest type1 [type2 ...] : Compare RTB and GTB WDL tables\n";
    std::cerr << " dtztest type1 [type2 ...] : Compare RTB DTZ and GTB DTM tables\n";
    std::cerr << " dtz fen                   : Retrieve DTZ value for a position\n";
    std::cerr << std::flush;
    ::exit(2);
}

void
parseParamDomains(int argc, char* argv[], std::vector<ParamDomain>& params) {
    int i = 2;
    while (i + 3 < argc) {
        ParamDomain pd;
        pd.name = argv[i];
        if (!str2Num(std::string(argv[i+1]), pd.minV) ||
            !str2Num(std::string(argv[i+2]), pd.step) || (pd.step <= 0) ||
            !str2Num(std::string(argv[i+3]), pd.maxV))
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
getParams(int argc, char* argv[], std::vector<ParamDomain>& params) {
    Parameters& uciPars = Parameters::instance();
    for (int i = 2; i < argc; i++) {
        ParamDomain pd;
        std::string parName(argv[i]);
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
    for (ParamDomain& pd : params) {
        std::shared_ptr<Parameters::ParamBase> p = uciPars.getParam(pd.name);
        const Parameters::SpinParam& sp = dynamic_cast<const Parameters::SpinParam&>(*p.get());
        pd.minV = sp.getMinValue();
        pd.step = 1;
        pd.maxV = sp.getMaxValue();
        pd.value = sp.getIntPar();
    }
}

int
main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    try {
        ComputerPlayer::initEngine();
        bool useEntropyErrorFunction = false;
        while (true) {
            if ((argc >= 3) && (std::string(argv[1]) == "-iv")) {
                setInitialValues(argv[2]);
                argc -= 2;
                argv += 2;
            } else if ((argc >= 2) && (std::string(argv[1]) == "-e")) {
                useEntropyErrorFunction = true;
                argc -= 1;
                argv += 1;
            } else
                break;
        }
        if (argc < 2)
            usage();

        std::string cmd = argv[1];
        ChessTool chessTool(useEntropyErrorFunction);
        if (cmd == "p2f") {
            chessTool.pgnToFen(std::cin);
        } else if (cmd == "f2p") {
            chessTool.fenToPgn(std::cin);
        } else if (cmd == "pawnadv") {
            chessTool.pawnAdvTable(std::cin);
        } else if (cmd == "filter") {
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
                bool minorEqual = std::string(argv[3]) == "-m";
                int first = minorEqual ? 4 : 3;
                std::vector<std::pair<bool,int>> mtrlPattern;
                for (int i = first; i < 8; i++) {
                    if (std::string(argv[i]) == "x")
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
                bool minorEqual = std::string(argv[3]) == "-m";
                if (argc != (minorEqual ? 12 : 13))
                    usage();
                int first = minorEqual ? 4 : 3;
                std::vector<std::pair<bool,int>> mtrlPattern;
                for (int i = first; i < argc; i++) {
                    if (std::string(argv[i]) == "x")
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
        } else if (cmd == "gnopt") {
            if (useEntropyErrorFunction)
                usage();
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            chessTool.gnOptimize(std::cin, params);
        } else if (cmd == "localopt") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            chessTool.localOptimize(std::cin, params);
        } else if (cmd == "localopt2") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            chessTool.localOptimize2(std::cin, params);
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
            bool includePosGameNr = (std::string(argv[3]) != "0");
            chessTool.printResiduals(std::cin, xTypeStr, includePosGameNr);
        } else if (cmd == "genfen") {
            if ((argc < 3) || !PosGenerator::generate(argv[2]))
                usage();
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
        } else {
            usage();
        }
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    return 0;
}
