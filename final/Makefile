# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Targets
TARGETS = server client

# Server and client sources
SERVER_SRC = ./server/server.c
CLIENT_SRC = ./client/client.c

# Server and client executables
SERVER_EXE = server
CLIENT_EXE = client

.PHONY: all clean server client

all: $(TARGETS)

server: $(SERVER_EXE)
	./$(SERVER_EXE)

client: $(CLIENT_EXE)
	./$(CLIENT_EXE)

$(SERVER_EXE): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_EXE) $(SERVER_SRC)

$(CLIENT_EXE): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_EXE) $(CLIENT_SRC)

clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE)
