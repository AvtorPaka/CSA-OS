CXX = g++
CXXFLAGS = -Wall -std=c++20

SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

INCLUDE_DIRS = $(shell find $(SRC_DIR) -type d)
CXXFLAGS += $(foreach dir, $(INCLUDE_DIRS), -I$(dir))

SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))

TARGET = $(BIN_DIR)/tcp_table_server_4_5

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)