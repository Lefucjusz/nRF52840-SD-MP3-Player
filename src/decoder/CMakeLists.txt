add_library(decoder STATIC)

target_include_directories(decoder
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/decoder_mp3
        ${CMAKE_CURRENT_LIST_DIR}/decoder_wav
        ${CMAKE_CURRENT_LIST_DIR}/decoder_flac
)

target_sources(decoder
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/decoder.c
        ${CMAKE_CURRENT_LIST_DIR}/decoder_task.c
        ${CMAKE_CURRENT_LIST_DIR}/decoder_mp3/decoder_mp3.c
        ${CMAKE_CURRENT_LIST_DIR}/decoder_wav/decoder_wav.c
        ${CMAKE_CURRENT_LIST_DIR}/decoder_flac/decoder_flac.c
)

# Suppress warning from dr_wav
set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/decoder_wav/decoder_wav.c
    PROPERTIES COMPILE_FLAGS
    "-Wno-unused-function"
)

target_link_libraries(decoder
    PRIVATE
        zephyr
        utils
        dr-libs
)
