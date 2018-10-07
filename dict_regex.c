#include "postgres.h"
#include "fmgr.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#include "commands/defrem.h"
#include "tsearch/dicts/spell.h"
#include "tsearch/ts_locale.h"
#include "tsearch/ts_public.h"
#include "tsearch/ts_utils.h"
#include "utils/builtins.h"
#include "utils/memutils.h"

#include "parser.h"

PG_FUNCTION_INFO_V1(dregex_init);
Datum dregex_init(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(dregex_lexize);
Datum dregex_lexize(PG_FUNCTION_ARGS);

Datum
dregex_init(PG_FUNCTION_ARGS) {
    parser_str *parser = parser_create();
	List *dictoptions = (List *) PG_GETARG_POINTER(0);
	ListCell *l;

	foreach(l, dictoptions){
		DefElem *defel = (DefElem *) lfirst(l);

		if (pg_strcasecmp(defel->defname, "RULES") == 0) {
			parser_read_rules(parser, defGetString(defel));
		} else {
			elog(ERROR,"Unknown option: %s => %s",  defel->defname, defGetString(defel));
		}
	}

	MemoryContextDeleteChildren(CurrentMemoryContext);

    PG_RETURN_POINTER(parser);
}

static TSLexeme *make_lexeme(char *result)
{
    char *pos = result;
    TSLexeme *res = palloc(sizeof(TSLexeme));
    int length = 0;
    
    res[0].lexeme = NULL;
        
    while(pos && *pos){
        char *token = strsep((char **)&pos, " ");
        
        res = repalloc(res, sizeof(TSLexeme)*(length + 2));
        
        res[length].lexeme = token ? pstrdup(token) : NULL;
        res[length].nvariant = 0;
        res[length + 1].lexeme = NULL;
        
        length ++;
    }

    return res;
}

Datum
dregex_lexize(PG_FUNCTION_ARGS) {
    parser_str *parser = (parser_str *)PG_GETARG_POINTER(0);
    DictSubState *dstate = (DictSubState *) PG_GETARG_POINTER(3);
    char *in = (char*)PG_GETARG_POINTER(1);
    char *txt = pnstrdup(in, PG_GETARG_INT32(2));

    dstate->getnext = false;

    if (dstate->isend){
        char *result = parser_match(parser);
            
        parser_reset(parser);

        if(result){
            TSLexeme *res = make_lexeme(result);
            
            parser_reset(parser);
            
            if(result)
                free(result);
            
            PG_RETURN_POINTER(res);
        } else
            PG_RETURN_POINTER(NULL);
    }

    if(!txt || !strlen(txt))
        elog(ERROR, "Empty word passed!");
        
    parser_process_word(parser, txt);

    pfree(txt);

    if(parser_is_failed(parser)){
        /* Failed matching, reset the system */
        parser_reset(parser);

        PG_RETURN_POINTER(NULL);
    } else if(parser_is_matched(parser)){
        /* Matched the whole pattern, returning the results */
        char *result = parser_match(parser);
        TSLexeme *res = make_lexeme(result);

        if(parser_is_processing(parser))
            dstate->getnext = true;
        else
            parser_reset(parser);
        
        if(result)
            free(result);

        PG_RETURN_POINTER(res);
    } else {
        /* Matched part of pattern, requesting next word */
        dstate->getnext = true;
        PG_RETURN_POINTER(NULL);
    }
}
