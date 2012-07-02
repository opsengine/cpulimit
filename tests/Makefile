CC?=gcc
CFLAGS?=-Wall -g
TARGETS=busy process_iterator_test
SRC=../src
SYSLIBS?=-lpthread
LIBS=$(SRC)/list.o $(SRC)/process_iterator.o $(SRC)/process_group.o
UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
LIBS+=-lkvm
endif

all::	$(TARGETS)

busy:	busy.c
	$(CC) -o busy busy.c $(SYSLIBS) $(CFLAGS)

process_iterator_test: process_iterator_test.c $(LIBS)
	$(CC) -I$(SRC) -o process_iterator_test process_iterator_test.c $(LIBS) $(SYSLIBS) $(CFLAGS)

clean:
	rm -f *~ *.o $(TARGETS)

