CC=gcc
CFLAGS=

# Header file
INCLUDES = helper.h

all: server client

server: server.c $(INCLUDES)
	$(CC) $(CFLAGS) -g -o neHosServer server.c

client: client.c $(INCLUDES)
	$(CC) $(CFLAGS) -o neHosClient client.c

.PHONY: clean

clean:
	rm -f neHosServer neHosClient
