CC := g++
CFLAGS := -Wall -shared -fPIC

SRC_DIR := .
LIB_DIR := ${CONDA_PREFIX}/lib
INC_DIR := ${CONDA_PREFIX}/include

SRC_FILES := $(wildcard $(SRC_DIR)/*.cc)
HEADERS := $(wildcard $(SRC_DIR)/*.h)

TARGET := libsocketlib.so

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

install: all
	mkdir -p $(LIB_DIR)
	mkdir -p $(INC_DIR)
	cp $(TARGET) $(LIB_DIR)/
	cp $(HEADERS) $(INC_DIR)/
