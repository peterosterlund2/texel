/*
 * posgen.hpp
 *
 *  Created on: Jan 6, 2014
 *      Author: petero
 */

#ifndef POSGEN_HPP_
#define POSGEN_HPP_

#include <string>

class PosGenerator {
public:
    /** Generate a FEN containing all (or a sample of) positions of a certain type. */
    static bool generate(const std::string& type);

private:
    static void genQvsN();
};


#endif /* POSGEN_HPP_ */
