#include "decoder_mp3.h"
#include <zephyr/fs/fs.h>
#include <errno.h>

// #define USE_HELIX

#ifdef USE_HELIX
#include <helix_mp3.h>
#else
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_ONLY_MP3
#define DR_MP3_NO_STDIO
#define DRMP3_DATA_CHUNK_SIZE (DRMP3_MIN_DATA_CHUNK_SIZE * 2) // Default is DRMP3_MIN_DATA_CHUNK_SIZE * 4 = 64KB
#include <dr_mp3.h>
#endif

/* Internal context */
struct decoder_ctx_t
{
    struct fs_file_t fd;
#ifdef USE_HELIX
    helix_mp3_t mp3;
    helix_mp3_io_t mp3_io;
#else
	drmp3 mp3;
#endif
	struct decoder_interface_t interface;
};

static struct decoder_ctx_t ctx;

/* Internal functions */
#ifdef USE_HELIX
static size_t decoder_on_read(void *user_data, void *buffer, size_t size)
{
    struct fs_file_t *fd = user_data;
    const ssize_t bytes_read = fs_read(fd, buffer, size);
    return (bytes_read > 0) ? bytes_read : 0;
}

static int decoder_on_seek(void *user_data, int offset)
{
    struct fs_file_t *fd = user_data;
    return fs_seek(fd, offset, FS_SEEK_SET);
}
#else
static size_t decoder_on_read(void *pUserData, void *pBufferOut, size_t bytesToRead)
{
    struct fs_file_t *fd = pUserData;
    const ssize_t bytes_read = fs_read(fd, pBufferOut, bytesToRead);
    return (bytes_read > 0) ? bytes_read : 0;
}

static drmp3_bool32 decoder_on_seek(void *pUserData, int offset, drmp3_seek_origin origin)
{
    struct fs_file_t *fd = pUserData;
    const int err = fs_seek(fd, offset, origin);
    return (err == 0) ? DRMP3_TRUE : DRMP3_FALSE;
}

static drmp3_bool32 decoder_on_tell(void *pUserData, drmp3_int64 *pCursor)
{
    struct fs_file_t *fd = pUserData;

    const off_t pos = fs_tell(fd);
    if (pos < 0) {
        return DRMP3_FALSE;
    }

    *pCursor = (drmp3_int64)pos;

    return DRMP3_TRUE;
}
#endif

static int decoder_init(const char *path)
{
    /* Open file */
    fs_file_t_init(&ctx.fd);
    int err = fs_open(&ctx.fd, path, FS_O_READ);
    if (err) {
        return err;
    }

    /* Initialize decoder */
#ifdef USE_HELIX
    ctx.mp3_io.read = decoder_on_read;
    ctx.mp3_io.seek = decoder_on_seek;
    ctx.mp3_io.user_data = &ctx.fd;

    err = helix_mp3_init(&ctx.mp3, &ctx.mp3_io);
    if (err) {
        fs_close(&ctx.fd);
        return err;
    }
#else
    if (drmp3_init(&ctx.mp3, decoder_on_read, decoder_on_seek, decoder_on_tell, NULL, (void *)&ctx.fd, NULL) != DRMP3_TRUE) {
        fs_close(&ctx.fd);
        return -EIO;
    }
#endif

	return 0;
}

static void decoder_deinit(void)
{
#ifdef USE_HELIX
    helix_mp3_deinit(&ctx.mp3);
#else
	drmp3_uninit(&ctx.mp3);
#endif

    fs_close(&ctx.fd);
}

static size_t decoder_read_pcm_frames(int16_t *buffer, size_t frames_to_read)
{
#ifdef USE_HELIX
    return helix_mp3_read_pcm_frames_s16(&ctx.mp3, buffer, frames_to_read);
#else
	return drmp3_read_pcm_frames_s16(&ctx.mp3, frames_to_read, buffer);
#endif
}

static size_t decoder_get_pcm_frames_played(void)
{
#ifdef USE_HELIX
    return helix_mp3_get_pcm_frames_decoded(&ctx.mp3);
#else
    return ctx.mp3.currentPCMFrame;
#endif
}

static size_t decoder_get_pcm_frames_total(void)
{
	return 0; // The value is not available without decoding whole file
}

static uint32_t decoder_get_sample_rate(void)
{
#ifdef USE_HELIX
    return helix_mp3_get_sample_rate(&ctx.mp3);
#else
	return ctx.mp3.sampleRate;
#endif
}

static uint32_t decoder_get_current_bitrate(void)
{
#ifdef USE_HELIX
    return helix_mp3_get_bitrate(&ctx.mp3);
#else
    return ctx.mp3.mp3FrameBitrate;
#endif
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
