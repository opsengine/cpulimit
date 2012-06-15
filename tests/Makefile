CC?=gcc
CFLAGS?=-Wall -g
TARGETS=busy process_iterator_test
LIBS?=-lpthread

UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
LIBS+=-lkvm
endif

all::	$(TARGETS)

busy:	busy.c $(LIBS)
	$(CC) -o busy busy.c $(LIBS) $(CFLAGS)

process_iterator_test: process_iterator_test.c $(LIBS)
	$(CC) -I../src -o process_iterator_test process_iterator_test.c  ../src/process_iterator.o $(LIBS) $(CFLAGS)

clean:
	rm -f *~ *.o $(TARGETS)

