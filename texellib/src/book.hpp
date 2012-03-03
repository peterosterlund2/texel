/*
 * book.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BOOK_HPP_
#define BOOK_HPP_

#include "move.hpp"
#include "util.hpp"

#include <map>
#include <vector>


/**
 * Implements an opening book.
 */
class Book {
public:
    struct BookEntry {
        Move move;
        int count;
        BookEntry(const Move& m) {
            move = m;
            count = 1;
        }
    };

private:
    static std::map<U64, std::vector<BookEntry> > bookMap;
//    static Random rndGen;
    static int numBookMoves;
    bool verbose;

public:
    Book(bool verbose) {
        this->verbose = verbose;
    }

#if 0
    private final void initBook() {
        if (numBookMoves >= 0)
            return;
        U64 t0 = currentTimeMillis();
        bookMap = new HashMap<U64, List<BookEntry>>();
        rndGen = new SecureRandom();
        rndGen.setSeed(currentTimeMillis());
        numBookMoves = 0;
        try {
            InputStream inStream = getClass().getResourceAsStream("/book.bin");
            List<Byte> buf = new ArrayList<Byte>(8192);
            byte[] tmpBuf = new byte[1024];
            while (true) {
                int len = inStream.read(tmpBuf);
                if (len <= 0) break;
                for (int i = 0; i < len; i++)
                    buf.add(tmpBuf[i]);
            }
            inStream.close();
            Position startPos = TextIO::readFEN(TextIO::startPosFEN);
            Position pos = new Position(startPos);
            UndoInfo ui = new UndoInfo();
            int len = buf.size();
            for (int i = 0; i < len; i += 2) {
                int b0 = buf.get(i); if (b0 < 0) b0 += 256;
                int b1 = buf.get(i+1); if (b1 < 0) b1 += 256;
                int move = (b0 << 8) + b1;
                if (move == 0) {
                    pos = new Position(startPos);
                } else {
                    bool bad = ((move >> 15) & 1) != 0;
                    int prom = (move >> 12) & 7;
                    Move m(move & 63, (move >> 6) & 63,
                           promToPiece(prom, pos.whiteMove));
                    if (!bad)
                        addToBook(pos, m);
                    pos.makeMove(m, ui);
                }
            }
        } catch (ChessParseError ex) {
            throw new RuntimeException();
        } catch (IOException ex) {
            printf("Can't read opening book resource\n");
            throw new RuntimeException();
        }
        if (verbose) {
            U64 t1 = currentTimeMillis();
            printf("Book moves:%d (parse time:%.3f)%n", numBookMoves,
                    (t1 - t0) / 1000.0);
        }
    }

    /** Add a move to a position in the opening book. */
    private final void addToBook(const Position& pos, Move moveToAdd) {
        List<BookEntry> ent = bookMap.get(pos.zobristHash());
        if (ent == null) {
            ent = new ArrayList<BookEntry>();
            bookMap.put(pos.zobristHash(), ent);
        }
        for (int i = 0; i < ent.size(); i++) {
            BookEntry be = ent.get(i);
            if (be.move.equals(moveToAdd)) {
                be.count++;
                return;
            }
        }
        BookEntry be = new BookEntry(moveToAdd);
        ent.add(be);
        numBookMoves++;
    }

    /** Return a random book move for a position, or null if out of book. */
    public final Move getBookMove(const Position& pos) {
        initBook();
        List<BookEntry> bookMoves = bookMap.get(pos.zobristHash());
        if (bookMoves == null) {
            return null;
        }

        MoveGen::MoveList legalMoves;
	MoveGen::pseudoLegalMoves(pos, legalMoves);
        MoveGen::removeIllegal(pos, legalMoves);
        int sum = 0;
        for (int i = 0; i < bookMoves.size(); i++) {
            BookEntry be = bookMoves.get(i);
            bool contains = false;
            for (int mi = 0; mi < legalMoves.size; mi++)
                if (legalMoves.m[mi].equals(be.move)) {
                    contains = true;
                    break;
                }
            if  (!contains) {
                // If an illegal move was found, it means there was a hash collision.
                return null;
            }
            sum += getWeight(bookMoves.get(i).count);
        }
        if (sum <= 0) {
            return null;
        }
        int rnd = rndGen.nextInt(sum);
        sum = 0;
        for (int i = 0; i < bookMoves.size(); i++) {
            sum += getWeight(bookMoves.get(i).count);
            if (rnd < sum) {
                return bookMoves.get(i).move;
            }
        }
        // Should never get here
        throw new RuntimeException();
    }

    final private int getWeight(int count) {
        double tmp = Math.sqrt(count);
        return (int)(tmp * Math.sqrt(tmp) * 100 + 1);
    }

    /** Return a string describing all book moves. */
    public final std::string getAllBookMoves(const Position& pos) {
        initBook();
        StringBuilder ret = new StringBuilder();
        List<BookEntry> bookMoves = bookMap.get(pos.zobristHash());
        if (bookMoves != null) {
            for (BookEntry be : bookMoves) {
		std::string moveStr = TextIO::moveToString(pos, be.move, false);
                ret.append(moveStr);
                ret.append("(");
                ret.append(be.count);
                ret.append(") ");
            }
        }
        return ret.toString();
    }

    /** Creates the book.bin file. */
    public static void main(String[] args) throws IOException {
        List<Byte> binBook = createBinBook();
        FileOutputStream out = new FileOutputStream("../src/book.bin");
        int bookLen = binBook.size();
        byte[] binBookA = new byte[bookLen];
        for (int i = 0; i < bookLen; i++)
            binBookA[i] = binBook.get(i);
        out.write(binBookA);
        out.close();
    }

    public static List<Byte> createBinBook() {
        List<Byte> binBook = new ArrayList<Byte>(0);
        try {
            InputStream inStream = new Object().getClass().getResourceAsStream("/book.txt");
            InputStreamReader inFile = new InputStreamReader(inStream);
            BufferedReader inBuf = new BufferedReader(inFile);
            LineNumberReader lnr = new LineNumberReader(inBuf);
	    std::string line;
            while ((line = lnr.readLine()) != null) {
                if (line.startsWith("#") || (line.length() == 0)) {
                    continue;
                }
                if (!addBookLine(line, binBook)) {
                    printf("Book parse error, line:%d\n", lnr.getLineNumber());
                    throw new RuntimeException();
                }
//              printf("no:%d line:%s%n", lnr.getLineNumber(), line);
            }
            lnr.close();
        } catch (ChessParseError ex) {
            throw new RuntimeException();
        } catch (IOException ex) {
            printf("Can't read opening book resource\n");
            throw new RuntimeException();
        }
        return binBook;
    }

    /** Add a sequence of moves, starting from the initial position, to the binary opening book. */
    private static bool addBookLine(const std::string& line, List<Byte> binBook) throws ChessParseError {
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        UndoInfo ui = new UndoInfo();
        String[] strMoves = line.split(" ");
        for (std::string strMove : strMoves) {
//            printf("Adding move:%s\n", strMove);
            int bad = 0;
            if (strMove.endsWith("?")) {
                strMove = strMove.substring(0, strMove.length() - 1);
                bad = 1;
            }
            Move m = TextIO::stringToMove(pos, strMove);
            if (m.isEmpty())
                return false;
            int prom = pieceToProm(m.promoteTo);
            int val = m.from + (m.to() << 6) + (prom << 12) + (bad << 15);
            binBook.add((byte)(val >> 8));
            binBook.add((byte)(val & 255));
            pos.makeMove(m, ui);
        }
        binBook.add((byte)0);
        binBook.add((byte)0);
        return true;
    }

    private static int pieceToProm(int p) {
        switch (p) {
        case Piece::WQUEEN: case Piece::BQUEEN:
            return 1;
        case Piece::WROOK: case Piece::BROOK:
            return 2;
        case Piece::WBISHOP: case Piece::BBISHOP:
            return 3;
        case Piece::WKNIGHT: case Piece::BKNIGHT:
            return 4;
        default:
            return 0;
        }
    }

    private static int promToPiece(int prom, bool whiteMove) {
        switch (prom) {
        case 1: return whiteMove ? Piece::WQUEEN : Piece::BQUEEN;
        case 2: return whiteMove ? Piece::WROOK  : Piece::BROOK;
        case 3: return whiteMove ? Piece::WBISHOP : Piece::BBISHOP;
        case 4: return whiteMove ? Piece::WKNIGHT : Piece::BKNIGHT;
        default: return Piece::EMPTY;
        }
    }
#endif
};

#endif /* BOOK_HPP_ */
