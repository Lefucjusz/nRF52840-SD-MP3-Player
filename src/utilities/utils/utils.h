#pragma once

#include <string.h>
#include <stdbool.h>
#include <strings.h>

#define UTILS_BITS_TO_KBITS(x) ((x) / 1000)
#define UTILS_KBITS_TO_BYTES(x) ((1000 * (x)) / 8)

#define UTILS_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define UTILS_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define UTILS_CLAMP(val, min, max) UTILS_MIN(max, UTILS_MAX(val, min))
#define UTILS_MAP(x, in_min, in_max, out_min, out_max) (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

#define UTILS_FLOAT_TO_Q15(x) ((int16_t)((x) * 32767.0f))
#define UTILS_Q15_MUL(x, y) ((int16_t)(((int32_t)(x) * (int32_t)(y)) >> 15))

inline static bool utils_is_extension(const char *filename, const char *ext)
{
    const char *dot_ptr = strrchr(filename, '.');
    return ((dot_ptr != NULL) && (strcasecmp(dot_ptr, ext) == 0));
}

inline static size_t utils_strlcpy(char *dst, const char *src, size_t maxlen)
{
    const size_t srclen = strlen(src);
    if ((srclen + 1) < maxlen) {
        memcpy(dst, src, srclen + 1);
    }
    else if (maxlen > 0) {
        memcpy(dst, src, maxlen - 1);
        dst[maxlen - 1] = '\0';
    }
    return srclen;
}
