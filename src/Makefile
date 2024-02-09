CC ?= gcc
override CFLAGS := $(filter-out -std=% -ansi -W%,$(CFLAGS)) \
			-D_GNU_SOURCE -std=gnu89 \
			-Wall -Wextra -pedantic \
			-Wmissing-prototypes -Wstrict-prototypes \
			-Wold-style-definition
TARGET := cpulimit

UNAME ?= $(shell uname)
ifeq ($(UNAME), FreeBSD)
  override LDFLAGS += -lkvm
endif

ifeq ($(shell echo "int main(void){return 0;}" | $(CC) -x c - -lrt -o /dev/null 2>&1),)
  override LDFLAGS += -lrt
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(wildcard *.c *.h)
	$(CC) $(CFLAGS) $(filter-out process_iterator_%.c %.h, $^) $(LDFLAGS) -o $@

clean:
	rm -f *~ $(TARGET)
