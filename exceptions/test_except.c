#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "except.h"

static const Except_T Foo = {"Foo exception"};
static const Except_T Bar = {"Bar exception"};

static void test_catch(void) {
    int caught = 0;
    int finalized = 0;

    TRY
        RAISE(Foo);
        assert(0);
    EXCEPT(Foo)
        caught = 1;
    FINALLY
        finalized = 1;
    END_TRY;

    assert(caught == 1);
    assert(finalized == 1);
    assert(Except_stack == NULL);
}

static void test_else(void) {
    int caught = 0;

    TRY
        RAISE(Bar);
    EXCEPT(Foo)
        assert(0);
    ELSE
        caught = 1;
    END_TRY;

    assert(caught == 1);
    assert(Except_stack == NULL);
}

static void test_finally_on_normal_exit(void) {
    int finalized = 0;

    TRY
        assert(Except_stack != NULL);
    FINALLY
        finalized = 1;
    END_TRY;

    assert(finalized == 1);
    assert(Except_stack == NULL);
}

static void test_reraise(void) {
    int outer_caught = 0;

    TRY
        TRY
            RAISE(Foo);
        EXCEPT(Foo)
            RERAISE;
        END_TRY;

        assert(0);
    EXCEPT(Foo)
        outer_caught = 1;
    END_TRY;

    assert(outer_caught == 1);
    assert(Except_stack == NULL);
}

static void test_finally_before_reraise(void) {
    volatile int finalized = 0;
    int outer_caught = 0;

    TRY
        TRY
            RAISE(Foo);
        FINALLY
            finalized = 1;
        END_TRY;
    EXCEPT(Foo)
        outer_caught = 1;
    END_TRY;

    assert(finalized == 1);
    assert(outer_caught == 1);
    assert(Except_stack == NULL);
}

static int return_from_try(void) {
    TRY
        RETURN 42;
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
    test_catch();
    test_else();
    test_finally_on_normal_exit();
    test_reraise();
    test_finally_before_reraise();
    test_return();
    test_uncaught_exception();

    puts("All tests passed");
    return 0;
}
