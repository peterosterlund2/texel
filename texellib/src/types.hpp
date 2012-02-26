/*
 * types.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <stddef.h>
#include <string>
#include <sstream>
#include <vector>

typedef unsigned long U64;
typedef signed char byte;

template <typename T, size_t N> char (&_ArraySizeHelper(T(&array)[N]))[N];
#define COUNT_OF(array) (sizeof(_ArraySizeHelper(array)))


/** Split a string using " " as delimiter. Append words to out. */
inline void
stringSplit(const std::string& str, std::vector<std::string>& out)
{
    std::string word;
    std::istringstream iss(str, std::istringstream::in);
    while (iss >> word)
        out.push_back(word);
}

/** Convert a string to a number. Return 0 on failure. */
template <typename T>
T str2Num(const std::string& str) {
    std::stringstream ss(str);
    T result;
    return ss >> result ? result : 0;
}

#endif /* TYPES_HPP_ */
