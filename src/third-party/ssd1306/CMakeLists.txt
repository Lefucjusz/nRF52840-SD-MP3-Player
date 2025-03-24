add_library(ssd1306 INTERFACE)

target_sources(ssd1306 
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/ssd1306.c
        ${CMAKE_CURRENT_LIST_DIR}/ssd1306_fonts.c
)

target_include_directories(ssd1306 
    INTERFACE 
        ${CMAKE_CURRENT_LIST_DIR}
)
