set(src_torchutil_lib
  dataset.cpp      dataset.hpp
  randperm.cpp     randperm.hpp
  featureperm.cpp  featureperm.hpp
  )

set(src_torchutil
  torchutil.cpp
  )

set(all_ok OFF)
if(NOT CMAKE_CROSSCOMPILING)
  find_package(Torch
    HINTS $ENV{LIBTORCH_DIR})
  if(TORCH_LIBRARIES)
    set(all_ok ON)
  endif()
endif()

set(ENABLE_TORCHUTIL ${all_ok} CACHE STRING "" FORCE)
mark_as_advanced(ENABLE_TORCHUTIL)

if(ENABLE_TORCHUTIL)
  add_library(torchutillib STATIC
    ${src_torchutil_lib}
    )
  target_link_libraries(torchutillib texellib texelutillib)
  target_include_directories(texelutillib
    PUBLIC . pg
    )

  add_executable(torchutil ${src_torchutil})
  target_compile_options(torchutil PUBLIC ${TORCH_CXX_FLAGS})
  target_link_libraries(torchutil "${TORCH_LIBRARIES}")
  target_link_libraries(torchutil torchutillib)
else()
  message("Not building torchutil")
endif()
