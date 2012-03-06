/*
 * parameters.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "parameters.hpp"

Parameters&
Parameters::instance() {
    static Parameters inst;
    return inst;
}
