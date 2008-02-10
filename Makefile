CFLAGS=-Wall -O2 -D_GNU_SOURCE
TARGETS=cpulimit processtest procutils.o list.o loop
LIBS=process.o procutils.o list.o

all::	$(TARGETS)

cpulimit:	cpulimit.c $(LIBS)
	gcc -o cpulimit cpulimit.c $(LIBS) -lrt $(CFLAGS)

processtest: processtest.c process.o
	gcc -o processtest processtest.c process.o -lrt $(CFLAGS)

loop: loop.c
	gcc -o loop loop.c -lpthread $(CFLAGS)

process.o: process.c process.h
	gcc -c process.c $(CFLAGS)

procutils.o: procutils.c procutils.h list.o
	gcc -c procutils.c $(CFLAGS)

list.o: list.c list.h
	gcc -c list.c $(CFLAGS)

clean:
	rm -f *~ *.o $(TARGETS)

