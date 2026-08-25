set(src_texelutiltest
  bookBuildTest.cpp
  cspsolverTest.cpp
  gameTreeTest.cpp
  nnutilTest.cpp
  proofgameTest.cpp
  proofkernelTest.cpp
  revmovegenTest.cpp
  texelutiltest.cpp
  )

add_executable(texelutiltest ${src_texelutiltest})
target_link_libraries(texelutiltest texelutillib gtest)
if(BUILD_TESTING)
  gtest_discover_tests(texelutiltest)
endif()
