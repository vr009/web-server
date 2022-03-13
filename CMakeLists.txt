CMAKE_MINIMUM_REQUIRED(VERSION 2.9)

project(web_server)

set(SOURCE queue/queue.c)
set(HEADER queue/queue.h)

ADD_EXECUTABLE(main src/main.c)

include_directories (${LIBEV})

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set (EV_SRC
        libev/ev.c
        libev/event.c
)

add_library(ev ${EV_SRC})
target_link_libraries (ev ${EV_LIBS})
target_include_directories(ev PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(ev PUBLIC libev)

add_library(ev::ev ALIAS ev)

target_link_libraries(main ev)
