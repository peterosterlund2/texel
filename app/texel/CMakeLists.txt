set(src_texel
  enginecontrol.cpp
  texel.cpp
  tuigame.cpp
  uciprotocol.cpp
  )

add_executable(texel ${src_texel})
target_link_libraries(texel texellib)
