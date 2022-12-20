CC=gcc
CFLAGS=-g -O2 -Wall -Werror -I../../include/
LDLIBS=-lpthread
CSAPP=../../src/csapp.c

all: echoservers

echoservers: echoservers.c $(CSAPP)

clean:
	rm -f *.o *~ echoservers
