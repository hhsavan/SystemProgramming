CC = gcc
CFLAGS = -Wall -pthread

all: PideShop HungryVeryMuch

PideShop: server.o
	$(CC) $(CFLAGS) -o PideShop server.o

HungryVeryMuch: client.o
	$(CC) $(CFLAGS) -o HungryVeryMuch client.o

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f *.o PideShop HungryVeryMuch
