/*
 * types.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <stddef.h>

typedef unsigned long U64;
typedef unsigned char byte;

template <typename T, size_t N> char (&_ArraySizeHelper(T(&array)[N]))[N];
#define COUNT_OF(array) (sizeof(_ArraySizeHelper(array)))


#endif /* TYPES_HPP_ */
