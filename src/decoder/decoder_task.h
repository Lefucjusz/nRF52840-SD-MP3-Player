#pragma once

#include <zephyr/kernel.h>

typedef enum 
{
    PLAY,
    STOP
} decoder_state_t;

typedef struct 
{
    decoder_state_t state;
    char path[255 + 1];
} decoder_req_t;

void decoder_task(void);

struct k_msgq *decoder_get_queue(void);
decoder_state_t decoder_get_state(void);
