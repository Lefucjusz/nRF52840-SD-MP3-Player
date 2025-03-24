add_library(decoder STATIC)

target_include_directories(decoder
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/decoder_mp3
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/decoder_wav
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/decoder_flac
)

target_sources(decoder
    PRIVATE
        decoder.c
        decoder_task.c
        decoder_mp3/decoder_mp3.c
        decoder_wav/decoder_wav.c
        decoder_flac/decoder_flac.c
)

# Suppress warning from dr_wav
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder_wav/decoder_wav.c
    PROPERTIES COMPILE_FLAGS
    "-Wno-unused-function"
)

target_link_libraries(decoder
    PRIVATE
        zephyr
        utils
        dr-libs
)
