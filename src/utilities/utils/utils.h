#pragma once

#include <string.h>
#include <stdbool.h>
#include <strings.h>

#define BITS_TO_KBITS(x) ((x) / 1000)
#define KBITS_TO_BYTES(x) ((1000 * (x)) / 8)

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define CLAMP(val, min, max) MIN(max, MAX(val, min))
#define MAP(x, in_min, in_max, out_min, out_max) (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

inline static bool is_extension(const char *filename, const char *ext)
{
    const char *dot_ptr = strrchr(filename, '.');
    return ((dot_ptr != NULL) && (strcasecmp(dot_ptr, ext) == 0));
}

inline static size_t strlcpy(char *dst, const char *src, size_t maxlen)
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
