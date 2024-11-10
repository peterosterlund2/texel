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
 * parameters.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef PARAMETERS_HPP_
#define PARAMETERS_HPP_

#include "util.hpp"
#include "piece.hpp"
#include "square.hpp"

#include <memory>
#include <functional>
#include <map>
#include <string>
#include <cassert>


/** Handles all UCI parameters. */
class Parameters {
public:
    enum Type {
        CHECK,
        SPIN,
        COMBO,
        BUTTON,
        STRING
    };

    /** Observer pattern. */
    class Listener {
        using Func = std::function<void()>;
    public:
        int addListener(Func f, bool callNow = true);
        void removeListener(int id);

    protected:
        virtual ~Listener() = default;
        void notify();

    private:
        int nextId = 0;
        std::map<int, Func> listeners;
    };

    /** Base class for UCI parameters. */
    class ParamBase : public Listener {
    public:
        ParamBase(const std::string& n, Type t) : name(n), type(t) { }
        ParamBase(const ParamBase& other) = delete;
        ParamBase& operator=(const ParamBase& other) = delete;

        const std::string& getName() const { return name; }
        Type getType() const { return type; }

        virtual bool getBoolPar() const { assert(false); return false; }
        virtual int getIntPar() const { assert(false); return 0; }
        virtual std::string getStringPar() const { assert(false); return ""; }
        virtual void set(const std::string& value) { assert(false); }
    private:
        std::string name;
        Type type;
    };

    /** A boolean parameter. */
    class CheckParam : public ParamBase {
    public:
        CheckParam(const std::string& name, bool def)
            : ParamBase(name, CHECK) {
            this->value = def;
            this->defaultValue = def;
        }

        bool getBoolPar() const final { return value; }
        bool getDefaultValue() const { return defaultValue; }

        void set(const std::string& value) override {
            if (toLowerCase(value) == "true")
                this->value = true;
            else if (toLowerCase(value) == "false")
                this->value = false;
            notify();
        }
    private:
        bool value;
        bool defaultValue;
    };

    /** An integer parameter. */
    class SpinParam : public ParamBase {
    public:
        SpinParam(const std::string& name, int minV, int maxV, int def)
            : ParamBase(name, SPIN), minValue(minV), maxValue(maxV),
              value(def), defaultValue(def) {
        }

        int getIntPar() const final { return value; }

        void set(const std::string& value) override {
            int val;
            if (str2Num(value, val) && (val >= minValue) && (val <= maxValue)) {
                this->value = val;
                notify();
            }
        }

        int getDefaultValue() const { return defaultValue; }
        int getMinValue() const { return minValue; }
        int getMaxValue() const { return maxValue; }
    private:
        int minValue;
        int maxValue;
        int value;
        int defaultValue;
    };

    /** A multi-choice parameter. */
    class ComboParam : public ParamBase {
    public:
        ComboParam(const std::string& name, const std::vector<std::string>& allowed,
                   const std::string& def)
            : ParamBase(name, COMBO), allowedValues(allowed),
              value(def), defaultValue(def) {
        }

        std::string getStringPar() const override { return value; }
        const std::string& getDefaultValue() const { return defaultValue; }
        const std::vector<std::string>& getAllowedValues() const { return allowedValues; }

        void set(const std::string& value) override {
            for (size_t i = 0; i < allowedValues.size(); i++) {
                const std::string& allowed = allowedValues[i];
                if (toLowerCase(allowed) == toLowerCase(value)) {
                    this->value = allowed;
                    notify();
                    break;
                }
            }
        }
    private:
        std::vector<std::string> allowedValues;
        std::string value;
        std::string defaultValue;
    };

    /** An action parameter. */
    class ButtonParam : public ParamBase {
    public:
        explicit ButtonParam(const std::string& name)
            : ParamBase(name, BUTTON) { }

        void set(const std::string& value) override {
            notify();
        }
    };

    /** A string parameter. */
    class StringParam : public ParamBase {
    public:
        StringParam(const std::string& name, const std::string& def)
            : ParamBase(name, STRING), value(def), defaultValue(def) {
        }

        std::string getStringPar() const override { return value; }
        const std::string& getDefaultValue() const { return defaultValue; }

        void set(const std::string& value) override {
            this->value = value;
            notify();
        }
    private:
        std::string value;
        std::string defaultValue;
    };

    /** Get singleton instance. */
    static Parameters& instance();

    /** Retrieve list of all parameters. */
    void getParamNames(std::vector<std::string>& parNames);

    std::shared_ptr<ParamBase> getParam(const std::string& name) const;

    bool getBoolPar(const std::string& name) const;
    int getIntPar(const std::string& name) const;
    std::string getStringPar(const std::string& name) const;

    void set(const std::string& name, const std::string& value);

    /** Register a parameter. */
    void addPar(const std::shared_ptr<ParamBase>& p);

private:
    Parameters();

    std::map<std::string, std::shared_ptr<ParamBase>> params;
    std::vector<std::string> paramNames; // Names in insertion order
};

// ----------------------------------------------------------------------------

/** Param can be either a UCI parameter or a compile time constant. */
template <int defaultValue, int minValue, int maxValue, bool uci> class Param;

template <int defaultValue, int minValue, int maxValue>
class Param<defaultValue, minValue, maxValue, false> {
public:
    Param() {}
    operator int() const { return defaultValue; }
    void registerParam(const std::string& name, Parameters& pars) {}
    template <typename Func> void addListener(Func f) { f(); }
};

template <int defaultValue, int minValue, int maxValue>
class Param<defaultValue, minValue, maxValue, true> {
public:
    Param() : value(0) {}
    operator int() const { return value; }
    void registerParam(const std::string& name, Parameters& pars) {
        par = std::make_shared<Parameters::SpinParam>(name, minValue, maxValue, defaultValue);
        pars.addPar(par);
        par->addListener([this](){ value = par->getIntPar(); });
    }
    template <typename Func> void addListener(Func f) {
        if (par)
            par->addListener(f, false);
        f();
    }
private:
    int value;
    std::shared_ptr<Parameters::SpinParam> par;
};

#define DECLARE_PARAM(name, defV, minV, maxV, uci) \
    using name##ParamType = Param<defV,minV,maxV,uci>; \
    extern name##ParamType name;

#define DEFINE_PARAM(name) \
    name##ParamType name;

#define REGISTER_PARAM(varName, uciName) \
    varName.registerParam(uciName, *this);

// ----------------------------------------------------------------------------

/** Non-template base class to reduce executable code size. */
class ParamTableBase : public Parameters::Listener {
public:
    int getMinValue() const { return minValue; }
    int getMaxValue() const { return maxValue; }

protected:
    ParamTableBase(bool uci0, int minVal0, int maxVal0) :
        uci(uci0), minValue(minVal0), maxValue(maxVal0) {}
    void registerParamsN(const std::string& name, Parameters& pars,
                         int* table, int* parNo, int N);
    void modifiedN(int* table, int* parNo, int N);

private:
    const bool uci;
    const int minValue;
    const int maxValue;

    std::vector<std::shared_ptr<Parameters::SpinParam>> params;
};

template <int N>
class ParamTable : public ParamTableBase {
public:
    ParamTable(int minVal, int maxVal, bool uci,
               std::initializer_list<int> data,
               std::initializer_list<int> parNo);

    int operator[](int i) const { return table[i]; }
    const int* getTable() const { return table; }

    void registerParams(const std::string& name, Parameters& pars) {
        registerParamsN(name, pars, table, parNo, N);
    }

private:
    int table[N];
    int parNo[N];
};

template <int N>
ParamTable<N>::ParamTable(int minVal0, int maxVal0, bool uci0,
                          std::initializer_list<int> table0,
                          std::initializer_list<int> parNo0)
    : ParamTableBase(uci0, minVal0, maxVal0) {
    assert(table0.size() == N);
    assert(parNo0.size() == N);
    for (int i = 0; i < N; i++) {
        table[i] = table0.begin()[i];
        parNo[i] = parNo0.begin()[i];
    }
}

inline bool
Parameters::getBoolPar(const std::string& name) const {
    return getParam(name)->getBoolPar();
}

inline int
Parameters::getIntPar(const std::string& name) const {
    return getParam(name)->getIntPar();
}

inline std::string
Parameters::getStringPar(const std::string& name) const {
    return getParam(name)->getStringPar();
}

inline void
Parameters::set(const std::string& name, const std::string& value) {
    auto it = params.find(toLowerCase(name));
    if (it == params.end())
        return;
    it->second->set(value);
}

// ----------------------------------------------------------------------------
// UCI parameters

namespace UciParams {
    extern std::shared_ptr<Parameters::SpinParam> threads;

    extern std::shared_ptr<Parameters::SpinParam> hash;
    extern std::shared_ptr<Parameters::SpinParam> multiPV;
    extern std::shared_ptr<Parameters::CheckParam> ponder;
    extern std::shared_ptr<Parameters::CheckParam> analyseMode;

    extern std::shared_ptr<Parameters::CheckParam> ownBook;
    extern std::shared_ptr<Parameters::StringParam> bookFile;

    extern std::shared_ptr<Parameters::CheckParam> useNullMove;
    extern std::shared_ptr<Parameters::CheckParam> analysisAgeHash;
    extern std::shared_ptr<Parameters::ButtonParam> clearHash;

    extern std::shared_ptr<Parameters::SpinParam> strength;
    extern std::shared_ptr<Parameters::SpinParam> maxNPS;
    extern std::shared_ptr<Parameters::CheckParam> limitStrength;
    extern std::shared_ptr<Parameters::SpinParam> elo;

    extern std::shared_ptr<Parameters::SpinParam> contempt;
    extern std::shared_ptr<Parameters::SpinParam> analyzeContempt;
    extern std::shared_ptr<Parameters::CheckParam> autoContempt;
    extern std::shared_ptr<Parameters::StringParam> contemptFile;
    extern std::shared_ptr<Parameters::StringParam> opponent;

    extern std::shared_ptr<Parameters::StringParam> gtbPath;
    extern std::shared_ptr<Parameters::SpinParam> gtbCache;
    extern std::shared_ptr<Parameters::StringParam> rtbPath;
    extern std::shared_ptr<Parameters::SpinParam> minProbeDepth;     // Generic min TB probe depth
    extern std::shared_ptr<Parameters::SpinParam> minProbeDepth6;    // Min probe depth for 6-men
    extern std::shared_ptr<Parameters::SpinParam> minProbeDepth6dtz; // Min probe depth for 6-men DTZ
    extern std::shared_ptr<Parameters::SpinParam> minProbeDepth7;    // Min probe depth for 7-men
    extern std::shared_ptr<Parameters::SpinParam> minProbeDepth7dtz; // Min probe depth for 7-men DTZ
}

// ----------------------------------------------------------------------------
// Tuning parameters

const bool useUciParam = false;

extern int pieceValue[Piece::nPieceTypes];

// Evaluation parameters

DECLARE_PARAM(pV, 100, 1, 200, useUciParam);
DECLARE_PARAM(nV, 398, 1, 800, useUciParam);
DECLARE_PARAM(bV, 398, 1, 800, useUciParam);
DECLARE_PARAM(rV, 607, 1, 1200, useUciParam);
DECLARE_PARAM(qV, 1254, 1, 2400, useUciParam);
DECLARE_PARAM(kV, 9900, 9900, 9900, false); // Used by SEE algorithm but not included in board material sums

DECLARE_PARAM(knightVsQueenBonus1, 125, 0, 200, useUciParam);
DECLARE_PARAM(knightVsQueenBonus2, 380, 0, 600, useUciParam);
DECLARE_PARAM(knightVsQueenBonus3, 405, 0, 800, useUciParam);
DECLARE_PARAM(krkpBonus,           107, 0, 400, useUciParam);
DECLARE_PARAM(krpkbBonus,          131, -200, 200, useUciParam);
DECLARE_PARAM(krpkbPenalty,        69,  0, 128, useUciParam);
DECLARE_PARAM(krpknBonus,          149, 0, 400, useUciParam);

extern ParamTable<10> halfMoveFactor;


// Search parameters

DECLARE_PARAM(aspirationWindow, 9, 1, 100, useUciParam);
DECLARE_PARAM(rootLMRMoveCount, 2, 0, 100, useUciParam);

DECLARE_PARAM(razorMargin1, 86, 1, 500, useUciParam);
DECLARE_PARAM(razorMargin2, 353, 1, 1000, useUciParam);

DECLARE_PARAM(reverseFutilityMargin1, 102, 1, 1000, useUciParam);
DECLARE_PARAM(reverseFutilityMargin2, 210, 1, 1000, useUciParam);
DECLARE_PARAM(reverseFutilityMargin3, 267, 1, 2000, useUciParam);
DECLARE_PARAM(reverseFutilityMargin4, 394, 1, 3000, useUciParam);

DECLARE_PARAM(futilityMargin1,  61, 1,  500, useUciParam);
DECLARE_PARAM(futilityMargin2, 144, 1,  500, useUciParam);
DECLARE_PARAM(futilityMargin3, 268, 1, 1000, useUciParam);
DECLARE_PARAM(futilityMargin4, 334, 1, 1000, useUciParam);

DECLARE_PARAM(lmpMoveCountLimit1,  3, 1, 256, useUciParam);
DECLARE_PARAM(lmpMoveCountLimit2,  6, 1, 256, useUciParam);
DECLARE_PARAM(lmpMoveCountLimit3, 12, 1, 256, useUciParam);
DECLARE_PARAM(lmpMoveCountLimit4, 24, 1, 256, useUciParam);

DECLARE_PARAM(lmrMoveCountLimit1,  3, 1, 256, useUciParam);
DECLARE_PARAM(lmrMoveCountLimit2, 12, 1, 256, useUciParam);

DECLARE_PARAM(quiesceMaxSortMoves, 8, 0, 256, useUciParam);
DECLARE_PARAM(deltaPruningMargin, 152, 0, 1000, useUciParam);


// Time management parameters

DECLARE_PARAM(timeMaxRemainingMoves, 35, 2, 200, useUciParam);
DECLARE_PARAM(bufferTime, 1000, 1, 10000, true);
DECLARE_PARAM(minTimeUsage, 85, 1, 100, useUciParam);
DECLARE_PARAM(maxTimeUsage, 400, 100, 1000, useUciParam);
DECLARE_PARAM(timePonderHitRate, 35, 0, 99, useUciParam);


#endif /* PARAMETERS_HPP_ */
