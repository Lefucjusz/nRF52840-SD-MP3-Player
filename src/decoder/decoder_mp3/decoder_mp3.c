#define DR_MP3_IMPLEMENTATION
#define DR_MP3_ONLY_MP3
#define DR_MP3_NO_STDIO
#define DRMP3_DATA_CHUNK_SIZE (DRMP3_MIN_DATA_CHUNK_SIZE) // Default is DRMP3_MIN_DATA_CHUNK_SIZE * 4 = 64KB, which is way too much for nRF52833's RAM

#include "decoder_mp3.h"
#include <dr_mp3.h>
#include <zephyr/fs/fs.h>
#include <errno.h>

/* Internal context */
struct decoder_ctx_t
{
    struct fs_file_t fd;
	drmp3 mp3;
	struct decoder_interface_t interface;
};

static struct decoder_ctx_t ctx;

/* Internal functions */
static size_t decoder_on_read(void *pUserData, void *pBufferOut, size_t bytesToRead)
{
    struct fs_file_t *fd = pUserData;
    const ssize_t bytes_read = fs_read(fd, pBufferOut, bytesToRead);
    return (bytes_read > 0) ? bytes_read : 0;
}

static drmp3_bool32 decoder_on_seek(void *pUserData, int offset, drmp3_seek_origin origin)
{
    struct fs_file_t *fd = pUserData;
    const int err = fs_seek(fd, offset, (origin == drmp3_seek_origin_start) ? FS_SEEK_SET : FS_SEEK_CUR);
    return (err == 0) ? DRMP3_TRUE : DRMP3_FALSE;
}

static int decoder_init(const char *path)
{
    /* Open file */
    fs_file_t_init(&ctx.fd);
    const int err = fs_open(&ctx.fd, path, FS_O_READ);
    if (err) {
        return err;
    }
    
    /* Initialize decoder */
    if (drmp3_init(&ctx.mp3, decoder_on_read, decoder_on_seek, (void *)&ctx.fd, NULL) != DRMP3_TRUE) {
        fs_close(&ctx.fd);
        return -EIO;
    }
    
	return 0;
}

static void decoder_deinit(void)
{
	drmp3_uninit(&ctx.mp3);
    fs_close(&ctx.fd);
}

static size_t decoder_read_pcm_frames(int16_t *buffer, size_t frames_to_read)
{
	return drmp3_read_pcm_frames_s16(&ctx.mp3, frames_to_read, buffer);
}

static size_t decoder_get_pcm_frames_played(void)
{
    return ctx.mp3.currentPCMFrame;
}

static size_t decoder_get_pcm_frames_total(void)
{
	return 0; // The value is not available without decoding whole file
}

static uint32_t decoder_get_sample_rate(void)
{
	return ctx.mp3.sampleRate;
}

static uint32_t decoder_get_current_bitrate(void)
{
    return ctx.mp3.mp3FrameBitrate;
}

/* API */
const struct decoder_interface_t *decoder_mp3_get_interface(void)
{
	ctx.interface.init = decoder_init;
	ctx.interface.deinit = decoder_deinit;
	ctx.interface.read_pcm_frames = decoder_read_pcm_frames;
	ctx.interface.get_pcm_frames_played = decoder_get_pcm_frames_played;
	ctx.interface.get_pcm_frames_total = decoder_get_pcm_frames_total;
	ctx.interface.get_sample_rate = decoder_get_sample_rate;
	ctx.interface.get_current_bitrate = decoder_get_current_bitrate;

	return &ctx.interface;
}
