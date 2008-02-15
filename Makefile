CC=gcc
CFLAGS=-Wall -D_GNU_SOURCE -O2
TARGETS=cpulimit ptest
LIBS=process.o procutils.o list.o

all::	$(TARGETS)

cpulimit:	cpulimit.c $(LIBS)
	$(CC) -o cpulimit cpulimit.c $(LIBS) -lrt $(CFLAGS)

ptest: ptest.c process.o procutils.o list.o
	$(CC) -o ptest ptest.c process.o procutils.o list.o -lrt $(CFLAGS)

process.o: process.c process.h
	$(CC) -c process.c $(CFLAGS)

procutils.o: procutils.c procutils.h
	$(CC) -c procutils.c $(CFLAGS)

list.o: list.c list.h
	$(CC) -c list.c $(CFLAGS)

clean:
	rm -f *~ *.o $(TARGETS)

