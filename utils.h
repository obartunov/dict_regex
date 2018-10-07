#ifndef DICT_REGEX_UTILS_H
#define DICT_REGEX_UTILS_H

#include <stdarg.h>
#include <stdio.h>

#define FALSE 0
#define TRUE  1

#ifdef DEBUG_MESSAGES
#define dprintf(x...) fprintf(stderr, x)
#else
#define dprintf(x...) {;}
#endif

char *make_string(const char *, ...) __attribute__ ((format(printf, 1, 2)));
void add_to_string(char **, char *);
#define add_to_string_and_free(x, y) {add_to_string(x, y); if(y) free(y);}
void add_pattern_to_string(char **, const char *, ...);

int file_exists_and_normal(char *);

char *file_read_string(FILE *);

#endif /* DICT_REGEX_UTILS_H */
