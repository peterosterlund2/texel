set(src_texelutillib
                 assignment.hpp
  bookbuild.cpp  bookbuild.hpp
  gametree.cpp   gametree.hpp
  proofgame.cpp  proofgame.hpp
                 stloutput.hpp
                 threadpool.hpp
  )

add_library(texelutillib STATIC
  ${src_texelutillib}
  )
target_link_libraries(texelutillib
  PUBLIC texellib
  )
target_include_directories(texelutillib
  INTERFACE .
  )
