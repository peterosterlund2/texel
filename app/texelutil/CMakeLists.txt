set(src_texelutil
  chesstool.cpp         chesstool.hpp
  matchbookcreator.cpp  matchbookcreator.hpp
  posgen.cpp            posgen.hpp
  spsa.cpp              spsa.hpp
  texelutil.cpp
  )

add_executable(texelutil ${src_texelutil})
target_link_libraries(texelutil
  PUBLIC texelutillib)

find_package(Armadillo)
if(ARMADILLO_FOUND)
  target_compile_definitions(texelutil
    PUBLIC "USE_ARMADILLO")
  target_include_directories(texelutil
    PUBLIC ${ARMADILLO_INCLUDE_DIRS})
  target_link_libraries(texelutil
    PUBLIC ${ARMADILLO_LIBRARIES})
endif()

find_package(GSL)
if(GSL_FOUND)
  target_compile_definitions(texelutil
    PUBLIC "USE_GSL")
  target_link_libraries(texelutil
    PUBLIC GSL::gsl)
endif()
