#ifndef TABLE_INCLUDED
#define TABLE_INCLUDED

#include <stddef.h>
typedef struct Table *Table_T;

typedef int (*Table_cmp_fn)(const void *x, const void *y);
typedef unsigned (*Table_hash_fn)(const void *key);

typedef void (*Table_apply_fn)(const void *key, void **value, void *cl);

Table_T Table_new(int hint, Table_cmp_fn cmp, Table_hash_fn hash);
void Table_free(Table_T *table);

int Table_length(Table_T);
void *Table_put(Table_T table, const void *key, void *value);
void *Table_get(Table_T table, const void *key);
void *Table_remove(Table_T table, const void *key);

void Table_map(Table_T table, Table_apply_fn apply, void *cl);
void **Table_to_array(Table_T table, size_t *count);

#endif
