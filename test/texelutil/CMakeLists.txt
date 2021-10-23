set(src_texelutiltest
  bookBuildTest.cpp   bookBuildTest.hpp
  gameTreeTest.cpp
  proofgameTest.cpp   proofgameTest.hpp
  proofkernelTest.cpp proofkernelTest.hpp
  revmovegenTest.cpp  revmovegenTest.hpp
  texelutiltest.cpp
  )

add_executable(texelutiltest ${src_texelutiltest})
target_link_libraries(texelutiltest texelutillib gtest)
