#ifndef ATOM_INCLUDED
#define ATOM_INCLUDED

#include <stddef.h>

#include "../view/view.h"

typedef struct AtomPool AtomPool;
typedef struct Atom Atom;

AtomPool* atom_pool_new(void);
void atom_pool_free(AtomPool** pool_ptr);

const Atom* atom_pool_intern(AtomPool* pool, ByteView bytes);
const void* atom_data(const Atom* atom);
size_t atom_length(const Atom* atom);

ByteView atom_view(const Atom* atom);

#endif
