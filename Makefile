CC=gcc
CFLAGS=-g -I.

spltcon: spltcon.o
	$(CC) -g -o spltcon spltcon.o
	cp spltcon split
	cp spltcon concat
