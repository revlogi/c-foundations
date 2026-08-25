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

List_T List_new(void) {
    List_T list;
    NEW(list);
    TRY NEW(list->sentinel);
    EXCEPT(Mem_Failed)
    FREE(list);
    RERAISE;
    END_TRY

    list->sentinel->prev = list->sentinel;
    list->sentinel->next = list->sentinel;
    list->sentinel->data = NULL;
    list->length = 0;

    return list;
}

void List_free(List_T *list) {
    assert(list && *list);

    List_clear(*list, NULL);
    FREE((*list)->sentinel);
    FREE(*list);
}

void List_clear(List_T list, List_destroy_fn destroy) {
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

size_t List_length(List_const_T list) {
    assert(list && list->sentinel);
    return list->length;
}

bool List_empty(List_const_T list) {
    assert(list && list->sentinel);
    return list->length == 0;
}

bool List_front(List_const_T list, void **value) {
    assert(list && list->sentinel);
    assert(value);

    if (list->sentinel->next == list->sentinel) {
        return false;
    }

    *value = list->sentinel->next->data;
    return true;
}

bool List_back(List_const_T list, void **value) {
    assert(list && list->sentinel);
    assert(value);

    if (list->sentinel->next == list->sentinel) {
        return false;
    }

    *value = list->sentinel->prev->data;
    return true;
}

void List_push_front(List_T list, void *value) {
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

bool List_pop_front(List_T list, void **value) {
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

void List_push_back(List_T list, void *value) {
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

bool List_pop_back(List_T list, void **value) {
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

void List_splice(List_T destination, List_T *source_ptr) {
    assert(source_ptr);

    List_T source = *source_ptr;
    assert(destination && destination->sentinel);
    assert(source && source->sentinel);
    assert(destination != source);

    if (source->length == 0) {
        List_free(source_ptr);
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
    List_free(source_ptr);
}

List_T List_copy(List_const_T list) {
    assert(list && list->sentinel);

    List_T new_list = List_new();
    Node_T p = list->sentinel->next;
    for (; p != list->sentinel; p = p->next) {
        List_push_back(new_list, p->data);
    }
    return new_list;
}

void List_reverse(List_T list) {
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

void List_map(List_T list, List_apply_fn apply, void *context) {
    assert(list && list->sentinel);
    assert(apply);

    Node_T p = list->sentinel->next;
    for (; p != list->sentinel; p = p->next) {
        apply(&p->data, context);
    }
}

void **List_to_array(List_const_T list, size_t *count) {
    assert(count);

    size_t n = List_length(list);
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
