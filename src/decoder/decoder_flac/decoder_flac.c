#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_OGG
#define DR_FLAC_NO_CRC
#define DR_FLAC_NO_SIMD
#define DR_FLAC_NO_WCHAR
#define DR_FLAC_NO_STDIO

#include "decoder_flac.h"
#include <dr_flac.h>
#include <zephyr/fs/fs.h>
#include <errno.h>

/* Internal context */
struct decoder_ctx_t
{
    struct fs_file_t fd;
	drflac* flac;
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

static drflac_bool32 decoder_on_seek(void *pUserData, int offset, drflac_seek_origin origin)
{
    struct fs_file_t *fd = pUserData;
    const int err = fs_seek(fd, offset, (origin == drflac_seek_origin_start) ? FS_SEEK_SET : FS_SEEK_CUR);
    return (err == 0) ? DRFLAC_TRUE : DRFLAC_FALSE;
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
    ctx.flac = drflac_open(decoder_on_read, decoder_on_seek, (void *)&ctx.fd, NULL);
    if (ctx.flac == NULL) {
        fs_close(&ctx.fd);
        return -EIO;
    }
    
	return 0;
}

static void decoder_deinit(void)
{
	drflac_close(ctx.flac);
    fs_close(&ctx.fd);
}

static size_t decoder_read_pcm_frames(int16_t *buffer, size_t frames_to_read)
{
	return drflac_read_pcm_frames_s16(ctx.flac, frames_to_read, buffer);
}

static size_t decoder_get_pcm_frames_played(void)
{
	return ctx.flac->currentPCMFrame;
}

static size_t decoder_get_pcm_frames_total(void)
{
	return ctx.flac->totalPCMFrameCount;
}

static uint32_t decoder_get_sample_rate(void)
{
	return ctx.flac->sampleRate;
}

static uint32_t decoder_get_current_bitrate(void)
{
	return 0; // Defined only when total frame count not available
}

/* API */
const struct decoder_interface_t *decoder_flac_get_interface(void)
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
