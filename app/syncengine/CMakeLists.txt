set(src_syncengine
  syncengine.cpp
  )

if(UNIX)
  add_executable(syncengine ${src_syncengine})
  target_link_libraries(syncengine texellib)
endif()
