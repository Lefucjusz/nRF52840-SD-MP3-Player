#pragma once

#include <string.h>
#include <stdbool.h>
#include <strings.h>

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
