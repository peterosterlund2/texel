set(src_texelutiltest
  bookBuildTest.cpp
  gameTreeTest.cpp
  proofgameTest.cpp
  texelutiltest.cpp
  )

add_executable(texelutiltest ${src_texelutiltest})
target_link_libraries(texelutiltest texelutillib cute)
