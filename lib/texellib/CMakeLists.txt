set(src_book
  book/book.cpp           book/book.hpp
  book/polyglot.cpp       book/polyglot.hpp
  )

set(src_debug
                          debug/histogram.hpp
  debug/logger.cpp        debug/logger.hpp
  debug/treeLogger.cpp    debug/treeLogger.hpp
  )

set(src_gtb
  gtb/compression/lzma/Lzma86Dec.c
  gtb/compression/lzma/LzFind.c
  gtb/compression/lzma/Lzma86Enc.c
  gtb/compression/lzma/LzmaDec.c
  gtb/compression/lzma/Alloc.c
  gtb/compression/lzma/Bra86.c
  gtb/compression/lzma/LzmaEnc.c
  gtb/compression/wrap.c
  gtb/gtb-dec.c
  gtb/gtb-att.c
  gtb/sysport/sysport.c
  gtb/gtb-probe.c
  )

set(src_syzygy
  syzygy/rtb-probe.cpp    syzygy/rtb-probe.hpp
                          syzygy/rtb-core-impl.hpp
  )

set(src_util
                          util/alignedAlloc.hpp
  util/random.cpp         util/random.hpp
  util/timeUtil.cpp       util/timeUtil.hpp
  util/util.cpp           util/util.hpp
  )

set(src_texellib
  bitBoard.cpp            bitBoard.hpp
                          chessError.hpp
  cluster.cpp             cluster.hpp
  clustertt.cpp           clustertt.hpp
  computerPlayer.cpp      computerPlayer.hpp
                          constants.hpp
  endGameEval.cpp         endGameEval.hpp
  evaluate.cpp            evaluate.hpp
  game.cpp                game.hpp
  history.cpp             history.hpp
  humanPlayer.cpp         humanPlayer.hpp
  killerTable.cpp         killerTable.hpp
  kpkTable.cpp
  krkpTable.cpp
  krpkrTable.cpp
  largePageAlloc.cpp      largePageAlloc.hpp
  material.cpp            material.hpp
  move.cpp                move.hpp
  moveGen.cpp             moveGen.hpp
  numa.cpp                numa.hpp
  parallel.cpp            parallel.hpp
  parameters.cpp          parameters.hpp
  piece.cpp               piece.hpp
                          player.hpp
  position.cpp            position.hpp
  search.cpp              search.hpp
                          searchUtil.hpp
                          square.hpp
  tbgen.cpp               tbgen.hpp
  tbprobe.cpp             tbprobe.hpp
  textio.cpp              textio.hpp
  transpositionTable.cpp  transpositionTable.hpp
  undoInfo.hpp
  )

add_library(texellib STATIC
  ${src_book} ${src_debug} ${src_util}
  ${src_gtb} ${src_syzygy}
  ${src_texellib}
  )
target_include_directories(texellib
  PUBLIC . book debug util
  PRIVATE gtb/sysport gtb/compression gtb/compression/lzma
  )

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

if(USE_BMI2)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    require_compiler_flag("-mbmi2")
    target_compile_options(texellib
      PUBLIC "-mbmi2")
  endif()
  target_compile_definitions(texellib
    PUBLIC "HAS_BMI2")
endif()

if(USE_POPCNT)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    require_compiler_flag("-mpopcnt")
    target_compile_options(texellib
      PUBLIC "-mpopcnt")
  endif()
  target_compile_definitions(texellib
    PUBLIC "HAS_POPCNT")
endif()

if(USE_CTZ)
  target_compile_definitions(texellib
    PUBLIC "HAS_CTZ")
endif()

if(USE_PREFETCH)
  target_compile_definitions(texellib
    PUBLIC "HAS_PREFETCH")
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
