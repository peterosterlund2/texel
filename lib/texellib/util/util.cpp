/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * util.cpp
 *
 *  Created on: Mar 2, 2012
 *      Author: petero
 */

#include "util.hpp"

static_assert(sizeof(int) >= sizeof(S32), "sizeof(int) too small");

void
splitString(const std::string& str, std::vector<std::string>& out) {
    std::string word;
    std::istringstream iss(str, std::istringstream::in);
    while (iss >> word)
        out.push_back(word);
}

bool
startsWith(const std::string& str, const std::string& startsWith) {
    size_t N = startsWith.length();
    if (str.length() < N)
        return false;
    for (size_t i = 0; i < N; i++)
        if (str[i] != startsWith[i])
            return false;
    return true;
}

bool
endsWith(const std::string& str, const std::string& endsWith) {
    size_t N = endsWith.length();
    size_t sN = str.length();
    if (sN < N)
        return false;
    for (size_t i = 0; i < N; i++)
        if (str[sN - N + i] != endsWith[i])
            return false;
    return true;
}

std::string
trim(const std::string& s) {
    for (int i = 0; i < (int)s.length(); i++) {
        if (!isspace(s[i])) {
            for (int j = (int)s.length()-1; j >= i; j--)
                if (!isspace(s[j]))
                    return s.substr(i, j-i+1);
            return "";
        }
    }
    return "";
}
