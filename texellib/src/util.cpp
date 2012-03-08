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
