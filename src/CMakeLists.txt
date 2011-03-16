include(FindPkgConfig)

pkg_check_modules(GST gstreamer-0.10)
pkg_check_modules(TAG taglib)
pkg_check_modules(NCURSES ncurses menu panel)
include_directories(../include ${GST_INCLUDE_DIRS} ${TAG_INCLUDE_DIRS} ${NCURSES_INCLUDE_DIRS})

link_directories(${GST_LIBRARY_DIRS} ${TAG_LIBRARY_DIRS} ${NCURSES_LIBRARY_DIRS})
add_definitions(-O2)
link_libraries(pthread  ${GST_LIBRARIES} ${TAG_LIBRARIES} ${NCURSES_LIBRARIES})
add_executable(../ncrok ncrok.cpp output.cpp playlist.cpp tune.cpp	util.cpp window.cpp)
INSTALL_TARGETS(/bin ../ncrok)
