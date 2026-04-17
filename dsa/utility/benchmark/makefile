# Utility Benchmark Makefile
CXX = g++
CXXFLAGS = -std=c++2c -O3 -Wall -Wextra -pthread -march=native -mtune=native
CXXFLAGS_DEBUG = -std=c++2c -Wall -Wextra -g -O0 -pthread -fsanitize=address
LDFLAGS = -pthread

INCLUDES = -I.. -I../../common -I../../benchmark -I../../benchmark/detail

BUILD_DIR = build
BIN_DIR = bin

# Targets
TARGET_SIGNAL = $(BIN_DIR)/benchmark_signal
TARGET_AFFINITY = $(BIN_DIR)/benchmark_coreAffinity
TARGET_DETECTOR = $(BIN_DIR)/benchmark_coreDetector
TARGET_DATE = $(BIN_DIR)/benchmark_date
TARGET_OVERLOADED = $(BIN_DIR)/benchmark_overloaded

ALL_TARGETS = $(TARGET_SIGNAL) $(TARGET_AFFINITY) $(TARGET_DETECTOR) $(TARGET_DATE) $(TARGET_OVERLOADED)

# Default
all: directories $(ALL_TARGETS)

directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(TARGET_SIGNAL): benchmark_signal.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

$(TARGET_AFFINITY): benchmark_coreAffinity.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

$(TARGET_DETECTOR): benchmark_coreDetector.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

$(TARGET_DATE): benchmark_date.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

$(TARGET_OVERLOADED): benchmark_overloaded.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

run: all
	@echo "=== Running signal benchmark ==="
	./$(TARGET_SIGNAL)
	@echo "=== Running affinity benchmark ==="
	./$(TARGET_AFFINITY)
	@echo "=== Running detector benchmark ==="
	./$(TARGET_DETECTOR)
	@echo "=== Running date benchmark ==="
	./$(TARGET_DATE)
	@echo "=== Running overloaded benchmark ==="
	./$(TARGET_OVERLOADED)

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: directories $(ALL_TARGETS)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

profile: CXXFLAGS += -pg
profile: directories $(ALL_TARGETS)

.PHONY: all run debug clean profile
