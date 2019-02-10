set(src_texellibtest
  bitBoardTest.cpp            bitBoardTest.hpp
  bookTest.cpp                bookTest.hpp
  computerPlayerTest.cpp      computerPlayerTest.hpp
  evaluateTest.cpp            evaluateTest.hpp
  gameTest.cpp                gameTest.hpp
  historyTest.cpp             historyTest.hpp
  killerTableTest.cpp         killerTableTest.hpp
  moveGenTest.cpp             moveGenTest.hpp
  moveTest.cpp                moveTest.hpp
  parallelTest.cpp            parallelTest.hpp
  pieceTest.cpp               pieceTest.hpp
  polyglotTest.cpp            polyglotTest.hpp
  positionTest.cpp            positionTest.hpp
  searchTest.cpp              searchTest.hpp
                              suiteBase.hpp
  tbTest.cpp                  tbTest.hpp
  tbgenTest.cpp               tbgenTest.hpp
  texelTest.cpp
  textioTest.cpp              textioTest.hpp
  transpositionTableTest.cpp  transpositionTableTest.hpp
  treeLoggerTest.cpp          treeLoggerTest.hpp
  utilTest.cpp                utilTest.hpp
  )

add_executable(texellibtest ${src_texellibtest})
target_link_libraries(texellibtest texellib cute)
