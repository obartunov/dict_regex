SET search_path = public;

CREATE OR REPLACE FUNCTION dregex_init(internal)
        RETURNS internal
        AS 'MODULE_PATHNAME'
        LANGUAGE C;

CREATE OR REPLACE FUNCTION dregex_lexize(internal, internal, internal, internal)
        RETURNS internal
        AS 'MODULE_PATHNAME'
        LANGUAGE C STRICT;

CREATE TEXT SEARCH TEMPLATE regex_template (
        LEXIZE = 'dregex_lexize',
	    INIT   = 'dregex_init'
);

CREATE TEXT SEARCH DICTIONARY regex (
	   TEMPLATE = regex_template
);

COMMENT ON TEXT SEARCH DICTIONARY regex IS 'Dictionary for normalization based on regular expressions';
