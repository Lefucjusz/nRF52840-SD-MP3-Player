add_subdirectory(dr_libs)
add_subdirectory(Helix-MP3-Decoder)

target_include_directories(dr_libs_interface
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/dr_libs
)

# Propagate compilation flags
target_link_libraries(helix_mp3
    PUBLIC
        zephyr_interface
)
