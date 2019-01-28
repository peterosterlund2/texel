set(src_texellibtest
  bitBoardTest.cpp
  bookTest.cpp
  computerPlayerTest.cpp
  evaluateTest.cpp
  gameTest.cpp
  historyTest.cpp
  killerTableTest.cpp
  moveGenTest.cpp
  moveTest.cpp
  parallelTest.cpp
  pieceTest.cpp
  polyglotTest.cpp
  positionTest.cpp
  searchTest.cpp
  tbTest.cpp
  tbgenTest.cpp
  texelTest.cpp
  textioTest.cpp
  transpositionTableTest.cpp
  treeLoggerTest.cpp
  utilTest.cpp
  )

add_executable(texellibtest ${src_texellibtest})
target_link_libraries(texellibtest texellib cute)
