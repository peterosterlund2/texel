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
 * util.cpp
 *
 *  Created on: Mar 2, 2012
 *      Author: petero
 */

#include "util.hpp"

#include <chrono>
#include <iostream>

S64 currentTimeMillis() {
    auto t = std::chrono::system_clock::now();
    auto t0 = t.time_since_epoch();
    auto x = t0.count();
    typedef decltype(t0) T0Type;
    auto n = T0Type::period::num;
    auto d = T0Type::period::den;
    return (S64)(x * (1000.0 * n / d));
}
