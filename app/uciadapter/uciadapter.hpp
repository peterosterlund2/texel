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
 * uciadapter.hpp
 *
 *  Created on: Jul 14, 2016
 *      Author: petero
 */

#ifndef UCIADAPTER_HPP_
#define UCIADAPTER_HPP_

#include "ctgbook.hpp"

#include <string>
#include <vector>


class ProcessStarter {
public:
    /** Constructor. Start child process and set up IO pipes. */
    ProcessStarter(const std::vector<std::string>& progAndArgs);

    int getChildOutFd() const { return childOutFd; }
    int getChildInFd() const { return childInFd; }

private:
    /** Start child process and set up IO pipes. */
    void openPipe(const std::vector<std::string>& progAndArgs);

    int childOutFd; // File descriptor for writing to child process
    int childInFd;  // File descriptor for reading from child process
};

/** Abstract class for line based IO adapter behavior.
 * The line break character is not included in the "line" parameter. */
class IOAdapter {
public:
    /** Constructor. */
    IOAdapter(int parentIn, int parentOut, int childIn, int childOut);

    /** Run the main loop. */
    void mainLoop();

    /** Called when parent process has sent a line to this process. */
    virtual void fromParent(const std::string& line) = 0;

    /** Called when child process has sent a line to this process. */
    virtual void fromChild(const std::string& line) = 0;

protected:
    /** Queue a line for sending to the parent process. */
    void toParent(const std::string& line);

    /** Queue a line for sending to the child process. */
    void toChild(const std::string& line);

private:
    /** Read from fd into buf and return the next complete line.
     * @return True if a complete line is available, false otherwise. */
    bool getLine(int fd, std::vector<char>& buf, std::string& line);

    /** Write data from buf to fd. */
    void writeData(int fd, std::vector<char>& buf);

    int parentInFd;  // Parent input file descriptor
    int parentOutFd; // Parent output file descriptor
    int childInFd;   // Child input file descriptor
    int childOutFd;  // Child output file descriptor

    std::vector<char> piBuf; // Parent input buffer
    std::vector<char> poBuf; // Parent output buffer
    std::vector<char> ciBuf; // Child input buffer
    std::vector<char> coBuf; // Child output buffer
};

class BookAdapter : public IOAdapter {
public:
    /** Constructor. */
    BookAdapter(const std::string& bookFile, int parentIn, int parentOut, int childIn, int childOut);

    void fromParent(const std::string& line) override;
    void fromChild(const std::string& line) override;

private:
    /** Get position corresponding to last "position" command. */
    bool getPosition(Position& pos) const;

    CtgBook ctgBook;
    int searchCount; // Number of unanswered "go" commands sent to the engine
    std::string lastPositionCmd;
};

#endif /* UCIADAPTER_HPP_ */
