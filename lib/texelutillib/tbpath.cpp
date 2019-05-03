/*
    Texel - A UCI chess engine.
    Copyright (C) 2019  Peter Österlund, peterosterlund2@gmail.com

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
 * tbpath.hpp
 *
 *  Created on: May 3, 2019
 *      Author: petero
 */

#include "parameters.hpp"
#include "tbpath.hpp"
#include <cstdlib>

namespace TBPath {

void setDefaultTBPaths() {
    const char* gtbEnv = std::getenv("GTBPATH");
    if (gtbEnv)
        UciParams::gtbPath->set(gtbEnv);
    else
        UciParams::gtbPath->set("/home/petero/chess/gtb");
    UciParams::gtbCache->set("2047");

    const char* rtbEnv = std::getenv("RTBPATH");
    if (rtbEnv)
        UciParams::rtbPath->set(rtbEnv);
    else
        UciParams::rtbPath->set("/home/petero/chess/rtb/wdl:"
                            "/home/petero/chess/rtb/dtz:"
                            "/home/petero/chess/rtb/6wdl:"
                            "/home/petero/chess/rtb/6dtz:"
                            "/home/petero/chess/rtb/7wdl:"
                            "/home/petero/chess/rtb/7dtz");
}

}
