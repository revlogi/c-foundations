#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../memory/mem.h"
#include "list.h"

static void test_new_list(void) {
    List_T list = List_new();

    assert(List_empty(list));
    assert(List_length(list) == 0);

    void *value = NULL;
    assert(!List_front(list, &value));
    assert(!List_back(list, &value));

    List_free(&list);
    assert(list == NULL);
}

static void test_push_and_peek(void) {
    int a = 1, b = 2, c = 2;

    List_T list = List_new();
    List_push_back(list, &b);
    List_push_back(list, &c);
    List_push_front(list, &a);

    assert(List_length(list) == 3);
    assert(!List_empty(list));

    void *value = NULL;
    assert(List_back(list, &value));
    assert(value == &c);

    assert(List_front(list, &value));
    assert(value == &a);

    List_free(&list);
}

static void test_pop(void) {
    int a = 1, b = 2, c = 3;

    List_T list = List_new();
    void *value = NULL;
    assert(List_pop_front(list, &value) == false);
    assert(value == NULL);

    List_push_back(list, &a);
    List_push_back(list, &b);
    List_push_back(list, &c);

    assert(List_pop_front(list, &value) == true);
    assert(value == &a);
    assert(List_length(list) == 2);

    assert(List_pop_back(list, &value) == true);
    assert(value == &c);
    assert(List_length(list) == 1);

    List_free(&list);
}

static void test_reverse(void) {
    int a = 1, b = 2, c = 3;

    List_T list = List_new();
    List_push_back(list, &a);
    List_push_back(list, &b);
    List_push_back(list, &c);

    List_reverse(list);

    void *value = NULL;
    assert(List_front(list, &value) == true);
    assert(value == &c);

    assert(List_back(list, &value) == true);
    assert(value == &a);

    List_free(&list);
}

static void test_copy(void) {
    int a = 1, b = 2, c = 3;

    List_T list = List_new();
    List_push_back(list, &a);
    List_push_back(list, &b);
    List_push_back(list, &c);

    List_T new_list = List_copy(list);
    void *value = NULL;
    assert(List_front(new_list, &value) == true);
    assert(value == &a);

    assert(List_back(new_list, &value));
    assert(value == &c);

    List_free(&list);
    List_free(&new_list);
}

static void test_splice(void) {
    int a = 1, b = 2, c = 3;

    List_T list1 = List_new();
    List_T list2 = List_new();

    List_push_back(list1, &a);
    List_push_back(list1, &b);
    List_push_back(list2, &c);

    List_splice(list1, &list2);

    void *value = NULL;
    assert(List_front(list1, &value) == true);
    assert(value == &a);

    assert(List_back(list1, &value));
    assert(value == &c);

    assert(list2 == NULL);

    List_free(&list1);
}

static void double_int(void **value, void *context) {
    (void)context;

    int *number = *value;
    *number *= 2;
}

static void test_map(void) {
    int a = 1, b = 2, c = 3;

    List_T list = List_new();
    List_push_back(list, &a);
    List_push_back(list, &b);
    List_push_back(list, &c);

    List_map(list, double_int, NULL);

    assert(a == 2);
    assert(b == 4);
    assert(c == 6);

    List_free(&list);
}

static void test_to_array(void) {
    int a = 1, b = 2, c = 3;

    List_T list = List_new();
    List_push_back(list, &a);
    List_push_back(list, &b);
    List_push_back(list, &c);

    size_t count = 0;
    void **array = List_to_array(list, &count);

    assert(count == 3);
    assert(array != NULL);
    assert(array[0] == &a);
    assert(array[1] == &b);
    assert(array[2] == &c);

    FREE(array);
    List_free(&list);
}

static void test_empty_to_array(void) {
    List_T list = List_new();

    size_t count = 123;
    void **array = List_to_array(list, &count);

    assert(array == NULL);
    assert(count == 0);

    List_free(&list);
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

    List_T list = List_new();
    List_push_back(list, &a);
    List_push_back(list, &b);
    List_push_back(list, &c);

    destroy_count = 0;
    destroy_sum = 0;

    List_clear(list, record_destroy);

    assert(List_empty(list));
    assert(List_length(list) == 0);
    assert(destroy_count == 3);
    assert(destroy_sum == 6);

    List_push_back(list, &a);
    assert(List_length(list) == 1);

    List_free(&list);
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
