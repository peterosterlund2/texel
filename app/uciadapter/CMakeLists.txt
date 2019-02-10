set(src_uciadapter
  ctgbook.cpp     ctgbook.hpp
  uciadapter.cpp  uciadapter.hpp
  )

if(UNIX)
  add_executable(uciadapter ${src_uciadapter})
  target_link_libraries(uciadapter texellib)
endif()
