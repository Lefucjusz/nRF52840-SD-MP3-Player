#include "decoder_task.h"
#include "decoder.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>

#define SAMPLE_RATE_HZ 44100
#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE sizeof(int16_t)
#define CHANNELS_NUM 2

#define BLOCK_SIZE_FRAMES 2048
#define BLOCK_SIZE_SAMPLES (CHANNELS_NUM * BLOCK_SIZE_FRAMES)
#define BLOCK_SIZE (BYTES_PER_SAMPLE * BLOCK_SIZE_SAMPLES)

#define I2S_BUFFER_BLOCKS 2
#define ALIGNMENT_4BYTE 4

LOG_MODULE_REGISTER(decoder_task, LOG_LEVEL_DBG);

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, I2S_BUFFER_BLOCKS, ALIGNMENT_4BYTE);

static const float volume = 0.12f;

static decoder_req_t state = {
    .state = STOP
};

K_MSGQ_DEFINE(req_queue, sizeof(decoder_req_t), 2, 4);

static void volume_scale(int16_t *samples, size_t samples_count, float scale_factor)
{
    for (size_t i = 0; i < samples_count; ++i) {
        samples[i] *= scale_factor;
    }
}

static int decode_and_push_stream(const struct device *i2s, const struct decoder_interface_t *dec)
{
    void *mem_block;
    int err = k_mem_slab_alloc(&mem_slab, &mem_block, K_FOREVER);
    if (err) {
        return err;
    }

    const size_t bytes_read = dec->read_pcm_frames((int16_t *)mem_block, BLOCK_SIZE_FRAMES);
    if (bytes_read <= 0) {
        k_mem_slab_free(&mem_slab, mem_block);
        return -ENODATA;
    }

    volume_scale((int16_t *)mem_block, BLOCK_SIZE_SAMPLES, volume);

    err = i2s_write(i2s, mem_block, BLOCK_SIZE);
    if (err) {
        k_mem_slab_free(&mem_slab, mem_block);
        return err;
    }
    return 0;
}

static int initialize_stream(const struct device *i2s, const struct decoder_interface_t *dec)
{
    i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP);
    for (size_t i = 0; i < I2S_BUFFER_BLOCKS; ++i) {
        decode_and_push_stream(i2s, dec);
    }
    return i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_START);
}

void decoder_task(void)
{
    const struct device *i2s_tx = DEVICE_DT_GET(DT_ALIAS(audio_i2s));
    struct i2s_config i2s_cfg = {
        .word_size = SAMPLE_BIT_WIDTH,
        .channels = CHANNELS_NUM,
        .format = I2S_FMT_DATA_FORMAT_I2S,
        .options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
        .frame_clk_freq = SAMPLE_RATE_HZ,
        .mem_slab = &mem_slab,
        .block_size = BLOCK_SIZE,
        .timeout = 1000,
    };
    
    static size_t err_count = 0;

    while (1) {
        k_msgq_get(&req_queue, &state, K_FOREVER);
        if (state.state == STOP) {
            continue;
        }

        LOG_INF("Starting playback");

        const struct decoder_interface_t *decoder = decoder_get_interface(state.path);
        if (decoder == NULL) {
            LOG_ERR("Failed to get decoder for file '%s'", state.path);
            return;
        }

        do {
            LOG_INF("Initializing decoder for path '%s'", state.path);
            int err = decoder->init(state.path);
            if (err) {
                LOG_ERR("Failed to initialize decoder");
                break;
            }

            LOG_INF("Configuring I2S");
            LOG_INF("Detected sample rate: %uHz", decoder->get_sample_rate());
            i2s_cfg.frame_clk_freq = decoder->get_sample_rate(); // Set frame clock to current audio file sample rate
            err = i2s_configure(i2s_tx, I2S_DIR_TX, &i2s_cfg);
            if (err < 0) {
                LOG_ERR("Failed to configure TX stream: %d", err);
                break;
            }
           
            LOG_INF("Starting I2S");
            err = initialize_stream(i2s_tx, decoder);
            if (err < 0) {
                LOG_ERR("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_START, err);
                break;
            }

            LOG_INF("Entering playback loop");
            while (1) {
                err = decode_and_push_stream(i2s_tx, decoder);
                if (err == -EIO) {
                    LOG_WRN("Buffer underrun! Count: %zu Restarting stream...", ++err_count);
                    err = initialize_stream(i2s_tx, decoder);
                    if (err) { 
                        LOG_ERR("Failed to recover, error %d", err); 
                    }
                }
                if (err == -ENODATA) {
                    LOG_WRN("EOF!");
                }
                if (err < 0) {
                    break;
                }
            }

            LOG_INF("Stopping I2S");
            err = i2s_trigger(i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
            if (err < 0) {
                LOG_ERR("Failed to trigger command %d on TX: %d\n", I2S_TRIGGER_DROP, err);
            }
        } while (0);

        state.state = STOP;
        decoder->deinit();
        LOG_INF("Playback done!");
    }
}

struct k_msgq *decoder_get_queue(void)
{
    return &req_queue;
}

decoder_state_t decoder_get_state(void)
{
    return state.state;
}
