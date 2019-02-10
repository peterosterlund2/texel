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
                          util/histogram.hpp
  util/logger.cpp         util/logger.hpp
  util/random.cpp         util/random.hpp
  util/timeUtil.cpp       util/timeUtil.hpp
  util/util.cpp           util/util.hpp
  )

set(src_texellib
  bitBoard.cpp            bitBoard.hpp
  book.cpp                book.hpp
                          chessParseError.hpp
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
  polyglot.cpp            polyglot.hpp
  position.cpp            position.hpp
  search.cpp              search.hpp
                          searchUtil.hpp
                          square.hpp
  tbgen.cpp               tbgen.hpp
  tbprobe.cpp             tbprobe.hpp
  textio.cpp              textio.hpp
  transpositionTable.cpp  transpositionTable.hpp
  treeLogger.cpp          treeLogger.hpp
  undoInfo.hpp
  )

add_library(texellib STATIC
  ${src_gtb}
  ${src_syzygy}
  ${src_texellib}
  ${src_util}
  )
target_include_directories(texellib
  INTERFACE .
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

if(USE_BMI2)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    require_compiler_flag("-mbmi2")
    target_compile_options(texellib
      PUBLIC "-mbmi2")
  endif()
  target_compile_definitions(texellib
    PUBLIC "HAS_BMI2")
endif()

if(USE_POPCNT)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
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
