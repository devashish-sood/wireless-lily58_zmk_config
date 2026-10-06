if(CONFIG_ZMK_DISPLAY)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(render.c screen.c sync.c)
endif()
