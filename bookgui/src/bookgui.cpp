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
 * bookgui.cpp
 *
 *  Created on: Apr 2, 2016
 *      Author: petero
 */

#include "bookgui.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.petero.bookgui");
    BookGui bookGui(app);
    bookGui.run();
}

BookGui::BookGui(Glib::RefPtr<Gtk::Application> app0)
    : app(app0), mainWindow(nullptr),
      bbControl(*this),
      gameTree(std::cin), // FIXME!!
      searching(false), analysing(false) {
    builder = Gtk::Builder::create();
    builder->add_from_resource("/main/bookgui_glade.xml");
}

void
BookGui::run() {
    builder->get_widget("mainWindow", mainWindow);
    connectSignals();
    app->run(*mainWindow);
}

void
BookGui::connectSignals() {
    Gtk::MenuItem* quitItem;
    builder->get_widget("quitMenuItem", quitItem);
    quitItem->signal_activate().connect([this]{ mainWindow->hide(); });

    dispatcher.connect(sigc::mem_fun(*this, &BookGui::bookStateChanged));
}

void
BookGui::notify() {
    dispatcher.emit();
}

void
BookGui::bookStateChanged() {
    std::vector<BookBuildControl::Change> changes;
    bbControl.getChanges(changes);
    for (auto& change : changes) {
        switch (change) {
        case BookBuildControl::Change::TREE:
            break;
        case BookBuildControl::Change::QUEUE:
            break;
        case BookBuildControl::Change::PV:
            break;
        }
    }
}
