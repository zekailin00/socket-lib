CC := g++

LIB_DIR := ..
INC_DIR := ..
CFLAGS := -I$(INC_DIR) # -L$(LIB_DIR) -lsocketlib

.PHONY: all clean install

all: server client

server: server.cpp
	$(CC) $(CFLAGS) -o $@ $^ ../socketlib.cc

client: client.cpp
	$(CC) $(CFLAGS) -o $@ $^ ../socketlib.cc

clean:
	rm -f server client
