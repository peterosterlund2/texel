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
 * proofgamefilter.hpp
 *
 *  Created on: Dec 24, 2021
 *      Author: petero
 */

#ifndef PROOFGAMEFILTER_HPP_
#define PROOFGAMEFILTER_HPP_

#include "util/util.hpp"
#include "proofgame.hpp"

#include <string>
#include <vector>
#include <map>

class MultiBoard;

/** Finds proof games for a list of positions in a text file. */
class ProofGameFilter {
    friend class ProofGameTest;
public:
    /** Constructor. */
    ProofGameFilter(int nWorkers = 1, U64 rndSeed = 0);

    /** Read a list of FENs from a stream and classify them as legal/illegal/unknown
     *  with regards to reachability from the starting position. */
    void filterFens(std::istream& is, std::ostream& os, bool retry = false);

    /** Read a list of FENs from a stream and classify them as legal/illegal/unknown
     *  with regards to reachability from the starting position. The classification
     *  iteratively tries more and more expensive computations to determine the status.
     *  The result from each iteration is written to a file with name outFileBaseName + NN,
     *  where NN is 00, 01, ..., for the different iterations. */
    void filterFensIterated(std::istream& is, const std::string& outFileBaseName,
                            bool retry);

private:
    /** Type of information that can be attached to a position in a line in a text file. */
    enum Info {
        ILLEGAL,       // Position known to be illegal
        UNKNOWN,       // Not known whether position is legal or illegal
        LEGAL,         // Position known to be legal

        FORCED,        // For illegal positions, list of forced initial captures

        KERNEL,        // For unknown positions, proof kernel
        EXT_KERNEL,    // For unknown positions, extended proof kernel
        PATH,          // For unknown positions, legal chess moves corresponding to extended kernel
        STATUS,        // For unknown positions, status of attempts to find proof game
        FAIL,          // For unknown positions, algorithm failed to find a PATH or PROOF
        INFO,          // For unknown positions, reason for not yet finding a PATH or PROOF

        PROOF,         // Legal chess moves (proof game) leading to the goal position
    };

    /** Status of legality classification. */
    enum class Legality {
        INITIAL,   // No computation performed yet
        ILLEGAL,   // Position is illegal
        FAIL,      // Position is unknown, more computation time will not help
        KERNEL,    // Position is unknown, a proof kernel has been found
        PATH,      // Position is unknown, a path corresponding to proof kernel has been found
        LEGAL,     // Position is legal, a proof game has been found
        nLegality
    };

    struct Line {
        std::string fen;
        std::map<Info, std::vector<std::string>> data;

        /** Read data from a line of input from "is". */
        bool read(std::istream& is);
        /** Write data as one line of text to "os". */
        void write(std::ostream& os) const;

        /** Return legality status. */
        Legality getStatus() const;

        /** Return true if there is data for a given token type. */
        bool hasToken(Info tokType) const;

        /** Get data for a given token type. */
        std::vector<std::string>& tokenData(Info tokType);

        /** Remove data for a given token type. */
        void eraseToken(Info tokType);

        /** Return a status value with given name, or default value
         *  if not present in this object. */
        int getStatusInt(const std::string& name, int defVal) const;
        /** Add or update status value with given name. */
        void setStatusInt(const std::string& name, int value);
    };

    /** Process all lines in "is" and write result to "os".
     *  Return true if any work remains to be done. */
    bool runOneIteration(std::istream& is, std::ostream& os, bool firstIteration,
                         bool showProgress, bool retry, int& maxNodesAllPos);

    /** Determine if position is illegal, unknown or legal, based on existence
     *  of an extended proof kernel. */
    void computeExtProofKernel(const Position& startPos, Line& line,
                               std::ostream& log) const;

    /** Try to find a sequence of moves from "start" to "goal" using the
     *  ProofGame class. */
    int pgSearch(const std::string& start, const std::string& goal,
                 const std::vector<Move>& initialPath, std::ostream& log,
                 bool useNonForcedIrreversible,
                 ProofGame::Options& opts, ProofGame::Result& result) const;

    /** Compute a sequence of moves corresponding to an extended proof kernel.
     *  This computation can fail even if a solution exists.
     *  Return true if any work remains to be done. */
    bool computePath(const Position& startPos, Line& line,
                     std::ostream& log) const;

    /** For pawns on first/last row, replace them with suitable promoted pieces. */
    void decidePromotions(std::vector<MultiBoard>& brdVec, const Position& initPos,
                          const Position& goalPos) const;

    struct PathOptions {
        int maxNodes;
        int weightA;
        int weightB;
    };

    /** Compute a sequence of moves from brdVec[startIdx] to brdVec[endIdx], appending
     *  the required moves to "path". */
    void computePath(std::vector<MultiBoard>& brdVec, int startIdx, int endIdx,
                     const Position& initPos, const Position& goalPos,
                     const PathOptions& pathOpts, std::vector<Move>& path,
                     std::ostream& log) const;

    /** If pieces need to move away from their original position, try to advance
     *  suitable pawns to allow the pieces to move. */
    void freePieces(std::vector<MultiBoard>& brdVec, int startIdx,
                    const Position& initPos, const Position& goalPos) const;

    /** Try to compute a proof game starting with a given path and ending in a
     *  goal position, both specified by "line". The STATUS data in "line" keeps
     *  track of how much effort has been spent so far on the computation.
     *  This method has three possible outcomes:
     *  1. LEGAL, PROOF    : A proof game is found.
     *  2. UNKNOWN, FAIL   : A proof game is not found and more computation will not help.
     *  3. UNKNOWN, STATUS : A proof game is not found but searching more might help.
     *  Return true if any work remains to be done. */
    bool computeProofGame(const Position& startPos, Line& line,
                          std::ostream& log) const;

    /** Data for conversion between Info enum and string representation. */
    struct InfoData {
        ProofGameFilter::Info info;
        std::string tokenStr;
    };
    static std::vector<InfoData> infoStrData;
    static void init();

    /** Convert between string and Info enum value. */
    static Info str2Info(const std::string& infoStr);
    static std::string info2Str(Info info);

    const int nWorkers; // Number of worker threads to use
    const int rndSeed;  // Random number seed
    double startTime;   // Time when object was constructed
    int statusCnt[(int)Legality::nLegality] = { 0 };
};

/** A chess board where each square can have more than one piece.
 *  Only the position of pieces are stored in this class. Other things
 *  like side to move, castling rights, etc are ignored. */
class MultiBoard {
    friend class ProofGameTest;
public:
    /** Constructor. Create empty board. */
    MultiBoard();
    /** Constructor. Create from "pos". */
    MultiBoard(const Position& pos);

    /** Get number of pieces on a square. */
    int nPieces(int square) const;
    /** Get a piece from a square. 0 <= pieceNo < nPieces(square). */
    int getPiece(int square, int pieceNo) const;
    /** Return true if there is a piece of a certain type on a square. */
    bool hasPiece(int square, int piece) const;
    /** Return number of pieces of a given type on a square. */
    int nPiecesOfType(int square, int piece) const;

    /** Add a piece to a square. */
    void addPiece(int square, int piece);
    /** Remove a piece from a square. */
    void removePieceType(int square, int piece);
    /** Remove a piece from a square. 0 <= pieceNo < nPieces(square). */
    void removePieceNo(int square, int pieceNo);

    /** Move pieces so there is at most one piece on each square.
     *  Also move kings out of check. */
    void expel();

    /** Return true if it is possible to push a pawn at least up to toSq without
     *  interference from any other pawn. Also return true if there is no pawn
     *  that needs pushing. */
    bool canMovePawn(bool white, int toSq) const;

    /** If there is a piece of type oldPiece on square, replace it with newPiece
     *  and return true. Otherwise return false. */
    bool replacePiece(int square, int oldPiece, int newPiece);

    /** Copy piece configuration to "pos". Assumes there is at most
     *  one piece in each square. */
    void toPos(Position& pos);

private:
    static const int maxPerSquare = 8;
    int squares[64][maxPerSquare + 1];
};


inline bool
ProofGameFilter::Line::hasToken(Info tokType) const {
    return data.find(tokType) != data.end();
}

inline std::vector<std::string>&
ProofGameFilter::Line::tokenData(Info tokType) {
    return data[tokType];
}

inline void
ProofGameFilter::Line::eraseToken(Info tokType) {
    data.erase(tokType);
}

#endif /* PROOFGAMEFILTER_HPP_ */
