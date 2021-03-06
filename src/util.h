#ifndef UTIL_H
#define UTIL_H

#include <dirent.h>
#ifdef HAVE_PCRE
#include <pcre.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"
#include "log.h"
#include "options.h"
#ifdef HAVE_PCRE2
#include "pcre_api.h"
#endif

extern FILE *out_fd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define H_SIZE (64 * 1024)

#ifdef __clang__
#define NO_SANITIZE_ALIGNMENT __attribute__((no_sanitize("alignment")))
#else
#define NO_SANITIZE_ALIGNMENT
#endif

#define _GL_ATTRIBUTE_HOT __attribute__((__hot__))
#define _GL_ATTRIBUTE_PURE __attribute__((__pure__))
#define _GL_ATTRIBUTE_NOTHROW __attribute__((__nothrow__))


void *ag_malloc(size_t size);
void *ag_realloc(void *ptr, size_t size);
void *ag_calloc(size_t nelem, size_t elsize);
char *ag_strdup(const char *s);
char *ag_strndup(const char *s, size_t size);

typedef struct {
    size_t start; /* Byte at which the match starts */
    size_t end;   /* and where it ends */
} match_t;

typedef struct {
    size_t total_bytes;
    size_t total_files;
    size_t total_matches;
    size_t total_file_matches;
    struct timeval time_start;
    struct timeval time_end;
} ag_stats;


extern ag_stats stats;

/* Union to translate between chars and words without violating strict aliasing */
typedef union {
    char as_chars[sizeof(uint16_t)];
    uint16_t as_word;
} word_t;

typedef const char *(*strncmp_fp)(const char *, const char *, const size_t, const size_t, const size_t[], const size_t *);

void free_strings(char **strs, const size_t strs_len);

void generate_alpha_skip(const char *find, size_t f_len, size_t skip_lookup[], const int case_sensitive);
int is_prefix(const char *s, const size_t s_len, const size_t pos, const int case_sensitive);
size_t suffix_len(const char *s, const size_t s_len, const size_t pos, const int case_sensitive);
void generate_find_skip(const char *find, const size_t f_len, size_t **skip_lookup, const int case_sensitive);
void generate_hash(const char *find, const size_t f_len, uint8_t *H, const int case_sensitive);
void generate_bad_char_skip(const char *needle, size_t nlen, size_t bad_char_skip_lookup[], const int case_sensitive);

const char *boyer_moore_strnstr(const char *s, const char *find, const size_t s_len, const size_t f_len,
                                const size_t alpha_skip_lookup[], const size_t *find_skip_lookup)
    _GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW;
const char *boyer_moore_strncasestr(const char *s, const char *find, const size_t s_len, const size_t f_len,
                                    const size_t alpha_skip_lookup[], const size_t *find_skip_lookup)
    _GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW;
const char *boyer_moore_horspool_strnstr(const char *haystack, const char *needle, size_t hlen, size_t nlen,
                                         const size_t bad_char_skip_lookup[], const size_t *find_skip_lookup)
    _GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW;
const char *boyer_moore_horspool_strncasestr(const char *haystack, const char *needle, size_t hlen, size_t nlen,
                                             const size_t bad_char_skip_lookup[], const size_t *find_skip_lookup)
    _GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW;
const char *hash_strnstr(const char *s, const char *find, const size_t s_len, const size_t f_len, uint8_t *h_table, const int case_sensitive)
    _GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW;
strncmp_fp get_strstr(enum case_behavior casing, enum algorithm_type algorithm);

size_t invert_matches(const char *buf, const size_t buf_len, match_t matches[], size_t matches_len);
void realloc_matches(match_t **matches, size_t *matches_size, size_t matches_len);
#ifdef HAVE_PCRE
void compile_study(pcre **re, pcre_extra **re_extra, char *q, const int pcre_opts, const int study_opts);
#endif

// https://github.com/ggreer/the_silver_searcher/pull/204
int is_binary(const char *buf, const size_t buf_len);
int is_regex(const char *query);
int is_fnmatch(const char *filename);
int binary_search(const char *needle, char **haystack, int start, int end);

void init_wordchar_table(void);
int is_wordchar(char ch);

int is_lowercase(const char *s);

int is_directory(const char *path, const struct dirent *d);
int is_symlink(const char *path, const struct dirent *d);
int is_named_pipe(const char *path, const struct dirent *d);

void die(const char *fmt, ...);

void ag_asprintf(char **ret, const char *fmt, ...);

ssize_t buf_getline(const char **line, const char *buf, const size_t buf_len, const size_t buf_offset);

#ifndef HAVE_FGETLN
char *fgetln(FILE *fp, size_t *lenp);
#endif
#ifndef HAVE_GETLINE
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif
#ifndef HAVE_REALPATH
char *realpath(const char *path, char *resolved_path);
#endif
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size);
#endif
#ifndef HAVE_VASPRINTF
int vasprintf(char **ret, const char *fmt, va_list args);
#endif

#endif
