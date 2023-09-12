/*
    Texel - A UCI chess engine.
    Copyright (C) 2017  Peter Österlund, peterosterlund2@gmail.com

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
 * syncengine.cpp
 *
 *  Created on: Oct 24, 2017
 *      Author: petero
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


inline bool
startsWith(const std::string& str, const std::string& startsWith) {
    size_t N = startsWith.length();
    if (str.length() < N)
        return false;
    for (size_t i = 0; i < N; i++)
        if (str[i] != startsWith[i])
            return false;
    return true;
}

/** Split a string using " " as delimiter. Append words to out. */
inline void
splitString(const std::string& str, std::vector<std::string>& out)
{
    std::string word;
    std::istringstream iss(str, std::istringstream::in);
    while (iss >> word)
        out.push_back(word);
}

// ------------------------------------------------------------

class Process {
public:
    Process(const std::string& command);
    ~Process();

    void writeLine(std::string line);

    std::string readLine();

    void waitFor(const std::string& s);

private:
    int fd[2];
    pid_t childpid;
};

Process::Process(const std::string& command) {
    int fd1[2];		/* parent -> child */
    int fd2[2];		/* child -> parent */

    if (pipe(fd1) || pipe(fd2))
    	throw "pipe error";

    if ((childpid = fork()) == -1)
        throw "fork error";
    if (childpid == 0) {
	close(fd1[1]);
	close(fd2[0]);
	close(0); dup(fd1[0]); close(fd1[0]);
    	close(1); dup(fd2[1]); close(fd2[1]);
	execlp(command.c_str(), command.c_str(), NULL);
	perror("execlp");
	exit(1);
    } else {
	close(fd1[0]);
	close(fd2[1]);
	fd[0] = fd2[0];
	fd[1] = fd1[1];
    }
}

Process::~Process() {
    close(fd[0]);
    close(fd[1]);
    int status;
    waitpid(childpid, &status, 0);
}

void
Process::writeLine(std::string line) {
    // cout << line << endl;
    line += "\n";
    const int len = line.length();
    int pos = 0;
    while (pos < len) {
        int cnt = write(fd[1], &line[pos], len - pos);
        if ((cnt <= 0) || (cnt > len)) {
            std::cerr << "pos:" << pos << " len:" << len << " cnt:" << cnt << std::endl;
            throw "write error";
        }
        pos += cnt;
    }
}

std::string
Process::readLine() {
    std::string ret;
    char buf;
    while (true) {
        int cnt = read(fd[0], &buf, 1);
        if (cnt != 1)
            throw "read error";
        if (buf == '\n')
            break;
        ret += buf;
    }
    // cout << ret << endl;
    return ret;
}

void
Process::waitFor(const std::string& s) {
    std::string line;
    do {
        line = readLine();
        std::cout << line << std::endl;
    } while (!startsWith(line, s));
}

// ------------------------------------------------------------

static void usage() {
    std::cerr << "Usage: syncengine engine" << std::endl;
    ::exit(2);
}

int main(int argc, char* argv[]) {
    if (argc != 2)
        usage();
    std::string engine = argv[1];

    static std::map<std::string,std::string> cmdPairs = {
        { "uci", "uciok" },
        { "isready", "readyok" },
        { "go", "bestmove" },
    };

    try {
        Process p(engine);
        std::vector<std::string> words;
        while (true) {
            std::string line;
            std::getline(std::cin, line);
            if (std::cin.good()) {
                p.writeLine(line);
                words.clear();
                splitString(line, words);
                if (words.size() > 0) {
                    auto it = cmdPairs.find(words[0]);
                    if (it != cmdPairs.end())
                        p.waitFor(it->second);
                    if (words[0] == "quit")
                        break;
                }
            } else {
                std::cin.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    } catch (const char* err) {
        std::cerr << "Exception: " << err << std::endl;
    }
}
