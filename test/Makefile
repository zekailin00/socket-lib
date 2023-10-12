CC := g++

LIB_DIR := ${HOME}/.local/lib
INC_DIR := ${HOME}/.local/include
CFLAGS := -I$(INC_DIR) -L$(LIB_DIR) -lsocketlib

.PHONY: all clean install

all: server client

server: server.cpp
	$(CC) $(CFLAGS) -o $@ $^

client: client.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f server client
