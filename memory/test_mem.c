#include "mem.h"

#include <assert.h>
#include <stdio.h>

static void test_alloc_pattern(void) {
    unsigned char *bytes = ALLOC(32);

    for (int i = 0; i < 32; i++)
        assert(bytes[i] == 0xA5);

    FREE(bytes);
    assert(bytes == NULL);
}

static void test_calloc_zeroes_memory(void) {
    unsigned char *bytes = CALLOC(16, sizeof(*bytes));

    for (int i = 0; i < 16; i++)
        assert(bytes[i] == 0);

    FREE(bytes);
}

static void test_resize_preserves_contents(void) {
    unsigned char *bytes = ALLOC(8);

    for (int i = 0; i < 8; i++)
        bytes[i] = (unsigned char)i;

    RESIZE(bytes, 32);
    for (int i = 0; i < 8; i++)
        assert(bytes[i] == (unsigned char)i);

    FREE(bytes);
}

int main(void) {
    test_alloc_pattern();
    test_calloc_zeroes_memory();
    test_resize_preserves_contents();

    puts("All memory tests passed");
    return 0;
}
