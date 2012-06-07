CC?=gcc
CFLAGS?=-Wall -O2 -D_GNU_SOURCE
TARGETS=cpulimit
LIBS=process.o procutils.o list.o

UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
LIBS+=-lkvm
endif

all::	$(TARGETS) process_iterator.o

cpulimit:	cpulimit.c $(LIBS)
	$(CC) -o cpulimit cpulimit.c $(LIBS) $(CFLAGS)

process_iterator.o: process_iterator.c process_iterator.h
	$(CC) -c process_iterator.c $(CFLAGS)

process.o: process.c process.h
	$(CC) -c process.c $(CFLAGS)

procutils.o: procutils.c procutils.h
	$(CC) -c procutils.c $(CFLAGS)

list.o: list.c list.h
	$(CC) -c list.c $(CFLAGS)

clean:
	rm -f *~ *.o $(TARGETS)

