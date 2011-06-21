cmake_minimum_required(VERSION 2.6)
project(ncrok)
include(FindPkgConfig)

aux_source_directory(src sources)

pkg_check_modules(GST gstreamer-0.10)
pkg_check_modules(TAG taglib)

include_directories(include ${GST_INCLUDE_DIRS} ${TAG_INCLUDE_DIRS})

link_directories(${GST_LIBRARY_DIRS} ${TAG_LIBRARY_DIRS})
add_definitions(-O2 -g)
link_libraries(pthread  ${GST_LIBRARIES} ${TAG_LIBRARIES} menu panel ncurses)
add_executable(ncrok ${sources})

install(TARGETS ncrok
	RUNTIME DESTINATION bin
)

