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
 * binfile.hpp
 *
 *  Created on: Jul 24, 2022
 *      Author: petero
 */

#ifndef BINFILE_HPP_
#define BINFILE_HPP_

#include "util.hpp"
#include <type_traits>

static_assert(sizeof(U8) == 1, "Unsupported byte size");

/** Helper for writing binary data to a file in an endianness-independent way. */
class BinaryFileWriter {
public:
    BinaryFileWriter(std::ostream& os) : os(os) {}

    /** Write a scalar value. */
    template <typename Type>
    void writeScalar(Type value) {
        static_assert(std::is_integral<Type>::value, "Unsupported type");
        const int s = sizeof(Type);
        U8 buf[s];
        toBytes(buf, value);
        writeBytes(buf, s);
    }

    /** Write an array of known size. */
    template <typename Type>
    void writeArray(const Type* arr, int nElem) {
        static_assert(std::is_integral<Type>::value, "Unsupported type");
        const int s = sizeof(Type);
        const int nBytes = s * nElem;
        std::vector<U8> buf(nBytes);
        for (int i = 0; i < nElem; i++)
            toBytes(&buf[i*s], arr[i]);
        writeBytes(buf.data(), nBytes);
    }

private:
    /** Convert a value to a sequence of bytes. */
    template <typename Type>
    void toBytes(U8* buf, Type value) {
        using UType = typename std::make_unsigned<Type>::type;
        UType tmp = static_cast<UType>(value);
        for (size_t i = 0; i < sizeof(UType); i++) {
            *buf++ = tmp & 0xff;
            if (sizeof(UType) > 1)
                tmp >>= 8;
        }
    }

    /** Write a number of bytes to the output stream. */
    void writeBytes(const U8* buf, int n) {
        os.write((const char*)buf, n);
    }

    std::ostream& os;
};


/** Helper for reading binary data from a file in an endianness-independent way. */
class BinaryFileReader {
public:
    BinaryFileReader(std::istream& is) : is(is) {}

    /** Read a scalar value. */
    template <typename Type>
    void readScalar(Type& value) {
        static_assert(std::is_integral<Type>::value, "Unsupported type");
        const int s = sizeof(Type);
        U8 buf[s];
        readBytes(buf, s);
        value = fromBytes<Type>(buf);
    }

    /** Read an array of known size. */
    template <typename Type>
    void readArray(Type* arr, int nElem) {
        static_assert(std::is_integral<Type>::value, "Unsupported type");
        const int s = sizeof(Type);
        const int nBytes = s * nElem;
        std::vector<U8> buf(nBytes);
        readBytes(buf.data(), nBytes);
        for (int i = 0; i < nElem; i++)
            arr[i] = fromBytes<Type>(&buf[i*s]);
    }

private:
    /** Convert a sequence of bytes to a value. */
    template <typename Type>
    Type fromBytes(const U8* buf) {
        using UType = typename std::make_unsigned<Type>::type;
        UType tmp = 0;
        for (size_t i = 0; i < sizeof(UType); i++)
            tmp += static_cast<UType>(buf[i]) << (i * 8);
        return static_cast<Type>(tmp);
    }

    /** Read a number of bytes from the input stream. */
    void readBytes(U8* buf, int n) {
        is.read((char*)buf, n);
    }

    std::istream& is;
};


#endif /* BINFILE_HPP_ */
