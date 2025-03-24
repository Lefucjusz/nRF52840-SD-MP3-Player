add_library(list STATIC)

target_sources(list
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/list.c
)

target_include_directories(list
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)
