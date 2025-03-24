add_library(dr-libs INTERFACE)

target_include_directories(dr-libs
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(dr-libs
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/dr_flac.h
        ${CMAKE_CURRENT_LIST_DIR}/dr_mp3.h
        ${CMAKE_CURRENT_LIST_DIR}/dr_wav.h
)
