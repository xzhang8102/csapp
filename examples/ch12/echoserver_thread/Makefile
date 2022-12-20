CC=gcc
CFLAGS=-g -O2 -Wall -Werror -I../../include/
LDLIBS=-lpthread
CSAPP=../../src/csapp.c

all: echoservert

echoservert: echoservert.c $(CSAPP)

clean:
	rm -f *.o *~ echoservert
