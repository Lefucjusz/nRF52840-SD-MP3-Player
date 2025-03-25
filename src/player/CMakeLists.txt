add_library(player INTERFACE)

target_include_directories(player
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(player
    INTERFACE
        player.c
)

target_link_libraries(player
    INTERFACE
        decoder
        utils
)
