add_library(dr-libs INTERFACE)

target_include_directories(dr-libs
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_sources(dr-libs
    INTERFACE
        dr_flac.h
        dr_mp3.h
        dr_wav.h
)
