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

#include <iostream>
#include <fstream>
#include <string>

void setInitialValues(const std::string& fname) {
    Parameters& uciPars = Parameters::instance();
    std::vector<std::string> lines = ChessTool::readFile(fname);
    for (const std::string& line : lines) {
        std::vector<std::string> fields;
        splitString(line, fields);
        if (fields.size() < 2)
            throw ChessParseError("Invalid initial value:" + line);
        if (!uciPars.getParam(fields[0]))
            throw ChessParseError("No such parameter:" + fields[0]);
        int value;
        if (str2Num(fields[1], value))
            uciPars.set(fields[0], fields[1]);
    }
}

void usage() {
    std::cerr << "Usage: texelutil [-iv file] cmd params\n";
    std::cerr << " -iv file : Set initial parameter values\n";
    std::cerr << "cmd is one of:\n";
    std::cerr << " p2f      : Convert from PGN to FEN\n";
    std::cerr << " f2p      : Convert from FEN to PGN\n";
    std::cerr << " filter type pars : Keep positions that satisfy a condition\n";
    std::cerr << "        score scLimit prLimit : qScore and search score differ less than limits\n";
    std::cerr << "        mtrl [-m] dQ dR dB [dN] dP : material difference satisfies pattern\n";
    std::cerr << "                                     -m treat bishop and knight as same type\n";
    std::cerr << " outliers threshold  : Print positions with unexpected game result\n";
    std::cerr << " pawnadv  : Compute evaluation error for different pawn advantage\n";
    std::cerr << " parrange p a b c    : Compare evaluation error for different parameter values\n";
    std::cerr << " localopt p1 p2 ...  : Optimize parameters using local search\n";
    std::cerr << " localopt2 p1 p2 ... : Optimize parameters using local search with big jumps\n";
    std::cerr << " printpar : Print evaluation tables and parameters\n";
    std::cerr << " evalstat p1 p2 ...  : Print parameter statistics\n";
    std::cerr << " residual xType inclNo : Print evaluation error as function of material\n";
    std::cerr << "                         xType is mtrlsum, mtrldiff or eval. inclNo is 0 or 1\n";
    std::cerr << " genfen qvsn : Generate all positions of a given type\n";
    std::cerr << std::flush;
    ::exit(2);
}

void parseParamDomains(int argc, char* argv[], std::vector<ParamDomain>& params) {
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

void getParams(int argc, char* argv[], std::vector<ParamDomain>& params) {
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
        const Parameters::SpinParamBase& sp = dynamic_cast<const Parameters::SpinParamBase&>(*p.get());
        pd.minV = sp.getMinValue();
        pd.step = 1;
        pd.maxV = sp.getMaxValue();
        pd.value = sp.getIntPar();
    }
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    try {
        if ((argc >= 3) && (std::string(argv[1]) == "-iv")) {
            setInitialValues(argv[2]);
            argc -= 2;
            argv += 2;
        }

        if (argc < 2)
            usage();

        std::string cmd = argv[1];
        if (cmd == "p2f") {
            ChessTool::pgnToFen(std::cin);
        } else if (cmd == "f2p") {
            ChessTool::fenToPgn(std::cin);
        } else if (cmd == "pawnadv") {
            ChessTool::pawnAdvTable(std::cin);
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
                ChessTool::filterScore(std::cin, scLimit, prLimit);
            } else if (type == "mtrl") {
                if (argc != 8)
                    usage();
                bool minorEqual = (std::string(argv[3]) == "-m");
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
                ChessTool::filterMtrlBalance(std::cin, minorEqual, mtrlPattern);
            } else
                usage();
        } else if (cmd == "outliers") {
            int threshold;
            if ((argc < 3) || !str2Num(argv[2], threshold))
                usage();
            ChessTool::outliers(std::cin, threshold);
        } else if (cmd == "parrange") {
            std::vector<ParamDomain> params;
            parseParamDomains(argc, argv, params);
            if (params.size() != 1)
                usage();
            ChessTool::paramEvalRange(std::cin, params[0]);
        } else if (cmd == "localopt") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            ChessTool::localOptimize(std::cin, params);
        } else if (cmd == "localopt2") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            ChessTool::localOptimize2(std::cin, params);
        } else if (cmd == "printpar") {
            ChessTool::printParams();
        } else if (cmd == "evalstat") {
            std::vector<ParamDomain> params;
            getParams(argc, argv, params);
            ChessTool::evalStat(std::cin, params);
        } else if (cmd == "residual") {
            if (argc != 4)
                usage();
            std::string xTypeStr = argv[2];
            bool includePosGameNr = (std::string(argv[3]) != "0");
            ChessTool::printResiduals(std::cin, xTypeStr, includePosGameNr);
        } else if (cmd == "genfen") {
            if ((argc < 3) || !PosGenerator::generate(argv[2]))
                usage();
        } else {
            ScoreToProb sp(300.0);
            for (int i = -100; i <= 100; i++)
                std::cout << "i:" << i << " p:" << sp.getProb(i) << std::endl;
        }
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    return 0;
}
