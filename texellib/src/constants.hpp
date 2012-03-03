/*
 * constants.hpp
 *
 *  Created on: Mar 2, 2012
 *      Author: petero
 */

#ifndef CONSTANTS_HPP_
#define CONSTANTS_HPP_

namespace SearchConst {
    const int MATE0 = 32000;
    const int plyScale = 8; // Fractional ply resolution
}

namespace TType {
    const int T_EXACT = 0;   // Exact score
    const int T_GE = 1;      // True score >= this->score
    const int T_LE = 2;      // True score <= this->score
    const int T_EMPTY = 3;   // Empty hash slot
}

#endif /* CONSTANTS_HPP_ */
