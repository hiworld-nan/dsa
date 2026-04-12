# 基准测试套件 Makefile
CXX = g++
CXXFLAGS = -std=c++2c -O3 -Wall -Wextra -pthread -march=native -mtune=native
CXXFLAGS_DEBUG = -std=c++2c -Wall -Wextra -g -O0 -pthread -fsanitize=address
LDFLAGS = -pthread

# 目标文件

INCLUDES = -I.. -I../../common
SRC = benchmark_example.cpp

BUILD_DIR = build
BIN_DIR = bin
TARGET = $(BIN_DIR)/benchmark_example

# 默认目标
all: directories $(TARGET)

# Create directories
directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# 编译
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)

# 运行
run: $(TARGET)
	./$(TARGET)

# 调试编译
debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: directories $(TARGET)

# 清理
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# 带分析运行
profile: CXXFLAGS += -pg
profile: directories $(TARGET)
	./$(TARGET)
	gprof $(TARGET) gmon.out > analysis.txt

.PHONY: all run debug clean profile
