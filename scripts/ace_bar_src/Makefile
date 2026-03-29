CXX = g++
CXXFLAGS = $(shell pkg-config --cflags gtk4 gtk4-layer-shell-0) -Wall -std=c++17 -O2
LDFLAGS = $(shell pkg-config --libs gtk4 gtk4-layer-shell-0)

SRC_DIR = .
OBJ_DIR = obj
OUT_DIR = ..

SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/modules/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
TARGET = $(OUT_DIR)/ace_bar

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean
