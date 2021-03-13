#ifndef LANG_H
#define LANG_H

#define MAX_EXTENSIONS 12

typedef struct {
    const char *name;
    const char *extensions[MAX_EXTENSIONS];
} lang_spec_t;

extern lang_spec_t langs[];

/**
 Return the language count.
 */
size_t get_lang_count(void);

// https://github.com/ggreer/the_silver_searcher/pull/1398
/**
Combine selected language indexes
into a regular expression of the form \.(extension1|extension2...)$
Caller is responsible for freeing the returned string.
*/
char *make_lang_regex(size_t *selected_langs, size_t count);
#endif
