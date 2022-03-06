set(GOOGLETEST_VERSION "1.10.0")
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

if(BUILD_TESTING)
  include(GoogleTest)
endif()

add_subdirectory(gtest)
add_subdirectory(texellib)
add_subdirectory(texelutil)
