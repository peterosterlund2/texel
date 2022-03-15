/*
    Texel - A UCI chess engine.
    Copyright (C) 2021  Peter Österlund, peterosterlund2@gmail.com

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
 * cspsolver.cpp
 *
 *  Created on: Nov 21, 2021
 *      Author: petero
 */

#include "cspsolver.hpp"
#include "bitBoard.hpp"
#include <cassert>

#include <iostream>


//#define CSPDEBUG

#ifdef CSPDEBUG
#define LOG(x) log << x << std::endl
template <int N, int offs>
static std::string bits(const BitSet<N,offs>& mask) {
    std::stringstream ss;
    ss << "{";
    bool first = true;
    for (int i = offs; i < offs+N; i++) {
        if (mask.getBit(i)) {
            if (!first) {
                ss << ",";
            }
            first = false;
            ss << " " << i;
        }
    }
    ss << " }";
    return ss.str();
}
#else
#define LOG(x) do { } while (false)
#endif


CspSolver::CspSolver(std::ostream& log, bool silent)
    : log(log), silent(silent) {
}

int
CspSolver::addVariable(PrefVal pref, int minVal, int maxVal) {
    int id = domain.size();
    assert(minVal >= Domain::minAllowed);
    assert(minVal < Domain::minAllowed + Domain::numBits);
    assert(maxVal >= Domain::minAllowed);
    assert(maxVal < Domain::minAllowed + Domain::numBits);

    Domain d;
    d.setRange(minVal, maxVal);
    domain.push_back(d);
    prefVal.push_back(pref);

    LOG("v:" << id << " pref:" << static_cast<int>(pref) << " d:" << bits(d));
    return id;
}

void
CspSolver::makeEven(int varNo) {
    LOG("v" << varNo << " even");
    domain[varNo].removeOdd();
}

void
CspSolver::makeOdd(int varNo) {
    LOG("v" << varNo << " odd");
    domain[varNo].removeEven();
}

void
CspSolver::addMinVal(int varNo, int minVal) {
    LOG("v" << varNo << " >= " << minVal);
    domain[varNo].removeSmaller(minVal);
}

void
CspSolver::addMaxVal(int varNo, int maxVal) {
    LOG("v" << varNo << " <= " << maxVal);
    domain[varNo].removeLarger(maxVal);
}

void
CspSolver::addIneq(int v1, Oper op, int v2, int offs) {
#ifdef CSPDEBUG
    int id = constr.size();
    log << "id:" << id << " v" << v1 << (op == LE ? " <= v" : " >= v") << v2;
    if (offs < 0)
        log << " - " << (-offs);
    else if (offs > 0)
        log << " + " << offs;
    log << std::endl;
#endif

    if (op == GE) {
        std::swap(v1, v2);
        offs = -offs;
    }

    const int nVars = domain.size();
    assert(v1 < nVars);
    assert(v2 < nVars);

    constr.emplace_back(v1, v2, offs);
}

bool
CspSolver::solve(std::vector<int>& values) {
    nodes = 0;
    const int nVars = domain.size();
    values.assign(nVars, -1);
    if (nVars == 0)
        return true;

    if (!silent)
        log << "nVars:" << domain.size() << " nConstr:" << constr.size() << std::endl;
    assert(constr.size() <= ConstrSet::numBits);

    varToConstr.assign(nVars, ConstrSet());
    const int nConstr = constr.size();
    for (int ci = 0; ci < nConstr; ci++) {
        const Constraint& c = constr[ci];
        varToConstr[c.v1].setBit(ci);
        varToConstr[c.v2].setBit(ci);
    }

    if (!makeArcConsistent())
        return false;

    bool ret = solveRecursive(0, values);
    if (!silent)
        log << "CSP nodes: " << nodes << std::endl;
    return ret;
}

bool
CspSolver::solveRecursive(int varNo, std::vector<int>& values) {
    nodes++;
    const int nValues = values.size();
    const PrefVal pref = prefVal[varNo];
    Domain d = domain[varNo];
    while (!d.empty()) {
        int val = getBitVal(d, pref);
        d.clearBit(val);
        values[varNo] = val;
        ConstrSet constrMask = varToConstr[varNo];
        bool allValid = true;
        while (!constrMask.empty()) {
            int ci = constrMask.getMinBit();
            constrMask.clearBit(ci);
            const Constraint& c = constr[ci];
            if (c.v1 <= varNo && c.v2 <= varNo) {
                if (values[c.v1] > values[c.v2] + c.c) {
                    allValid = false;
                    break;
                }
            }
        }
        if (allValid) {
            if (varNo == nValues - 1)
                return true;
            bool ret = solveRecursive(varNo + 1, values);
            if (ret)
                return true;
        }
    }
    values[varNo] = -1;
    return false;
}

int
CspSolver::getBitVal(CspSolver::Domain d, CspSolver::PrefVal pref) {
    switch (pref) {
    case PrefVal::SMALL:
        return d.getMinBit();
    case PrefVal::LARGE:
        return d.getMaxBit();
    case PrefVal::MIDDLE_SMALL:
        for (int b = 3; b >= 1; b--)
            if (d.getBit(b))
                return b;
        return d.getMinBit();
    case PrefVal::MIDDLE_LARGE:
        for (int b = 4; b <= 6; b++)
            if (d.getBit(b))
                return b;
        return d.getMaxBit();
    }
    assert(false);
}

bool
CspSolver::makeArcConsistent() {
    ConstrSet constrMask;
    constrMask.setRange(0, constr.size() - 1);
    while (!constrMask.empty()) {
        int ci = constrMask.getMinBit();
        const Constraint& c = constr[ci];

        for (int vi = 0; vi < 2; vi++) {
            int v = vi == 0 ? c.v1 : c.v2;
            Domain dOld = domain[v];
            Domain d = dOld;
            if (vi == 0) {
                int maxVal = domain[c.v2].getMaxBit() + c.c;
                if (maxVal >= minAllowedValue + Domain::numBits)
                    continue;
                if (maxVal < minAllowedValue)
                    return false;
                d.removeLarger(maxVal);
            } else {
                int minVal = domain[c.v1].getMinBit() - c.c;
                if (minVal <= minAllowedValue)
                    continue;
                if (minVal >= minAllowedValue + Domain::numBits)
                    return false;
                d.removeSmaller(minVal);
            }
            if (d != dOld) {
                LOG("ci:" << ci << " v:" << v << " dOld:" << bits(dOld) << " d:" << bits(d));
                if (d.empty())
                    return false;
                domain[v] = d;
                constrMask |= varToConstr[v];
            }
        }
        constrMask.clearBit(ci);
    }

    return true;
}
