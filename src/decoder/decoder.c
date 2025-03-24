#include "decoder.h"
#include "decoder_mp3.h"
#include "decoder_wav.h"
#include "decoder_flac.h"
#include <utils.h>

const struct decoder_interface_t *decoder_get_interface(const char *filename)
{
	if (is_extension(filename, ".mp3")) {
		return decoder_mp3_get_interface();
	}
	if (is_extension(filename, ".wav")) {
		return decoder_wav_get_interface();
	}
	if (is_extension(filename, ".flac")) {
		return decoder_flac_get_interface();
	}
	return NULL;
}
