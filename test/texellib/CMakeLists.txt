set(src_texellibtest
  bitBoardTest.cpp
  bookTest.cpp
  computerPlayerTest.cpp
  evaluateTest.cpp            evaluateTest.hpp
  gameTest.cpp
  historyTest.cpp
  killerTableTest.cpp
  moveGenTest.cpp
  moveTest.cpp
  nnTest.cpp
  parallelTest.cpp
  pieceTest.cpp
  polyglotTest.cpp
  positionTest.cpp            positionTest.hpp
  searchTest.cpp              searchTest.hpp
  tbgenTest.cpp
  tbTest.cpp                  tbTest.hpp
  texelTest.cpp
  textioTest.cpp
  transpositionTableTest.cpp
  treeLoggerTest.cpp
  utilTest.cpp
  )

add_executable(texellibtest ${src_texellibtest})
target_link_libraries(texellibtest texellib texelutillib gtest)
if(BUILD_TESTING)
  gtest_discover_tests(texellibtest)
endif()
