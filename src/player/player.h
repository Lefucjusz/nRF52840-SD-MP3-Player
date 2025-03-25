#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum 
{
    PLAYER_STOPPED,
    PLAYER_PAUSED,
    PLAYER_PLAYING
} player_state_t;

void player_init(void);

void player_start(const char *path);
void player_pause(void);
void player_resume(void);
void player_stop(void);

void player_set_volume(uint8_t volume);

player_state_t player_get_state(void);
size_t player_get_pcm_frames_played(void);
size_t player_get_pcm_frames_total(void);
uint32_t player_get_pcm_sample_rate(void);
uint32_t player_get_current_bitrate(void);
