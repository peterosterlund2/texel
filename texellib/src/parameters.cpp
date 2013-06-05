/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * parameters.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "parameters.hpp"

Parameters::Parameters() {
//    addPar(std::make_shared<SpinParam>("doubled", true, 0, 2048, 25));
//    addPar(std::make_shared<SpinParam>("island", true, 0, 2048, 15));
//    addPar(std::make_shared<SpinParam>("isolated", true, 0, 2048, 15));
//    addPar(std::make_shared<SpinParam>("nOutpost", true, 0, 2048, 128));
}

Parameters&
Parameters::instance() {
    static Parameters inst;
    return inst;
}
