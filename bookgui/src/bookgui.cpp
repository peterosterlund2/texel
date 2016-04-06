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
    : app(app0), mainWindow(nullptr), bbControl(*this),
      gameTree(std::cin), // FIXME!!
      loadingBook(false), searchState(SearchState::STOPPED),
      analysing(false), bookDirty(false) {
    builder = Gtk::Builder::create();
    builder->add_from_resource("/main/bookgui_glade.xml");
}

void
BookGui::run() {
    builder->get_widget("mainWindow", mainWindow);
    connectSignals();
    getWidgets();
    updateEnabledState();
    app->run(*mainWindow);
}

// --------------------------------------------------------------------------------

void
BookGui::getWidgets() {
    builder->get_widget("pvInfo", pvInfo);
}

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

    // Settings
    builder->get_widget("threads", threads);
    threads->signal_value_changed().connect([this]{ threadsValueChanged(); });

    builder->get_widget("compTime", compTime);
    compTime->signal_value_changed().connect([this]{ compTimeChanged(); });

    builder->get_widget("depthCost", depthCost);
    depthCost->signal_value_changed().connect([this]{ depthCostChanged(); });

    builder->get_widget("ownPathErrCost", ownPathErrCost);
    ownPathErrCost->signal_value_changed().connect([this]{ ownPathErrCostChanged(); });

    builder->get_widget("otherPathErrCost", otherPathErrCost);
    otherPathErrCost->signal_value_changed().connect([this]{ otherPathErrCostChanged(); });

    builder->get_widget("pgnMaxPly", pgnMaxPly);
    pgnMaxPly->signal_value_changed().connect([this]{ pgnMaxPlyChanged(); });

    // Start/stop buttons
    builder->get_widget("startButton", startButton);
    startButton->signal_clicked().connect([this]{ startSearch(); });

    builder->get_widget("softStopButton", softStopButton);
    softStopButton->signal_clicked().connect([this]{ softStopSearch(); });

    builder->get_widget("hardStopButton", hardStopButton);
    hardStopButton->signal_clicked().connect([this]{ hardStopSearch(); });

    // Focus buttons
    builder->get_widget("setFocusButton", setFocusButton);
    setFocusButton->signal_clicked().connect([this]{ setFocus(); });

    builder->get_widget("getFocusButton", getFocusButton);
    getFocusButton->signal_clicked().connect([this]{ getFocus(); });

    builder->get_widget("clearFocusButton", clearFocusButton);
    clearFocusButton->signal_clicked().connect([this]{ clearFocus(); });

    // PGN buttons
    builder->get_widget("importPgnButton", importPgnButton);
    importPgnButton->signal_clicked().connect([this]{ importPgn(); });

    builder->get_widget("addPgnButton", addPgnButton);
    addPgnButton->signal_clicked().connect([this]{ addPgn(); });

    builder->get_widget("applyPgnButton", applyPgnButton);
    applyPgnButton->signal_clicked().connect([this]{ applyPgn(); });

    builder->get_widget("clearPgnButton", clearPgnButton);
    clearPgnButton->signal_clicked().connect([this]{ clearPgn(); });

    // Navigate buttons
    builder->get_widget("backButton", backButton);
    backButton->signal_clicked().connect([this]{ posGoBack(); });

    builder->get_widget("forwardButton", forwardButton);
    forwardButton->signal_clicked().connect([this]{ posGoForward(); });

    // Analyze buttons
    builder->get_widget("nextGenButton", nextGenButton);
    nextGenButton->signal_clicked().connect([this]{ nextGeneration(); });

    builder->get_widget("analyzeToggle", analyzeToggle);
    analyzeToggle->signal_clicked().connect([this]{ toggleAnalyzeMode(); });
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
            if (bbControl.nRunningThreads() == 0)
                searchState = SearchState::STOPPED;
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
    std::string pv;
    if (analysing)
        bbControl.getPVInfo(pv);
    pvInfo->get_buffer()->set_text(pv);
}

void
BookGui::updatePGNView() {

}

void
BookGui::updateEnabledState() {
    // Menu items
    bool builderIdle = searchState == SearchState::STOPPED &&
                       bbControl.nRunningThreads() == 0 && !loadingBook;
    newItem->set_sensitive(builderIdle);
    openItem->set_sensitive(builderIdle);
    saveItem->set_sensitive(!bbControl.getBookFileName().empty() && !loadingBook);
    saveAsItem->set_sensitive(!loadingBook);
//  quitItem->set_sensitive(true);

    // Settings widgets
    bool searchStopped = searchState == SearchState::STOPPED;
    threads->set_sensitive(searchStopped);
//  compTime->set_sensitive(true);
    depthCost->set_sensitive(searchStopped);
    ownPathErrCost->set_sensitive(searchState == SearchState::STOPPED);
    otherPathErrCost->set_sensitive(searchState == SearchState::STOPPED);
//  pgnMaxPly->set_sensitive(true);

    // Start/stop buttons
    startButton->set_sensitive(searchState == SearchState::STOPPED);
    softStopButton->set_sensitive(searchState == SearchState::RUNNING);
    hardStopButton->set_sensitive(searchState != SearchState::STOPPED);

    // Focus buttons
//  setFocusButton->set_sensitive(true);
//  getFocusButton->set_sensitive(true);
//  clearFocusButton->set_sensitive(true);

    // PGN buttons
//  importPgnButton->set_sensitive(true);
//  addPgnButton->set_sensitive(true);
//  applyPgnButton->set_sensitive(true);
//  clearPgnButton->set_sensitive(true);

    // Navigate buttons
    backButton->set_sensitive(!moves.empty());
    forwardButton->set_sensitive(!nextMoves.empty());

    // Analyze buttons
//  nextGenButton->set_sensitive(true);
//  analyzeToggle->set_sensitive(true);
}

// --------------------------------------------------------------------------------

void
BookGui::newBook() {
    if (searchState != SearchState::STOPPED || bbControl.nRunningThreads() > 0 || loadingBook)
        return;
    if (!askSaveIfDirty())
        return;

    bbControl.newBook();
    pos = TextIO::readFEN(TextIO::startPosFEN);
    moves.clear();
    nextMoves.clear();
    bookDirty = false;
    updateBoardAndTree();
    if (analysing)
        bbControl.startAnalysis(moves);
    updateEnabledState();
}

void
BookGui::openBookFile() {
    if (searchState != SearchState::STOPPED || bbControl.nRunningThreads() > 0 || loadingBook)
        return;
    if (!askSaveIfDirty())
        return;

    Gtk::FileChooserDialog dialog("Open book", Gtk::FILE_CHOOSER_ACTION_OPEN);
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

void
BookGui::threadsValueChanged() {
    BookBuildControl::Params params;
    bbControl.getParams(params);
    params.nThreads = threads->get_value_as_int();
    bbControl.setParams(params);
}

void
BookGui::compTimeChanged() {
    BookBuildControl::Params params;
    bbControl.getParams(params);
    params.computationTime = compTime->get_value_as_int();
    bbControl.setParams(params);
}

void
BookGui::depthCostChanged() {
    BookBuildControl::Params params;
    bbControl.getParams(params);
    params.bookDepthCost = depthCost->get_value_as_int();
    bbControl.setParams(params);
}

void
BookGui::ownPathErrCostChanged() {
    BookBuildControl::Params params;
    bbControl.getParams(params);
    params.ownPathErrorCost = ownPathErrCost->get_value_as_int();
    bbControl.setParams(params);
}

void
BookGui::otherPathErrCostChanged() {
    BookBuildControl::Params params;
    bbControl.getParams(params);
    params.otherPathErrorCost = otherPathErrCost->get_value_as_int();
    bbControl.setParams(params);
}

void
BookGui::pgnMaxPlyChanged() {
    pgnImportMaxPly = pgnMaxPly->get_value_as_int();
}

// --------------------------------------------------------------------------------

void
BookGui::startSearch() {
    if (searchState != SearchState::STOPPED)
        return;

    bbControl.startSearch();
    searchState = SearchState::RUNNING;
    updateEnabledState();
}

void
BookGui::softStopSearch() {
    if (searchState != SearchState::RUNNING)
        return;
    bbControl.stopSearch(false);
    searchState = SearchState::STOPPING;
    updateEnabledState();
}

void
BookGui::hardStopSearch() {
    if (searchState == SearchState::STOPPED)
        return;
    bbControl.stopSearch(true);
    searchState = SearchState::STOPPING;
    updateEnabledState();
}

// --------------------------------------------------------------------------------

void
BookGui::setFocus() {
    bbControl.setFocus(pos);
}

void
BookGui::getFocus() {
    bbControl.getFocus(pos);
    moves.clear(); // FIXME!! Compute reasonable move path
    nextMoves.clear(); // FIXME!! Compute reasonable move path by following "book PV"
    updateBoardAndTree();
}

void
BookGui::clearFocus() {
    Position startPos = TextIO::readFEN(TextIO::startPosFEN);
    bbControl.setFocus(startPos);
}

// --------------------------------------------------------------------------------

void
BookGui::importPgn() {
    Gtk::FileChooserDialog dialog("Import PGN", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*mainWindow);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);

    std::string filename = pgnImportFilename;
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(filename);
    if (file && !filename.empty())
        file = file->get_parent();
    if (file)
        dialog.set_current_folder_file(file);

    auto bookFilter = Gtk::FileFilter::create();
    bookFilter->set_name("PGN files");
    bookFilter->add_pattern("*.pgn");
    dialog.add_filter(bookFilter);
    auto allFilter = Gtk::FileFilter::create();
    allFilter->set_name("All files");
    allFilter->add_pattern("*");
    dialog.add_filter(allFilter);

    if (dialog.run() != Gtk::RESPONSE_OK)
        return;

    pgnImportFilename = dialog.get_filename();

    // FIXME!! Perform import

    updatePGNView();
}

void
BookGui::addPgn() {
    // FIXME!!
    updatePGNView();
}

void
BookGui::applyPgn() {
    // FIXME!!
    updatePGNView();
}

void
BookGui::clearPgn() {
    // FIXME!!
    updatePGNView();
}

// --------------------------------------------------------------------------------

void
BookGui::posGoBack() {
    if (moves.empty())
        return;

    pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    int N = moves.size();
    for (int i = 0; i < N - 1; i++)
        pos.makeMove(moves[i], ui);

    nextMoves.insert(nextMoves.begin(), moves[N-1]);
    moves.pop_back();

    updateBoardAndTree();
}

void
BookGui::posGoForward() {
    if (nextMoves.empty())
        return;

    moves.push_back(nextMoves[0]);
    UndoInfo ui;
    pos.makeMove(nextMoves[0], ui);
    nextMoves.erase(nextMoves.begin());

    updateBoardAndTree();
}

// --------------------------------------------------------------------------------

void
BookGui::nextGeneration() {
    bbControl.nextGeneration();
}

void
BookGui::toggleAnalyzeMode() {
    if (analyzeToggle->get_active()) {
        bbControl.startAnalysis(moves);
        analysing = true;
    } else {
        bbControl.stopAnalysis();
        analysing = false;
    }
}
