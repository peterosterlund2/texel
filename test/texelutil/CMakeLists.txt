set(src_texelutiltest
  bookBuildTest.cpp  bookBuildTest.hpp
  gameTreeTest.cpp   gameTreeTest.hpp
  proofgameTest.cpp  proofgameTest.hpp
  texelutiltest.cpp
                     utilSuiteBase.hpp
  )

add_executable(texelutiltest ${src_texelutiltest})
target_link_libraries(texelutiltest texelutillib cute)
