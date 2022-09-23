CC ?= gcc
CFLAGS := $(filter-out -std=% -ansi -W%,$(CFLAGS)) -Wall -std=gnu89
TARGET := cpulimit

UNAME ?= $(shell uname)

ifeq ($(UNAME), FreeBSD)
LDFLAGS += -lkvm
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(wildcard *.c *.h)
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter-out process_iterator_%.c, $(filter %.c, $^)) -o $@

clean:
	rm -f *~ $(TARGET)
