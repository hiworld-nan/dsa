# Utility Test Makefile
CXX = g++
CXXFLAGS = -std=c++2c -Wall -Wextra -O3 -pthread -march=native -mtune=native
CXXFLAGS_DEBUG = -std=c++2c -Wall -Wextra -g -O0 -pthread -fsanitize=address

# Directories
BUILD_DIR = build
BIN_DIR = bin

# Includes
INCLUDES = -I.. -I../../common -I../detail -I../../test -I../../test/detail

# Source files
SRC_SIGNAL = test_signal.cpp
SRC_AFFINITY = test_coreAffinity.cpp
SRC_DETECTOR = test_coreDetector.cpp
SRC_DATE = test_date.cpp
SRC_OVERLOADED = test_overloaded.cpp

# Targets
TARGET_SIGNAL = $(BIN_DIR)/test_signal
TARGET_AFFINITY = $(BIN_DIR)/test_coreAffinity
TARGET_DETECTOR = $(BIN_DIR)/test_coreDetector
TARGET_DATE = $(BIN_DIR)/test_date
TARGET_OVERLOADED = $(BIN_DIR)/test_overloaded

ALL_TARGETS = $(TARGET_SIGNAL) $(TARGET_AFFINITY) $(TARGET_DETECTOR) $(TARGET_DATE) $(TARGET_OVERLOADED)

# Default
all: directories $(ALL_TARGETS)

directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(TARGET_SIGNAL): $(SRC_SIGNAL)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

$(TARGET_AFFINITY): $(SRC_AFFINITY)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ -lpthread

$(TARGET_DETECTOR): $(SRC_DETECTOR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ -lpthread

$(TARGET_DATE): $(SRC_DATE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

$(TARGET_OVERLOADED): $(SRC_OVERLOADED)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

run: all
	@echo "=== Running signal tests ==="
	./$(TARGET_SIGNAL)
	@echo "=== Running affinity tests ==="
	./$(TARGET_AFFINITY)
	@echo "=== Running detector tests ==="
	./$(TARGET_DETECTOR)
	@echo "=== Running date tests ==="
	./$(TARGET_DATE)
	@echo "=== Running overloaded tests ==="
	./$(TARGET_OVERLOADED)

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: clean all

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all run debug clean
