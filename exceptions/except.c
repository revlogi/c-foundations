#include "except.h"

#include <stdio.h>
#include <stdlib.h>

#include "assert.h"

Except_Frame *Except_stack = NULL;

_Noreturn void Except_raise(const Exception *e, const char *file, int line) {
    Except_Frame *p = Except_stack;

    assert(e);
    if (p == NULL) {
        fprintf(stderr, "Uncaught exception");
        if (e->reason)
            fprintf(stderr, " %s", e->reason);
        else
            fprintf(stderr, " at %p", (const void *)e);
        if (file && line > 0)
            fprintf(stderr, " raised at %s:%d\n", file, line);
        fprintf(stderr, "aborting...\n");
        fflush(stderr);
        abort();
    }

    p->exception = e;
    p->file = file;
    p->line = line;

    longjmp(p->env, EXCEPT_RAISED);
}
