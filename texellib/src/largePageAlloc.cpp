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
 * largePageAlloc.cpp
 *
 *  Created on: Oct 19, 2016
 *      Author: petero
 */

#include "largePageAlloc.hpp"

#ifdef USE_LARGE_PAGES
#ifdef _WIN32
#else
#include <sys/mman.h>
#endif
#endif

std::shared_ptr<void>
LargePageAlloc::allocBytes(size_t numBytes) {
#ifdef USE_LARGE_PAGES
#ifdef _WIN32
#else
    auto deleter = [numBytes](void* mem) {
        munmap(mem, numBytes);
    };
    const int prot = PROT_READ | PROT_WRITE;
    const int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;
    if (numBytes >= 1024 * 1024 * 1024) {
        const int MAP_HUGE_1GB = 30 << MAP_HUGE_SHIFT;
        void* mem = mmap(NULL, numBytes, prot, flags | MAP_HUGE_1GB, -1, 0);
        if (mem != MAP_FAILED)
            return std::shared_ptr<void>(mem, deleter);
    }
    if (numBytes > 4 * 1024 * 1024) {
        void* mem = mmap(NULL, numBytes, prot, flags, -1, 0);
        if (mem != MAP_FAILED)
            return std::shared_ptr<void>(mem, deleter);
    }
#endif
#endif
    return nullptr;
}
