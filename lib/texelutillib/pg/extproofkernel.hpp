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
 * extproofkernel.hpp
 *
 *  Created on: Nov 30, 2021
 *      Author: petero
 */

#ifndef EXTPROOFKERNEL_HPP_
#define EXTPROOFKERNEL_HPP_

#include "proofkernel.hpp"
#include "cspsolver.hpp"

class ExtProofKernelTest;

/** Converts a proof kernel to an extended proof kernel if possible. */
class ExtProofKernel {
    friend class ExtProofKernelTest;
public:
    /** Constructor. */
    ExtProofKernel(const Position& initialPos, const Position& goalPos,
                   std::ostream& log = std::clog, bool silent = false);

    using PieceColor = ProofKernel::PieceColor;
    using PieceType = ProofKernel::PieceType;
    using PkMove = ProofKernel::PkMove;
    using ExtPkMove = ProofKernel::ExtPkMove;
    using PrefVal = CspSolver::PrefVal;

    /** Converts a proof kernel to an extended proof kernel if possible.
     *  @return true if an extended proof kernel exists, false otherwise. */
    bool findExtKernel(const std::vector<PkMove>& path,
                       std::vector<ExtPkMove>& extPath);

    /** Return number of search nodes used by findExtKernel(). */ 
    U64 getNumNodes() const;

private:
    const Position& initialPos;
    const Position& goalPos;
    CspSolver csp;

    struct Pawn {
        Pawn(std::ostream& log, int idx, bool w) : idx(idx), white(w), log(log) {}
        void addVar(int varNo, CspSolver& csp, bool addIneq = true);

        int idx;                  // Index in allPawns
        bool white;               // True for white pawn, false for black
        std::vector<int> varIds;  // All variables associated with this pawn
        std::ostream& log;
    };
    std::vector<Pawn> allPawns;

    class PawnColumn {
    public:
        /** Number of pawns in the column. */
        int nPawns() const;
        /** Get index in "allPawns" for the i:th pawn. */
        int getPawn(int i) const;

        /** Sets the i:th pawn, represented by the index in "allPawns". 0 <= i < nPawns(). */
        void setPawn(int i, int pawnIdx);
        /** Insert a pawn at position "i". 0 <= i <= nPawns(). */
        void addPawn(int i, int pawnIdx);
        /** Remove the i:th pawn. 0 <= i < nPawns(). */
        void removePawn(int i);

    private:
        int data[8];
        int numPawns = 0;
    };
    std::array<PawnColumn, 8> columns;

    /** A square on the chess board with Y position optionally specified by a variable. */
    struct VarSquare {
        int x;    // X position
        int y;    // Y position, or -1 if variable
        int yVar; // Y position variable, or -1 if fixed Y position
    };

    /** Like ExtPkMove but with potentially variable squares. */
    struct ExtMove {
        PieceColor color;
        PieceType movingPiece;
        VarSquare from;
        bool capture;
        VarSquare to;
        PieceType promotedPiece;
    };

    /** Add CSP inequalities corresponding to all pawns in a column. */
    void addColumnIneqs(const PawnColumn& col);

    /** Move all pawns in a column an unspecified amount forward.
     *  Used for making room for a new pawn entering the column. */
    void movePawns(int x, const PawnColumn& col, std::vector<ExtMove>& varExtPath);

    /** For pawns in a file, compute the corresponding Y position in the goal position. */
    void getGoalPawnYPos(int x, int (&goalYPos)[ProofKernel::maxPawns]) const;

    friend std::ostream& operator<<(std::ostream& os, const VarSquare& vsq);
    friend std::ostream& operator<<(std::ostream& os, const ExtMove& move);

    std::ostream& log;
    bool silent;
};

std::ostream& operator<<(std::ostream& os, const ExtProofKernel::VarSquare& vsq);
std::ostream& operator<<(std::ostream& os, const ExtProofKernel::ExtMove& move);


inline U64
ExtProofKernel::getNumNodes() const {
    return csp.getNumNodes();
}

inline int
ExtProofKernel::PawnColumn::nPawns() const {
    return numPawns;
}

inline int
ExtProofKernel::PawnColumn::getPawn(int i) const {
    return data[i];
}

inline void
ExtProofKernel::PawnColumn::setPawn(int i, int pawnIdx) {
    data[i] = pawnIdx;
}

inline void
ExtProofKernel::PawnColumn::addPawn(int i, int pawnIdx) {
    for (int k = numPawns - 1; k >= i; k--)
        data[k + 1] = data[k];
    data[i] = pawnIdx;
    numPawns++;
}

inline void
ExtProofKernel::PawnColumn::removePawn(int i) {
    for (int k = i; k + 1 < numPawns; k++)
        data[k] = data[k + 1];
    numPawns--;
}

#endif /* EXTPROOFKERNEL_HPP_ */
