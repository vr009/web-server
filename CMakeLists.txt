CMAKE_MINIMUM_REQUIRED(VERSION 2.9)

project(web_server)

set(SOURCE http/http.c)
set(HEADER http/http.h)

ADD_EXECUTABLE(main src/main.c http/http.c config_parser/config_parser.c URLDecode/urldecode.c)

include_directories (libev)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set (EV_SRC
        libev/ev.c
        libev/event.c
)

add_subdirectory(libev-cmake)
target_link_libraries(main ev)
