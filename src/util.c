#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

#include "config.h"
#include "util.h"

#ifdef _WIN32
#include <windows.h>
#define flockfile(x)
#define funlockfile(x)
#define getc_unlocked(x) getc(x)
#endif

#define CHECK_AND_RETURN(ptr)             \
    if (ptr == NULL) {                    \
        die("Memory allocation failed."); \
    }                                     \
    return ptr;

FILE *out_fd = NULL;
ag_stats stats;
void *ag_malloc(size_t size) {
    void *ptr = malloc(size);
    CHECK_AND_RETURN(ptr)
}

void *ag_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    CHECK_AND_RETURN(new_ptr)
}

void *ag_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    CHECK_AND_RETURN(ptr)
}

char *ag_strdup(const char *s) {
    char *str = strdup(s);
    CHECK_AND_RETURN(str)
}

char *ag_strndup(const char *s, size_t size) {
    char *str = NULL;
#ifdef HAVE_STRNDUP
    str = strndup(s, size);
    CHECK_AND_RETURN(str)
#else
    str = (char *)ag_malloc(size + 1);
    strlcpy(str, s, size + 1);
    return str;
#endif
}

void free_strings(char **strs, const size_t strs_len) {
    if (strs == NULL) {
        return;
    }
    size_t i;
    for (i = 0; i < strs_len; i++) {
        free(strs[i]);
    }
    free(strs);
}

void generate_alpha_skip(const char *find, size_t f_len, size_t skip_lookup[], const int case_sensitive) {
    size_t i;

    for (i = 0; i < UCHAR_MAX + 1; i++) {
        skip_lookup[i] = f_len;
    }

    f_len--;

    for (i = 0; i < f_len; i++) {
        if (case_sensitive) {
            skip_lookup[(unsigned char)find[i]] = f_len - i;
        } else {
            skip_lookup[(unsigned char)tolower(find[i])] = f_len - i;
            skip_lookup[(unsigned char)toupper(find[i])] = f_len - i;
        }
    }
}

void generate_bad_char_skip(const char *needle, size_t nlen, size_t bad_char_skip_lookup[], const int case_sensitive) {
    size_t scan = 0;

    /* ---- Preprocess ---- */
    /* Initialize the table to default value */
    /* When a character is encountered that does not occur
     * in the needle, we can safely skip ahead for the whole
     * length of the needle.
     */
    for (scan = 0; scan <= UCHAR_MAX; ++scan) {
        bad_char_skip_lookup[scan] = nlen;
    }

    /* C arrays have the first byte at [0], therefore:
     * [nlen - 1] is the last byte of the array. */
    size_t last = nlen - 1;

    /* Then populate it with the analysis of the needle */
    for (scan = 0; scan < last; ++scan) {
        if (case_sensitive) {
            bad_char_skip_lookup[(unsigned char)needle[scan]] = last - scan;
        } else {
            bad_char_skip_lookup[tolower(needle[scan])] = last - scan;
            bad_char_skip_lookup[toupper(needle[scan])] = last - scan;
        }
    }
}


int is_prefix(const char *s, const size_t s_len, const size_t pos, const int case_sensitive) {
    size_t i;

    for (i = 0; pos + i < s_len; i++) {
        if (case_sensitive) {
            if (s[i] != s[i + pos]) {
                return 0;
            }
        } else {
            if (tolower(s[i]) != tolower(s[i + pos])) {
                return 0;
            }
        }
    }

    return 1;
}

size_t suffix_len(const char *s, const size_t s_len, const size_t pos, const int case_sensitive) {
    size_t i;

    for (i = 0; i < pos; i++) {
        if (case_sensitive) {
            if (s[pos - i] != s[s_len - i - 1]) {
                break;
            }
        } else {
            if (tolower(s[pos - i]) != tolower(s[s_len - i - 1])) {
                break;
            }
        }
    }

    return i;
}

void generate_find_skip(const char *find, const size_t f_len, size_t **skip_lookup, const int case_sensitive) {
    size_t i;
    size_t s_len;
    size_t *sl = ag_malloc(f_len * sizeof(size_t));
    *skip_lookup = sl;
    size_t last_prefix = f_len;

    for (i = last_prefix; i > 0; i--) {
        if (is_prefix(find, f_len, i, case_sensitive)) {
            last_prefix = i;
        }
        sl[i - 1] = last_prefix + (f_len - i);
    }

    for (i = 0; i < f_len; i++) {
        s_len = suffix_len(find, f_len, i, case_sensitive);
        if (find[i - s_len] != find[f_len - 1 - s_len]) {
            sl[f_len - 1 - s_len] = f_len - 1 - i + s_len;
        }
    }
}

#define AG_MAX(a, b) ((b > a) ? b : a)

void generate_hash(const char *find, const size_t f_len, uint8_t *h_table, const int case_sensitive) {
    int i;
    for (i = f_len - sizeof(uint16_t); i >= 0; i--) {
        // Add all 2^sizeof(uint16_t) combinations of capital letters to the hash table
        int caps_set;
        for (caps_set = 0; caps_set < (1 << sizeof(uint16_t)); caps_set++) {
            word_t word;
            memcpy(&word.as_chars, find + i, sizeof(uint16_t));
            int cap_index;
            // Capitalize the letters whose corresponding bits in caps_set are 1
            for (cap_index = 0; caps_set >> cap_index; cap_index++) {
                if ((caps_set >> cap_index) & 1)
                    word.as_chars[cap_index] -= 'a' - 'A';
            }
            size_t h;
            // Find next free cell
            for (h = word.as_word % H_SIZE; h_table[h]; h = (h + 1) % H_SIZE)
                ;
            h_table[h] = i + 1;
            // Don't add capital letters if case sensitive
            if (case_sensitive)
                break;
        }
    }
}

/* Boyer-Moore strstr */
_GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW const char *boyer_moore_strnstr(const char *s, const char *find, const size_t s_len, const size_t f_len,
                                                                                           const size_t alpha_skip_lookup[], const size_t *find_skip_lookup) {
    ssize_t i;
    size_t pos = f_len - 1;

    while (pos < s_len) {
        for (i = f_len - 1; i >= 0 && s[pos] == find[i]; pos--, i--) {
        }
        if (i < 0) {
            return s + pos + 1;
        }
        pos += AG_MAX(alpha_skip_lookup[(unsigned char)s[pos]], find_skip_lookup[i]);
    }

    return NULL;
}


/* Copy-pasted from above. Yes I know this is bad. One day I might even fix it. */
_GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW const char *boyer_moore_strncasestr(const char *s, const char *find, const size_t s_len, const size_t f_len,
                                                                                               const size_t alpha_skip_lookup[], const size_t *find_skip_lookup) {
    ssize_t i;
    size_t pos = f_len - 1;

    while (pos < s_len) {
        for (i = f_len - 1; i >= 0 && tolower(s[pos]) == find[i]; --pos, --i) {
        }
        if (i < 0) {
            return s + pos + 1;
        }
        pos += AG_MAX(alpha_skip_lookup[(unsigned char)s[pos]], find_skip_lookup[i]);
    }

    return NULL;
}

/* Returns a pointer to the first occurrence of "needle"
 * within "haystack", or NULL if not found. Works like
 * memmem().
 */

/* Note: In this example needle is a C string. The ending
 * 0x00 will be cut off, so you could call this example with
 * boyermoore_horspool_strcasestr(haystack, hlen, "abc", sizeof("abc")-1)
 */
_GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW const char *boyer_moore_horspool_strnstr(const char *haystack, const char *needle, size_t hlen, size_t nlen,
                                                                                                    const size_t bad_char_skip_lookup[], const size_t *find_skip_lookup) {
    printf("In boyer_moore_horspool_strnstr\n");

    /* Sanity checks on the parameters */
    if (nlen <= 0 || !haystack || !needle)
        return NULL;

    size_t scan = 0;

    /* C arrays have the first byte at [0], therefore:
     * [nlen - 1] is the last byte of the array. */
    size_t last = nlen - 1;

    /* ---- Do the matching ---- */

    /* Search the haystack, while the needle can still be within it. */
    while (hlen >= nlen) {
        /* scan from the end of the needle */
        for (scan = last; haystack[scan] == needle[scan]; --scan)
            if (scan == 0) /* If the first byte matches, we've found it. */
                return haystack;

        /* otherwise, we need to skip some bytes and start again.
           Note that here we are getting the skip value based on the last byte
           of needle, no matter where we didn't match. So if needle is: "abcd"
           then we are skipping based on 'd' and that value will be 4, and
           for "abcdd" we again skip on 'd' but the value will be only 1.
           The alternative of pretending that the mismatched character was
           the last character is slower in the normal case (E.g. finding
           "abcd" in "...azcd..." gives 4 by using 'd' but only
           4-2==2 using 'z'. */
        hlen -= bad_char_skip_lookup[(unsigned char)haystack[last]];
        haystack += bad_char_skip_lookup[(unsigned char)haystack[last]];
    }

    return NULL;
}

/* Returns a pointer to the first occurrence of "needle"
 * within "haystack", or NULL if not found. Works like
 * memmem().
 */

/* Note: In this example needle is a C string. The ending
 * 0x00 will be cut off, so you could call this example with
 * boyermoore_horspool_strcasestr(haystack, hlen, "abc", sizeof("abc")-1)
 */
_GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW const char *boyer_moore_horspool_strncasestr(const char *haystack, const char *needle, size_t hlen, size_t nlen,
                                                                                                        const size_t bad_char_skip_lookup[], const size_t *find_skip_lookup) {
    printf("In boyer_moore_horspool_strncasestr\n");
    /* Sanity checks on the parameters */
    if (nlen <= 0 || !haystack || !needle)
        return NULL;

    size_t scan = 0;

    /* C arrays have the first byte at [0], therefore:
     * [nlen - 1] is the last byte of the array. */
    size_t last = nlen - 1;

    /* ---- Do the matching ---- */

    /* Search the haystack, while the needle can still be within it. */
    while (hlen >= nlen) {
        /* scan from the end of the needle */
        for (scan = last; tolower(haystack[scan]) == needle[scan]; --scan)
            if (scan == 0) /* If the first byte matches, we've found it. */
                return haystack;

        /* otherwise, we need to skip some bytes and start again.
           Note that here we are getting the skip value based on the last byte
           of needle, no matter where we didn't match. So if needle is: "abcd"
           then we are skipping based on 'd' and that value will be 4, and
           for "abcdd" we again skip on 'd' but the value will be only 1.
           The alternative of pretending that the mismatched character was
           the last character is slower in the normal case (E.g. finding
           "abcd" in "...azcd..." gives 4 by using 'd' but only
           4-2==2 using 'z'. */
        hlen -= bad_char_skip_lookup[(unsigned char)haystack[last]];
        haystack += bad_char_skip_lookup[(unsigned char)haystack[last]];
    }

    return NULL;
}


strncmp_fp get_strstr(enum case_behavior casing, enum algorithm_type algorithm) {
    strncmp_fp ag_strncmp_fp = NULL;

    if (algorithm == ALGORITHM_BOYER_MOORE) {
        ag_strncmp_fp = (casing == CASE_INSENSITIVE) ? &boyer_moore_strncasestr : &boyer_moore_strnstr;
    } else if (algorithm == ALGORITHM_BOYER_MOORE_HORSPOOL) {
        ag_strncmp_fp = (casing == CASE_INSENSITIVE) ? &boyer_moore_horspool_strncasestr : &boyer_moore_horspool_strnstr;
    }

    return ag_strncmp_fp;
}

// Clang's -fsanitize=alignment (included in -fsanitize=undefined) will flag
// the intentional unaligned access here, so suppress it for this function
_GL_ATTRIBUTE_PURE _GL_ATTRIBUTE_HOT _GL_ATTRIBUTE_NOTHROW NO_SANITIZE_ALIGNMENT const char *hash_strnstr(const char *s, const char *find, const size_t s_len, const size_t f_len, uint8_t *h_table, const int case_sensitive) {
    if (s_len < f_len)
        return NULL;

    // Step through s
    const size_t step = f_len - sizeof(uint16_t) + 1;
    size_t s_i = f_len - sizeof(uint16_t);
    for (; s_i <= s_len - f_len; s_i += step) {
        size_t h;
        for (h = *(const uint16_t *)(s + s_i) % H_SIZE; h_table[h]; h = (h + 1) % H_SIZE) {
            const char *R = s + s_i - (h_table[h] - 1);
            size_t i;
            // Check putative match
            for (i = 0; i < f_len; i++) {
                if ((case_sensitive ? R[i] : tolower(R[i])) != find[i])
                    goto next_hash_cell;
            }
            return R; // Found
        next_hash_cell:
            ;
        }
    }
    // Check tail
    for (s_i = s_i - step + 1; s_i <= s_len - f_len; s_i++) {
        size_t i;
        const char *R = s + s_i;
        for (i = 0; i < f_len; i++) {
            char s_c = case_sensitive ? R[i] : tolower(R[i]);
            if (s_c != find[i])
                goto next_start;
        }
        return R;
    next_start:
        ;
    }
    return NULL;
}

size_t invert_matches(const char *buf, const size_t buf_len, match_t matches[], size_t matches_len) {
    size_t i;
    size_t match_read_index = 0;
    size_t inverted_match_count = 0;
    size_t inverted_match_start = 0;
    size_t last_line_end = 0;
    int in_inverted_match = TRUE;
    match_t next_match;

    log_debug("Inverting %u matches.", matches_len);

    if (matches_len > 0) {
        next_match = matches[0];
    } else {
        next_match.start = buf_len + 1;
    }

    /* No matches, so the whole buffer is now a match. */
    if (matches_len == 0) {
        matches[0].start = 0;
        matches[0].end = buf_len - 1;
        return 1;
    }

    for (i = 0; i < buf_len; i++) {
        if (i == next_match.start) {
            i = next_match.end - 1;

            match_read_index++;

            if (match_read_index < matches_len) {
                next_match = matches[match_read_index];
            }

            if (in_inverted_match && last_line_end > inverted_match_start) {
                matches[inverted_match_count].start = inverted_match_start;
                matches[inverted_match_count].end = last_line_end - 1;

                inverted_match_count++;
            }

            in_inverted_match = FALSE;
        } else if (i == buf_len - 1 && in_inverted_match) {
            matches[inverted_match_count].start = inverted_match_start;
            matches[inverted_match_count].end = i;

            inverted_match_count++;
        } else if (buf[i] == '\n') {
            last_line_end = i + 1;

            if (!in_inverted_match) {
                inverted_match_start = last_line_end;
            }

            in_inverted_match = TRUE;
        }
    }

    for (i = 0; i < matches_len; i++) {
        log_debug("Inverted match %i start %i end %i.", i, matches[i].start, matches[i].end);
    }

    return inverted_match_count;
}

void realloc_matches(match_t **matches, size_t *matches_size, size_t matches_len) {
    if (matches_len < *matches_size) {
        return;
    }
    /* TODO: benchmark initial size of matches. 100 may be too small/big */
    *matches_size = *matches ? *matches_size * 2 : 100;
    *matches = ag_realloc(*matches, *matches_size * sizeof(match_t));
}

#ifdef HAVE_PCRE
void compile_study(pcre **re, pcre_extra **re_extra, char *q, const int pcre_opts, const int study_opts) {
    const char *pcre_err = NULL;
    int pcre_err_offset = 0;

    *re = pcre_compile(q, pcre_opts, &pcre_err, &pcre_err_offset, NULL);
    if (*re == NULL) {
        die("Bad regex! pcre_compile() failed at position %i: %s\nIf you meant to search for a literal string, run ag with -Q",
            pcre_err_offset,
            pcre_err);
    }
    *re_extra = pcre_study(*re, study_opts, &pcre_err);
    if (*re_extra == NULL) {
        log_debug("pcre_study returned nothing useful. Error: %s", pcre_err);
    }
}
#endif

// https://github.com/ggreer/the_silver_searcher/pull/204
/* This function is very hot. It's called on every file. */
int is_binary(const char *buf, const size_t buf_len) {
    const unsigned char *s = (const unsigned char *)buf;
    int suspicious_bytes = 0;
    int total_bytes = buf_len > 512 ? 512 : buf_len;
    int valid_utf8 = 1;
    int i;

    if (buf_len == 0) {
        /* Is an empty file binary? Is it text? */
        return 0;
    }

    if (buf_len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        /* UTF-8 BOM. This isn't binary. */
        return 0;
    }

    if (buf_len >= 5 && strncmp(buf, "%PDF-", 5) == 0) {
        /* PDF. This is binary. */
        return 1;
    }

    for (i = 0; i < total_bytes; i++) {
        const unsigned char c = s[i];

        /* We order the character tests such that we optimize for the case
         * of checking ASCII text. Typically a 'binary' file will contain
         * a NUL char early on and will return quickly; we want to minimize
         * the time spent examining plain text. */

        if (c <= 126) {
            if (c >= 32) {
                continue; /* ASCII text in 32..126 range. */
            } else if (c >= 8 && c <= 13) {
                continue; /* ASCII BS, TAB, LF, VT, FF or CR. */
            } else {
                if (c == 0) {
                    return 1; /* A NUL char - it's binary. */
                }
                /* Assert: c is in (1..8) or (14..31) */
            }
        } else if (c == 127) {
            /* ASCII DEL. Treat as suspicious. */
        } else if (valid_utf8) {
/* Assert: (c > 127)
             * Maybe it's the first of a multibyte UTF-8 character.
             * NB. RFC 3629 states "The octet values C0, C1, F5 to FF never appear". */

/* A valid continuation byte matches 10xxxxxx. */
#define not_utf8_continuation_byte(c) (((c)&0xC0) != 0x80)

            if (c >= 0xc2) {
                if (c < 0xe0) /* start byte 110xxxxx */
                    goto check_2_byte_utf8_char;
                if (c < 0xf0) /* start byte 1110xxxx */
                    goto check_3_byte_utf8_char;
                if (c < 0xf5) { /* start byte 11110xxx */
                    if (not_utf8_continuation_byte(s[++i]))
                        goto not_utf8;
                check_3_byte_utf8_char:
                    if (not_utf8_continuation_byte(s[++i]))
                        goto not_utf8;
                check_2_byte_utf8_char:
                    if (not_utf8_continuation_byte(s[++i]))
                        goto not_utf8;

                    continue; /* A valid UTF-8 multibyte character. */
                }
            }
        not_utf8:
            valid_utf8 = 0;
        }
        /* If we get here then we're suspicious it's binary! */
        ++suspicious_bytes;

        /* Disk IO is so slow that it's worthwhile to do this calculation after every suspicious byte. */
        /* This is true even on a 1.6Ghz Atom with an Intel 320 SSD. */
        /* Read at least 32 bytes before making a decision */
        if (i >= 32 && 10 * suspicious_bytes > total_bytes) {
            return 1;
        }
    }

    if (10 * suspicious_bytes > total_bytes) {
        return 1;
    }

    return 0;
}

int is_regex(const char *query) {
    char regex_chars[] = {
        '$',
        '(',
        ')',
        '*',
        '+',
        '.',
        '?',
        '[',
        '\\',
        '^',
        '{',
        '|',
        '\0'
    };

    return (strpbrk(query, regex_chars) != NULL);
}

int is_fnmatch(const char *filename) {
    char fnmatch_chars[] = {
        '!',
        '*',
        '?',
        '[',
        ']',
        '\0'
    };

    return (strpbrk(filename, fnmatch_chars) != NULL);
}

int binary_search(const char *needle, char **haystack, int start, int end) {
    int mid;
    int rc;

    if (start == end) {
        return -1;
    }

    mid = start + ((end - start) / 2);

    rc = strcmp(needle, haystack[mid]);
    if (rc < 0) {
        return binary_search(needle, haystack, start, mid);
    } else if (rc > 0) {
        return binary_search(needle, haystack, mid + 1, end);
    }

    return mid;
}

static int wordchar_table[UCHAR_MAX + 1];

void init_wordchar_table(void) {
    int i;
    for (i = 0; i < UCHAR_MAX + 1; ++i) {
        char ch = (char)i;
        wordchar_table[i] =
            ('a' <= ch && ch <= 'z') ||
            ('A' <= ch && ch <= 'Z') ||
            ('0' <= ch && ch <= '9') ||
            ch == '_';
    }
}

int is_wordchar(char ch) {
    return wordchar_table[(unsigned char)ch];
}

int is_lowercase(const char *s) {
    int i;
    for (i = 0; s[i] != '\0'; i++) {
        if (!isascii(s[i]) || isupper(s[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

int is_directory(const char *path, const struct dirent *d) {
#ifdef HAVE_DIRENT_DTYPE
    /* Some filesystems, e.g. ReiserFS, always return a type DT_UNKNOWN from readdir or scandir. */
    /* Call stat if we don't find DT_DIR to get the information we need. */
    /* Also works for symbolic links to directories. */
    if (d->d_type != DT_UNKNOWN && d->d_type != DT_LNK) {
        return d->d_type == DT_DIR;
    }
#endif
    char *full_path;
    struct stat s;
    ag_asprintf(&full_path, "%s/%s", path, d->d_name);
    if (stat(full_path, &s) != 0) {
        free(full_path);
        return FALSE;
    }
#ifdef _WIN32
    int is_dir = GetFileAttributesA(full_path) & FILE_ATTRIBUTE_DIRECTORY;
#else
    int is_dir = S_ISDIR(s.st_mode);
#endif
    free(full_path);
    return is_dir;
}

int is_symlink(const char *path, const struct dirent *d) {
#ifdef _WIN32
    char full_path[MAX_PATH + 1] = { 0 };
    sprintf(full_path, "%s\\%s", path, d->d_name);
    return (GetFileAttributesA(full_path) & FILE_ATTRIBUTE_REPARSE_POINT);
#else
#ifdef HAVE_DIRENT_DTYPE
    /* Some filesystems, e.g. ReiserFS, always return a type DT_UNKNOWN from readdir or scandir. */
    /* Call lstat if we find DT_UNKNOWN to get the information we need. */
    if (d->d_type != DT_UNKNOWN) {
        return (d->d_type == DT_LNK);
    }
#endif
    char *full_path;
    struct stat s;
    ag_asprintf(&full_path, "%s/%s", path, d->d_name);
    if (lstat(full_path, &s) != 0) {
        free(full_path);
        return FALSE;
    }
    free(full_path);
    return S_ISLNK(s.st_mode);
#endif
}

int is_named_pipe(const char *path, const struct dirent *d) {
#ifdef HAVE_DIRENT_DTYPE
    if (d->d_type != DT_UNKNOWN && d->d_type != DT_LNK) {
        return d->d_type == DT_FIFO || d->d_type == DT_SOCK;
    }
#endif
    char *full_path;
    struct stat s;
    ag_asprintf(&full_path, "%s/%s", path, d->d_name);
    if (stat(full_path, &s) != 0) {
        free(full_path);
        return FALSE;
    }
    free(full_path);
    return S_ISFIFO(s.st_mode)
#ifdef S_ISSOCK
           || S_ISSOCK(s.st_mode)
#endif
        ;
}

void ag_asprintf(char **ret, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vasprintf(ret, fmt, args) == -1) {
        die("vasprintf returned -1");
    }
    va_end(args);
}

void die(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vplog(LOG_LEVEL_ERR, fmt, args);
    va_end(args);
    exit(2);
}

#ifndef HAVE_FGETLN
char *fgetln(FILE *fp, size_t *lenp) {
    char *buf = NULL;
    int c, used = 0, len = 0;

    flockfile(fp);
    while ((c = getc_unlocked(fp)) != EOF) {
        if (!buf || len >= used) {
            size_t nsize;
            char *newbuf;
            nsize = used + BUFSIZ;
            if (!(newbuf = realloc(buf, nsize))) {
                funlockfile(fp);
                if (buf)
                    free(buf);
                return NULL;
            }
            buf = newbuf;
            used = nsize;
        }
        buf[len++] = c;
        if (c == '\n') {
            break;
        }
    }
    funlockfile(fp);
    *lenp = len;
    return buf;
}
#endif

#ifndef HAVE_GETLINE
/*
 * Do it yourself getline() implementation
 */
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    size_t len = 0;
    char *srcln = NULL;
    char *newlnptr = NULL;

    /* get line, bail on error */
    if (!(srcln = fgetln(stream, &len))) {
        return -1;
    }

    if (len >= *n) {
        /* line is too big for buffer, must realloc */
        /* double the buffer, bail on error */
        if (!(newlnptr = realloc(*lineptr, len * 2))) {
            return -1;
        }
        *lineptr = newlnptr;
        *n = len * 2;
    }

    memcpy(*lineptr, srcln, len);

#ifndef HAVE_FGETLN
    /* Our own implementation of fgetln() returns a malloc()d buffer that we
     * must free
     */
    free(srcln);
#endif

    (*lineptr)[len] = '\0';
    return len;
}
#endif

ssize_t buf_getline(const char **line, const char *buf, const size_t buf_len, const size_t buf_offset) {
    const char *cur = buf + buf_offset;
    ssize_t i;
    for (i = 0; (buf_offset + i < buf_len) && cur[i] != '\n'; i++) {
    }
    *line = cur;
    return i;
}

#ifndef HAVE_REALPATH
/*
 * realpath() for Windows. Turns slashes into backslashes and calls _fullpath
 */
char *realpath(const char *path, char *resolved_path) {
    char *p;
    char tmp[_MAX_PATH + 1];
    strlcpy(tmp, path, sizeof(tmp));
    p = tmp;
    while (*p) {
        if (*p == '/') {
            *p = '\\';
        }
        p++;
    }
    return _fullpath(resolved_path, tmp, _MAX_PATH);
}
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size) {
    char *d = dst;
    const char *s = src;
    size_t n = size;

    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0') {
                break;
            }
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (size != 0) {
            *d = '\0'; /* NUL-terminate dst */
        }

        while (*s++) {
        }
    }

    return (s - src - 1); /* count does not include NUL */
}
#endif

#ifndef HAVE_VASPRINTF
int vasprintf(char **ret, const char *fmt, va_list args) {
    int rv;
    *ret = NULL;
    va_list args2;
/* vsnprintf can destroy args, so we need to copy it for the second call */
#ifdef __va_copy
    /* non-standard macro, but usually exists */
    __va_copy(args2, args);
#elif va_copy
    /* C99 macro. We compile with -std=c89 but you never know */
    va_copy(args2, args);
#else
    /* Ancient compiler. This usually works but there are no guarantees. */
    memcpy(args2, args, sizeof(va_list));
#endif
    rv = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (rv < 0) {
        return rv;
    }
    *ret = malloc(++rv); /* vsnprintf doesn't count \0 */
    if (*ret == NULL) {
        return -1;
    }
    rv = vsnprintf(*ret, rv, fmt, args2);
    va_end(args2);
    if (rv < 0) {
        free(*ret);
    }
    return rv;
}
#endif
