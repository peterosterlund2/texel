/*
 * posgen.hpp
 *
 *  Created on: Jan 6, 2014
 *      Author: petero
 */

#ifndef POSGEN_HPP_
#define POSGEN_HPP_

#include <string>
#include <vector>

class PosGenerator {
public:
    /** Generate a FEN containing all (or a sample of) positions of a certain type. */
    static bool generate(const std::string& type);

    /** Print all tablebase types containing a given number of pieces. */
    static void tbList(int nPieces);

    /** Generate tablebase statistics. */
    static void tbStat(const std::vector<std::string>& tbTypes);

    /** Compare RTB probe results to GTB probe results, report any differences. */
    static void rtbTest(const std::vector<std::string>& tbTypes);

private:
    static void genQvsN();
};


#endif /* POSGEN_HPP_ */
