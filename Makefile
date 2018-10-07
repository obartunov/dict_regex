# dict_regex/Makefile

EXTENSION = dict_regex
PGFILEDESC = "Dictionary with regular expression support"

MODULE_big = dict_regex
DOCS = README.dict_regex
REGRESS = dict_regex
OBJS= dict_regex.o parser.o utils.o
LDFLAGS_SL += -L/usr/local/lib
SHLIB_LINK = -lpcre
PG_CPPFLAGS = -I/usr/local/include
DATA = dict_regex--1.0.sql

ifdef USE_PGXS
PGXS = $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/dict_regex
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

include $(top_srcdir)/contrib/contrib-global.mk
endif
