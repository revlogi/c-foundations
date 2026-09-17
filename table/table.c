#include "table.h"

#include <stddef.h>
#include <stdint.h>

#include "../memory/mem.h"

#define INITIAL_SIZE 6
#define LOAD_FACTOR 0.75

typedef struct Node *Node_T;

struct Node {
    Node_T next;
    ByteView key;
    ByteView value;
};

struct Table {
    Node_T *buckets;
    size_t capacity;
    size_t count;
};

Table *table_new(void) {
    Table *table;
    NEW(table);

    table->capacity = INITIAL_SIZE;
    table->count = 0;
    table->buckets = CALLOC(INITIAL_SIZE, sizeof(Node_T));

    return table;
}

void table_free(Table **table_ptr) {
    Table *table = *table_ptr;
    for (size_t i = 0; i < table->capacity; i++) {
        for (Node_T curr = table->buckets[i]; curr;) {
            Node_T next = curr->next;
            FREE(curr);
            curr = next;
        }
    }
    FREE(table->buckets);
    FREE(*table_ptr);
}

size_t table_size(const Table *table) { return table->count; }

static size_t bucket_index(size_t capacity, ByteView key) {
    return (size_t)(byte_view_hash(key) % capacity);
}

static void add_node(Node_T node, Node_T *buckets, size_t index) {
    node->next = buckets[index];
    buckets[index] = node;
}

static void migrate_list(Node_T curr, Node_T *new_buckets, size_t new_capacity) {
    if (!curr) {
        return;
    }

    Node_T next = curr->next;
    uint64_t index = bucket_index(new_capacity, curr->key);
    add_node(curr, new_buckets, index);
    migrate_list(next, new_buckets, new_capacity);
}

static void resize(Table *table) {
    size_t new_size = table->capacity * 2;

    Node_T *new_buckets = CALLOC(new_size, sizeof(Node_T));
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->buckets[i]) {
            migrate_list(table->buckets[i], new_buckets, new_size);
        }
    }
    FREE(table->buckets);
    table->buckets = new_buckets;
    table->capacity = new_size;
}

TableInstallResult table_install(Table *table, ByteView key, ByteView value) {
    uint64_t index = bucket_index(table->capacity, key);
    Node_T head = table->buckets[index];
    for (Node_T curr = head; curr; curr = curr->next) {
        if (byte_view_equal(curr->key, key)) {
            curr->value = value;
            return TABLE_REPLACED;
        }
    }

    if ((double)(table->count + 1) / table->capacity > LOAD_FACTOR) {
        resize(table);
        index = bucket_index(table->capacity, key);
    }

    Node_T node;
    NEW(node);

    node->key = key;
    node->value = value;

    add_node(node, table->buckets, index);
    table->count++;

    return TABLE_INSERTED;
}

bool table_lookup(const Table *table, ByteView key, ByteView *out_value) {
    uint64_t index = bucket_index(table->capacity, key);
    for (Node_T curr = table->buckets[index]; curr; curr = curr->next) {
        if (byte_view_equal(curr->key, key)) {
            *out_value = curr->value;
            return true;
        };
    }
    return false;
}

bool table_remove(Table *table, ByteView key) {
    uint64_t index = bucket_index(table->capacity, key);

    for (Node_T *link = &table->buckets[index]; (*link); link = &(*link)->next) {
        if (byte_view_equal((*link)->key, key)) {
            Node_T removed = *link;
            *link = removed->next;
            FREE(removed);
            table->count--;
            return true;
        }
    }
    return false;
}
