add_library(keyboard INTERFACE)

target_include_directories(keyboard
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(keyboard
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/keyboard.c
)

target_link_libraries(keyboard
    INTERFACE
        zephyr
)
