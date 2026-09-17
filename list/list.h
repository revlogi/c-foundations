#ifndef LIST_INCLUDED
#define LIST_INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef struct List List;

typedef void (*ListApplyFn)(void **value, void *context);
typedef void (*ListDestroyFn)(void *value);

List *list_new(void);
void list_free(List **list);
void list_clear(List *list, ListDestroyFn destroy);

size_t list_length(const List *list);
bool list_empty(const List *list);

List *list_copy(const List *list);
void list_reverse(List *list);

bool list_front(const List *list, void **value);
bool list_back(const List *list, void **value);

void list_push_front(List *list, void *value);
void list_push_back(List *list, void *value);
bool list_pop_front(List *list, void **value);
bool list_pop_back(List *list, void **value);

void list_splice(List *destination, List **source_ptr);

void list_map(List *list, ListApplyFn apply, void *context);
void **list_to_array(const List *list, size_t *count);

#endif
