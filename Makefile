CC = gcc -O2 -Wall
LIB = -lslang -I/usr/include/slang -I/usr/local/include
DIR = $(ROOT_DIR)/usr/bin

all:
	$(CC) mix.c -o mix $(LIB)

mini:
	$(CC) mix.c -o mix $(LIB) -DMINI

clean:
	rm -f mix *~ core

install:	all
	install -m 711 mix $(DIR)
