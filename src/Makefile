CC ?= gcc
CFLAGS := $(filter-out -std=% -ansi -W%,$(CFLAGS)) \
			-D_GNU_SOURCE -std=gnu89 \
			-Wall -Wextra -pedantic \
			-Wmissing-prototypes -Wstrict-prototypes \
			-Wold-style-definition
TARGET := cpulimit

ifeq ($(shell echo "int main(void){return 0;}" | $(CC) -x c - -lrt -o /dev/null 2>&1),)
  LDFLAGS += -lrt
endif

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
