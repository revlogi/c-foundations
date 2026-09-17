#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "except.h"

static const Exception Foo = {"Foo exception"};
static const Exception Bar = {"Bar exception"};

static const char *raise_file;
static int raise_line;

static void raise_foo(void) {
    raise_file = __FILE__;
    raise_line = __LINE__ + 1;
    RAISE(Foo);
}

static void test_normal_exit(void) {
    volatile int reached = 0;

    TRY {
        assert(Except_stack != NULL);
        reached = 1;
    } END_TRY;

    assert(reached == 1);
    assert(Except_stack == NULL);
}

static void test_matching_catch(void) {
    volatile int caught = 0;

    TRY {
        RAISE(Foo);
        assert(0);
    } EXCEPT(Foo) { caught = 1; }
    END_TRY;

    assert(caught == 1);
    assert(Except_stack == NULL);
}

static void test_catches_are_checked_in_order(void) {
    volatile int caught_foo = 0;
    volatile int caught_bar = 0;

    TRY { RAISE(Bar); }
    EXCEPT(Foo) { caught_foo = 1; }
    EXCEPT(Bar) { caught_bar = 1; }
    ELSE { assert(0); }
    END_TRY;

    assert(caught_foo == 0);
    assert(caught_bar == 1);
    assert(Except_stack == NULL);
}

static void test_else_catches_an_unmatched_exception(void) {
    volatile int caught = 0;

    TRY { RAISE(Bar); }
    EXCEPT(Foo) { assert(0); }
    ELSE { caught = 1; }
    END_TRY;

    assert(caught == 1);
    assert(Except_stack == NULL);
}

static void test_finally_on_normal_exit(void) {
    volatile int finalized = 0;

    TRY { assert(Except_stack != NULL); }
    FINALLY { finalized = 1; }
    END_TRY;

    assert(finalized == 1);
    assert(Except_stack == NULL);
}

static void test_finally_after_catch(void) {
    volatile int caught = 0;
    volatile int finalized = 0;

    TRY { RAISE(Foo); }
    EXCEPT(Foo) { caught = 1; }
    FINALLY { finalized = 1; }
    END_TRY;

    assert(caught == 1);
    assert(finalized == 1);
    assert(Except_stack == NULL);
}

static void test_unmatched_exception_propagates_after_finally(void) {
    volatile int caught = 0;
    volatile int finalized = 0;

    TRY {
        TRY { RAISE(Foo); }
        EXCEPT(Bar) { assert(0); }
        FINALLY { finalized = 1; }
        END_TRY;

        assert(0);
    } EXCEPT(Foo) { caught = 1; }
    END_TRY;

    assert(finalized == 1);
    assert(caught == 1);
    assert(Except_stack == NULL);
}

static void test_reraise_preserves_origin(void) {
    volatile int caught = 0;

    TRY {
        TRY { raise_foo(); }
        EXCEPT(Foo) { RERAISE; }
        END_TRY;

        assert(0);
    } EXCEPT(Foo) {
        caught = 1;
        assert(except_frame.exception == &Foo);
        assert(strcmp(except_frame.file, raise_file) == 0);
        assert(except_frame.line == raise_line);
    } END_TRY;

    assert(caught == 1);
    assert(Except_stack == NULL);
}

static int return_from_try(void) {
    TRY { RETURN 42; }
    END_TRY;

    return 0;
}

static void test_return(void) {
    assert(return_from_try() == 42);
    assert(Except_stack == NULL);
}

static void test_uncaught_exception(void) {
    int status;
    pid_t child = fork();

    assert(child >= 0);
    if (child == 0) {
        (void)close(STDERR_FILENO);
        RAISE(Foo);
        _Exit(EXIT_FAILURE);
    }

    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGABRT);
    assert(Except_stack == NULL);
}

int main(void) {
    test_normal_exit();
    test_matching_catch();
    test_catches_are_checked_in_order();
    test_else_catches_an_unmatched_exception();
    test_finally_on_normal_exit();
    test_finally_after_catch();
    test_unmatched_exception_propagates_after_finally();
    test_reraise_preserves_origin();
    test_return();
    test_uncaught_exception();

    puts("All exception tests passed");
    return 0;
}
