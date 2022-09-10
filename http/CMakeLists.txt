include_directories(../third_party/URLDecode)
add_library(http http.c)
target_include_directories(http
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
        )
