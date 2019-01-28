set(src_uciadapter
  ctgbook.cpp
  uciadapter.cpp
  )

if(UNIX)
  add_executable(uciadapter ${src_uciadapter})
  target_link_libraries(uciadapter texellib)
endif()
