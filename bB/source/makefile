SHELL=/bin/sh
CHMOD=chmod
CP=cp
RM=rm
CFLAGS=-O2
ifneq (${HOSTTYPE},)
 CFLAGS+=-arch ${HOSTTYPE}
endif
CC=cc
LEX=lex
LEXFLAGS=-t
CCC=${CC} ${CFLAGS}

all:            2600basic preprocess postprocess optimize

2600basic: 2600bas.c statements.c keywords.c statements.h keywords.h
	${CCC} -o 2600basic 2600bas.c statements.c keywords.c

postprocess: postprocess.c
	${CCC} -o postprocess postprocess.c

preprocess: preprocess.lex
	${LEX} ${LEXFLAGS}<preprocess.lex>lex.yy.c
	${CCC} -o preprocess lex.yy.c
	${RM} -f lex.yy.c

optimize: optimize.lex
	${LEX} ${LEXFLAGS}<optimize.lex>lex.yy.c
	${CCC} -o optimize lex.yy.c
	${RM} -f lex.yy.c

install:        all

clean:
	${RM} -f a.out core 2600basic preprocess postprocess optimize

love:
	@echo "not war"
