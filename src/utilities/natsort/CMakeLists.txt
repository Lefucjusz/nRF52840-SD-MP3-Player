add_library(natsort STATIC)

target_sources(natsort
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/strnatcmp.c
)

target_include_directories(natsort
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)
