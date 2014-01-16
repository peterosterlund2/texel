/*
 * stloutput.hpp
 *
 *  Created on: Dec 24, 2013
 *      Author: petero
 */

#ifndef STLOUTPUT_HPP_
#define STLOUTPUT_HPP_

#include <iostream>
#include <vector>

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    bool first = true;
    for (const T& e : v) {
        if (!first)
            os << ", ";
        os << e;
        first = false;
    }
    os << " ]";
    return os;
}

#endif /* STLOUTPUT_HPP_ */
