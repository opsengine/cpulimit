CC ?= gcc
CFLAGS := $(filter-out -std=% -ansi -W%,$(CFLAGS)) \
			-D_GNU_SOURCE -std=gnu89
TARGETS = busy process_iterator_test
SRC = ../src
SYSLIBS ?= -lpthread
LIBS := $(SRC)/list.c $(SRC)/process_iterator.c $(SRC)/process_group.c

ifeq ($(shell echo "int main(void){return 0;}" | $(CC) -x c - -lrt -o /dev/null 2>&1),)
  LDFLAGS += -lrt
endif

UNAME ?= $(shell uname)
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
