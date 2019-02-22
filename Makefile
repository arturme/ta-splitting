## Makefile

CC = gcc
FLAGS =	-Wall -Werror -m64 -DNDEBUG
FLAGS_F = -m64
OPTFLAGS = -O2

all: verifier

makefile.dep: *.[ch]
	for i in *.c; do gcc -MM "$${i}"; done > $@

include makefile.dep

%.o: %.c Makefile
	$(CC) $(FLAGS) $(OPTFLAGS) -c $<

verifier: anread.o anproc.o antclass.o anpsmod.o y.tab.o lex.yy.o dbm.o
	$(CC) $(FLAGS) $(OPTFLAGS) anread.o anproc.o antclass.o anpsmod.o lex.yy.o y.tab.o dbm.o -o verifier

lex.yy.o: lex.yy.c
	$(CC) $(FLAGS_F) $(OPTFLAGS) -c lex.yy.c

lex.yy.c: verifier.l y.tab.c
	lex verifier.l

y.tab.o: y.tab.c
	$(CC) $(FLAGS_F) $(OPTFLAGS) -c y.tab.c

y.tab.c: verifier.y
	yacc -d verifier.y

dbm_testing:
	$(CC) $(FLAGS) $(OPTFLAGS) -o dbm_testing dbm_testing.c dbm.o

# clean lex&yacc
cleanly:
	rm -f lex.yy.c y.tab.c y.tab.h y.output

clean: cleanly
	rm -f *.o verifier dbm_testing

test: verifier
	./verifier < netfiles/tgc.aut

