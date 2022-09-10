add_library(config_parser config_parser.c)
target_include_directories(config_parser
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
        )
