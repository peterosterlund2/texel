set(src_texelutillib
  bookbuild.cpp
  gametree.cpp
  proofgame.cpp
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
