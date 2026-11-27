CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -Iinclude
LDFLAGS ?=
LDLIBS ?=

BUILD_DIR := build
BIN_DIR := bin
SRC := $(wildcard src/*.c)
LIB_SRC := $(filter-out src/main.c,$(SRC))
LIB_OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(LIB_SRC))
MAIN_OBJ := $(BUILD_DIR)/main.o
TEST_SRC := $(wildcard tests/*.c)
TEST_OBJ := $(patsubst tests/%.c,$(BUILD_DIR)/tests/%.o,$(TEST_SRC))
BENCH_OBJ := $(BUILD_DIR)/bench.o
TARGET := $(BIN_DIR)/leviathan-log
TEST_BIN := $(BIN_DIR)/test_leviathan-log
BENCH_BIN := $(BIN_DIR)/bench_leviathan-log

.PHONY: all test bench asan ubsan valgrind clean

all: $(TARGET)

$(BUILD_DIR) $(BIN_DIR) $(BUILD_DIR)/tests:
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.c | $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) -Itests -c $< -o $@

$(BUILD_DIR)/bench.o: benches/bench.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(LIB_OBJ) $(MAIN_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BENCH_BIN): $(LIB_OBJ) $(BENCH_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

bench: $(BENCH_BIN)
	./$(BENCH_BIN)

asan:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
		LDFLAGS="-fsanitize=address,undefined" test

ubsan:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -g -fsanitize=undefined -fno-omit-frame-pointer" \
		LDFLAGS="-fsanitize=undefined" test

valgrind: $(TEST_BIN)
	valgrind --leak-check=full --error-exitcode=1 ./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
