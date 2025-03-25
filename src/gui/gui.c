/*
 * gui.c
 *
 *  Created on: Sep 28, 2023
 *      Author: lefucjusz
 */

#include "gui.h"
#include <keyboard.h>
#include <display.h>
#include <dir.h>
#include <utils.h>
#include <player.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>
#include <stdlib.h>

#define GUI_BITRATE_VBR -1
#define GUI_FRAMES_TO_ANALYZE_BITRATE 5
#define GUI_MINS_PER_HOUR 60
#define GUI_PLAYBACK_REFRESH_INTERVAL_MS 250
#define GUI_VOLUME_VIEW_DISPLAY_TIME_MS 2000

#define GUI_EMPTY_BAR_CHAR '-'
#define GUI_FILLED_BAR_CHAR '#'

typedef enum {
	GUI_VIEW_EXPLORER,
	GUI_VIEW_PLAYBACK,
	GUI_VIEW_VOLUME
} gui_view_t;

typedef enum {
	GUI_REFRESH_ALL,
	GUI_REFRESH_TIME
} gui_refresh_t;

typedef struct {
	gui_view_t view;
	dir_list_t *dirs;
	dir_entry_t *current_dir;
	dir_entry_t *last_playback_dir; // Stores entry that was played before leaving to explorer view
	uint32_t last_refresh_tick; // Used to periodically refresh playback view
	int8_t volume;
	uint32_t last_volume_tick; // Used to return from volume view
	uint32_t last_bitrate; // Used to determine whether current song is VBR
	uint32_t frames_analyzed; // Frames analyzed by VBR detector
} gui_ctx_t;

static gui_ctx_t ctx;

static bool is_directory(const struct fs_dirent *entry)
{
	return (entry->type == FS_DIR_ENTRY_DIR);
}

static uint32_t get_elapsed_time(void)
{
	const uint32_t pcm_sample_rate = player_get_pcm_sample_rate();
	if (pcm_sample_rate != 0) {
		return player_get_pcm_frames_played() / pcm_sample_rate;
	}
	return 0;
}

static int32_t get_total_time(size_t file_size)
{
	const size_t frames_total = player_get_pcm_frames_total();
	if (frames_total > 0) {
		return frames_total / player_get_pcm_sample_rate();
	}

	/* If no total frames count info fallback to approximation algorithm */
	const uint32_t current_bitrate = player_get_current_bitrate();
	if (current_bitrate == 0) {
		return 0;
	}

	if (ctx.last_bitrate != current_bitrate) {
		ctx.last_bitrate = GUI_BITRATE_VBR;
		return GUI_BITRATE_VBR;
	}

	++ctx.frames_analyzed;
	if (ctx.frames_analyzed < GUI_FRAMES_TO_ANALYZE_BITRATE) {
		return 0;
	}

	return file_size / KBITS_TO_BYTES(current_bitrate);
}

static void refresh_list(void)
{
	dir_list_free(ctx.dirs);
	ctx.dirs = dir_list();
	ctx.current_dir = ctx.dirs->head;
}

static void fill_bar_buffer(char *buffer, size_t items_to_fill, size_t items_total)
{
	memset(buffer, GUI_FILLED_BAR_CHAR, items_to_fill);
	memset(&buffer[items_to_fill], GUI_EMPTY_BAR_CHAR, items_total - items_to_fill);
	buffer[items_total] = '\0';
}

void start_playback(const char *filename)
{
	const char *const fs_path = dir_get_fs_path();
	const size_t path_length = strlen(fs_path) + strlen(filename) + 2; // Additional '/' and null-teminator

	char *path = calloc(1, path_length);
	if (path == NULL) {
		return;
	}
	snprintf(path, path_length, "%s/%s", fs_path, filename);

	player_start(path);
	player_set_volume(ctx.volume);
	ctx.frames_analyzed = 0;
	ctx.last_bitrate = player_get_current_bitrate();

	free(path);
}

static void render_view_explorer(void)
{
	/* Empty directory case */
	if (ctx.current_dir == NULL) {
		display_set_text_sync((const char *[]){"Directory is empty!", "", "", ""}, GUI_SCROLL_DELAY_MS);
		return;
	}

	const char *filenames[DISPLAY_LINES_NUM] = {"", "", "", ""};
	dir_entry_t *current_dir = ctx.current_dir;

	for (size_t line = 0; line < DISPLAY_LINES_NUM; ++line) {
		const struct fs_dirent *entry = current_dir->data;
		filenames[line] = entry->name;

		current_dir = dir_get_next(ctx.dirs, current_dir);
		if (current_dir == ctx.current_dir) {
			break;
		}
	}

	display_set_text_sync(filenames, GUI_SCROLL_DELAY_MS);
}

static void render_view_playback(gui_refresh_t refresh_mode)
{
	const struct fs_dirent *entry = ctx.current_dir->data;

	/* Compute elapsed and total time */
	const uint32_t elapsed_time = get_elapsed_time();
	const int32_t total_time = get_total_time(entry->size);

	/* Prepare bottom line of the view in buffer */
	size_t offset;
	char info_line_buffer[DISPLAY_LINE_LENGTH + 1];
	const char state_char = (player_get_state() == PLAYER_PLAYING) ? DISPLAY_PLAY_GLYPH : DISPLAY_PAUSE_GLYPH;

	offset = snprintf(info_line_buffer, sizeof(info_line_buffer), "%c    %02u:%02u", state_char, elapsed_time / GUI_MINS_PER_HOUR, elapsed_time % GUI_MINS_PER_HOUR);

	if (total_time > 0) {
		snprintf(&info_line_buffer[offset], sizeof(info_line_buffer) - offset, "/%02u:%02u", total_time / GUI_MINS_PER_HOUR, total_time % GUI_MINS_PER_HOUR);
	}
	else if (total_time == GUI_BITRATE_VBR) {
		snprintf(&info_line_buffer[offset], sizeof(info_line_buffer) - offset, "/VBR");
	}

	/* Prepare song progress bar */
	char progress_bar_buffer[DISPLAY_LINE_LENGTH + 1];
	if (total_time > 0) {
		const size_t progress_bar_length = MAP(elapsed_time, 0, total_time, 0, DISPLAY_LINE_LENGTH);
		fill_bar_buffer(progress_bar_buffer, progress_bar_length, DISPLAY_LINE_LENGTH);
	}
	else {
		progress_bar_buffer[0] = '\0'; // Do not display anything
	}

	switch (refresh_mode) {
		case GUI_REFRESH_ALL:
			display_set_text_sync((const char *[]){entry->name, "", progress_bar_buffer, info_line_buffer}, GUI_SCROLL_DELAY_MS);
			break;

		case GUI_REFRESH_TIME:
			display_set_text(progress_bar_buffer, 3, GUI_SCROLL_DELAY_MS);
			display_set_text(info_line_buffer, 4, GUI_SCROLL_DELAY_MS);
			break;

		default:
			break;
	}
}

static void render_view_volume(void)
{
	const uint8_t volume_bar_length = MAP(ctx.volume, GUI_VOLUME_MIN, GUI_VOLUME_MAX, 1, DISPLAY_LINE_LENGTH);

	char first_line[DISPLAY_LINE_LENGTH + 1];
	snprintf(first_line, sizeof(first_line), "Volume level: %d%%", ctx.volume);

	char second_line[DISPLAY_LINE_LENGTH + 1];
	fill_bar_buffer(second_line, volume_bar_length, DISPLAY_LINE_LENGTH);

	display_set_text_sync((const char *[]){first_line, "", second_line, ""}, GUI_SCROLL_DELAY_MS);

	ctx.last_volume_tick = k_uptime_get_32();
}

static void callback_up(void)
{
	switch (ctx.view) {
		case GUI_VIEW_EXPLORER:
			ctx.current_dir = dir_get_prev(ctx.dirs, ctx.current_dir);
			render_view_explorer();
			break;

		case GUI_VIEW_PLAYBACK: {
			ctx.current_dir = dir_get_prev(ctx.dirs, ctx.current_dir);
			const struct fs_dirent *entry = ctx.current_dir->data;
			start_playback(entry->name);
			render_view_playback(GUI_REFRESH_ALL);
		} break;

		default:
			break;
	}
}

static void callback_down(void)
{
	switch (ctx.view) {
		case GUI_VIEW_EXPLORER:
			ctx.current_dir = dir_get_next(ctx.dirs, ctx.current_dir);
			render_view_explorer();
			break;

		case GUI_VIEW_PLAYBACK: {
			ctx.current_dir = dir_get_next(ctx.dirs, ctx.current_dir);
			const struct fs_dirent *entry = ctx.current_dir->data;
			start_playback(entry->name);
			render_view_playback(GUI_REFRESH_ALL);
		} break;

		default:
			break;
	}
}

static void callback_left(void)
{
	switch (ctx.view) {
		case GUI_VIEW_EXPLORER:
			if (dir_return() == 0) {
				ctx.last_playback_dir = NULL;
				refresh_list();
				render_view_explorer();
			}
			break;

		case GUI_VIEW_PLAYBACK:
			if (player_get_state() == PLAYER_PLAYING) {
				ctx.volume -= GUI_VOLUME_STEP;
				ctx.volume = CLAMP(ctx.volume, GUI_VOLUME_MIN, GUI_VOLUME_MAX);
				player_set_volume(ctx.volume);
				render_view_volume();
				ctx.view = GUI_VIEW_VOLUME;
			}
			else {
				ctx.last_playback_dir = ctx.current_dir;
				ctx.view = GUI_VIEW_EXPLORER;
				render_view_explorer();
			}
			break;

		case GUI_VIEW_VOLUME:
			ctx.volume -= GUI_VOLUME_STEP;
			ctx.volume = CLAMP(ctx.volume, GUI_VOLUME_MIN, GUI_VOLUME_MAX);
			player_set_volume(ctx.volume);
			render_view_volume();
			break;

		default:
			break;
	}
}

static void callback_right(void)
{
	switch (ctx.view) {
		case GUI_VIEW_EXPLORER:
			if ((player_get_state() == PLAYER_PAUSED) && (ctx.last_playback_dir != NULL)) {
				ctx.current_dir = ctx.last_playback_dir;
				render_view_playback(GUI_REFRESH_ALL);
				ctx.view = GUI_VIEW_PLAYBACK;
			}
			break;

		case GUI_VIEW_PLAYBACK:
			if (player_get_state() == PLAYER_PLAYING) {
				ctx.volume += GUI_VOLUME_STEP;
				ctx.volume = CLAMP(ctx.volume, GUI_VOLUME_MIN, GUI_VOLUME_MAX);
				player_set_volume(ctx.volume);
				render_view_volume();
				ctx.view = GUI_VIEW_VOLUME;
			}
			break;

		case GUI_VIEW_VOLUME:
			ctx.volume += GUI_VOLUME_STEP;
			ctx.volume = CLAMP(ctx.volume, GUI_VOLUME_MIN, GUI_VOLUME_MAX);
			player_set_volume(ctx.volume);
			render_view_volume();
			break;

		default:
			break;
	}
}

static void callback_enter(void)
{
	switch (ctx.view) {
		case GUI_VIEW_EXPLORER: {
			/* Empty directory case */
			if (ctx.current_dir == NULL) {
				break;
			}

			const struct fs_dirent *entry = ctx.current_dir->data;
			if (is_directory(entry)) {
				/* Get inside the directory */
				if (dir_enter(entry->name) == 0) {
					ctx.last_playback_dir = NULL;
					refresh_list();
					render_view_explorer();
				}
			}
			else {
				start_playback(entry->name);
				render_view_playback(GUI_REFRESH_ALL);
				ctx.view = GUI_VIEW_PLAYBACK;
			}
		} break;

		case GUI_VIEW_PLAYBACK: {
			const player_state_t state = player_get_state();
			if (state == PLAYER_PAUSED) {
				player_resume();
			}
			else if (state == PLAYER_PLAYING) {
				player_pause();
			}
		} break;

		default:
			break;
	}
}

/* It's VERY BAD that it's here, but I had no better idea... */
static void refresh_task(void)
{
	const uint32_t current_tick = k_uptime_get_32();

	switch (ctx.view) {
		case GUI_VIEW_PLAYBACK: {
			/* Refresh playback elapsed time */
			if ((current_tick - ctx.last_refresh_tick) > GUI_PLAYBACK_REFRESH_INTERVAL_MS) {
				render_view_playback(GUI_REFRESH_TIME);
				ctx.last_refresh_tick = current_tick;
			}

			/* Check if next song should be played */
			const dir_entry_t *first_dir = ctx.dirs->head;
			dir_entry_t *next_dir = dir_get_next(ctx.dirs, ctx.current_dir);

			if ((player_get_state() == PLAYER_STOPPED) && (first_dir != next_dir)) {
				ctx.current_dir = next_dir;

				const struct fs_dirent *entry = ctx.current_dir->data;
				start_playback(entry->name);
				render_view_playback(GUI_REFRESH_ALL);
			}
		} break;

		case GUI_VIEW_VOLUME:
			if ((current_tick - ctx.last_volume_tick) > GUI_VOLUME_VIEW_DISPLAY_TIME_MS) {
				render_view_playback(GUI_REFRESH_ALL);
				ctx.view = GUI_VIEW_PLAYBACK;
			}
			break;

		default:
			break;
	}
}

int gui_init(void)
{
	/* Clear context */
	memset(&ctx, 0, sizeof(gui_ctx_t));

	/* Initialize display */
	display_init();

	/* Initialize keyboard */
	keyboard_init();
	keyboard_attach_callback(KEYBOARD_UP, callback_up);
	keyboard_attach_callback(KEYBOARD_DOWN, callback_down);
	keyboard_attach_callback(KEYBOARD_LEFT, callback_left);
	keyboard_attach_callback(KEYBOARD_RIGHT, callback_right);
	keyboard_attach_callback(KEYBOARD_ENTER, callback_enter);

	/* Get initial directory listing */
	refresh_list();

	/* Set initial volume */
    ctx.volume = GUI_VOLUME_DEFAULT;

	/* Set default view and render it */
	ctx.view = GUI_VIEW_EXPLORER;
	render_view_explorer();

	return 0;
}

void gui_task(void)
{
	gui_init();
	
	while (1) {
		display_task();
		refresh_task();
		k_msleep(50);
	}
}

void gui_deinit(void)
{
	dir_list_free(ctx.dirs);
}
