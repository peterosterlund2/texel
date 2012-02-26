/*
 * moveGenTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "moveGenTest.hpp"

#include "cute.h"

#if 0
    /**
     * Test of pseudoLegalMoves method, of class MoveGen.
     */
    @Test
    public void testPseudoLegalMoves() throws ChessParseError {
	std::string fen = "8/3k4/8/2n2pP1/1P6/1NB5/2QP4/R3K2R w KQ f6 0 2";
        Position pos = TextIO::readFEN(fen);
        ASSERT_EQUAL(fen, TextIO::toFEN(pos));
        List<String> strMoves = getMoveList(pos, false);
        ASSERT(strMoves.contains("a1d1"));
        ASSERT(!strMoves.contains("a1e1"));
        ASSERT(!strMoves.contains("a1f1"));
        ASSERT(strMoves.contains("a1a7"));
        ASSERT(strMoves.contains("e1f2"));
        ASSERT(!strMoves.contains("e1g3"));
        ASSERT(strMoves.contains("c3f6"));
        ASSERT(!strMoves.contains("b3d2"));

        // Test castling
        ASSERT(strMoves.contains("e1g1"));
        ASSERT(strMoves.contains("e1c1"));
        ASSERT_EQUAL(49, strMoves.size());

        pos.setPiece(Position::getSquare(4,3), Piece::BROOK);
        strMoves = getMoveList(pos, false);
        ASSERT(!strMoves.contains("e1g1"));      // In check, no castling possible
        ASSERT(!strMoves.contains("e1c1"));

        pos.setPiece(Position::getSquare(4, 3), Piece::EMPTY);
        pos.setPiece(Position::getSquare(5, 3), Piece::BROOK);
        strMoves = getMoveList(pos, false);
        ASSERT(!strMoves.contains("e1g1"));      // f1 attacked, short castle not possible
        ASSERT(strMoves.contains("e1c1"));

        pos.setPiece(Position::getSquare(5, 3), Piece::EMPTY);
        pos.setPiece(Position::getSquare(6, 3), Piece::BBISHOP);
        strMoves = getMoveList(pos, false);
        ASSERT(strMoves.contains("e1g1"));      // d1 attacked, long castle not possible
        ASSERT(!strMoves.contains("e1c1"));

        pos.setPiece(Position::getSquare(6, 3), Piece::EMPTY);
        pos.setCastleMask(1 << Position::A1_CASTLE);
        strMoves = getMoveList(pos, false);
        ASSERT(!strMoves.contains("e1g1"));      // short castle right has been lost
        ASSERT(strMoves.contains("e1c1"));
    }

    /**
     * Test of pseudoLegalMoves method, of class MoveGen. Pawn moves.
     */
    @Test
    public void testPawnMoves() throws ChessParseError {
        std::string fen = "1r2k3/P1pppp1p/8/1pP3p1/1nPp2P1/n4p1P/1P2PP2/4KBNR w K b6 0 1";
        Position pos = TextIO::readFEN(fen);
        ASSERT_EQUAL(fen, TextIO::toFEN(pos));
        List<std::string> strMoves = getMoveList(pos, false);
        ASSERT(strMoves.contains("c5b6"));     // En passant capture
        ASSERT(strMoves.contains("a7a8q"));    // promotion
        ASSERT(strMoves.contains("a7a8n"));    // under promotion
        ASSERT(strMoves.contains("a7b8r"));   // capture promotion
        ASSERT(strMoves.contains("b2b3"));     // pawn single move
        ASSERT(strMoves.contains("b2a3"));     // pawn capture to the left
        ASSERT(strMoves.contains("e2e4"));     // pawn double move
        ASSERT(strMoves.contains("e2f3"));     // pawn capture to the right
        ASSERT_EQUAL(22, strMoves.size());

        pos.setEpSquare(-1);
        strMoves = getMoveList(pos, false);
        ASSERT_EQUAL(21, strMoves.size());          // No ep, one less move possible
        
        // Check black pawn moves
        pos.setWhiteMove(false);
        strMoves = getMoveList(pos, false);
        ASSERT(strMoves.contains("f3e2"));
        ASSERT(strMoves.contains("d4d3"));
        ASSERT(strMoves.contains("e7e6"));
        ASSERT(strMoves.contains("e7e5"));
        ASSERT_EQUAL(28, strMoves.size());

        // Check black pawn promotion
        pos.setPiece(Position::getSquare(0,1), Piece::BPAWN);
        strMoves = getMoveList(pos, false);
        ASSERT(strMoves.contains("a2a1q"));
        ASSERT(strMoves.contains("a2a1r"));
        ASSERT(strMoves.contains("a2a1n"));
        ASSERT(strMoves.contains("a2a1b"));
    }

    /**
     * Test of inCheck method, of class MoveGen.
     */
    @Test
    public void testInCheck() {
        Position pos = new Position();
        pos.setPiece(Position::getSquare(4,2), Piece::WKING);
        pos.setPiece(Position::getSquare(4,7), Piece::BKING);
        ASSERT_EQUAL(false, MoveGen::inCheck(pos));

        pos.setPiece(Position::getSquare(3,3), Piece::BQUEEN);
        ASSERT_EQUAL(true, MoveGen::inCheck(pos));
        pos.setPiece(Position::getSquare(3,3), Piece::BROOK);
        ASSERT_EQUAL(false, MoveGen::inCheck(pos));
        pos.setPiece(Position::getSquare(3,3), Piece::BPAWN);
        ASSERT_EQUAL(true, MoveGen::inCheck(pos));

        pos.setPiece(Position::getSquare(3,3), Piece::EMPTY);
        pos.setPiece(Position::getSquare(5,3), Piece::WQUEEN);
        ASSERT_EQUAL(false, MoveGen::inCheck(pos));
        
        pos.setPiece(Position::getSquare(4, 6), Piece::BROOK);
        ASSERT_EQUAL(true, MoveGen::inCheck(pos));
        pos.setPiece(Position::getSquare(4, 4), Piece::WPAWN);        
        ASSERT_EQUAL(false, MoveGen::inCheck(pos));

        pos.setPiece(Position::getSquare(2, 3), Piece::BKNIGHT);                
        ASSERT_EQUAL(true, MoveGen::inCheck(pos));

        pos.setPiece(Position::getSquare(2, 3), Piece::EMPTY);
        pos.setPiece(Position::getSquare(0, 4), Piece::BKNIGHT);
        ASSERT_EQUAL(false, MoveGen::inCheck(pos));
    }

    /**
     * Test of givesCheck method, of class MoveGen.
     */
    @Test
    public void testGivesCheck() throws ChessParseError {
        Position pos = new Position();
        UndoInfo ui = new UndoInfo();
        pos.setPiece(TextIO::getSquare("e3"), Piece::WKING);
        pos.setPiece(TextIO::getSquare("e8"), Piece::BKING);
        pos.setPiece(TextIO::getSquare("c2"), Piece::WROOK);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Rc8")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Rc6")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Rc7")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Re2")));
        
        pos.setPiece(TextIO::getSquare("c2"), Piece::EMPTY);
        pos.setPiece(TextIO::getSquare("e2"), Piece::WROOK);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kd3")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kd4")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Ke4")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kf2")));

        pos.setPiece(TextIO::getSquare("e4"), Piece::WBISHOP);
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Bd5")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Bc6")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kd3")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Re1")));

        pos = TextIO::readFEN("4k3/3p4/8/8/4B3/2K5/4R3/8 w - - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Bc6")));
        pos = TextIO::readFEN("4k3/8/5K2/8/6N1/8/8/8 w - - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Ke6")));
        ASSERT(!MoveGen::givesCheck(pos, new Move(TextIO::getSquare("f6"),
                                                     TextIO::getSquare("e7"),
                                                     Piece::EMPTY)));

        pos = TextIO::readFEN("8/2k5/8/4N3/8/2K3B1/8/8 w - - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Kc4")));
        pos.setPiece(TextIO::getSquare("g3"), Piece::WROOK);
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        pos.setPiece(TextIO::getSquare("g3"), Piece::WQUEEN);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        pos.setPiece(TextIO::getSquare("g3"), Piece::WKNIGHT);
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        pos.setPiece(TextIO::getSquare("g3"), Piece::WPAWN);
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        pos.setPiece(TextIO::getSquare("c3"), Piece::EMPTY);
        pos.setPiece(TextIO::getSquare("g3"), Piece::WKING);
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        
        pos = TextIO::readFEN("8/2k5/3p4/4N3/8/2K3B1/8/8 w - - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));

        pos = TextIO::readFEN("8/2k5/8/4N3/8/6q1/2K5/8 w - - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        pos = TextIO::readFEN("8/2k5/8/4N3/8/8/2K5/8 w - - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "Nf7")));
        pos = TextIO::readFEN("2nk4/3P4/8/8/3R4/8/2K5/8 w - - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc8N")));

        pos = TextIO::readFEN("8/2k5/2p5/1P1P4/8/2K5/8/8 w - - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "d6")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "bxc6")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b6")));
        
        pos = TextIO::readFEN("8/8/R1PkP2R/8/8/2K5/8/8 w - - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c7")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e7")));

        // Test pawn promotion
        pos = TextIO::readFEN("8/1P6/2kP4/8/8/2K5/8/8 w - - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "d7")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8Q")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8N")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8R")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "b8B")));
        
        pos = TextIO::readFEN("8/2P1P3/2k5/8/8/2K5/8/8 w - - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8Q")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8N")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8R")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "e8B")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8Q")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8N")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8R")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c8B")));

        // Test castling
        pos = TextIO::readFEN("8/8/8/8/5k2/8/8/R3K2R w KQ - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
        pos = TextIO::readFEN("8/8/8/8/6k1/8/8/R3K2R w KQ - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
        pos = TextIO::readFEN("8/8/8/8/3k4/8/8/R3K2R w KQ - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
        pos = TextIO::readFEN("8/8/8/8/5k2/8/5P2/R3K2R w KQ - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
        pos = TextIO::readFEN("8/8/8/8/8/8/8/R3K2k w Q - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
        pos = TextIO::readFEN("8/8/8/8/8/8/8/2k1K2R w K - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
        pos.setPiece(TextIO::getSquare("d1"), Piece::WKNIGHT);
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));

        // Test en passant
        pos = TextIO::readFEN("8/1kp5/8/3P4/8/8/8/4K3 b - - 0 1");
        pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

        pos = TextIO::readFEN("3k4/2p5/8/3P4/8/8/3R4/4K3 b - - 0 1");
        pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));
        
        pos = TextIO::readFEN("5k2/2p5/8/3P4/8/B7/8/4K3 b - - 0 1");
        pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));
        
        pos = TextIO::readFEN("5k2/2p5/8/3P4/1P6/B7/8/4K3 b - - 0 1");
        pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

        pos = TextIO::readFEN("8/2p5/8/R2P1k2/8/8/8/4K3 b - - 0 1");
        pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "dxc6")));

        // Black pawn moves
        pos = TextIO::readFEN("8/2p5/8/R4k2/1K6/8/8/8 b - - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c5")));
        pos = TextIO::readFEN("8/2p5/8/R4k2/2K5/8/8/8 b - - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c5")));
        pos = TextIO::readFEN("8/2p5/8/R4k2/3K4/8/8/8 b - - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "c5")));

        // Black castling
        pos = TextIO::readFEN("r3k2r/8/8/5K2/8/8/8/8 b kq - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
        pos = TextIO::readFEN("r3k2r/8/8/6K1/8/8/8/8 b kq - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O")));
        pos = TextIO::readFEN("r3k2r/8/8/2K5/8/8/8/8 b kq - 0 1");
        ASSERT(!MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));
        pos = TextIO::readFEN("r3k2r/8/8/3K4/8/8/8/8 b kq - 0 1");
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "O-O-O")));

        // Black en passant
        pos = TextIO::readFEN("8/8/4k3/8/4p3/8/5PK1/8 w - - 0 1");
        pos.makeMove(TextIO::stringToMove(pos, "f4"), ui);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "exf3")));

        pos = TextIO::readFEN("8/8/4k3/8/K3p1r1/8/5P2/8 w - - 0 1");
        pos.makeMove(TextIO::stringToMove(pos, "f4"), ui);
        ASSERT(MoveGen::givesCheck(pos, TextIO::stringToMove(pos, "exf3")));
    }

    /**
     * Test of removeIllegal method, of class MoveGen.
     */
    @Test
    public void testRemoveIllegal() throws ChessParseError {
        Position pos = TextIO::readFEN("8/3k4/8/2n1rpP1/1P6/1NB5/2QP4/R3K2R w KQ f6 0 1");
        List<std::string> strMoves = getMoveList(pos, true);
        ASSERT(strMoves.contains("c2e4"));
        ASSERT(strMoves.contains("c3e5"));
        ASSERT(strMoves.contains("e1d1"));
        ASSERT(strMoves.contains("e1f1"));
        ASSERT(strMoves.contains("e1f2"));
        ASSERT_EQUAL(5, strMoves.size());

        pos = TextIO::readFEN("4k3/8/8/2KPp1r1/8/8/8/8 w - e6 0 2");
        strMoves = getMoveList(pos, true);
        ASSERT(!strMoves.contains("d5e6"));
        ASSERT_EQUAL(7, strMoves.size());

        pos = TextIO::readFEN("8/6p1/4p3/2k1Pp1B/4KP1p/6rP/8/8 w - f6 0 55");
        strMoves = getMoveList(pos, true);
        ASSERT(strMoves.contains("e5f6"));
        ASSERT_EQUAL(1, strMoves.size());
    }
    
    /**
     * Test that if king capture is possible, only a king capture move is returned in the move list.
     */
    @Test
    public void testKingCapture() throws ChessParseError {
        Position pos = TextIO::readFEN("8/4k3/8/8/8/8/8/4RK2 b - - 0 1");
        pos.setWhiteMove(true);
        List<std::string> strMoves = getMoveList(pos, false);
        ASSERT_EQUAL(1, strMoves.size());
        ASSERT_EQUAL("e1e7", strMoves.get(0));

        pos.setPiece(Position::getSquare(0, 2), Piece::WBISHOP);
        pos.setPiece(Position::getSquare(4, 1), Piece::WPAWN);
        strMoves = getMoveList(pos, false);
        ASSERT_EQUAL(1, strMoves.size());
        ASSERT_EQUAL("a3e7", strMoves.get(0));

        pos.setPiece(Position::getSquare(1, 3), Piece::WPAWN);
        pos.setPiece(Position::getSquare(5, 5), Piece::WPAWN);
        strMoves = getMoveList(pos, false);
        ASSERT_EQUAL(1, strMoves.size());
        ASSERT_EQUAL("f6e7", strMoves.get(0));
    }

    /** Test that captureList and captureAndcheckList are generated correctly. */
    @Test
    public void testCaptureList() throws ChessParseError {
        Position pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/3p1p2/R7/4N3/8/PPPPQPPP/2B1KB1R w Kkq - 0 1");
        getMoveList(pos, false);
        
        pos = TextIO::readFEN("rnb1kbn1/ppp1qppp/5p2/4p3/3N3r/3P4/PPP2PPP/R1BQKB1R b KQq - 0 1");
        getMoveList(pos, false);
        
        pos = TextIO::readFEN("rnb1k1n1/ppp1qppp/5p2/b3p3/1r1N4/3P4/PPP2PPP/R1BQKB1R b KQq - 0 1");
        getMoveList(pos, false);
        
        pos = TextIO::readFEN("8/8/8/8/3k4/8/4P3/4K3 w - - 0 1");
        getMoveList(pos, false);
        
        pos = TextIO::readFEN("8/8/8/3k4/8/8/4P3/4K3 w - - 0 1");
        getMoveList(pos, false);
        
        pos = TextIO::readFEN("8/8/8/3k4/4p3/8/3KP3/8 b - - 0 1");
        getMoveList(pos, false);

        pos = TextIO::readFEN("3k4/r2p2K1/8/8/8/8/8/8 b - - 0 1");
        getMoveList(pos, false);
    }

    @Test
    public void testCheckEvasions() throws ChessParseError {
        Position pos = TextIO::readFEN("n7/8/8/7k/5pP1/5K2/8/8 b - g3 0 1");
        getMoveList(pos, false);

        pos = TextIO::readFEN("rn1qkbnr/pppB1ppp/3p4/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1");
        getMoveList(pos, false);

        // King captures must be included in check evasions
        pos = TextIO::readFEN("r1bq2r1/pp3pbk/2p1p1P1/8/3P4/2PB1N2/PP3PPR/2KR4 b - - 0 1");
        UndoInfo ui = new UndoInfo();
        pos.makeMove(TextIO::uciStringToMove("g7h6"), ui);
        getMoveList(pos, false);
        List<std::string> evList = getCheckEvasions(pos, false);
        ASSERT(evList.contains("g6h7"));

        pos = TextIO::readFEN("1R6/1brk2p1/2P1p2p/p3Pp2/P7/6P1/1P4P1/2R3K1 b - - 0 1");
        getMoveList(pos, false);
        evList = getCheckEvasions(pos, false);
        ASSERT(evList.contains("b7c6"));
    }

    private List<std::string> getMoveList(const Position& pos, bool onlyLegal) {
        Position swap = EvaluateTest.swapColors(pos);
        List<std::string> swapList = getMoveList0(swap, onlyLegal);
        List<std::string> ret = getMoveList0(pos, onlyLegal);
        ASSERT_EQUAL(swapList.size(), ret.size());
        // FIXME! Test that swapList contains swapped moves compared to ret
        return ret;
    }

    private List<std::string> getMoveList0(const Position& pos, bool onlyLegal) {
        MoveGen moveGen = new MoveGen();
        MoveGen::MoveList moves = moveGen.pseudoLegalMoves(pos);
        if (onlyLegal)
            MoveGen::removeIllegal(pos, moves);
        ArrayList<std::string> strMoves = new ArrayList<std::string>();
        for (int mi = 0; mi < moves.size; mi++) {
            const Move& m = moves.m[mi];
            std::string mStr = TextIO::moveToUCIString(m);
            strMoves.add(mStr);
        }

        List<std::string> capList1 = getCaptureList(pos, false, onlyLegal);
        ASSERT(strMoves.containsAll(capList1));

        List<std::string> capList2 = getCaptureList(pos, true, onlyLegal);
        ASSERT(strMoves.containsAll(capList2));

        List<std::string> evList = getCheckEvasions(pos, onlyLegal);
        if (evList != null)
            ASSERT(strMoves.containsAll(evList));
        UndoInfo ui = new UndoInfo();
        for (std::string sm : strMoves) {
            Move m = TextIO::uciStringToMove(sm);
            if (m != null) {
                pos.makeMove(m, ui);
                bool invalid = MoveGen::canTakeKing(pos);
                pos.unMakeMove(m, ui);
                if (invalid) m = null;
            }
            if (m == null) // Move was illegal (but pseudo-legal)
                continue;
            bool qProm = false; // Promotion types considered in qsearch
            switch (m.promoteTo) {
            case Piece::WQUEEN: case Piece::BQUEEN:
            case Piece::WKNIGHT: case Piece::BKNIGHT:
            case Piece::EMPTY:
                qProm = true;
                break;
            default:
                break;
            }
            if (!MoveGen::canTakeKing(pos) && MoveGen::givesCheck(pos, m)) {
                if (qProm)
                    ASSERT(capList2.contains(sm));
            } else {
                switch (m.promoteTo) {
                case Piece::WQUEEN: case Piece::BQUEEN:
                case Piece::WKNIGHT: case Piece::BKNIGHT:
                    ASSERT(capList1.contains(sm)); // All queen/knight promotions
                    ASSERT(capList2.contains(sm)); // All queen/knight promotions
                    break;
                case Piece::EMPTY:
                    break;
                default:
                    ASSERT(!capList1.contains(sm)); // No rook/bishop promotions
                    ASSERT(!capList2.contains(sm)); // No rook/bishop promotions
                    break;
                }
            }
            if (pos.getPiece(m.to) != Piece::EMPTY) {
                if (qProm) {
                    ASSERT(capList1.contains(sm));
                    ASSERT(capList2.contains(sm));
                }
            }
            if (evList != null) {
                ASSERT(evList.contains(sm));
            }
        }

        return strMoves;
    }

    private List<std::string> getCaptureList(const Position& pos, bool includeChecks, bool onlyLegal) {
        MoveGen::MoveList moves;
        if (includeChecks) {
            moves = new MoveGen().pseudoLegalCapturesAndChecks(pos);
        } else {
            moves = new MoveGen().pseudoLegalCaptures(pos);
        }
        if (onlyLegal)
            MoveGen::removeIllegal(pos, moves);
        ArrayList<std::string> strMoves = new ArrayList<std::string>();
        for (int mi = 0; mi < moves.size; mi++) {
            const Move& m = moves.m[mi];
            std::string mStr = TextIO::moveToUCIString(m);
            strMoves.add(mStr);
        }
        return strMoves;
    }

    private List<std::string> getCheckEvasions(const Position& pos, bool onlyLegal) {
        if (!MoveGen::inCheck(pos))
            return null;
        MoveGen::MoveList moves = new MoveGen().checkEvasions(pos);
        if (onlyLegal)
            MoveGen::removeIllegal(pos, moves);
        ArrayList<std::string> strMoves = new ArrayList<std::string>();
        for (int mi = 0; mi < moves.size; mi++) {
            const Move& m = moves.m[mi];
            std::string mStr = TextIO::moveToUCIString(m);
            strMoves.add(mStr);
        }
        return strMoves;
     }
#endif



cute::suite
MoveGenTest::getSuite() const {
    cute::suite s;
//    s.push_back(CUTE());
    return s;
}
