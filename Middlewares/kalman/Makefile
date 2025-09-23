# Makefile for Adaptive Kalman Filter
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -ffast-math
LDFLAGS = -lm

# 源文件
SOURCES = kalman_filter.c
HEADERS = kalman_filter.h
OBJECTS = $(SOURCES:.c=.o)

# 目标
TARGET_LIB = libkalman_filter.a
TARGET_EXAMPLE = kalman_example
TARGET_TEST = test_kalman
TARGET_INTEGRATION = kalman_integration

# 默认目标
all: $(TARGET_LIB) $(TARGET_EXAMPLE) $(TARGET_TEST) $(TARGET_INTEGRATION)

# 编译静态库
$(TARGET_LIB): $(OBJECTS)
	ar rcs $@ $^

# 编译示例程序
$(TARGET_EXAMPLE): adaptive_kalman_example.c $(TARGET_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 编译测试程序
$(TARGET_TEST): test_adaptive_kalman.c $(TARGET_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 编译集成示例
$(TARGET_INTEGRATION): adaptive_kalman_integration.c $(TARGET_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 编译对象文件
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 运行测试
test: $(TARGET_TEST)
	./$(TARGET_TEST)

# 运行示例
example: $(TARGET_EXAMPLE)
	./$(TARGET_EXAMPLE)

# 运行集成示例
integration: $(TARGET_INTEGRATION)
	./$(TARGET_INTEGRATION)

# 清理
clean:
	rm -f $(OBJECTS) $(TARGET_LIB) $(TARGET_EXAMPLE) $(TARGET_TEST) $(TARGET_INTEGRATION)

# 安装（复制到系统目录）
install: $(TARGET_LIB) $(HEADERS)
	mkdir -p /usr/local/lib
	mkdir -p /usr/local/include/kalman
	cp $(TARGET_LIB) /usr/local/lib/
	cp $(HEADERS) /usr/local/include/kalman/

# 卸载
uninstall:
	rm -f /usr/local/lib/$(TARGET_LIB)
	rm -rf /usr/local/include/kalman

# 帮助
help:
	@echo "Available targets:"
	@echo "  all         - Build library, example, test, and integration"
	@echo "  lib         - Build only the library"
	@echo "  example     - Build and run example"
	@echo "  test        - Build and run tests"
	@echo "  integration - Build and run integration example"
	@echo "  clean       - Clean build files"
	@echo "  install     - Install library to system"
	@echo "  uninstall   - Remove library from system"
	@echo "  help        - Show this help"

.PHONY: all test example integration clean install uninstall help
