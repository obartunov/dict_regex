CREATE EXTENSION dict_regex;
ALTER TEXT SEARCH DICTIONARY regex (RULES='/home/teodor/Downloads/dict_regex/data/dict_regex.rules');

--lexize
SELECT ts_lexize('regex', 'catalogues');
SELECT ts_lexize('regex', 'catalog');
SELECT ts_lexize('regex', 'nuclei');
SELECT ts_lexize('regex', 'uv');
SELECT ts_lexize('regex', 'ir');
SELECT ts_lexize('regex', 'ia');
SELECT ts_lexize('regex', 'cm');
SELECT ts_lexize('regex', 'mm');
SELECT ts_lexize('regex', 'bh');
SELECT ts_lexize('regex', 'bhs');
SELECT ts_lexize('regex', 'x-ray');
SELECT ts_lexize('regex', 'gamma ray');
SELECT ts_lexize('regex', 'ngc 1234');
SELECT ts_lexize('regex', 'sao 12');
SELECT ts_lexize('regex', 'wmap');
SELECT ts_lexize('regex', 'sne Ia');
SELECT ts_lexize('regex', '1997a');
