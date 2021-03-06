#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <stdio.h>

#include "config.h"
#include "log.h"
#include "options.h"

typedef enum {
    AG_NO_COMPRESSION,
    AG_GZIP,
    AG_COMPRESS,
    AG_ZIP,
    AG_XZ,
} ag_compression_type;

ag_compression_type is_zipped(const void *buf, const int buf_len);

// https://github.com/ggreer/the_silver_searcher/pull/1221/
void *decompress(const ag_compression_type zip_type, void *buf, const size_t buf_len, const char *dir_full_path, size_t *new_buf_len);

#if HAVE_FOPENCOOKIE
FILE *decompress_open(int fd, const char *mode, ag_compression_type ctype);
#endif

#endif
