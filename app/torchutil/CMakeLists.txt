set(src_torchutil
  torchutil.cpp
  dataset.cpp      dataset.hpp
  randperm.cpp     randperm.hpp
  featureperm.cpp  featureperm.hpp
  )

if(NOT CMAKE_CROSSCOMPILING)
  find_package(Torch
    HINTS $ENV{LIBTORCH_DIR})

  if(TORCH_LIBRARIES)
    add_executable(torchutil ${src_torchutil})
    set_property(TARGET torchutil PROPERTY CXX_STANDARD 14)
    target_compile_options(torchutil PUBLIC ${TORCH_CXX_FLAGS})
    target_link_libraries(torchutil "${TORCH_LIBRARIES}")
    target_link_libraries(torchutil texellib texelutillib)
  else()
    message("Not building torchutil")
  endif()
endif()
