set(src_torchtrain
  torchtrain.cpp
  )

if(NOT CMAKE_CROSSCOMPILING)
  find_package(Torch
    HINTS $ENV{LIBTORCH_DIR})

  if(TORCH_LIBRARIES)
    add_executable(torchtrain ${src_torchtrain})
    set_property(TARGET torchtrain PROPERTY CXX_STANDARD 14)
    target_compile_options(torchtrain PUBLIC ${TORCH_CXX_FLAGS})
    target_link_libraries(torchtrain "${TORCH_LIBRARIES}")
  else()
    message("Not building PyTorch trainer")
  endif()
endif()
