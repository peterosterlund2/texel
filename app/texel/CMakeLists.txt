set(src_texel
  enginecontrol.cpp  enginecontrol.hpp
                     searchparams.hpp
  texel.cpp
  tuigame.cpp        tuigame.hpp
  uciprotocol.cpp    uciprotocol.hpp
  )

add_executable(texel ${src_texel})
target_link_libraries(texel texellib)
