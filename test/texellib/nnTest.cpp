/*
    Texel - A UCI chess engine.
    Copyright (C) 2023  Peter Österlund, peterosterlund2@gmail.com

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
 * nnTest.hpp
 *
 *  Created on: Jul 25, 2023
 *      Author: petero
 */

#include "nntypes.hpp"
#include "vectorop.hpp"

#include "gtest/gtest.h"

TEST(NNTest, testMatMul) {
    {
        Matrix<S8,1,32> w;
        Vector<S8,32> in;
        Vector<S32,1> res;
        res(0) = 0;
        for (int i = 0; i < 32; i++) {
            w(0,i) = i+1;
            in(i) = i+2;
        }
        matMul(res, w, in);
        ASSERT_EQ(11968, res(0));
    }

    {
        Matrix<S8,1,32> w;
        Vector<S8,32> in;
        Vector<S32,1> res;
        res(0) = 0;
        for (int i = 0; i < 32; i++) {
            w(0,i) = (i & 1) ? -(i+1) : (i+1);
            in(i) = i+2;
        }
        matMul(res, w, in);
        ASSERT_EQ(-544, res(0));
    }

    {
        Matrix<S8,2,32> w;
        Vector<S8,32> in;
        Vector<S32,2> res;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 32; j++) {
                w(i,j) = i == 0 ? 127 : -127;
                in(j) = 127;
            }
            res(i) = 0;
        }
        matMul(res, w, in);
        ASSERT_EQ(127*127*32, res(0));
        ASSERT_EQ(-127*127*32, res(1));
    }

    {
        Matrix<S8,32,512> w;
        Vector<S8,512> in;
        Vector<S32,32> res;
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 512; j++)
                w(i,j) = (i + j * 7) % 64 - 32;
            res(i) = i;
        }
        for (int j = 0; j < 512; j++)
            in(j) = (j * j + 1) % 11;

        matMul(res, w, in);
        std::vector<int> expected = {
            -627, -820, -1077, -1270, -1271, -952, -889, -954,
            -1019, -1084, -1021, -702, -703, -896, -1153, -1410,
            -1603, -1604, -1285, -1222, -1287, -1352, -1417, -1354,
            -1035, -1036, -1229, -1486, -1679, -1872, -1873, -1554
        };
        for (int j = 0; j < 32; j++) {
            ASSERT_EQ(expected[j], res(j));
        }
    }
}
