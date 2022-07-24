/*
    Texel - A UCI chess engine.
    Copyright (C) 2022  Peter Österlund, peterosterlund2@gmail.com

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
 * nntypes.cpp
 *
 *  Created on: Jul 23, 2022
 *      Author: petero
 */

#include "nntypes.hpp"
#include "chessError.hpp"

static const U64 magicHeader = 0xb3828c6bdf56c56cULL;
static const int netVersion = 0;


std::shared_ptr<NetData>
NetData::create() {
    return std::shared_ptr<NetData>(new NetData);
}

void
NetData::save(std::ostream& os) const {
    BinaryFileWriter writer(os);
    writer.writeScalar(magicHeader);
    writer.writeScalar(netVersion);

    writer.writeArray(&weight1.data[0], COUNT_OF(weight1.data));
    writer.writeArray(&bias1.data[0], COUNT_OF(bias1.data));
    lin2.save(writer);
    lin3.save(writer);
    lin4.save(writer);

    writer.writeScalar(computeHash());
}

void
NetData::load(std::istream& is) {
    BinaryFileReader reader(is);

    U64 header;
    reader.readScalar(header);
    if (header != magicHeader)
        throw ChessError("Incorrect file type");

    int ver;
    reader.readScalar(ver);
    if (ver != netVersion)
        throw ChessError("Incorrect network version number");

    reader.readArray(&weight1.data[0], COUNT_OF(weight1.data));
    reader.readArray(&bias1.data[0], COUNT_OF(bias1.data));
    lin2.load(reader);
    lin3.load(reader);
    lin4.load(reader);

    U64 hash;
    reader.readScalar(hash);

    if (hash != computeHash())
        throw ChessError("Network checksum error");
}

U64
NetData::computeHash() const {
    U64 ret = hashU64(1);
    ret = hashU64(ret + weight1.computeHash());
    ret = hashU64(ret + bias1.computeHash());
    ret = hashU64(ret + lin2.computeHash());
    ret = hashU64(ret + lin3.computeHash());
    ret = hashU64(ret + lin4.computeHash());
    return ret;
}
