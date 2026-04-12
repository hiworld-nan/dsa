# TscClock Test Makefile
CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -pthread -march=native -mtune=native
LDFLAGS = -pthread

# Directories
COMMON_DIR = ..
TEST_DIR = ../../test
TEST_DETAIL_DIR = $(TEST_DIR)/detail

INCLUDES = -I$(COMMON_DIR) -I$(TEST_DIR) -I$(TEST_DETAIL_DIR)

SRC = test_tsc_clock.cpp
TARGET = bin/test_tsc_clock

# Default target
all: directories $(TARGET)

directories:
	@mkdir -p bin

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf bin

.PHONY: all run clean
