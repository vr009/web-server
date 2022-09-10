CMAKE_MINIMUM_REQUIRED(VERSION 2.9)

project(web_server)

add_executable(web_server src/main.c)

add_subdirectory(http)
add_subdirectory(config_parser)
add_subdirectory(third_party/URLDecode)

find_library(EV_LIBRARY
        NAMES libev ev evlib libev_static
        PATHS /usr/local/lib
              /usr/lib
              ${PROJECT_BINARY_DIR}/lib
              ${CMAKE_CURRENT_SOURCE_DIR}/libev
              ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libev
        )

list(APPEND EXTRA_LIBS http config_parser URLDecode)

if (EV_LIBRARY)
    message(${EV_LIBRARY})
    list(APPEND EXTRA_LIBS ${EV_LIBRARY})
endif()

include_directories(/usr/local/include http config_parser)

target_link_libraries(web_server PUBLIC ${EXTRA_LIBS})

target_include_directories(web_server PUBLIC
        "${PROJECT_BINARY_DIR}"
        )
