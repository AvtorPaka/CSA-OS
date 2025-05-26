CC = gcc
CFLAGS = -Wall -pedantic -std=c17 -g

SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

INCLUDE_DIRS = $(shell find $(SRC_DIR) -type d)
CFLAGS += $(foreach dir, $(INCLUDE_DIRS), -I$(dir))

SOURCES = $(shell find $(SRC_DIR) -name '*.c')
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

TARGET = $(BIN_DIR)/max_link_open_depth

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
