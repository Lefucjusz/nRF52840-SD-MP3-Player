#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_CONVERSION_API
#define DR_WAV_NO_WCHAR
#define DR_WAV_NO_STDIO

#include "decoder_wav.h"
#include <dr_wav.h>
#include <zephyr/fs/fs.h>
#include <errno.h>

/* Internal context */
struct decoder_ctx_t
{
    struct fs_file_t fd;
	drwav wav;
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

static drwav_bool32 decoder_on_seek(void *pUserData, int offset, drwav_seek_origin origin)
{
    struct fs_file_t *fd = pUserData;
    const int err = fs_seek(fd, offset, (origin == drwav_seek_origin_start) ? FS_SEEK_SET : FS_SEEK_CUR);
    return (err == 0) ? DRWAV_TRUE : DRWAV_FALSE;
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
    if (drwav_init(&ctx.wav, decoder_on_read, decoder_on_seek, (void *)&ctx.fd, NULL) != DRWAV_TRUE) {
        fs_close(&ctx.fd);
        return -EIO;
    }
    
	return 0;
}

static void decoder_deinit(void)
{
	drwav_uninit(&ctx.wav);
    fs_close(&ctx.fd);
}

static size_t decoder_read_pcm_frames(int16_t *buffer, size_t frames_to_read)
{
	return drwav_read_pcm_frames_le(&ctx.wav, frames_to_read, buffer);
}

static uint32_t decoder_get_sample_rate(void)
{
	return ctx.wav.sampleRate;
}

/* API */
const struct decoder_interface_t *decoder_wav_get_interface(void)
{
	ctx.interface.init = decoder_init;
	ctx.interface.deinit = decoder_deinit;
	ctx.interface.read_pcm_frames = decoder_read_pcm_frames;
	ctx.interface.get_sample_rate = decoder_get_sample_rate;

	return &ctx.interface;
}
