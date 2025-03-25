cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nRF52840-SD-MP3-Player)

add_compile_options(
    -mfloat-abi=hard
    -mcpu=cortex-m4
    -mthumb
    -mabi=aapcs
    -mfpu=fpv4-sp-d16
    -mfp16-format=ieee
    -ffunction-sections
    -fdata-sections
    -fsingle-precision-constant
    -flto=auto
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wl,--gc-sections
)

add_subdirectory(src/decoder)
add_subdirectory(src/third-party)
add_subdirectory(src/utilities)
add_subdirectory(src/keyboard)
add_subdirectory(src/display)
add_subdirectory(src/gui)
add_subdirectory(src/player)
add_subdirectory(src/ssd1306)

target_sources(app 
    PRIVATE
        src/main.c
)

target_link_libraries(app
    PRIVATE
        player
        gui
)
