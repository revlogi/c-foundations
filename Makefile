CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
BUILD_DIR := build
EXCEPTION_SOURCES := exceptions/except.c exceptions/assert.c
LIST_TEST_SOURCES := $(EXCEPTION_SOURCES) memory/mem.c list/list.c list/test_list.c
TABLE_TEST_SOURCES := $(EXCEPTION_SOURCES) memory/mem.c view/view.c table/table.c table/test_table.c

.PHONY: all test clean

all: $(BUILD_DIR)/test_except \
	$(BUILD_DIR)/test_memchk \
	$(BUILD_DIR)/test_list \
	$(BUILD_DIR)/test_table

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/test_except: $(EXCEPTION_SOURCES) exceptions/test_except.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_memchk: $(EXCEPTION_SOURCES) memory/memchk.c memory/test_mem.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_list: $(LIST_TEST_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_table: $(TABLE_TEST_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

test: all
	$(BUILD_DIR)/test_except
	$(BUILD_DIR)/test_memchk
	$(BUILD_DIR)/test_list
	$(BUILD_DIR)/test_table

clean:
	rm -rf $(BUILD_DIR)
