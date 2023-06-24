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
 * cspsolver.hpp
 *
 *  Created on: Nov 21, 2021
 *      Author: petero
 */

#ifndef CSPSOLVER_HPP_
#define CSPSOLVER_HPP_

#include "bitSet.hpp"
#include <vector>
#include <iostream>

/** Solves constraint satisfaction problems (CSPs) that arise when trying to
 *  determine the ranks at which captures in a proof kernel occurs. */
class CspSolver {
    friend class CspSolverTest;
public:
    CspSolver(std::ostream& log = std::clog, bool silent = false);

    /** Minimum supported variable value. */
    constexpr static int minAllowedValue = -16;

    /** Controls which variable values are tried first when
     *  searching for a solution. */
    enum class PrefVal {
        SMALL,        // Try smaller values before larger
        LARGE,        // Try larger values before smaller
        MIDDLE_SMALL, // Prefer values 3,2,1, then increasing
        MIDDLE_LARGE, // Prefer values 4,5,6, then decreasing
    };

    /** Add a new integer variable. The variable satisfies:
     *  -16 <= minVal <= var <= maxVal < 48.
     *  @return Variable identifier. Zero for first added variable, then
     *          incremented for each additional added variable. */
    int addVariable(PrefVal pref = PrefVal::SMALL, int minVal = 1, int maxVal = 6);

    /** Restrict variable "varNo" to 2*n for some integer n. */
    void makeEven(int varNo);
    /** Restrict variable "varNo" to 2*n+1 for some integer n. */
    void makeOdd(int varNo);

    /** Restrict variable "varNo" to be >= minVal. */
    void addMinVal(int varNo, int minVal);
    /** Restrict variable "varNo" to be <= maxVal. */
    void addMaxVal(int varNo, int maxVal);

    enum Oper { LE, GE };
    /** Add constraint "var_v1 OP var_v2 + offs", where OP is LE (<=) or GE (>=). */
    void addIneq(int v1, Oper op, int v2, int offs);
    /** Add constraint var_v1 = var_v2 + offs. */
    void addEq(int v1, int v2, int offs);

    /** Solve the CSP.
     *  @param values  Set to variable values for a solution if one exists.
     *  @return true if a solution exists, false otherwise. */
    bool solve(std::vector<int>& values);

    /** Return number of search nodes used by solve(). */
    U64 getNumNodes() const;

private:
    using Domain = BitSet<64,minAllowedValue>;

    /** Recursively try to assign valid values to all variables. */
    bool solveRecursive(int varNo, std::vector<int>& values);

    /** Return the first set bit in "d", according to order defined by "pref". */
    int getBitVal(Domain d, CspSolver::PrefVal pref);

    /** Make the CSP problem arc consistent.
     *  @return false if it is found that the CSP has no solution, true otherwise. */
    bool makeArcConsistent();

    std::vector<Domain> domain; // Domain of each variable, i.e. set of potentially legal values
    std::vector<PrefVal> prefVal; // [varNo] preferred values when solving

    /** Represents the constraint var_v1 <= var_v2 + c */
    struct Constraint {
        const int v1;
        const int v2;
        const int c;
        Constraint(int v1, int v2, int c) : v1(v1), v2(v2), c(c) {}
    };
    std::vector<Constraint> constr;

    using ConstrSet = BitSet<64*3>;
    std::vector<ConstrSet> varToConstr; // [varNo] -> bitmask of constraints using varNo

    U64 nodes; // Number of nodes visited by solveRecursive()

    std::ostream& log;
    bool silent;
};

inline void
CspSolver::addEq(int v1, int v2, int offs) {
    addIneq(v1, LE, v2, offs);
    addIneq(v1, GE, v2, offs);
}

inline U64
CspSolver::getNumNodes() const {
    return nodes;
}

#endif /* CSPSOLVER_HPP_ */
