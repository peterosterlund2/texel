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
 * bookgui.hpp
 *
 *  Created on: Apr 2, 2016
 *      Author: petero
 */

#ifndef BOOKGUI_HPP_
#define BOOKGUI_HPP_

#include <gtkmm.h>
#include "bookbuildcontrol.hpp"
#include "gametree.hpp"

class BookGui : public BookBuildControl::ChangeNotifier {
public:
    BookGui(Glib::RefPtr<Gtk::Application> app);
    BookGui(const BookGui& other) = delete;
    BookGui& operator=(const BookGui& other) = delete;

    void run();

private:
    void connectSignals();

    /** Called by book builder worker thread when book state has changed. */
    void notify();

    /** Called by GUI thread when notify() is called by worker thread. */
    void bookStateChanged();

    void updateBoardAndTree();
    void updateQueueView();
    void updatePVView();
    void updateEnabledState();

    void newBook();
    void openBookFile();
    void saveBookFile();
    bool saveBookFileAs(); // Return true if book was saved
    void quit();
    bool deleteEvent(_GdkEventAny* e);
    bool askSaveIfDirty(); // Return false if user canceled


    Glib::RefPtr<Gtk::Application> app;
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Window* mainWindow;

    Glib::Dispatcher dispatcher;

    // Menu items
    Gtk::MenuItem* newItem = nullptr;
    Gtk::MenuItem* openItem = nullptr;
    Gtk::MenuItem* saveItem = nullptr;
    Gtk::MenuItem* saveAsItem = nullptr;
    Gtk::MenuItem* quitItem = nullptr;

    BookBuildControl bbControl;
    Position pos;            // Position corresponding to chess board and tree view.
    std::vector<Move> moves; // Moves leading to pos.
    GameTree gameTree;       // Game tree corresponding to the PGN view.
    BookBuildControl::Params searchParams; // Search parameters.
    bool loadingBook; // True when loading a book file.
    bool searching;   // True when book building threads are running.
    bool analysing;   // True when analysis thread is running.
    bool bookDirty;   // True if book is unsaved.
};

#endif /* BOOKGUI_HPP_ */
