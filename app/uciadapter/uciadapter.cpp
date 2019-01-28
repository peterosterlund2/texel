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
 * uciadapter.cpp
 *
 *  Created on: Jul 14, 2016
 *      Author: petero
 */

#include "uciadapter.hpp"
#include "position.hpp"
#include "textio.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


static void usage() {
    std::cerr << "Usage: uciadapter [-ctg ctgbookfile] program arg1 arg2 ..." << std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    std::string bookFile;

    if (argc < 2)
        usage();

    int arg = 1;
    if ((argc > 2) && (std::string(argv[1]) == "-ctg")) {
        bookFile = argv[2];
        arg = 3;
    } else if (startsWith(argv[1], "-"))
        usage();
    std::vector<std::string> progAndArgs;
    for (int i = arg; i < argc; i++)
        progAndArgs.push_back(argv[i]);
    if (progAndArgs.empty())
        usage();

    ProcessStarter pa(progAndArgs);
    BookAdapter ba(bookFile, STDIN_FILENO, STDOUT_FILENO,
                   pa.getChildInFd(), pa.getChildOutFd());
    ba.mainLoop();
}

// --------------------------------------------------------------------------------

ProcessStarter::ProcessStarter(const std::vector<std::string>& progAndArgs)
    : childOutFd(-1), childInFd(-1) {
    openPipe(progAndArgs);
}

void
ProcessStarter::openPipe(const std::vector<std::string>& progAndArgs) {
    int fd1[2];         /* parent -> child */
    int fd2[2];         /* child -> parent */
    long childpid;

    if (pipe(fd1) || pipe(fd2)) {
        perror("pipe");
        exit(1);
    }

    if ((childpid = fork()) == -1) {
        perror("fork");
        exit(1);
    }
    if (childpid == 0) {
        close(fd1[1]);
        close(fd2[0]);
        close(0); dup(fd1[0]); close(fd1[0]);
        close(1); dup(fd2[1]); close(fd2[1]);
        std::vector<char*> args;
        for (size_t i = 0; i < progAndArgs.size(); i++)
            args.push_back((char*)progAndArgs[i].c_str());
        args.push_back(nullptr);
        execvp(progAndArgs[0].c_str(), &args[0]);
        perror("execvp");
        exit(1);
    } else {
        close(fd1[0]);
        close(fd2[1]);
        childInFd  = fd2[0];
        childOutFd = fd1[1];
    }
}

// --------------------------------------------------------------------------------

IOAdapter::IOAdapter(int parentIn, int parentOut, int childIn, int childOut)
    : parentInFd(parentIn), parentOutFd(parentOut),
      childInFd(childIn), childOutFd(childOut) {
    if (fcntl(parentInFd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        exit(1);
    }
    if (fcntl(parentOutFd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        exit(1);
    }
    if (fcntl(childInFd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        exit(1);
    }
    if (fcntl(childOutFd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        exit(1);
    }
}

void
IOAdapter::mainLoop() {
    std::vector<int> allFds { parentInFd, childInFd, parentOutFd, childOutFd };
    int maxFd = *std::max_element(allFds.begin(), allFds.end()) + 1;
    fd_set readFds, writeFds, exceptFds;
    while (true) {
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);
        FD_ZERO(&exceptFds);

        FD_SET(parentInFd, &readFds);
        FD_SET(childInFd, &readFds);
        if (!poBuf.empty())
            FD_SET(parentOutFd, &writeFds);
        if (!coBuf.empty())
            FD_SET(childOutFd, &writeFds);
        for (int fd : allFds)
            FD_SET(fd, &exceptFds);

        int ret = select(maxFd, &readFds, &writeFds, &exceptFds, NULL);
        if (ret < 0) {
            perror("select");
            exit(1);
        }
        if (ret == 0)
            continue;

        if (FD_ISSET(parentInFd, &readFds)) {
            while (true) {
                std::string s;
                if (!getLine(parentInFd, piBuf, s))
                    break;
                fromParent(s);
            }
        }
        if (FD_ISSET(childInFd, &readFds)) {
            while (true) {
                std::string s;
                if (!getLine(childInFd, ciBuf, s))
                    break;
                fromChild(s);
            }
        }

        if (FD_ISSET(parentOutFd, &writeFds))
            writeData(parentOutFd, poBuf);
        if (FD_ISSET(childOutFd, &writeFds))
            writeData(childOutFd, coBuf);

        for (int fd : allFds)
            if (FD_ISSET(fd, &exceptFds)) {
                std::cerr << "select exception" << std::endl;
                exit(1);
            }
    }
}

/** Return true if errno indicates a temporary failure. */
static bool
isTempFail() {
    return (errno == EAGAIN || errno == EINTR);
}

bool
IOAdapter::getLine(int fd, std::vector<char>& buf, std::string& line) {
    bool hasRead = false;
    while (true) {
        for (int i = 0; i < (int)buf.size(); i++) {
            if (buf[i] == '\n') {
                for (int j = 0; j < i; j++)
                    line += buf[j];
                buf.erase(buf.begin(), buf.begin() + i + 1);
                return true;
            }
        }
        if (hasRead)
            return false;
        char tmpBuf[1024];
        int nRead = read(fd, &tmpBuf[0], sizeof(tmpBuf));
        if (nRead < 0 && !isTempFail()) {
            perror("read");
            exit(1);
        }
        if (nRead == 0) // EOF
            exit(0);
        for (int i = 0; i < nRead; i++)
            buf.push_back(tmpBuf[i]);
        hasRead = true;
    }
}

void
IOAdapter::writeData(int fd, std::vector<char>& buf) {
    int nWrite = write(fd, &buf[0], buf.size());
    if (nWrite > 0)
        buf.erase(buf.begin(), buf.begin() + nWrite);
    if (nWrite < 0 && !isTempFail()) {
        perror("write");
        exit(1);
    }
}

void
IOAdapter::toParent(const std::string& line) {
    for (char c: line)
        poBuf.push_back(c);
    poBuf.push_back('\n');
}

void
IOAdapter::toChild(const std::string& line) {
    for (char c: line)
        coBuf.push_back(c);
    coBuf.push_back('\n');
}

// --------------------------------------------------------------------------------

BookAdapter::BookAdapter(const std::string& bookFile,
                         int parentIn, int parentOut, int childIn, int childOut)
    : IOAdapter(parentIn, parentOut, childIn, childOut),
      ctgBook(bookFile, true, true),
      searchCount(0) {
#if 0
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    pos.makeMove(TextIO::uciStringToMove("e2e4"), ui);
    Move m;
    bool result = ctgBook.getBookMove(pos, m);
    std::cout << "result: " << (result?1:0) << " move: " << TextIO::moveToUCIString(m) << std::endl;
    exit(0);
#endif
}

void
BookAdapter::fromParent(const std::string& line) {
    if (startsWith(line, "position")) {
        lastPositionCmd = line;
    } else if (startsWith(line, "go")) {
        std::vector<std::string> tokens;
        splitString(line, tokens);
        if (searchCount == 0 && !contains(tokens, "ponder") && !contains(tokens, "infinite")) {
            Position pos;
            if (getPosition(pos)) {
                Move move;
                if (ctgBook.getBookMove(pos, move)) {
                    toParent("bestmove " + TextIO::moveToUCIString(move));
                    return;
                }
            }
        }
        searchCount++;
    } else if (startsWith(line, "ucinewgame")) {
        lastPositionCmd.clear();
    }
    toChild(line);
}

void
BookAdapter::fromChild(const std::string& line) {
    if (startsWith(line, "bestmove"))
        searchCount--;
    toParent(line);
}

bool
BookAdapter::getPosition(Position& pos) const {
    std::vector<std::string> tokens;
    splitString(lastPositionCmd, tokens);

    int nTok = tokens.size();
    if (nTok < 2)
        return false;

    int idx = 1;
    std::string fen;
    if (tokens[idx] == "startpos") {
        idx++;
        fen = TextIO::startPosFEN;
    } else if (tokens[idx] == "fen") {
        idx++;
        std::string s;
        while ((idx < nTok) && (tokens[idx] != "moves")) {
            s += tokens[idx++];
            s += ' ';
        }
        fen = trim(s);
    }
    if (fen.empty())
        return false;

    pos = TextIO::readFEN(fen);
    UndoInfo ui;
    if ((idx < nTok) && (tokens[idx++] == "moves")) {
        for (int i = idx; i < nTok; i++) {
            Move m = TextIO::uciStringToMove(tokens[i]);
            if (m.isEmpty())
                return false;
            pos.makeMove(m, ui);
        }
    }
    return true;
}
