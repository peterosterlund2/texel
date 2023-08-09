cmake_minimum_required(VERSION 3.12)


# Define project
project(texel
  DESCRIPTION "Texel chess engine and utilities"
  LANGUAGES CXX C)

set(is_64bit false)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(is_64bit true)
endif()

# Options
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(CPU_TYPE "generic" CACHE STRING "Type of CPU to generate code for")
  set_property(CACHE CPU_TYPE PROPERTY STRINGS
    generic
    athlon64-sse3
    corei7)
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_" OR
   CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
  option(USE_SSSE3 "Use SSSE3 CPU instructions" OFF)
  option(USE_AVX2 "Use AVX2 CPU instructions" OFF)
  if(is_64bit)
    option(USE_BMI2 "Use BMI2 CPU instructions" OFF)
  endif()
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_" OR
   CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
  option(USE_POPCNT "Use popcount CPU instructions" OFF)
endif()
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR
   CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7-a")
  option(USE_NEON "Use NEON CPU instructions" OFF)
  option(USE_NEON_DOT "Use NEON dot product CPU instructions" OFF)
endif()
option(USE_CTZ "Use CTZ (BitScanForward) CPU instructions" OFF)
option(USE_PREFETCH "Use prefetch CPU instructions" OFF)
if(NOT ANDROID)
  option(USE_LARGE_PAGES "Use large pages when allocating memory" OFF)
  option(USE_NUMA "Optimize thread affinity on NUMA hardware" OFF)
  option(USE_CLUSTER "Use MPI to distribute search to several computers" OFF)
endif()
if(WIN32)
  option(USE_WIN7 "Compile for Windows 7 and later" OFF)
endif()


# Require C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "CYGWIN" AND
   NOT CMAKE_SYSTEM_NAME STREQUAL "MSYS")
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()


# Set default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  message(STATUS "Setting build type: ${CMAKE_BUILD_TYPE}")
endif()


# Functions to check for supported compiler flags
include(CheckCXXCompilerFlag)
function(add_compiler_flag_if_supported flag)
  check_cxx_compiler_flag(${flag} supported_${flag})
  if(supported_${flag})
    add_compile_options(${flag})
  endif()
endfunction()

function(require_compiler_flag flag)
  check_cxx_compiler_flag(${flag} supported_${flag})
  if(NOT supported_${flag})
    message(FATAL_ERROR "Unsupported compiler flag ${flag}")
  endif()
endfunction()


# Enable compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options("-Wall")
endif()
add_compiler_flag_if_supported("-Wno-misleading-indentation")
add_compiler_flag_if_supported("-Wno-unused-result")
add_compiler_flag_if_supported("-Wno-psabi")

# Use -O3 instead of -O2, remove -DNDEBUG
foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  if(${flag_var} MATCHES "-O2")
    string(REGEX REPLACE "-O2" "-O3" ${flag_var} "${${flag_var}}")
  endif()
  if(${flag_var} MATCHES "-DNDEBUG")
    string(REGEX REPLACE "-DNDEBUG" "" ${flag_var} "${${flag_var}}")
  endif()
endforeach()
add_compiler_flag_if_supported("-fno-stack-protector")

# Enable CPU architecture
if(CPU_TYPE STREQUAL "corei7")
  require_compiler_flag("-march=corei7")
  add_compile_options("-march=corei7")
elseif(CPU_TYPE STREQUAL "athlon64-sse3")
  require_compiler_flag("-march=athlon64-sse3")
  add_compile_options("-march=athlon64-sse3")
endif()

if(USE_SSSE3)
  add_compiler_flag_if_supported("-mssse3")
endif()
if(USE_AVX2)
  add_compiler_flag_if_supported("-mavx2")
endif()

# Set Windows target version
if(USE_WIN7)
  add_compile_definitions("_WIN32_WINNT=_WIN32_WINNT_WIN7")
endif()

# Link with threads library, needed by gcc/clang when using C++11 threads
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  find_package(Threads REQUIRED)
  link_libraries(Threads::Threads)
endif()

# Put all executable files in the build directory root
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})


# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
endif()


# MSVC settings
if(MSVC)
  add_compile_definitions("NOMINMAX")
  add_compile_options("/wd4146" "/wd4244" "/wd4267" "/wd4334" "/wd4996")
  set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
               PROPERTY VS_STARTUP_PROJECT texel)
endif()


# Eclipse settings
if(CMAKE_EXTRA_GENERATOR STREQUAL "Eclipse CDT4" AND
    NOT CDT_ALREADY_CONFIGURED)
  set(CMAKE_ECLIPSE_VERSION "4.5 (Mars)" CACHE STRING "" FORCE)
  set(CMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES "OFF" CACHE BOOL "" FORCE)
  set(CDT_ALREADY_CONFIGURED "ON" CACHE INTERNAL "")
endif()


# Testing
if(NOT CMAKE_CROSSCOMPILING)
  find_program(MEMORYCHECK_COMMAND NAMES valgrind)
  set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")
  include(CTest)
  enable_testing()
endif()


# Subdirectories
add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(test)
