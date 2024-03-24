CC	= gcc
CFLAGS	= -g -Wall -pedantic-errors
SRC_DIR	= src

all: main

main: main.c helper.c
	$(CC) $(CFLAGS) main.c helper.c -o main


clean:
	rm -rf main
