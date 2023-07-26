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
           -DUSE_AVX2=on \
           -DUSE_BMI2=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &


# Windows 64-bit, SSSE3
mkdir -p build/win64_ssse3
(cd build/win64_ssse3 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=off \
           -DCPU_TYPE=athlon64-sse3 \
           -DUSE_SSSE3=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=off \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit, AVX2
mkdir -p build/win64_avx2
(cd build/win64_avx2 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=corei7 \
           -DUSE_AVX2=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=off \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit, AVX2 + POPCNT
mkdir -p build/win64
(cd build/win64 &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=corei7 \
           -DUSE_AVX2=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &

# Windows 64-bit with AVX2 + POPCNT + BMI2
mkdir -p build/win64_bmi
(cd build/win64_bmi &&
     cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/win64.cmake \
           -DUSE_WIN7=on \
           -DCPU_TYPE=corei7 \
           -DUSE_AVX2=on \
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
           -DUSE_AVX2=on \
           -DUSE_CLUSTER=on \
           -DUSE_CTZ=on \
           -DUSE_LARGE_PAGES=on \
           -DUSE_NUMA=on \
           -DUSE_POPCNT=on \
           -DUSE_PREFETCH=on
) &


# Android 64-bit with NEON
mkdir -p build/android64
(cd build/android64 &&
     cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
           -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 ../.. \
           -DCMAKE_BUILD_TYPE=Release \
           -DUSE_CTZ=on \
           -DUSE_PREFETCH=on \
           -DUSE_NEON=on
) &

# Android 64-bit with NEON and dot product
mkdir -p build/android64_dot
(cd build/android64_dot &&
     cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
           -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 ../.. \
           -DCMAKE_BUILD_TYPE=Release \
           -DUSE_CTZ=on \
           -DUSE_PREFETCH=on \
           -DUSE_NEON_DOT=on
) &

wait

# Build all
para=8
cmake --build build/Release       -j ${para} || exit 2
cmake --build build/win64_ssse3   -j ${para} || exit 2
cmake --build build/win64_avx2    -j ${para} || exit 2
cmake --build build/win64         -j ${para} || exit 2
cmake --build build/win64_bmi     -j ${para} || exit 2
cmake --build build/win64cl       -j ${para} || exit 2
cmake --build build/android64     -j ${para} || exit 2
cmake --build build/android64_dot -j ${para} || exit 2
make -C doc

# Copy to bin and strip executables
mkdir -p bin

cp build/Release/texel bin/texel64
strip bin/texel64

cp build/win64_ssse3/texel.exe bin/texel64-ssse3.exe
x86_64-w64-mingw32-strip bin/texel64-ssse3.exe

cp build/win64_avx2/texel.exe bin/texel64-avx2.exe
x86_64-w64-mingw32-strip bin/texel64-avx2.exe

cp build/win64/texel.exe bin/texel64-avx2-pop.exe
x86_64-w64-mingw32-strip bin/texel64-avx2-pop.exe
cp build/win64/runcmd.exe bin/runcmd.exe
x86_64-w64-mingw32-strip bin/runcmd.exe
cp build/win64/texelutil.exe bin/texelutil.exe
x86_64-w64-mingw32-strip bin/texelutil.exe

cp build/win64_bmi/texel.exe bin/texel64-avx2-bmi.exe
x86_64-w64-mingw32-strip bin/texel64-avx2-bmi.exe

cp build/win64cl/texel.exe bin/texel64cl.exe
x86_64-w64-mingw32-strip bin/texel64cl.exe

cp build/android64/texel bin/texel-arm64
aarch64-linux-android-strip bin/texel-arm64

cp build/android64_dot/texel bin/texel-arm64-dot
aarch64-linux-android-strip bin/texel-arm64-dot

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
