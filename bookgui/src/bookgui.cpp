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
    app->set_flags(Gio::APPLICATION_NON_UNIQUE);
    BookGui bookGui(app);
    bookGui.run();
}

BookGui::BookGui(Glib::RefPtr<Gtk::Application> app0)
    : app(app0), mainWindow(nullptr),
      bbControl(*this),
      gameTree(std::cin), // FIXME!!
      loadingBook(false), searching(false), analysing(false), bookDirty(false) {
    builder = Gtk::Builder::create();
    builder->add_from_resource("/main/bookgui_glade.xml");
}

void
BookGui::run() {
    builder->get_widget("mainWindow", mainWindow);
    connectSignals();
    updateEnabledState();
    app->run(*mainWindow);
}

// --------------------------------------------------------------------------------

void
BookGui::connectSignals() {
    // Notifications from worker threads
    dispatcher.connect([this]{ bookStateChanged(); });

    // Menu items
    builder->get_widget("newMenuItem", newItem);
    newItem->signal_activate().connect([this]{ newBook(); });

    builder->get_widget("openMenuItem", openItem);
    openItem->signal_activate().connect([this]{ openBookFile(); });

    builder->get_widget("saveMenuItem", saveItem);
    saveItem->signal_activate().connect([this]{ saveBookFile(); });

    builder->get_widget("saveAsMenuItem", saveAsItem);
    saveAsItem->signal_activate().connect([this]{ saveBookFileAs(); });

    builder->get_widget("quitMenuItem", quitItem);
    quitItem->signal_activate().connect([this]{ quit(); });
    mainWindow->signal_delete_event().connect(sigc::mem_fun(*this, &BookGui::deleteEvent));
}

void
BookGui::notify() {
    dispatcher.emit();
}

void
BookGui::bookStateChanged() {
    std::vector<BookBuildControl::Change> changes;
    bbControl.getChanges(changes);
    bool updateEnabled = false;
    for (auto& change : changes) {
        switch (change) {
        case BookBuildControl::Change::TREE:
            bookDirty = true;
            updateBoardAndTree();
            break;
        case BookBuildControl::Change::QUEUE:
            updateEnabled = true;
            updateQueueView();
            break;
        case BookBuildControl::Change::PV:
            updatePVView();
            break;
        case BookBuildControl::Change::LOADING_COMPLETE:
            updateEnabled = true;
            loadingBook = false;
            break;
        }
    }
    if (updateEnabled)
        updateEnabledState();
}

void
BookGui::updateBoardAndTree() {

}

void
BookGui::updateQueueView() {

}

void
BookGui::updatePVView() {

}

void
BookGui::updateEnabledState() {
    bool builderIdle = bbControl.nRunningThreads() == 0 && !loadingBook;
    newItem->set_sensitive(builderIdle);
    openItem->set_sensitive(builderIdle);
    saveItem->set_sensitive(!bbControl.getBookFileName().empty() && !loadingBook);
    saveAsItem->set_sensitive(!loadingBook);
    quitItem->set_sensitive(true);
}

// --------------------------------------------------------------------------------

void
BookGui::newBook() {
    if (bbControl.nRunningThreads() > 0 || loadingBook)
        return;
    if (!askSaveIfDirty())
        return;

    bbControl.newBook();
    pos = TextIO::readFEN(TextIO::startPosFEN);
    moves.clear();
    bookDirty = false;
    updateBoardAndTree();
    if (analysing)
        bbControl.startAnalysis(pos);
    updateEnabledState();
}

void
BookGui::openBookFile() {
    if (bbControl.nRunningThreads() > 0 || loadingBook)
        return;
    if (!askSaveIfDirty())
        return;

    Gtk::FileChooserDialog dialog("Open", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*mainWindow);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);

    std::string filename = bbControl.getBookFileName();
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(filename);
    if (file && !filename.empty())
        file = file->get_parent();
    if (file)
        dialog.set_current_folder_file(file);

    auto bookFilter = Gtk::FileFilter::create();
    bookFilter->set_name("Texel book files");
    bookFilter->add_pattern("*.tbin");
    bookFilter->add_pattern("*.tbin.log");
    dialog.add_filter(bookFilter);
    auto allFilter = Gtk::FileFilter::create();
    allFilter->set_name("All files");
    allFilter->add_pattern("*");
    dialog.add_filter(allFilter);

    if (dialog.run() != Gtk::RESPONSE_OK)
        return;

    filename = dialog.get_filename();
    bbControl.readFromFile(filename);
    loadingBook = true;
    updateEnabledState();
}

void
BookGui::saveBookFile() {
    if (bbControl.getBookFileName().empty() || loadingBook)
        return;
    bbControl.saveToFile("");
    bookDirty = false;
}

bool
BookGui::saveBookFileAs() {
    if (loadingBook)
        return false;

    Gtk::FileChooserDialog dialog("Save As", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*mainWindow);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Save", Gtk::RESPONSE_OK);

    std::string filename = bbControl.getBookFileName();
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(filename);
    if (file && !filename.empty())
        dialog.set_filename(filename);
    else if (file)
        dialog.set_current_folder_file(file);

    auto bookFilter = Gtk::FileFilter::create();
    bookFilter->set_name("Texel book files");
    bookFilter->add_pattern("*.tbin");
    bookFilter->add_pattern("*.tbin.log");
    dialog.add_filter(bookFilter);
    auto allFilter = Gtk::FileFilter::create();
    allFilter->set_name("All files");
    allFilter->add_pattern("*");
    dialog.add_filter(allFilter);

    if (dialog.run() != Gtk::RESPONSE_OK)
        return false;
    filename = dialog.get_filename();
    if (filename.empty())
        return false;

    file = Gio::File::create_for_path(filename);
    if (file->get_basename().find('.') == std::string::npos)
        filename = filename + ".tbin";

    bbControl.saveToFile(filename);
    bookDirty = false;
    updateEnabledState();
    return true;
}

void
BookGui::quit() {
    if (!askSaveIfDirty())
        return;
    mainWindow->hide();
}

bool
BookGui::deleteEvent(_GdkEventAny* e) {
    if (!askSaveIfDirty())
        return true;

    return false;
}

bool
BookGui::askSaveIfDirty() {
    if (!bookDirty)
        return true;

    bool hasFilename = !bbControl.getBookFileName().empty();

    Gtk::MessageDialog dialog(*mainWindow, "Save book before closing?", false,
                              Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
    dialog.set_secondary_text("If you don't save, changes to the book will be lost.");
    const int NOSAVE = 0;
    const int SAVE = 1;
    const int CANCEL = 2;
    dialog.add_button(hasFilename ? "_Save" : "Save _As", SAVE);
    dialog.add_button("Close without saving", NOSAVE);
    dialog.add_button("_Cancel", CANCEL);

    int result = dialog.run();
    if (result == CANCEL)
        return false;
    if (result == NOSAVE)
        return true;

    if (hasFilename)
        saveBookFile();
    else
        return saveBookFileAs();

    return true;
}

// --------------------------------------------------------------------------------
