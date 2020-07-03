/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015,2020  Peter Österlund, peterosterlund2@gmail.com

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

#include "parameters.hpp"
#include "computerPlayer.hpp"
#include "tbTest.hpp"

#include "gtest/gtest.h"

class Environment : public ::testing::Environment {
public:
    void SetUp() override;
};

void
Environment::SetUp() {
    UciParams::gtbPath->set(gtbDefaultPath);
    UciParams::rtbPath->set(rtbDefaultPath);
    UciParams::gtbCache->set("128");
    ComputerPlayer::initEngine();
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
