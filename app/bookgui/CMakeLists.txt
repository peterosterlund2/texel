set(src_bookgui
  bookbuildcontrol.cpp
  bookgui.cpp
  chessboard.cpp
  )

set(all_ok false)
if(NOT CMAKE_CROSS_COMPILING)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(GTKMM IMPORTED_TARGET gtkmm-3.0)
    if(GTKMM_FOUND)
      find_package(Freetype)
      if(FREETYPE_FOUND)
        find_program(GCR_PROGRAM glib-compile-resources)
        if(GCR_PROGRAM)
          set(all_ok true)
        endif()
      endif()
    endif()
  endif()
endif()

if(all_ok)
  set(resource_cpp "${CMAKE_CURRENT_BINARY_DIR}/resource.cpp")
  add_custom_command(
    OUTPUT ${resource_cpp}
    COMMAND ${GCR_PROGRAM} --generate-source --target=${resource_cpp} gresource.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS gresource.xml bookgui_glade.xml ChessCases.ttf
    )

  add_executable(bookgui ${src_bookgui} ${resource_cpp})
  target_link_libraries(bookgui
    PUBLIC texellib texelutillib
    PUBLIC PkgConfig::GTKMM
    PUBLIC Freetype::Freetype
    )
else()
  message("Not building bookgui")
endif()
