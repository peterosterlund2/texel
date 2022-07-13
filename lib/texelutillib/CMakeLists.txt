set(src_pg
                         pg/assignment.hpp
                         pg/bitSet.hpp
  pg/cspsolver.cpp       pg/cspsolver.hpp
  pg/extproofkernel.cpp  pg/extproofkernel.hpp
  pg/pkseq.cpp           pg/pkseq.hpp
  pg/proofgame.cpp       pg/proofgame.hpp
  pg/proofgamefilter.cpp pg/proofgamefilter.hpp
  pg/proofkernel.cpp     pg/proofkernel.hpp
  )

set(src_texelutillib
  bookbuild.cpp       bookbuild.hpp
  gametree.cpp        gametree.hpp
                      gametreeutil.hpp
  nnutil.cpp          nnutil.hpp
  posutil.cpp         posutil.hpp
  revmovegen.cpp      revmovegen.hpp
                      stloutput.hpp
                      threadpool.hpp
  tbpath.cpp          tbpath.hpp
  )

add_library(texelutillib STATIC
  ${src_pg}
  ${src_texelutillib}
  )
target_link_libraries(texelutillib
  PUBLIC texellib
  )
target_include_directories(texelutillib
  PUBLIC . pg
  )
