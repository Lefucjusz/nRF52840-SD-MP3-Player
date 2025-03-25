add_library(gui INTERFACE)

target_include_directories(gui
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(gui
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/gui.c
)

target_link_libraries(gui
    INTERFACE
        utilities
        keyboard
        display
        player
        zephyr
)
