CC=gcc
CFLAGS=-pthread -g -O2 -Wall -I../../include/
CSAPP=../../src/csapp.c

all: tiny adder sample

tiny: tiny.c
	(mkdir content; $(CC) $(CFLAGS) -o content/tiny tiny.c $(CSAPP))

adder: adder.c
	(mkdir -p content/cgi-bin; $(CC) $(CFLAGS) -o content/cgi-bin/adder adder.c $(CSAPP))

sample:
	(cp static/* content/)

clean:
	rm -r content/
