find_library(YAML_LIBRARY
        NAMES yaml libyaml yamllib
        PATHS /usr/local/lib
              /usr/lib
              ${CMAKE_CURRENT_SOURCE_DIR}/yaml
              ${CMAKE_CURRENT_SOURCE_DIR}/lib/yaml
        )

if (YAML_LIBRARY)
    message("yaml parser built")
    include_directories(/usr/local/include .)
    add_library(config_parser yaml/yaml_config_parser.c)
    target_include_directories(config_parser
            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/yaml ${CMAKE_CURRENT_SOURCE_DIR}
            )
    target_link_libraries(config_parser PUBLIC ${YAML_LIBRARY})
else()
    message("custom parser built")
    add_library(config_parser config_parser.c)
    target_include_directories(config_parser
            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
            )
endif()
