# $Id: makefile,v 1.4 2010/04/07 12:33:09 bediger Exp $

LEX = lex
YACC = yacc -v -d
CC = gcc
CFLAGS = -I. -g -Wall -DYYDEBUG

OBJS = atom.o hashtable.o node.o abbreviations.o stmnt.o

all: lng

lex.yy.c: lex.l
	$(LEX) lex.l

lex.yy.o: lex.yy.c y.tab.h atom.h

y.tab.c y.tab.h: grammar.y
	$(YACC) grammar.y

y.tab.o: y.tab.c y.tab.h hashtable.h node.h atom.h \
	abbreviations.h stmnt.h

node.o: node.c node.h atom.h hashtable.h abbreviations.h
atom.o: atom.c atom.h hashtable.h
hashtable.o: hashtable.c hashtable.h
abbreviation.o: abbreviation.c abbreviation.h
stmnt.o: stmnt.c stmnt.h node.h

lng: y.tab.o lex.yy.o $(OBJS)
	$(CC) $(CFLAGS) -g -o lng y.tab.o lex.yy.o $(OBJS) $(LIBS)

clean:
	-rm -rf *.o *core lng
	-rm -rf y.tab.c y.tab.h lex.yy.c y.output
	-rm -f tests.output/*
