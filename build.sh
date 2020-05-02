#!/bin/bash
#
# This script is used to build the Texel distribution.
# It needs an environment where several cross compilers are available.
#
# To compile Texel for your computer, do not run this script.
# Instead follow the instructions in readme.txt in the section "Compiling".
#

rm -rf build bin

# Native Release build
mkdir -p build/Release
(cd build/Release &&
     cmake ../.. -DCMAKE_BUILD_TYPE=Release \
           -DCPU_TYPE=corei7 \
           -DUSE_BMI2=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Native debug build
mkdir -p build/Debug
(cd build/Debug &&
     cmake ../.. -DCMAKE_BUILD_TYPE=Debug \
           -DCPU_TYPE=corei7 \
           -DUSE_BMI2=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &


# Windows 64-bit
mkdir -p build/win64
(cd build/win64 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=corei7 \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit with BMI2
mkdir -p build/win64bmi
(cd build/win64bmi &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=corei7 \
           -DUSE_BMI2=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit with cluster support
mkdir -p build/win64cl
(cd build/win64cl &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=corei7 \
           -DUSE_CLUSTER=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit for AMD
mkdir -p build/win64amd
(cd build/win64amd &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DCPU_TYPE=athlon64-sse3 \
           -DUSE_CTZ=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit for "old" systems
mkdir -p build/win64old
(cd build/win64old &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_NUMA=on
) &

# Windows 32-bit
mkdir -p build/win32
(cd build/win32 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win32.cmake \
           -DUSE_CTZ=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Windows 32-bit for "old" systems
mkdir -p build/win32old
(cd build/win32old &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win32.cmake
) &


# Android 64-bit
mkdir -p build/android64
(cd build/android64 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/android64.cmake \
           -DCMAKE_BUILD_TYPE=Release \
           -DUSE_CTZ=on \
           -DUSE_PREFETCH=on
) &

# Android 32-bit
mkdir -p build/android32
(cd build/android32 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/android32.cmake \
           -DCMAKE_BUILD_TYPE=Release \
           -DUSE_CTZ=on \
           -DUSE_PREFETCH=on
) &
wait

# Build all
para=8
cmake --build build/Release   -j ${para} || exit 2
cmake --build build/Debug     -j ${para} || exit 2
cmake --build build/win64     -j ${para} || exit 2
cmake --build build/win64bmi  -j ${para} || exit 2
cmake --build build/win64cl   -j ${para} || exit 2
cmake --build build/win64amd  -j ${para} || exit 2
cmake --build build/win64old  -j ${para} || exit 2
cmake --build build/win32     -j ${para} || exit 2
cmake --build build/win32old  -j ${para} || exit 2
cmake --build build/android64 -j ${para} || exit 2
cmake --build build/android32 -j ${para} || exit 2

# Copy to bin and strip executables
mkdir -p bin

cp build/Release/texel bin/texel64
strip bin/texel64

cp build/win64/texel.exe bin/texel64.exe
x86_64-w64-mingw32-strip bin/texel64.exe
cp build/win64/runcmd.exe bin/runcmd.exe
x86_64-w64-mingw32-strip bin/runcmd.exe

cp build/win64bmi/texel.exe bin/texel64bmi.exe
x86_64-w64-mingw32-strip bin/texel64bmi.exe

cp build/win64cl/texel.exe bin/texel64cl.exe
x86_64-w64-mingw32-strip bin/texel64cl.exe

cp build/win64amd/texel.exe bin/texel64amd.exe
x86_64-w64-mingw32-strip bin/texel64amd.exe

cp build/win64old/texel.exe bin/texel64old.exe
x86_64-w64-mingw32-strip bin/texel64old.exe

cp build/win32/texel.exe bin/texel32.exe
i686-w64-mingw32-strip bin/texel32.exe

cp build/win32old/texel.exe bin/texel32old.exe
i686-w64-mingw32-strip bin/texel32old.exe

cp build/android64/texel bin/texel-arm64
aarch64-linux-android-strip bin/texel-arm64

cp build/android32/texel bin/texel-arm
arm-linux-androideabi-strip bin/texel-arm

# Create archive
VER=$(echo -e 'uci\nquit' | bin/texel64 | grep 'id name' | awk '{print $4}' | tr -d .)
rm -rf texel${VER}.7z texel${VER}
mkdir texel${VER}
cp --parents -r \
   CMakeLists.txt build.sh app bin cmake lib test \
   COPYING readme.txt texelbook.bin \
   texel${VER}/
chmod -R ug+w texel${VER}
7za a texel${VER}.7z texel${VER}
rm -rf texel${VER}
