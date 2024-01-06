set(src_torchutiltest
  datasetTest.cpp   datasetTest.hpp
  randpermTest.cpp  randpermTest.hpp
  torchutiltest.cpp
  )

if(ENABLE_TORCHUTIL)
  add_executable(torchutiltest ${src_torchutiltest})
  target_link_libraries(torchutiltest torchutillib gtest)
  if(BUILD_TESTING)
    gtest_discover_tests(torchutiltest)
  endif()
else()
  message("Not building torchutiltest")
endif()
