set(src_book
  book/book.cpp           book/book.hpp
  book/polyglot.cpp       book/polyglot.hpp
  )

set(src_debug
                              debug/histogram.hpp
  debug/logger.cpp            debug/logger.hpp
  debug/searchTreeSampler.cpp debug/searchTreeSampler.hpp
  debug/treeLogger.cpp        debug/treeLogger.hpp
  )

set(src_hw
                          hw/alignedAlloc.hpp
  hw/cluster.cpp          hw/cluster.hpp
  hw/clustertt.cpp        hw/clustertt.hpp
  hw/largePageAlloc.cpp   hw/largePageAlloc.hpp
  hw/numa.cpp             hw/numa.hpp
  hw/parallel.cpp         hw/parallel.hpp
  )

set(src_nn
                          nn/incbin.h
  nn/nneval.cpp           nn/nneval.hpp
  nn/nntypes.cpp          nn/nntypes.hpp
                          nn/vectorop.hpp
  )

set(src_tb
  tb/kpkTable.cpp
  tb/krkpTable.cpp
  tb/krpkrTable.cpp
  tb/tbgen.cpp            tb/tbgen.hpp
  tb/tbprobe.cpp          tb/tbprobe.hpp
  )

set(src_gtb
  tb/gtb/compression/lzma/Lzma86Dec.c
  tb/gtb/compression/lzma/LzFind.c
  tb/gtb/compression/lzma/Lzma86Enc.c
  tb/gtb/compression/lzma/LzmaDec.c
  tb/gtb/compression/lzma/Alloc.c
  tb/gtb/compression/lzma/Bra86.c
  tb/gtb/compression/lzma/LzmaEnc.c
  tb/gtb/compression/wrap.c
  tb/gtb/gtb-dec.c
  tb/gtb/gtb-att.c
  tb/gtb/sysport/sysport.c
  tb/gtb/gtb-probe.c
  )

set(src_syzygy
  tb/syzygy/rtb-probe.cpp tb/syzygy/rtb-probe.hpp
                          tb/syzygy/rtb-core-impl.hpp
  )

set(src_util
  util/random.cpp         util/random.hpp
  util/timeUtil.cpp       util/timeUtil.hpp
  util/util.cpp           util/util.hpp
  )

set(src_texellib
                          binfile.hpp
  bitBoard.cpp            bitBoard.hpp
                          chessError.hpp
  computerPlayer.cpp      computerPlayer.hpp
                          constants.hpp
  endGameEval.cpp         endGameEval.hpp
  evaluate.cpp            evaluate.hpp
  game.cpp                game.hpp
  history.cpp             history.hpp
  humanPlayer.cpp         humanPlayer.hpp
  killerTable.cpp         killerTable.hpp
  material.cpp            material.hpp
  move.cpp                move.hpp
  moveGen.cpp             moveGen.hpp
  parameters.cpp          parameters.hpp
  piece.cpp               piece.hpp
                          player.hpp
  position.cpp            position.hpp
  search.cpp              search.hpp
                          searchUtil.hpp
                          square.hpp
  textio.cpp              textio.hpp
                          threadpool.hpp
  transpositionTable.cpp  transpositionTable.hpp
  undoInfo.hpp
  )

set(NNDATA_FILE ${texel_SOURCE_DIR}/nndata.tbin.compr)
set(src_nndata ${CMAKE_CURRENT_BINARY_DIR}/nndata.cpp)
configure_file(nn/nndata.cpp.in ${src_nndata})

if(MSVC)
  add_executable(incbin nn/incbin.c)
  set(nndata_gen ${CMAKE_CURRENT_BINARY_DIR}/nndata_gen.c)
  add_custom_command(
    OUTPUT ${nndata_gen}
    COMMAND $<TARGET_FILE:incbin> ${src_nndata} -o ${nndata_gen}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS incbin ${src_nndata} ${NNDATA_FILE}
    )
endif()

add_library(texellib STATIC
  ${src_book} ${src_debug} ${src_hw} ${src_nn} ${src_tb} ${src_util}
  ${src_gtb} ${src_syzygy}
  ${src_texellib}
  ${src_nndata} ${nndata_gen}
  )
target_include_directories(texellib
  PUBLIC . book debug hw nn tb util
  PRIVATE tb/gtb/sysport tb/gtb/compression tb/gtb/compression/lzma
  )

set_source_files_properties(${src_nndata}
  PROPERTIES OBJECT_DEPENDS ${NNDATA_FILE})

if(UNIX AND NOT ANDROID)
  find_library(RT_LIB rt)
  if(RT_LIB)
    target_compile_definitions(texellib
      PRIVATE "HAS_RT"
      )
    target_link_libraries(texellib
      PUBLIC ${RT_LIB}
      )
  else()
    message(STATUS "librt not found.")
  endif()
elseif(ANDROID)
  target_compile_definitions(texellib
    PRIVATE "HAS_RT"
    )
endif()

if(UNIX AND NOT ANDROID)
  try_compile(ATOMIC_WITHOUT_LIB ${CMAKE_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/cmake/checks/atomic.cpp
    CXX_STANDARD 11
    CXX_EXTENSIONS OFF
    )
  if(NOT ATOMIC_WITHOUT_LIB)
    try_compile(ATOMIC_WITH_LIBATOMIC ${CMAKE_BINARY_DIR}
      ${PROJECT_SOURCE_DIR}/cmake/checks/atomic.cpp
      LINK_LIBRARIES atomic
      CXX_STANDARD 11
      CXX_EXTENSIONS OFF
      )
    if(ATOMIC_WITH_LIBATOMIC)
      message(STATUS "Linking with -latomic")
      target_link_libraries(texellib
        PUBLIC atomic
        )
    else()
      message(FATAL_ERROR "Cannot use std::atomic<T>")
    endif()
  endif()
endif()

if(USE_SSSE3 OR USE_AVX2 OR USE_AVX512)
  target_compile_definitions(texellib
    PUBLIC "USE_SSSE3")
endif()
if(USE_AVX2 OR USE_AVX512)
  target_compile_definitions(texellib
    PUBLIC "USE_AVX2")
endif()
if(USE_AVX512)
  target_compile_definitions(texellib
    PUBLIC "USE_AVX512")
endif()

if(USE_NEON OR USE_NEON_DOT)
  target_compile_definitions(texellib
    PUBLIC "USE_NEON")
endif()
if(USE_NEON_DOT)
  require_compiler_flag("-march=armv8.2-a+dotprod")
  target_compile_options(texellib
    PUBLIC "-march=armv8.2-a+dotprod")
  target_compile_definitions(texellib
    PUBLIC "USE_NEON_DOT")
endif()

if(USE_BMI2)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    require_compiler_flag("-mbmi2")
    target_compile_options(texellib
      PUBLIC "-mbmi2")
  endif()
  target_compile_definitions(texellib
    PUBLIC "USE_BMI2")
endif()

if(USE_POPCNT)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    require_compiler_flag("-mpopcnt")
    target_compile_options(texellib
      PUBLIC "-mpopcnt")
  endif()
  target_compile_definitions(texellib
    PUBLIC "USE_POPCNT")
endif()

if(USE_CTZ)
  target_compile_definitions(texellib
    PUBLIC "USE_CTZ")
endif()

if(USE_PREFETCH)
  target_compile_definitions(texellib
    PUBLIC "USE_PREFETCH")
endif()

if(USE_LARGE_PAGES)
  target_compile_definitions(texellib
    PRIVATE "USE_LARGE_PAGES")
endif()

if(USE_NUMA)
  target_compile_definitions(texellib
    PRIVATE "NUMA")

  if(UNIX)
    find_library(NUMA_LIB numa)
    if(NOT NUMA_LIB)
      message(FATAL_ERROR "numa library not found")
    endif()
    target_link_libraries(texellib
      PUBLIC ${NUMA_LIB}
      )
  endif()
endif()

if(USE_CLUSTER)
  target_compile_definitions(texellib
    PUBLIC "CLUSTER")
  find_package(MPI)
  if(MPI_CXX_FOUND)
    target_link_libraries(texellib
      PUBLIC MPI::MPI_CXX)
  elseif(MINGW AND DEFINED ENV{MINGW_MPI})
    target_include_directories(texellib
      PUBLIC "$ENV{MINGW_MPI}/include")
    target_link_libraries(texellib
      PUBLIC "$ENV{MINGW_MPI}/libmsmpi.a")
  else()
    message(FATAL_ERROR "MPI library not found")
  endif()
endif()
