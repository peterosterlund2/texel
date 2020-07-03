set(src_texelutiltest
  bookBuildTest.cpp  bookBuildTest.hpp
  gameTreeTest.cpp
  proofgameTest.cpp  proofgameTest.hpp
  texelutiltest.cpp
  )

add_executable(texelutiltest ${src_texelutiltest})
target_link_libraries(texelutiltest texelutillib gtest)
