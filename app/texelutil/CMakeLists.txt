set(src_texelutil
  chesstool.cpp
  matchbookcreator.cpp
  posgen.cpp
  spsa.cpp
  texelutil.cpp
  )

set(all_found true)

find_package(Armadillo)
if(NOT ARMADILLO_FOUND)
  message(STATUS "Armadillo library not found")
  set(all_found false)
endif()

find_package(GSL)
if(NOT GSL_FOUND)
  message(STATUS "GSL library not found")
  set(all_found false)
endif()

find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND)
  message(STATUS "OpenMP not found")
  set(all_found false)
endif()

if(all_found)
  add_executable(texelutil ${src_texelutil})
  target_include_directories(texelutil
    PUBLIC ${ARMADILLO_INCLUDE_DIRS})
  target_link_libraries(texelutil
    PUBLIC texelutillib
    PUBLIC ${ARMADILLO_LIBRARIES}
    PUBLIC GSL::gsl
    PUBLIC OpenMP::OpenMP_CXX
    )
else()
  message("Not building texelutil")
endif()
