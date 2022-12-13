CC=gcc
CFLAGS=-g -O2 -Wall -Werror -I../../include/
LDLIBS=-lpthread
CSAPP=../../src/csapp.c

all: echoclient echoserveri

echoclient: echoclient.c $(CSAPP)
echoserveri: echoserveri.c $(CSAPP)

clean:
	rm -f *.o *~ echoclient echoserveri
