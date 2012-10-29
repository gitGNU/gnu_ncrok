cmake_minimum_required(VERSION 2.8)
project(ncrok)

find_package(PkgConfig)

set(SRC_DIR src)

set(SOURCES
	${SRC_DIR}/ncrok.cpp
	${SRC_DIR}/playlist.cpp
	${SRC_DIR}/window.cpp
	${SRC_DIR}/playlistwindow.cpp
	${SRC_DIR}/helpwindow.cpp
	${SRC_DIR}/bottomwindow.cpp
	${SRC_DIR}/tune.cpp
	${SRC_DIR}/audio.cpp
)

pkg_check_modules(GST REQUIRED gstreamer-0.10)
pkg_check_modules(TAG REQUIRED taglib)

include_directories(include ${GST_INCLUDE_DIRS} ${TAG_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS} ${TAG_LIBRARY_DIRS})

add_definitions(-O2 -D_REENTRANT)
set(CMAKE_EXE_LINKER_FLAGS "-flto")

add_executable(ncrok ${SOURCES})
target_link_libraries(ncrok pthread  ${GST_LIBRARIES} ${TAG_LIBRARIES} menuw panelw ncursesw)

install(TARGETS ncrok
	RUNTIME DESTINATION bin
)

