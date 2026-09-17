#include "assert.h"

const Exception Assert_Failed = {"Assertion failed"};

void (assert)(int e) {
    assert(e);
}
