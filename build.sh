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
           -DCPU_TYPE=avx2 \
           -DUSE_BMI2=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit, no popcount
mkdir -p build/win64nopop
(cd build/win64nopop &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=avx2 \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=off \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit
mkdir -p build/win64
(cd build/win64 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=avx2 \
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
           -DCPU_TYPE=avx2 \
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
           -DCPU_TYPE=avx2 \
           -DUSE_CLUSTER=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Android 64-bit
mkdir -p build/android64
(cd build/android64 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/android64.cmake \
           -DCMAKE_BUILD_TYPE=Release \
           -DUSE_CTZ=on \
           -DUSE_PREFETCH=on
) &

wait

# Build all
para=8
cmake --build build/Release    -j ${para} || exit 2
cmake --build build/win64      -j ${para} || exit 2
cmake --build build/win64nopop -j ${para} || exit 2
cmake --build build/win64bmi   -j ${para} || exit 2
cmake --build build/win64cl    -j ${para} || exit 2
cmake --build build/android64  -j ${para} || exit 2
make -C doc

# Copy to bin and strip executables
mkdir -p bin

cp build/Release/texel bin/texel64
strip bin/texel64

cp build/win64/texel.exe bin/texel64.exe
x86_64-w64-mingw32-strip bin/texel64.exe
cp build/win64/runcmd.exe bin/runcmd.exe
x86_64-w64-mingw32-strip bin/runcmd.exe
cp build/win64/texelutil.exe bin/texelutil.exe
x86_64-w64-mingw32-strip bin/texelutil.exe

cp build/win64nopop/texel.exe bin/texel64nopop.exe
x86_64-w64-mingw32-strip bin/texel64nopop.exe

cp build/win64bmi/texel.exe bin/texel64bmi.exe
x86_64-w64-mingw32-strip bin/texel64bmi.exe

cp build/win64cl/texel.exe bin/texel64cl.exe
x86_64-w64-mingw32-strip bin/texel64cl.exe

cp build/android64/texel bin/texel-arm64
aarch64-linux-android-strip bin/texel-arm64

# Create archive
VER=$(echo -e 'uci\nquit' | bin/texel64 | grep 'id name' | awk '{print $4}' | tr -d .)
rm -rf texel${VER}.7z texel${VER}
mkdir texel${VER}
cp --parents -r \
   CMakeLists.txt build.sh app bin cmake lib test \
   COPYING readme.txt doc texelbook.bin nndata.tbin.compr \
   texel${VER}/
rm texel${VER}/doc/.gitignore
chmod -R ug+w texel${VER}
7za a texel${VER}.7z texel${VER}
rm -rf texel${VER}
