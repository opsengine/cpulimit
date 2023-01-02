CC ?= gcc
CFLAGS := $(filter-out -std=% -ansi -W%,$(CFLAGS)) \
			-D_GNU_SOURCE -std=gnu89 \
			-Wall -Wextra -pedantic \
			-Wmissing-prototypes -Wstrict-prototypes \
			-Wold-style-definition
TARGET := cpulimit

UNAME ?= $(shell uname)
ifeq ($(UNAME), FreeBSD)
LDFLAGS += -lkvm
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(wildcard *.c *.h)
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter-out process_iterator_%.c %.h, $^) -o $@

clean:
	rm -f *~ $(TARGET)
