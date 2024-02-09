CC ?= gcc
override CFLAGS := $(filter-out -std=% -ansi -W%,$(CFLAGS)) \
			-D_GNU_SOURCE -std=gnu89 \
			-Wall -Wextra -pedantic \
			-Wmissing-prototypes -Wstrict-prototypes \
			-Wold-style-definition
TARGETS = busy process_iterator_test
SRC = ../src
SYSLIBS ?= -lpthread
LIBS := $(SRC)/list.c $(SRC)/process_iterator.c $(SRC)/process_group.c

UNAME ?= $(shell uname)
ifeq ($(UNAME), FreeBSD)
  override LDFLAGS += -lkvm
endif

ifeq ($(shell echo "int main(void){return 0;}" | $(CC) -x c - -lrt -o /dev/null 2>&1),)
  override LDFLAGS += -lrt
endif

.PHONY: all clean

all: $(TARGETS)

busy: busy.c
	$(CC) $(CFLAGS) $^ $(SYSLIBS) $(LDFLAGS) -o $@

process_iterator_test: process_iterator_test.c $(LIBS)
	$(CC) $(CFLAGS) -I$(SRC) $^ $(LDFLAGS) -o $@

clean:
	rm -f *~ $(TARGETS)
