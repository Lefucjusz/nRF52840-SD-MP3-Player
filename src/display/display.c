/*
 * display.c
 *
 *  Created on: Sep 28, 2023
 *      Author: lefucjusz
 */

#include "display.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ssd1306_fonts.h>
#include <zephyr/kernel.h>

/* Two-space suffix for scrolling to look good */
#define DISPLAY_SUFFIX "  "
#define DISPLAY_SUFFIX_LENGTH 2

typedef struct 
{
	char *line_buffer[DISPLAY_LINES_NUM];
	size_t line_offset[DISPLAY_LINES_NUM];
	uint32_t scroll_delay[DISPLAY_LINES_NUM];
	uint32_t last_refresh_tick[DISPLAY_LINES_NUM];
} display_ctx_t;

static display_ctx_t ctx;

static bool display_only_one_not_fit(const char *lines_text[])
{
	bool found = false;
	for (size_t line = 0; line < DISPLAY_LINES_NUM; ++line) {
		if (strlen(lines_text[line]) > DISPLAY_LINE_LENGTH) {
			if (found) {
				return false;
			}
			found = true;
		}
	}
	return true;
}

static size_t display_get_max_length(const char *lines_text[])
{
	size_t max_length = 0;
	for (size_t line = 0; line < DISPLAY_LINES_NUM; ++line) {
		const size_t current_length = strlen(lines_text[line]);
		if (current_length > max_length) {
			max_length = current_length;
		}
	}
	return max_length;
}

static void display_gotoxy(size_t x, size_t y)
{
	ssd1306_SetCursor(y * DISPLAY_FONT_WIDTH, x * DISPLAY_FONT_HEIGHT);
}

void display_init(void)
{
	memset(&ctx, 0, sizeof(display_ctx_t));
	ssd1306_Init();
	ssd1306_Fill(Black);
}

int display_set_text(const char *text, size_t line_num, uint32_t scroll_delay)
{
	if ((line_num < 1) || (line_num > DISPLAY_LINES_NUM)) {
		return -EINVAL;
	}

	free(ctx.line_buffer[line_num - 1]);

	const size_t line_length = strlen(text);
	char *line;

	/* Case without scrolling */
	if (line_length <= DISPLAY_LINE_LENGTH) {
		line = calloc(1, DISPLAY_LINE_LENGTH + 1);
		if (line == NULL) {
			return -ENOMEM;
		}
		memcpy(line, text, line_length);

		const size_t padding = DISPLAY_LINE_LENGTH - line_length;
		memset(&line[line_length], ' ', padding);
		line[line_length + padding] = '\0';
	}
	else {
		line = calloc(1, line_length + DISPLAY_SUFFIX_LENGTH + 1);
		if (line == NULL) {
			return -ENOMEM;
		}
		memcpy(line, text, line_length);
		memcpy(&line[line_length], DISPLAY_SUFFIX, DISPLAY_SUFFIX_LENGTH + 1);
	}

	ctx.line_buffer[line_num - 1] = line;
	ctx.scroll_delay[line_num - 1] = scroll_delay;
	ctx.line_offset[line_num - 1] = 0;
	ctx.last_refresh_tick[line_num - 1] = 0;

	return 0;
}

int display_set_text_sync(const char *lines_text[], uint32_t scroll_delay)
{
	/* Fallback to regular text setting if only one is too long - no need to synchronize */
	if (display_only_one_not_fit(lines_text)) {
		int ret;
		for (size_t line = 0; line < DISPLAY_LINES_NUM; ++line) {
			ret = display_set_text(lines_text[line], line + 1, scroll_delay);
			if (ret) {
				return ret;
			}
		}
		return ret;
	}

	/* Synchronized scrolling */
	const size_t max_length = display_get_max_length(lines_text);
	for (size_t line = 0; line < DISPLAY_LINES_NUM; ++line) {
		/* Free previous buffer */
		free(ctx.line_buffer[line]);

		/* Create new buffer */
		char *current_line = calloc(1, max_length + DISPLAY_SUFFIX_LENGTH + 1);
		if (current_line == NULL) {
			return -ENOMEM;
		}

		/* Copy text to buffer */
		const size_t current_length = strlen(lines_text[line]);
		memcpy(current_line, lines_text[line], current_length);

		/* Apply space padding */
		if (current_length <= DISPLAY_LINE_LENGTH) {
			/* Pad all fitting lines to display line length to clear previous chars */
			const size_t padding = DISPLAY_LINE_LENGTH - current_length;
			memset(&current_line[current_length], ' ', padding);
		}
		else {
			/* Pad all not fitting lines to the length of the longest one */
			const size_t padding = max_length - current_length;
			memset(&current_line[current_length], ' ', padding);

			/* Apply suffix */
			memcpy(&current_line[max_length], DISPLAY_SUFFIX, DISPLAY_SUFFIX_LENGTH + 1);
		}

		/* Set line config in context */
		ctx.line_buffer[line] = current_line;
		ctx.scroll_delay[line] = scroll_delay;
		ctx.line_offset[line] = 0;
		ctx.last_refresh_tick[line] = 0;
	}

	return 0;
}

void display_task(void)
{
	const uint32_t current_tick = k_uptime_get_32();

	for (size_t line = 0; line < DISPLAY_LINES_NUM; ++line) {
		/* Skip if no line buffer or line empty */
		if ((ctx.line_buffer[line] == NULL) || (ctx.line_buffer[line][0] == '\0')) {
			continue;
		}

		if ((current_tick - ctx.last_refresh_tick[line]) >= ctx.scroll_delay[line]) {
			const size_t text_line_length = strlen(ctx.line_buffer[line]);

			/* Display statically if it fits */
			if (text_line_length <= DISPLAY_LINE_LENGTH) {
				if (ctx.line_offset[line] > 0) {
					continue;
				}

				display_gotoxy(line, 0);
				ssd1306_WriteString(ctx.line_buffer[line], Font_6x8, White);
				ssd1306_UpdateScreen();
				ctx.line_offset[line]++;
			}
			else {
				/* Scroll if not */
				char line_buffer[DISPLAY_LINE_LENGTH + 1];

				for (size_t column = 0; column < DISPLAY_LINE_LENGTH; ++column) {
					const size_t src_column = (ctx.line_offset[line] + column) % text_line_length;
					line_buffer[column] = ctx.line_buffer[line][src_column];
				}

				display_gotoxy(line, 0);
				ssd1306_WriteString(line_buffer, Font_6x8, White);
				ssd1306_UpdateScreen();

				ctx.line_offset[line]++;
				ctx.last_refresh_tick[line] = current_tick;
			}
		}
	}
}

void display_deinit(void)
{
	for (size_t i = 0; i < DISPLAY_LINES_NUM; ++i) {
		free(ctx.line_buffer[i]);
	}
}
