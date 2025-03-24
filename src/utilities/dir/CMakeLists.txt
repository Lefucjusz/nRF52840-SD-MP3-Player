add_library(dir STATIC)

target_sources(dir
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/dir.c
)

target_include_directories(dir
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(dir
    PUBLIC    
        list
    PRIVATE
        natsort
        zephyr
)

set_source_files_properties(dir.c
    PROPERTIES
        COMPILE_FLAGS "-Wno-stringop-truncation -Wno-format-truncation"
)