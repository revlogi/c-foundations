#ifndef LIST_INCLUDED
#define LIST_INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef struct List* List_T;
typedef const struct List* List_const_T;

typedef void (*List_apply_fn)(void** value, void* context);
typedef void (*List_destroy_fn)(void* value);

List_T List_new(void);
void List_free(List_T* list);
void List_clear(List_T list, List_destroy_fn destroy);

size_t List_length(List_const_T list);
bool List_empty(List_const_T list);

List_T List_copy(List_const_T list);
void List_reverse(List_T list);

bool List_front(List_const_T list, void** value);
bool List_back(List_const_T list, void** value);

void List_push_front(List_T list, void* value);
void List_push_back(List_T list, void* value);
bool List_pop_front(List_T list, void** value);
bool List_pop_back(List_T list, void** value);

void List_splice(List_T destination, List_T* source_ptr);

void List_map(List_T list, List_apply_fn apply, void* context);
void** List_to_array(List_const_T list, size_t* count);

#endif
