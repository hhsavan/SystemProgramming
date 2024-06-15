# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -pthread

# Targets
TARGETS = PideShop HungryVeryMuch

# Source files
SERVER_SRC = server.c linkedlist.c queue.c
CLIENT_SRC = client.c linkedlist.c queue.c

# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Include directories
INCLUDES = -I.

# Default target
all: $(TARGETS)

# Build server
PideShop: $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o PideShop $(SERVER_OBJ) -lm

# Build client
HungryVeryMuch: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o HungryVeryMuch $(CLIENT_OBJ)

# # Compile server source files
# server.o: server.c helper.h
# 	$(CC) $(CFLAGS) $(INCLUDES) -c server.c

# # Compile client source files
# client.o: client.c helper.h
# 	$(CC) $(CFLAGS) $(INCLUDES) -c client.c

# Compile helper source files
linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(INCLUDES) -c linkedlist.c

# Compile helper source files
queue.o: queue.c queue.h
	$(CC) $(CFLAGS) $(INCLUDES) -c queue.c

# Clean up object files and binaries
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(TARGETS)

# Phony targets
.PHONY: all clean
