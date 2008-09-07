CC?=gcc
CFLAGS?=-Wall -O2 -DPOSIXLY_CORRECT
TARGETS=cpulimit
LIBS=process.o procutils.o list.o

all::	$(TARGETS)

cpulimit:	cpulimit.c $(LIBS)
	$(CC) -o cpulimit cpulimit.c $(LIBS) $(CFLAGS)

process.o: process.c process.h
	$(CC) -c process.c $(CFLAGS)

procutils.o: procutils.c procutils.h
	$(CC) -c procutils.c $(CFLAGS)

list.o: list.c list.h
	$(CC) -c list.c $(CFLAGS)

clean:
	rm -f *~ *.o $(TARGETS)

