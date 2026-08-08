CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
BUILD_DIR := build
EXCEPTION_SOURCES := exceptions/except.c exceptions/assert.c

.PHONY: all test clean

all: $(BUILD_DIR)/test_except $(BUILD_DIR)/test_memchk

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/test_except: $(EXCEPTION_SOURCES) exceptions/test_except.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_memchk: $(EXCEPTION_SOURCES) memory/memchk.c memory/test_mem.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

test: all
	$(BUILD_DIR)/test_except
	$(BUILD_DIR)/test_memchk

clean:
	rm -rf $(BUILD_DIR)
