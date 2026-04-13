# TscClock Benchmark Makefile
CXX = g++
CXXFLAGS = -std=c++2c -O3 -Wall -Wextra -pthread -march=native -mtune=native
CXXFLAGS_DEBUG = -std=c++2c -Wall -Wextra -g -O0 -pthread -fsanitize=address
LDFLAGS = -pthread

# Directories
COMMON_DIR = ..
BENCHMARK_DIR = ../../benchmark
BENCHMARK_DETAIL_DIR = $(BENCHMARK_DIR)/detail

INCLUDES = -I$(COMMON_DIR) -I$(BENCHMARK_DIR) -I$(BENCHMARK_DETAIL_DIR)

SRC = tsc_clock_benchmark.cpp
TARGET = bin/tsc_clock_benchmark

# Default target
all: directories $(TARGET)

directories:
	@mkdir -p bin

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: directories $(TARGET)

clean:
	rm -rf bin

.PHONY: all run debug clean
