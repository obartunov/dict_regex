#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "postgres.h"

#define DEBUG_MESSAGES

#include "parser.h"
#include "utils.h"

#define MAX_MATCHES 10

void parser_rule_reset(parser_rule_str *);
parser_rule_str *parser_rule_create(char *, char *);
void parser_rule_delete(parser_rule_str *);
void parser_rule_process_word(parser_rule_str *, char *);
char *parser_rule_process_recipe(parser_rule_str *);

void parser_rule_reset(parser_rule_str *rule)
{
	if(!rule)
		return;
	
	if(rule->string)
		free(rule->string);

	rule->string = NULL;

	rule->nmatches = 0;
	
	rule->state = PROCESSING;
}

parser_rule_str *parser_rule_create(char *pattern, char *recipe)
{
	parser_rule_str *rule = NULL;
	const char *error = NULL;
	int erroffset = 0;
	char *full_pattern = make_string("^%s$", pattern);
	pcre *re = pcre_compile(full_pattern, PCRE_CASELESS, &error, &erroffset, NULL);

	if(re)
	{
		rule = (parser_rule_str *)malloc(sizeof(parser_rule_str));
		/* FIXME: study the regexp. It may speed up processing */
		rule->pattern = re;
		rule->source = make_string(pattern);
		rule->recipe = make_string(recipe);

		rule->ovector = (int *)malloc(sizeof(int)*MAX_MATCHES*3);

		rule->string = NULL;
		
		parser_rule_reset(rule);
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_REGULAR_EXPRESSION),
				 (errmsg("Failed compiling regular expression at pos %d: %s\n", erroffset, error))));
	
	free(full_pattern);

	return rule;
}

void
parser_rule_delete(parser_rule_str *rule)
{
	if(!rule)
		return;

	parser_rule_reset(rule);
	
	/* FIXME: is it enough to free all PCRE-allocated resources? */
	if(rule->source)
		free(rule->source);
	if(rule->pattern)
		free(rule->pattern);
	if(rule->recipe)
		free(rule->recipe);

	if(rule->ovector)
		free(rule->ovector);
	
	free(rule);
}

void
parser_rule_process_word(parser_rule_str *rule, char *word)
{
	int result;

	/* COnstruct current string for matching */
	if(rule->string)
		add_to_string(&rule->string, " ");
	add_to_string(&rule->string, word);
	
	result = pcre_exec(rule->pattern, NULL, rule->string, strlen(rule->string),
					   0, PCRE_PARTIAL, rule->ovector, MAX_MATCHES*3);

	if(result == PCRE_ERROR_PARTIAL)
		rule->state = PROCESSING;
	else if(result == PCRE_ERROR_BADPARTIAL)
	{
		dprintf("This regular expression can't be used for partial matching: %s\n", rule->recipe);
		rule->state = FAILED;
	}
	else if(result < 0)
		rule->state = FAILED;
	else
		rule->state = MATCHED;
	
	if(result >= 0)
	{
		rule->nmatches = result;
		
		if(!result)
			dprintf("Number of matches found exceed maximum limit of %d!\n", MAX_MATCHES);
	}
}

char *
parser_rule_process_recipe(parser_rule_str *rule)
{
	char *result = NULL;
	int pos = 0;

	while(pos < strlen(rule->recipe))
	{
		switch(rule->recipe[pos])
		{
		case('$'):
			/* Replace with numbered match */
			/* FIXME: for now, it handles only matches number 0-9 */
			if(rule->recipe[pos + 1] >= '0' && rule->recipe[pos + 1] <= '9')
			{
				int number = rule->recipe[pos + 1] - '0';

				if(number < rule->nmatches)
				{
					int length = result ? strlen(result) : 0;
					int substr_length = rule->ovector[2*number + 1] - rule->ovector[2*number];

					if(substr_length)
					{
						int d;
						
						result = realloc(result, length + substr_length + 1);

						for(d = 0; d < substr_length; d++)
							result[length + d] = tolower(rule->string[rule->ovector[2*number] + d]);

						result[length + substr_length] = '\0';
					}
				}

				pos ++;
			}

			break;
		default:
			add_pattern_to_string(&result, "%c", tolower(rule->recipe[pos]));
			break;
		}

		pos ++;
	}

	return result;
}

parser_str *
parser_create()
{
	parser_str *parser = (parser_str *)malloc(sizeof(parser_str));

	parser->is_matched = FALSE;
	parser->is_finished = FALSE;

	parser->nrules = 0;
	parser->rule = NULL;
	
	return parser;
}

void
parser_delete(parser_str *parser)
{
	int d;
	
	if(!parser)
		return;

	for(d = 0; d < parser->nrules; d++)
		parser_rule_delete(parser->rule[d]);

	if(parser->string)
		free(parser->string);

	free(parser);
}

void parser_add_rule(parser_str *parser, char *pattern, char *recipe)
{
	parser_rule_str *rule = parser_rule_create(pattern, recipe);

	if(rule){
		parser->rule = realloc(parser->rule, sizeof(parser_rule_str *)*(parser->nrules + 1));
		parser->rule[parser->nrules] = rule;

		parser->nrules ++;
	}
}

void
parser_read_rules(parser_str *parser, char *filename)
{
	FILE *file = NULL;

	if(file_exists_and_normal(filename))
		file = fopen(filename, "r");

	if(file)
	{
		while(!feof(file))
		{
			char *string = file_read_string(file);

			if(string)
			{
				char *rule = string;

				while(isspace(*rule))
					rule ++;

				if(rule[0] != '#')
				{
					char *recipe = rule;

					while(!isspace(*recipe))
						recipe ++;
					recipe[0] = '\0';
					recipe ++;

					parser_add_rule(parser, rule, recipe);
				}
				
				free(string);
			}
		}

		fclose(file);
	}
}

void
parser_reset(parser_str *parser)
{
	int d;

	parser->is_finished = FALSE;
	parser->is_matched = FALSE;

	for(d = 0; d < parser->nrules; d++)
		parser_rule_reset(parser->rule[d]);
}

void
parser_process_word(parser_str *parser, char *word)
{
	int d;

	parser->is_matched = FALSE;

	for(d = 0; d < parser->nrules; d++)
	{
		if(parser->rule[d]->state == PROCESSING)
			parser_rule_process_word(parser->rule[d], word);
	}
}

/* TRUE if all patterns have failed */
int
parser_is_failed(parser_str *parser)
{
	int result = TRUE;

	if(parser->is_matched)
		result = FALSE;
	else
	{
		int d;
		
		for(d = 0; d < parser->nrules; d++)
			if(parser->rule[d]->state != FAILED)
			{
				result = FALSE;
				break;
			}
	}

	return result;
}

/* TRUE if any pattern is still processing */
int
parser_is_processing(parser_str *parser)
{
	int is_processing = FALSE;
	int d;
	
	for(d = 0; d < parser->nrules; d++)
		if(parser->rule[d]->state == PROCESSING)
		{
			is_processing = TRUE;
			break;
		}
	
	return is_processing;
}

/* TRUE if any pattern is matched */
int
parser_is_matched(parser_str *parser)
{
	int is_matched = FALSE;
	int d;
	
	for(d = 0; d < parser->nrules; d++)
		if(parser->rule[d]->state == MATCHED)
		{
			is_matched = TRUE;
			break;
		}
	
	return is_matched;
}

/* Returns the longest match - match with longest rule->string actually */
char *
parser_match(parser_str *parser)
{
	char *result = NULL;
	int d;
	
	for(d = 0; d < parser->nrules; d++)
		if(parser->rule[d]->state == MATCHED)
		{
			result = parser_rule_process_recipe(parser->rule[d]);
			parser->rule[d]->state = USED;
			break;
		}
	
	return result;
}
