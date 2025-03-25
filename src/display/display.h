/*
 * display.h
 *
 *  Created on: Sep 28, 2023
 *      Author: lefucjusz
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/* Added manually to ssd1306_fonts.c */
#define DISPLAY_PAUSE_GLYPH 127
#define DISPLAY_PLAY_GLYPH 128

/* From SSD1306 config */
#define DISPLAY_FONT_WIDTH 6
#define DISPLAY_FONT_HEIGHT 8
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32

#define DISPLAY_LINE_LENGTH (DISPLAY_WIDTH / DISPLAY_FONT_WIDTH)
#define DISPLAY_LINES_NUM (DISPLAY_HEIGHT / DISPLAY_FONT_HEIGHT)

void display_init(void);

int display_set_text(const char *text, size_t line_num, uint32_t scroll_delay);
int display_set_text_sync(const char *lines_text[], uint32_t scroll_delay); // TODO this array of pointers is really inconvenient idea, should be refactored

void display_task(void);

void display_deinit(void);
