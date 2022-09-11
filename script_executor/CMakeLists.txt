#include_directories(/usr/local/lib/)
add_library(script_executor script_executor.c)
target_include_directories(script_executor
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
        )

find_library(LUA_LIBRARY
        NAMES script_executor liblua lualib
        PATHS /usr/local/lib
        /usr/lib
        ${PROJECT_BINARY_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/script_executor
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua
        )

if (LUA_LIBRARY)
    message(${LUA_LIBRARY})
    message(ok)
    target_link_libraries(script_executor ${LUA_LIBRARY})
endif()

include_directories(/usr/local/lib/)