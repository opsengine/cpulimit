CC ?= gcc
CFLAGS ?= -Wall
TARGET := cpulimit
OBJS := cpulimit.o list.o process_iterator.o process_group.o

UNAME ?= $(shell uname)

ifeq ($(UNAME), FreeBSD)
	LDFLAGS += -lkvm
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

cpulimit.o: cpulimit.c process_group.h list.h
	$(CC) $(CFLAGS) -c $< -o $@

process_iterator.o: process_iterator.c process_iterator.h process_iterator_linux.c process_iterator_freebsd.c process_iterator_apple.c
	$(CC) $(CFLAGS) -c $< -o $@

list.o: list.c list.h
	$(CC) $(CFLAGS) -c $< -o $@

process_group.o: process_group.c process_group.h process_iterator.h list.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *~ $(OBJS) $(TARGET)
