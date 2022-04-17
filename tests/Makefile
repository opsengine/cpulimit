CC ?= gcc
CFLAGS ?= -Wall
TARGETS = busy process_iterator_test
SRC = ../src
SYSLIBS ?= -lpthread
LIBS := $(SRC)/list.o $(SRC)/process_iterator.o $(SRC)/process_group.o
UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
	LDFLAGS += -lkvm
endif

.PHONY: all clean

all: $(TARGETS)

busy: busy.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(SYSLIBS)

process_iterator_test: process_iterator_test.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(SRC) -o $@ $< $(LIBS) $(SYSLIBS)

clean:
	rm -f *~ *.o $(TARGETS)
