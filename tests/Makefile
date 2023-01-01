CC ?= gcc
CFLAGS := $(filter-out -std=% -ansi -W%,$(CFLAGS)) -Wall -Wextra -std=gnu89
TARGETS = busy process_iterator_test
SRC = ../src
SYSLIBS ?= -lpthread
LIBS := $(SRC)/list.c $(SRC)/process_iterator.c $(SRC)/process_group.c
UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
	LDFLAGS += -lkvm
endif

.PHONY: all clean

all: $(TARGETS)

busy: busy.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(SYSLIBS) -o $@

process_iterator_test: process_iterator_test.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(SRC) $^ $(SYSLIBS) -o $@

clean:
	rm -f *~ $(TARGETS)
