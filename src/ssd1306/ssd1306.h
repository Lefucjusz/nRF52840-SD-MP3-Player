#pragma once

#include "ssd1306_conf.h"
#include <stdint.h>
// #include <stdbool.h>

typedef enum 
{
    SSD1306_BLACK = 0x00, // Pixel off
    SSD1306_WHITE = 0x01  // Pixel on, color depends on OLED
} ssd1306_color_t;

typedef struct 
{
	const uint8_t width;                // Font width in pixels
	const uint8_t height;               // Font height in pixels
	const uint16_t *const data;         // Pointer to font data array
    const uint8_t *const char_width;    // Proportional character width in pixels (NULL for monospaced)
} ssd1306_font_t;

/* API */
int ssd1306_init(void);
void ssd1306_deinit(void);

void ssd1306_set_cursor(uint16_t x, uint16_t y);
void ssd1306_set_contrast(uint8_t value);

void ssd1306_update_screen(void);

void ssd1306_fill(ssd1306_color_t color);
void ssd1306_draw_pixel(uint16_t x, uint16_t y, ssd1306_color_t color);
char ssd1306_write_char(char ch, ssd1306_font_t font, ssd1306_color_t color);
char ssd1306_write_string(const char *str, ssd1306_font_t font, ssd1306_color_t color);
