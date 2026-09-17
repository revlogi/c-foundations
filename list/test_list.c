#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../memory/mem.h"
#include "list.h"

static void test_new_list(void) {
    List *list = list_new();

    assert(list_empty(list));
    assert(list_length(list) == 0);

    void *value = NULL;
    assert(!list_front(list, &value));
    assert(!list_back(list, &value));

    list_free(&list);
    assert(list == NULL);
}

static void test_push_and_peek(void) {
    int a = 1, b = 2, c = 2;

    List *list = list_new();
    list_push_back(list, &b);
    list_push_back(list, &c);
    list_push_front(list, &a);

    assert(list_length(list) == 3);
    assert(!list_empty(list));

    void *value = NULL;
    assert(list_back(list, &value));
    assert(value == &c);

    assert(list_front(list, &value));
    assert(value == &a);

    list_free(&list);
}

static void test_pop(void) {
    int a = 1, b = 2, c = 3;

    List *list = list_new();
    void *value = NULL;
    assert(list_pop_front(list, &value) == false);
    assert(value == NULL);

    list_push_back(list, &a);
    list_push_back(list, &b);
    list_push_back(list, &c);

    assert(list_pop_front(list, &value) == true);
    assert(value == &a);
    assert(list_length(list) == 2);

    assert(list_pop_back(list, &value) == true);
    assert(value == &c);
    assert(list_length(list) == 1);

    list_free(&list);
}

static void test_reverse(void) {
    int a = 1, b = 2, c = 3;

    List *list = list_new();
    list_push_back(list, &a);
    list_push_back(list, &b);
    list_push_back(list, &c);

    list_reverse(list);

    void *value = NULL;
    assert(list_front(list, &value) == true);
    assert(value == &c);

    assert(list_back(list, &value) == true);
    assert(value == &a);

    list_free(&list);
}

static void test_copy(void) {
    int a = 1, b = 2, c = 3;

    List *list = list_new();
    list_push_back(list, &a);
    list_push_back(list, &b);
    list_push_back(list, &c);

    List *new_list = list_copy(list);
    void *value = NULL;
    assert(list_front(new_list, &value) == true);
    assert(value == &a);

    assert(list_back(new_list, &value));
    assert(value == &c);

    list_free(&list);
    list_free(&new_list);
}

static void test_splice(void) {
    int a = 1, b = 2, c = 3;

    List *list1 = list_new();
    List *list2 = list_new();

    list_push_back(list1, &a);
    list_push_back(list1, &b);
    list_push_back(list2, &c);

    list_splice(list1, &list2);

    void *value = NULL;
    assert(list_front(list1, &value) == true);
    assert(value == &a);

    assert(list_back(list1, &value));
    assert(value == &c);

    assert(list2 == NULL);

    list_free(&list1);
}

static void double_int(void **value, void *context) {
    (void)context;

    int *number = *value;
    *number *= 2;
}

static void test_map(void) {
    int a = 1, b = 2, c = 3;

    List *list = list_new();
    list_push_back(list, &a);
    list_push_back(list, &b);
    list_push_back(list, &c);

    list_map(list, double_int, NULL);

    assert(a == 2);
    assert(b == 4);
    assert(c == 6);

    list_free(&list);
}

static void test_to_array(void) {
    int a = 1, b = 2, c = 3;

    List *list = list_new();
    list_push_back(list, &a);
    list_push_back(list, &b);
    list_push_back(list, &c);

    size_t count = 0;
    void **array = list_to_array(list, &count);

    assert(count == 3);
    assert(array != NULL);
    assert(array[0] == &a);
    assert(array[1] == &b);
    assert(array[2] == &c);

    FREE(array);
    list_free(&list);
}

static void test_empty_to_array(void) {
    List *list = list_new();

    size_t count = 123;
    void **array = list_to_array(list, &count);

    assert(array == NULL);
    assert(count == 0);

    list_free(&list);
}

static int destroy_count;
static int destroy_sum;

static void record_destroy(void *value) {
    int *number = value;

    destroy_count++;
    destroy_sum += *number;
}

static void test_clear(void) {
    int a = 1, b = 2, c = 3;

    List *list = list_new();
    list_push_back(list, &a);
    list_push_back(list, &b);
    list_push_back(list, &c);

    destroy_count = 0;
    destroy_sum = 0;

    list_clear(list, record_destroy);

    assert(list_empty(list));
    assert(list_length(list) == 0);
    assert(destroy_count == 3);
    assert(destroy_sum == 6);

    list_push_back(list, &a);
    assert(list_length(list) == 1);

    list_free(&list);
}

int main(void) {
    test_new_list();
    test_pop();
    test_push_and_peek();
    test_reverse();
    test_copy();
    test_splice();
    test_map();
    test_to_array();
    test_empty_to_array();
    test_clear();

    puts("All list tests passed.");
    return 0;
}
