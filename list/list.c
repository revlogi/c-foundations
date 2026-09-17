#include "list.h"

#include <stddef.h>

#include "../exceptions/assert.h"
#include "../memory/mem.h"

typedef struct Node *Node_T;

struct Node {
    void *data;
    Node_T prev;
    Node_T next;
};

struct List {
    Node_T sentinel;
    size_t length;
};

List *list_new(void) {
    List *list;
    NEW(list);
    TRY {
        NEW(list->sentinel);
    } EXCEPT(Mem_Failed) {
        FREE(list);
        RERAISE;
    } END_TRY;

    list->sentinel->prev = list->sentinel;
    list->sentinel->next = list->sentinel;
    list->sentinel->data = NULL;
    list->length = 0;

    return list;
}

void list_free(List **list) {
    assert(list && *list);

    list_clear(*list, NULL);
    FREE((*list)->sentinel);
    FREE(*list);
}

void list_clear(List *list, ListDestroyFn destroy) {
    assert(list && list->sentinel);

    Node_T sentinel = list->sentinel;
    Node_T p = sentinel->next;

    while (p != sentinel) {
        Node_T next = p->next;
        if (destroy) {
            destroy(p->data);
        }
        FREE(p);
        p = next;
    }

    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    list->length = 0;
}

size_t list_length(const List *list) {
    assert(list && list->sentinel);
    return list->length;
}

bool list_empty(const List *list) {
    assert(list && list->sentinel);
    return list->length == 0;
}

bool list_front(const List *list, void **value) {
    assert(list && list->sentinel);
    assert(value);

    if (list->sentinel->next == list->sentinel) {
        return false;
    }

    *value = list->sentinel->next->data;
    return true;
}

bool list_back(const List *list, void **value) {
    assert(list && list->sentinel);
    assert(value);

    if (list->sentinel->next == list->sentinel) {
        return false;
    }

    *value = list->sentinel->prev->data;
    return true;
}

void list_push_front(List *list, void *value) {
    assert(list && list->sentinel);
    Node_T sentinel = list->sentinel;

    Node_T p;
    NEW(p);

    p->data = value;
    p->next = sentinel->next;
    p->prev = sentinel;

    sentinel->next->prev = p;
    sentinel->next = p;

    list->length++;
}

bool list_pop_front(List *list, void **value) {
    assert(list && list->sentinel);
    Node_T sentinel = list->sentinel;

    if (value) {
        *value = NULL;
    }

    if (sentinel->next == sentinel) {
        return false;
    }

    Node_T p = sentinel->next;
    sentinel->next->next->prev = sentinel;
    sentinel->next = sentinel->next->next;

    if (value) {
        *value = p->data;
    }
    list->length--;

    FREE(p);
    return true;
}

void list_push_back(List *list, void *value) {
    assert(list && list->sentinel);
    Node_T sentinel = list->sentinel;

    Node_T p;
    NEW(p);

    p->data = value;
    p->prev = sentinel->prev;
    p->next = sentinel;

    sentinel->prev->next = p;
    sentinel->prev = p;

    list->length++;
}

bool list_pop_back(List *list, void **value) {
    assert(list && list->sentinel);
    Node_T sentinel = list->sentinel;

    if (value) {
        *value = NULL;
    }

    if (sentinel->next == sentinel) {
        return false;
    }

    Node_T p = sentinel->prev;
    sentinel->prev->prev->next = sentinel;
    sentinel->prev = sentinel->prev->prev;

    if (value) {
        *value = p->data;
    }
    list->length--;

    FREE(p);
    return true;
}

void list_splice(List *destination, List **source_ptr) {
    assert(source_ptr);

    List *source = *source_ptr;
    assert(destination && destination->sentinel);
    assert(source && source->sentinel);
    assert(destination != source);

    if (source->length == 0) {
        list_free(source_ptr);
        return;
    }

    Node_T des_sentinel = destination->sentinel;
    Node_T sou_sentinel = source->sentinel;

    des_sentinel->prev->next = sou_sentinel->next;
    sou_sentinel->next->prev = des_sentinel->prev;
    des_sentinel->prev = sou_sentinel->prev;
    sou_sentinel->prev->next = des_sentinel;

    destination->length += source->length;

    sou_sentinel->next = sou_sentinel;
    sou_sentinel->prev = sou_sentinel;
    source->length = 0;
    list_free(source_ptr);
}

List *list_copy(const List *list) {
    assert(list && list->sentinel);

    List *new_list = list_new();
    Node_T p = list->sentinel->next;
    for (; p != list->sentinel; p = p->next) {
        list_push_back(new_list, p->data);
    }
    return new_list;
}

void list_reverse(List *list) {
    assert(list && list->sentinel);

    Node_T sentinel = list->sentinel;
    Node_T p = sentinel;

    do {
        Node_T next = p->next;
        p->next = p->prev;
        p->prev = next;
        p = next;
    } while (p != sentinel);
}

void list_map(List *list, ListApplyFn apply, void *context) {
    assert(list && list->sentinel);
    assert(apply);

    Node_T p = list->sentinel->next;
    for (; p != list->sentinel; p = p->next) {
        apply(&p->data, context);
    }
}

void **list_to_array(const List *list, size_t *count) {
    assert(count);

    size_t n = list_length(list);
    if (n == 0) {
        *count = 0;
        return NULL;
    }

    void **array = ALLOC(n * sizeof(*array));
    Node_T p = list->sentinel->next;

    for (size_t i = 0; i < n; i++, p = p->next) {
        array[i] = p->data;
    }

    *count = n;
    return array;
}
