/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * chessboard.hpp
 *
 *  Created on: Apr 10, 2016
 *      Author: petero
 */

#ifndef CHESSBOARD_HPP_
#define CHESSBOARD_HPP_

#include <gtkmm.h>
#include "position.hpp"

/** Handles drawing of the chess board and event processing for letting the user
 *  make moves on the board. */
class ChessBoard {
public:
    /** Constructor. */
    ChessBoard(const Position& pos, Gtk::DrawingArea* drawArea);
    ChessBoard(const ChessBoard& other) = delete;
    ChessBoard& operator=(const ChessBoard& other) = delete;

    /** Destructor. */
    ~ChessBoard();

    /** Redraw the chess board. */
    void queueDraw();

private:
    /** Draw chess board and pieces. */
    bool draw(const Cairo::RefPtr<Cairo::Context>& ctx);

    void drawPiece(const Cairo::RefPtr<Cairo::Context>& ctx,
                   double xCrd, double yCrd, double sqSize,
                   int piece);


    const Position& pos;
    Gtk::DrawingArea* drawArea;

    FT_Library ftLib;
    FT_Face ftFace;
    Cairo::RefPtr<Cairo::FtFontFace> fontFace;
};

#endif /* CHESSBOARD_HPP_ */
