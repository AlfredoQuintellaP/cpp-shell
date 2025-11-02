# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g
TARGET := cpp_shell

# Source files
SRC_DIR := src
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)

# Build directory
BUILD_DIR := build

.PHONY: all build run test debug clean help

all: build

# Main build target
build:
	@mkdir -p $(BUILD_DIR)
	@echo "Building $(TARGET)..."
	@$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$(TARGET) $(SOURCES)
	@echo "Build complete: $(BUILD_DIR)/$(TARGET)"

# Run with custom PATH for testing
run: build
	@echo "Running $(TARGET)..."
	@echo "Custom PATH: /tmp/test_bin:/usr/bin:/bin"
	@PATH="/tmp/test_bin:/usr/bin:/bin" ./$(BUILD_DIR)/$(TARGET)

# Test with specific commands
test: build
	@echo "Running tests..."
	@echo "=== Testing built-in commands ==="
	@printf "echo hello world\nexit\n" | ./$(BUILD_DIR)/$(TARGET) || true
	@echo ""
	@echo "=== Testing external commands ==="
	@printf "ls -la\nexit\n" | ./$(BUILD_DIR)/$(TARGET) || true
	@echo ""
	@echo "=== Testing command not found ==="
	@printf "nonexistent_command\nexit\n" | ./$(BUILD_DIR)/$(TARGET) || true
	@echo ""
	@echo "=== Testing type command ==="
	@printf "type echo\ntype ls\ntype nonexistent\nexit\n" | ./$(BUILD_DIR)/$(TARGET) || true

# Debug build with sanitizers
debug:
	@mkdir -p $(BUILD_DIR)
	@echo "Building debug version..."
	@$(CXX) $(CXXFLAGS) -fsanitize=address -fsanitize=undefined -o $(BUILD_DIR)/$(TARGET)_debug $(SOURCES)
	@echo "Debug build complete: $(BUILD_DIR)/$(TARGET)_debug"

# Create test directory structure
setup-test:
	@echo "Setting up test environment..."
	@mkdir -p /tmp/test_bin
	@echo '#!/bin/sh\necho "Test executable: $$@"' > /tmp/test_bin/test_cmd
	@chmod +x /tmp/test_bin/test_cmd
	@echo "Test environment ready. Use: test_cmd arg1 arg2"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# Show help
help:
	@echo "Available commands:"
	@echo "  make build      - Build the shell"
	@echo "  make run        - Build and run with custom PATH"
	@echo "  make test       - Run basic tests"
	@echo "  make debug      - Build with debug sanitizers"
	@echo "  make setup-test - Create test directory structure"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make help       - Show this help"
