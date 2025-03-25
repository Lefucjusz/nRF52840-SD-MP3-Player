#include "player.h"
#include <decoder.h>
#include <utils.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>
#include <math.h>

#define PLAYER_I2S_BLOCK_SIZE_FRAMES (1024 * 4)
#define PLAYER_I2S_BUFFER_BLOCKS 2

#define PLAYER_SAMPLE_BIT_WIDTH 16
#define PLAYER_BYTES_PER_SAMPLE sizeof(int16_t)
#define PLAYER_CHANNELS_NUM 2

#define PLAYER_VOLUME_MIN 0
#define PLAYER_VOLUME_MAX 100 // %

#define PLAYER_I2S_BLOCK_SIZE_SAMPLES (PLAYER_CHANNELS_NUM * PLAYER_I2S_BLOCK_SIZE_FRAMES)
#define PLAYER_I2S_BLOCK_SIZE (PLAYER_BYTES_PER_SAMPLE * PLAYER_I2S_BLOCK_SIZE_SAMPLES)
#define PLAYER_I2S_BUFFER_SIZE (PLAYER_I2S_BUFFER_BLOCKS * PLAYER_I2S_BLOCK_SIZE)

#define PLAYER_PATH_MAX (255 + 1)

#define PLAYER_THREAD_STACK_SIZE (1024 * 6)
#define PLAYER_THREAD_PRIORITY 9

typedef enum
{
    PLAYER_START,
    PLAYER_PAUSE,
    PLAYER_RESUME,
    PLAYER_STOP
} player_request_t;

#define PLAYER_REQUEST_QUEUE_LENGTH 2
#define PLAYER_REQUEST_SIZE sizeof(player_request_t)
#define PLAYER_REQUEST_QUEUE_SIZE (PLAYER_REQUEST_QUEUE_LENGTH * PLAYER_REQUEST_SIZE)

typedef struct
{
    float volume;
    struct k_mem_slab i2s_mem_slab;
    char __attribute__((aligned(4))) i2c_mem_slab_buf[PLAYER_I2S_BUFFER_SIZE];
    const struct device *i2s_tx;
    const struct decoder_interface_t *decoder;
    player_state_t state;
    struct k_msgq request_queue;
    char __attribute__((aligned(4))) request_queue_buf[PLAYER_REQUEST_QUEUE_SIZE];
    char file_path[PLAYER_PATH_MAX];
    struct k_thread player_thread;
} player_ctx_t;

static player_ctx_t ctx;

K_THREAD_STACK_DEFINE(player_stack, PLAYER_THREAD_STACK_SIZE);

LOG_MODULE_REGISTER(player);
   
static void volume_scale(int16_t *samples, size_t samples_count, float scale_factor)
{
    for (size_t i = 0; i < samples_count; ++i) {
        samples[i] *= scale_factor;
    }
}

static int decode_and_push_stream(void)
{
    void *block;

    int err = k_mem_slab_alloc(&ctx.i2s_mem_slab, &block, K_FOREVER);
    if (err) {
        return err;
    }

    const size_t bytes_read = ctx.decoder->read_pcm_frames(block, PLAYER_I2S_BLOCK_SIZE_FRAMES);
    if (bytes_read <= 0) {
        k_mem_slab_free(&ctx.i2s_mem_slab, block);
        return -ENODATA;
    }

    volume_scale(block, PLAYER_I2S_BLOCK_SIZE_SAMPLES, ctx.volume);

    err = i2s_write(ctx.i2s_tx, block, PLAYER_I2S_BLOCK_SIZE);
    if (err) {
        k_mem_slab_free(&ctx.i2s_mem_slab, block);
        return err;
    }
    return 0;
}

static int initialize_stream(void)
{
    i2s_trigger(ctx.i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
    for (size_t i = 0; i < PLAYER_I2S_BUFFER_BLOCKS; ++i) {
        decode_and_push_stream();
    }
    return i2s_trigger(ctx.i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
}

static void player_task(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    k_msgq_init(&ctx.request_queue, ctx.request_queue_buf, PLAYER_REQUEST_SIZE, PLAYER_REQUEST_QUEUE_LENGTH);

    int err = k_mem_slab_init(&ctx.i2s_mem_slab, ctx.i2c_mem_slab_buf, PLAYER_I2S_BLOCK_SIZE, PLAYER_I2S_BUFFER_BLOCKS);
    if (err) {
        LOG_ERR("Failed to initialize I2S memory slab, error %d!", err);
        return;
    }

    ctx.i2s_tx = DEVICE_DT_GET(DT_ALIAS(audio_i2s));
    ctx.state = PLAYER_STOPPED;

    struct i2s_config i2s_cfg = {
        .word_size = PLAYER_SAMPLE_BIT_WIDTH,
        .channels = PLAYER_CHANNELS_NUM,
        .format = I2S_FMT_DATA_FORMAT_I2S,
        .options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
        .mem_slab = &ctx.i2s_mem_slab,
        .block_size = PLAYER_I2S_BLOCK_SIZE,
        .timeout = 0
    };

    player_request_t request;
    
    while (1) {
        /* Wait for start message */
        k_msgq_get(&ctx.request_queue, &request, K_FOREVER);
        if (request != PLAYER_START) {
            LOG_WRN("Invalid request %d!", request);
            continue;
        }

        LOG_INF("Starting playback of '%s'", ctx.file_path);

        /* Get decoder for file */
        ctx.decoder = decoder_get_interface(ctx.file_path);
        if (ctx.decoder == NULL) {
            LOG_ERR("Could not find decoder for requested file!");
            continue;
        }

        do {
            /* Initialize decoder */
            err = ctx.decoder->init(ctx.file_path);
            if (err) {
                LOG_ERR("Failed to initialize decoder, error %d!", err);
                break;
            }

            /* Set sample rate and configure I2S */
            i2s_cfg.frame_clk_freq = ctx.decoder->get_sample_rate();
            err = i2s_configure(ctx.i2s_tx, I2S_DIR_TX, &i2s_cfg);
            if (err < 0) {
                LOG_ERR("Failed to configure I2S Tx stream, error %d!", err);
                break;
            }

            /* Start I2S */
            err = initialize_stream();
            if (err < 0) {
                LOG_ERR("Failed to initialize stream, error %d!", err);
                break;
            }
            ctx.state = PLAYER_PLAYING;

            /* Play until EOF or command received */
            while (1) {
                /* Get command */
                err = k_msgq_get(&ctx.request_queue, &request, K_NO_WAIT);
                if (!err) {
                    if (request == PLAYER_PAUSE) {
                        i2s_trigger(ctx.i2s_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
                        ctx.state = PLAYER_PAUSED;
                    }
                    else if (request == PLAYER_RESUME) {
                        initialize_stream();
                        ctx.state = PLAYER_PLAYING;
                    }
                    else if (request == PLAYER_STOP) {
                        i2s_trigger(ctx.i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
                        break;
                    }
                }

                /* Decode and push stream if playback in progress */
                if (ctx.state == PLAYER_PLAYING) {
                    err = decode_and_push_stream();
                    if (err == -EIO) {
                        LOG_WRN("Buffer underrun! Restarting stream...");
                        err = initialize_stream();
                        if (err) { 
                            LOG_ERR("Failed to recover, error %d", err);
                        }
                    }
                    if (err < 0) {
                        i2s_trigger(ctx.i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
                        break;
                    }
                }
                else {
                    k_msleep(50); // Give other tasks some time
                }
            }
        } while (0);

        ctx.state = PLAYER_STOPPED;
        ctx.decoder->deinit();
    }
}

void player_init(void)
{
    k_thread_create(&ctx.player_thread,
                    player_stack,
                    K_THREAD_STACK_SIZEOF(player_stack),
                    player_task,
                    NULL,
                    NULL,
                    NULL,
                    PLAYER_THREAD_PRIORITY,
                    0,
                    K_NO_WAIT);
}

void player_start(const char *path)
{
    if (path == NULL) {
        return;
    }

    if (ctx.state != PLAYER_STOPPED) {
        player_stop();
    }

    const player_request_t request = PLAYER_START;
    strlcpy(ctx.file_path, path, sizeof(ctx.file_path));
    k_msgq_put(&ctx.request_queue, &request, K_FOREVER);
}

void player_pause(void)
{
    const player_request_t request = PLAYER_PAUSE;
    k_msgq_put(&ctx.request_queue, &request, K_FOREVER);
}

void player_resume(void)
{
    const player_request_t request = PLAYER_RESUME;
    k_msgq_put(&ctx.request_queue, &request, K_FOREVER);
}

void player_stop(void)
{
    const player_request_t request = PLAYER_STOP;
    k_msgq_put(&ctx.request_queue, &request, K_FOREVER);
}

void player_set_volume(uint8_t volume)
{
    if (volume == 0) {
        ctx.volume = 0.0f;
        return;
    }

    const float volume_normalized = volume / (float)PLAYER_VOLUME_MAX;
    const float a = 1e-3f;
    const float b = 6.908f;
	ctx.volume = CLAMP(a * expf(b * volume_normalized), 0.0f, 1.0f);
}

player_state_t player_get_state(void)
{
   return ctx.state;
}

size_t player_get_pcm_frames_played(void)
{
    if (ctx.decoder != NULL) {
        return ctx.decoder->get_pcm_frames_played();
    }
    return 0;
}

size_t player_get_pcm_frames_total(void)
{
    if (ctx.decoder != NULL) {
        return ctx.decoder->get_pcm_frames_total();
    }
    return 0;
}

uint32_t player_get_pcm_sample_rate(void)
{
    if (ctx.decoder != NULL) {
        return ctx.decoder->get_sample_rate();
    }
    return 0;
}

uint32_t player_get_current_bitrate(void)
{
    if (ctx.decoder != NULL) {
        return ctx.decoder->get_current_bitrate();
    }
    return 0;
}
