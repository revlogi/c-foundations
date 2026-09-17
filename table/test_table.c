#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "table.h"

static ByteView bytes(const char *text) { return byte_view(text, strlen(text)); }

static void assert_bytes_equal(ByteView actual, ByteView expected) {
    assert(byte_view_equal(actual, expected));
}

static void test_empty_table(void) {
    Table *table = table_new();
    ByteView value = {0};

    assert(table_size(table) == 0);
    assert(!table_lookup(table, bytes("missing"), &value));
    assert(!table_remove(table, bytes("missing")));

    table_free(&table);
}

static void test_insert_and_lookup(void) {
    Table *table = table_new();
    ByteView value = {0};

    assert(table_install(table, bytes("language"), bytes("C")) == TABLE_INSERTED);
    assert(table_size(table) == 1);
    assert(table_lookup(table, bytes("language"), &value));
    assert_bytes_equal(value, bytes("C"));

    table_free(&table);
}

static void test_replace(void) {
    Table *table = table_new();
    ByteView value = {0};

    assert(table_install(table, bytes("language"), bytes("C")) == TABLE_INSERTED);
    assert(table_install(table, bytes("language"), bytes("C11")) == TABLE_REPLACED);

    assert(table_size(table) == 1);
    assert(table_lookup(table, bytes("language"), &value));
    assert_bytes_equal(value, bytes("C11"));

    table_free(&table);
}

static void test_remove(void) {
    Table *table = table_new();
    ByteView value = {0};

    table_install(table, bytes("one"), bytes("1"));
    table_install(table, bytes("two"), bytes("2"));

    assert(table_remove(table, bytes("one")));
    assert(table_size(table) == 1);
    assert(!table_lookup(table, bytes("one"), &value));
    assert(!table_remove(table, bytes("one")));

    assert(table_lookup(table, bytes("two"), &value));
    assert_bytes_equal(value, bytes("2"));

    table_free(&table);
}

static void test_binary_views(void) {
    static const unsigned char key[] = {'a', '\0', 'b'};
    static const unsigned char stored[] = {0x00, 0x7f, 0xff};
    ByteView value = {0};
    Table *table = table_new();

    assert(table_install(table, byte_view(key, sizeof key), byte_view(stored, sizeof stored)) ==
           TABLE_INSERTED);
    assert(table_lookup(table, byte_view(key, sizeof key), &value));
    assert_bytes_equal(value, byte_view(stored, sizeof stored));

    table_free(&table);
}

static void test_resize_preserves_bindings(void) {
    enum { ITEM_COUNT = 32, TEXT_SIZE = 24 };
    char keys[ITEM_COUNT][TEXT_SIZE];
    char values[ITEM_COUNT][TEXT_SIZE];
    Table *table = table_new();

    for (size_t i = 0; i < ITEM_COUNT; i++) {
        snprintf(keys[i], sizeof keys[i], "key-%zu", i);
        snprintf(values[i], sizeof values[i], "value-%zu", i);
        assert(table_install(table, bytes(keys[i]), bytes(values[i])) == TABLE_INSERTED);
    }

    assert(table_size(table) == ITEM_COUNT);

    for (size_t i = 0; i < ITEM_COUNT; i++) {
        ByteView value = {0};
        assert(table_lookup(table, bytes(keys[i]), &value));
        assert_bytes_equal(value, bytes(values[i]));
    }

    table_free(&table);
}

static void test_free_clears_handle(void) {
    Table *table = table_new();

    table_install(table, bytes("key"), bytes("value"));
    table_free(&table);

    assert(table == NULL);
}

int main(void) {
    test_empty_table();
    test_insert_and_lookup();
    test_replace();
    test_remove();
    test_binary_views();
    test_resize_preserves_bindings();
    test_free_clears_handle();

    puts("All table tests passed");
    return 0;
}
