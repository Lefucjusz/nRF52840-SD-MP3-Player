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
        utils
        natsort
        zephyr_interface
)
