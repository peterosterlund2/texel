/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * parameters.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "parameters.hpp"
#include "computerPlayer.hpp"

namespace UciParams {
    using SpinParam = Parameters::SpinParam;
    using CheckParam = Parameters::CheckParam;
    using StringParam = Parameters::StringParam;
    using ButtonParam = Parameters::ButtonParam;
#ifdef CLUSTER
    int maxThreads = 64*1024*1024;
#else
    int maxThreads = 512;
#endif
    std::shared_ptr<SpinParam> threads(std::make_shared<SpinParam>("Threads", 1, maxThreads, 1));

    std::shared_ptr<SpinParam> hash(std::make_shared<SpinParam>("Hash", 1, 1024*1024, 16));
    std::shared_ptr<SpinParam> multiPV(std::make_shared<SpinParam>("MultiPV", 1, 256, 1));
    std::shared_ptr<CheckParam> ponder(std::make_shared<CheckParam>("Ponder", false));
    std::shared_ptr<CheckParam> analyseMode(std::make_shared<CheckParam>("UCI_AnalyseMode", false));

    std::shared_ptr<CheckParam> ownBook(std::make_shared<CheckParam>("OwnBook", false));
    std::shared_ptr<StringParam> bookFile(std::make_shared<StringParam>("BookFile", ""));

    std::shared_ptr<CheckParam> useNullMove(std::make_shared<CheckParam>("UseNullMove", true));
    std::shared_ptr<CheckParam> analysisAgeHash(std::make_shared<CheckParam>("AnalysisAgeHash", true));
    std::shared_ptr<ButtonParam> clearHash(std::make_shared<ButtonParam>("Clear Hash"));

    std::shared_ptr<SpinParam> strength(std::make_shared<SpinParam>("Strength", 0, 1000, 1000));
    std::shared_ptr<SpinParam> maxNPS(std::make_shared<SpinParam>("MaxNPS", 0, 10000000, 0));
    std::shared_ptr<CheckParam> limitStrength(std::make_shared<CheckParam>("UCI_LimitStrength", false));
    std::shared_ptr<SpinParam> elo(std::make_shared<SpinParam>("UCI_Elo", -625, 2900, 1500));

    std::shared_ptr<SpinParam> contempt(std::make_shared<SpinParam>("Contempt", -2000, 2000, 0));
    std::shared_ptr<SpinParam> analyzeContempt(std::make_shared<SpinParam>("AnalyzeContempt", -2000, 2000, 0));
    std::shared_ptr<CheckParam> autoContempt(std::make_shared<CheckParam>("AutoContempt", false));
    std::shared_ptr<StringParam> contemptFile(std::make_shared<StringParam>("ContemptFile", ""));
    std::shared_ptr<StringParam> opponent(std::make_shared<StringParam>("UCI_Opponent", ""));

    std::shared_ptr<StringParam> gtbPath(std::make_shared<StringParam>("GaviotaTbPath", ""));
    std::shared_ptr<SpinParam> gtbCache(std::make_shared<SpinParam>("GaviotaTbCache", 1, 2047, 1));
    std::shared_ptr<StringParam> rtbPath(std::make_shared<StringParam>("SyzygyPath", ""));
    std::shared_ptr<SpinParam> minProbeDepth(std::make_shared<SpinParam>("MinProbeDepth", 0, 100, 1));
    std::shared_ptr<SpinParam> minProbeDepth6(std::make_shared<SpinParam>("MinProbeDepth6", 0, 100, 1));
    std::shared_ptr<SpinParam> minProbeDepth6dtz(std::make_shared<SpinParam>("MinProbeDepth6dtz", 0, 100, 1));
    std::shared_ptr<SpinParam> minProbeDepth7(std::make_shared<SpinParam>("MinProbeDepth7", 0, 100, 12));
    std::shared_ptr<SpinParam> minProbeDepth7dtz(std::make_shared<SpinParam>("MinProbeDepth7dtz", 0, 100, 12));
}

int pieceValue[Piece::nPieceTypes];

DEFINE_PARAM(pV);
DEFINE_PARAM(nV);
DEFINE_PARAM(bV);
DEFINE_PARAM(rV);
DEFINE_PARAM(qV);
DEFINE_PARAM(kV);

DEFINE_PARAM(knightVsQueenBonus1);
DEFINE_PARAM(knightVsQueenBonus2);
DEFINE_PARAM(knightVsQueenBonus3);
DEFINE_PARAM(krkpBonus);

DEFINE_PARAM(aspirationWindow);
DEFINE_PARAM(rootLMRMoveCount);

DEFINE_PARAM(razorMargin1);
DEFINE_PARAM(razorMargin2);

DEFINE_PARAM(reverseFutilityMargin1);
DEFINE_PARAM(reverseFutilityMargin2);
DEFINE_PARAM(reverseFutilityMargin3);
DEFINE_PARAM(reverseFutilityMargin4);

DEFINE_PARAM(futilityMargin1);
DEFINE_PARAM(futilityMargin2);
DEFINE_PARAM(futilityMargin3);
DEFINE_PARAM(futilityMargin4);

DEFINE_PARAM(lmpMoveCountLimit1);
DEFINE_PARAM(lmpMoveCountLimit2);
DEFINE_PARAM(lmpMoveCountLimit3);
DEFINE_PARAM(lmpMoveCountLimit4);

DEFINE_PARAM(lmrMoveCountLimit1);
DEFINE_PARAM(lmrMoveCountLimit2);

DEFINE_PARAM(quiesceMaxSortMoves);
DEFINE_PARAM(deltaPruningMargin);

DEFINE_PARAM(timeMaxRemainingMoves);
DEFINE_PARAM(bufferTime);
DEFINE_PARAM(minTimeUsage);
DEFINE_PARAM(maxTimeUsage);
DEFINE_PARAM(timePonderHitRate);

ParamTable<10> halfMoveFactor { 0, 192, useUciParam,
    {128,128,128,128, 44, 35, 29, 25, 20, 17 },
    {  0,  0,  0,  0,  1,  2,  3,  4,  5,  6 }
};

Parameters::Parameters() {
    std::string about = ComputerPlayer::engineName +
                        " by Peter Osterlund, see https://github.com/peterosterlund2/texel";
    addPar(std::make_shared<StringParam>("UCI_EngineAbout", about));

    addPar(UciParams::threads);

    addPar(UciParams::hash);
    addPar(UciParams::multiPV);
    addPar(UciParams::ponder);
    addPar(UciParams::analyseMode);

    addPar(UciParams::ownBook);
    addPar(UciParams::bookFile);

    addPar(UciParams::useNullMove);
    addPar(UciParams::analysisAgeHash);
    addPar(UciParams::clearHash);

    addPar(UciParams::strength);
    addPar(UciParams::maxNPS);
    addPar(UciParams::limitStrength);
    addPar(UciParams::elo);

    addPar(UciParams::contempt);
    addPar(UciParams::analyzeContempt);
    addPar(UciParams::autoContempt);
    addPar(UciParams::contemptFile);
    addPar(UciParams::opponent);

    addPar(UciParams::gtbPath);
    addPar(UciParams::gtbCache);
    addPar(UciParams::rtbPath);
    addPar(UciParams::minProbeDepth);
    addPar(UciParams::minProbeDepth6);
    addPar(UciParams::minProbeDepth6dtz);
    addPar(UciParams::minProbeDepth7);
    addPar(UciParams::minProbeDepth7dtz);

    // Evaluation parameters
    REGISTER_PARAM(pV, "PawnValue");
    REGISTER_PARAM(nV, "KnightValue");
    REGISTER_PARAM(bV, "BishopValue");
    REGISTER_PARAM(rV, "RookValue");
    REGISTER_PARAM(qV, "QueenValue");
    REGISTER_PARAM(kV, "KingValue");

    REGISTER_PARAM(knightVsQueenBonus1, "KnightVsQueenBonus1");
    REGISTER_PARAM(knightVsQueenBonus2, "KnightVsQueenBonus2");
    REGISTER_PARAM(knightVsQueenBonus3, "KnightVsQueenBonus3");
    REGISTER_PARAM(krkpBonus, "RookVsPawnBonus");

    halfMoveFactor.registerParams("HalfMoveFactor", *this);

    // Search parameters
    REGISTER_PARAM(aspirationWindow, "AspirationWindow");
    REGISTER_PARAM(rootLMRMoveCount, "RootLMRMoveCount");

    REGISTER_PARAM(razorMargin1, "RazorMargin1");
    REGISTER_PARAM(razorMargin2, "RazorMargin2");

    REGISTER_PARAM(reverseFutilityMargin1, "ReverseFutilityMargin1");
    REGISTER_PARAM(reverseFutilityMargin2, "ReverseFutilityMargin2");
    REGISTER_PARAM(reverseFutilityMargin3, "ReverseFutilityMargin3");
    REGISTER_PARAM(reverseFutilityMargin4, "ReverseFutilityMargin4");

    REGISTER_PARAM(futilityMargin1, "FutilityMargin1");
    REGISTER_PARAM(futilityMargin2, "FutilityMargin2");
    REGISTER_PARAM(futilityMargin3, "FutilityMargin3");
    REGISTER_PARAM(futilityMargin4, "FutilityMargin4");

    REGISTER_PARAM(lmpMoveCountLimit1, "LMPMoveCountLimit1");
    REGISTER_PARAM(lmpMoveCountLimit2, "LMPMoveCountLimit2");
    REGISTER_PARAM(lmpMoveCountLimit3, "LMPMoveCountLimit3");
    REGISTER_PARAM(lmpMoveCountLimit4, "LMPMoveCountLimit4");

    REGISTER_PARAM(lmrMoveCountLimit1, "LMRMoveCountLimit1");
    REGISTER_PARAM(lmrMoveCountLimit2, "LMRMoveCountLimit2");

    REGISTER_PARAM(quiesceMaxSortMoves, "QuiesceMaxSortMoves");
    REGISTER_PARAM(deltaPruningMargin, "DeltaPruningMargin");

    // Time management parameters
    REGISTER_PARAM(timeMaxRemainingMoves, "TimeMaxRemainingMoves");
    REGISTER_PARAM(bufferTime, "BufferTime");
    REGISTER_PARAM(minTimeUsage, "MinTimeUsage");
    REGISTER_PARAM(maxTimeUsage, "MaxTimeUsage");
    REGISTER_PARAM(timePonderHitRate, "TimePonderHitRate");
}

Parameters&
Parameters::instance() {
    static Parameters inst;
    return inst;
}

void
Parameters::getParamNames(std::vector<std::string>& parNames) {
    parNames = paramNames;
}

std::shared_ptr<Parameters::ParamBase>
Parameters::getParam(const std::string& name) const {
    auto it = params.find(toLowerCase(name));
    if (it == params.end())
        return nullptr;
    return it->second;
}

void
Parameters::addPar(const std::shared_ptr<ParamBase>& p) {
    std::string name = toLowerCase(p->getName());
    assert(params.find(name) == params.end());
    params[name] = p;
    paramNames.push_back(name);
}

int
Parameters::Listener::addListener(Func f, bool callNow) {
    int id = ++nextId;
    listeners[id] = f;
    if (callNow)
        f();
    return id;
}

void
Parameters::Listener::removeListener(int id) {
    listeners.erase(id);
}

void
Parameters::Listener::notify() {
    for (auto& e : listeners)
        (e.second)();
}

void
ParamTableBase::registerParamsN(const std::string& name, Parameters& pars,
                                int* table, int* parNo, int N) {
    // Check that each parameter has a single value
    std::map<int,int> parNoToVal;
    int maxParIdx = -1;
    for (int i = 0; i < N; i++) {
        if (parNo[i] == 0)
            continue;
        const int pn = std::abs(parNo[i]);
        const int sign = parNo[i] > 0 ? 1 : -1;
        maxParIdx = std::max(maxParIdx, pn);
        auto it = parNoToVal.find(pn);
        if (it == parNoToVal.end())
            parNoToVal.insert(std::make_pair(pn, sign*table[i]));
        else
            assert(it->second == sign*table[i]);
    }
    if (!uci)
        return;
    params.resize(maxParIdx+1);
    for (const auto& p : parNoToVal) {
        std::string pName = name + num2Str(p.first);
        params[p.first] = std::make_shared<Parameters::SpinParam>(pName, minValue, maxValue, p.second);
        pars.addPar(params[p.first]);
        params[p.first]->addListener([=]() { modifiedN(table, parNo, N); }, false);
    }
    modifiedN(table, parNo, N);
}

void
ParamTableBase::modifiedN(int* table, int* parNo, int N) {
    for (int i = 0; i < N; i++)
        if (parNo[i] > 0)
            table[i] = params[parNo[i]]->getIntPar();
        else if (parNo[i] < 0)
            table[i] = -params[-parNo[i]]->getIntPar();
    notify();
}
