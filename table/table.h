#ifndef TABLE_INCLUDED
#define TABLE_INCLUDED

#include <stddef.h>

#include "../view/view.h"

typedef struct Table Table;
typedef enum { TABLE_INSERTED, TABLE_REPLACED } TableInstallResult;

Table *table_new(void);
void table_free(Table **table_ptr);

size_t table_size(const Table *table);

TableInstallResult table_install(Table *table, ByteView key, ByteView value);
bool table_lookup(const Table *table, ByteView key, ByteView *out_value);
bool table_remove(Table *table, ByteView key);

#endif
