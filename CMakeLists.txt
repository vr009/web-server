CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(web_server)

set(SOURCE queue/queue.c)
set(HEADER queue/queue.h)

ADD_EXECUTABLE(main src/main.c)

set(LIBEV libev)
include_directories (${LIBEV})

target_link_libraries(main ${LIBEV}/*.c)
