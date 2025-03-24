add_library(utilities INTERFACE)

add_subdirectory(dir)
add_subdirectory(list)
add_subdirectory(natsort)
add_subdirectory(utils)

target_include_directories(utilities
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/dir
        ${CMAKE_CURRENT_LIST_DIR}/list
        ${CMAKE_CURRENT_LIST_DIR}/natsort
        ${CMAKE_CURRENT_LIST_DIR}/utils
)

target_link_libraries(utilities
    INTERFACE 
        list
        dir
        natsort
        utils
)
