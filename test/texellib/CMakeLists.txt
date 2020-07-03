set(src_texellibtest
  bitBoardTest.cpp
  bookTest.cpp
  computerPlayerTest.cpp      computerPlayerTest.hpp
  evaluateTest.cpp            evaluateTest.hpp
  gameTest.cpp                gameTest.hpp
  historyTest.cpp
  killerTableTest.cpp
  moveGenTest.cpp
  moveTest.cpp
  parallelTest.cpp
  pieceTest.cpp
  polyglotTest.cpp
  positionTest.cpp            positionTest.hpp
  searchTest.cpp              searchTest.hpp
  tbgenTest.cpp               tbgenTest.hpp
  tbTest.cpp                  tbTest.hpp
  texelTest.cpp
  textioTest.cpp
  transpositionTableTest.cpp
  treeLoggerTest.cpp          treeLoggerTest.hpp
  utilTest.cpp
  )

add_executable(texellibtest ${src_texellibtest})
target_link_libraries(texellibtest texellib gtest)
