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
    for (const T& e : v)
        os << ", " << e;
    os << " ]";
    return os;
}

#endif /* STLOUTPUT_HPP_ */
