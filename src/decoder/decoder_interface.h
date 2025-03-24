#pragma once

#include <stddef.h>
#include <stdint.h>

struct decoder_interface_t
{
    int (*init)(const char *path);
    void (*deinit)(void);

    size_t (*read_pcm_frames)(int16_t *buffer, size_t frames_to_read);
    
    uint32_t (*get_sample_rate)(void);
};
