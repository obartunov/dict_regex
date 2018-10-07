#ifndef DICT_REGEX_PARSER_H
#define DICT_REGEX_PARSER_H

#include <pcre.h>

typedef enum {
    PROCESSING,
    MATCHED,
    USED,
    FAILED
} parser_rule_state;

typedef struct {
    /* Source regexp */
    char *source;
    /* PCRE-compiled regexp */
    pcre *pattern;
    /* Output recipe string */
    char *recipe;
    
    /* Current string for the rule */
    char *string;

    /* Vector with matched substrings info */
    int *ovector;
    int nmatches;
    
    /* Current rule state */
    parser_rule_state state;
} parser_rule_str;

typedef struct {
    char *string;

    int is_matched;
    int is_finished;
    
    int nrules;
    parser_rule_str **rule;
} parser_str;

parser_str *parser_create(void);
void parser_delete(parser_str *);

void parser_add_rule(parser_str *, char *, char *);
void parser_read_rules(parser_str *, char *);
void parser_reset(parser_str *);

void parser_process_word(parser_str *, char *);

int parser_is_failed(parser_str *);
int parser_is_processing(parser_str *);
int parser_is_matched(parser_str *);
char *parser_match(parser_str *);

#endif /* DICT_REGEX_PARSER_H */
