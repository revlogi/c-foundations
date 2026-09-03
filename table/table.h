#ifndef TABLE_INCLUDED
#define TABLE_INCLUDED

#include <stddef.h>

#include "../view/view.h"

typedef struct Table *Table_T;
typedef enum { TABLE_INSERTED, TABLE_REPLACED } InstallResult;

Table_T Table_init(void);
void Table_free(Table_T *table_ptr);

size_t Table_size(Table_T table);

InstallResult Table_install(Table_T table, ByteView key, ByteView value);
bool Table_lookup(Table_T table, ByteView key, ByteView *out_value);
bool Table_remove(Table_T table, ByteView key);

#endif
