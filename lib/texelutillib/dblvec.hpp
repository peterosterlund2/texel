/*
    Texel - A UCI chess engine.
    Copyright (C) 2024  Peter Österlund, peterosterlund2@gmail.com

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
 * dblvec.hpp
 *
 *  Created on: Feb 4, 2024
 *      Author: petero
 */

#ifndef DBLVEC_HPP_
#define DBLVEC_HPP_

#include "stloutput.hpp"

#include <vector>
#include <cmath>
#include <cassert>


/** A vector of double elements that supports mathematical operations like
 *  addition and scalara multiplication. */
class DblVec {
    friend std::ostream& operator<<(std::ostream& os, const DblVec& v);
public:
    DblVec(int n) : v(n) {}

    size_t size() const { return v.size(); }
          double& operator[](int i)       { return v[i]; }
    const double& operator[](int i) const { return v[i]; }

    DblVec& operator+=(const DblVec& b) {
        int n = size();
        for (int i = 0; i < n; i++)
            v[i] += b.v[i];
        return *this;
    }

    DblVec& operator*=(double a) {
        for (double& e : v)
            e *= a;
        return *this;
    }

    double dot(const DblVec b) const {
        assert(size() == b.size());
        int n = size();
        double s = 0;
        for (int i = 0; i < n; i++)
            s += v[i] * b[i];
        return s;
    }

private:
    std::vector<double> v;
};

inline DblVec operator*(double a, const DblVec& v) {
    DblVec ret(v);
    ret *= a;
    return ret;
}

inline DblVec operator+(const DblVec& a, const DblVec& b) {
    DblVec ret(a);
    ret += b;
    return ret;
}

inline std::ostream& operator<<(std::ostream& os, const DblVec& v) {
    os << v.v;
    return os;
}

#endif /* DBLVEC_HPP_ */
