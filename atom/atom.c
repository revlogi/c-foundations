#include "atom.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../exceptions/assert.h"
#include "../memory/mem.h"

struct Atom {
    Atom *next;
    size_t length;
    unsigned char data[];
};

struct AtomPool {
    Atom **table;
    size_t capacity;
    size_t size;
};

AtomPool *atom_pool_new(void) {
    AtomPool *pool = ALLOC(sizeof(*pool));

    pool->capacity = 1024;
    pool->size = 0;

    TRY pool->table = CALLOC(pool->capacity, sizeof(pool->table[0]));
    EXCEPT(Mem_Failed)
    FREE(pool);
    RERAISE;
    END_TRY

    return pool;
}

const Atom *atom_pool_intern(AtomPool *pool, ByteView bytes) {
    assert(bytes.data && bytes.length);
    assert(pool);

    uint64_t hash = byte_view_hash(bytes) % pool->capacity;
    size_t length = bytes.length;

    for (Atom *atom = pool->table[hash]; atom; atom = atom->next) {
        if (atom->length == length && memcmp(atom->data, bytes.data, length) == 0) {
            return atom;
        }
    }

    Atom *atom = ALLOC(sizeof(*atom) + length);
    atom->length = length;
    memcpy(atom->data, bytes.data, length);

    atom->next = pool->table[hash];
    pool->table[hash] = atom;
    pool->size++;
    return atom;
}

void atom_pool_free(AtomPool **pool_ptr) {
    assert(pool_ptr && *pool_ptr);
    AtomPool *pool = (*pool_ptr);
    for (size_t i = 0; i < pool->capacity; i++) {
        for (Atom *atom = pool->table[i]; atom;) {
            Atom *next = atom->next;
            FREE(atom);
            atom = next;
        }
    }

    FREE(pool->table);
    FREE(*pool_ptr);
}

const void *atom_data(const Atom *atom) {
    assert(atom);
    return atom->data;
}

size_t atom_length(const Atom *atom) {
    assert(atom);
    return atom->length;
}

ByteView atom_view(const Atom *atom) {
    return (ByteView){
        .data = atom->data,
        .length = atom->length,
    };
}
