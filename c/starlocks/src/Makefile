CC=gcc 
CFLAGS=-g -Wall
CLIBS=-lpthread

all: clean starlocks 

starlocks: server.o addict.o check.h count.h queue.h starlocks.h main.c
	$(CC) $(CFLAGS) $(CLIBS) addict.o server.o main.c -o starlocks 

addict.o: addict.c addict.h queue.h timer.h server.o
	$(CC) $(CFLAGS) $(CLIBS) -c addict.c -o addict.o

server.o: server.c server.h check.h count.h queue.h
	$(CC) $(CFLAGS) $(CLIBS) server.h -c server.c

clean:
	rm -rf *.o *.gch starlocks 

