#ifndef EXCEPT_INCLUDED
#define EXCEPT_INCLUDED
#include <setjmp.h>

typedef struct Exception Exception;
struct Exception {
    const char *reason;
};

// exported types
typedef struct Except_Frame Except_Frame;
struct Except_Frame {
    Except_Frame *prev;
    jmp_buf env;
    const char *file;
    int line;
    const Exception *exception;
};

enum { EXCEPT_ENTERED = 0, EXCEPT_RAISED, EXCEPT_CAUGHT, EXCEPT_HANDLED };

// exported variables
extern Except_Frame *Except_stack;
extern const Exception Assert_Failed;

// exported functions
_Noreturn void Except_raise(const Exception *e, const char *file, int line);

// exported macros
#define RAISE(e) Except_raise(&(e), __FILE__, __LINE__)
#define RERAISE                                                                \
    Except_raise(except_frame.exception, except_frame.file, except_frame.line)
#define RETURN                                                                 \
    switch (Except_stack = Except_stack->prev, 0)                              \
    default:                                                                   \
        return

// helper function
static inline void Except_pop_once(volatile int *popped) {
    if (!*popped) {
        Except_stack = Except_stack->prev;
        *popped = 1;
    }
}

#define TRY                                                                    \
    do {                                                                       \
        volatile int except_state;                                             \
        volatile int except_popped = 0;                                        \
        Except_Frame except_frame;                                             \
        except_frame.prev = Except_stack;                                      \
        Except_stack = &except_frame;                                          \
        except_state = setjmp(except_frame.env);                               \
        if (except_state == EXCEPT_ENTERED)

#define EXCEPT(e)                                                              \
    else if ((Except_pop_once(&except_popped), except_frame.exception == &(e)) \
                 ? (except_state = EXCEPT_HANDLED, 1)                          \
                 : 0)

#define ELSE                                                                   \
    else if (Except_pop_once(&except_popped),                                  \
             (except_state = EXCEPT_HANDLED, 1))

#define FINALLY                                                                \
    Except_pop_once(&except_popped);                                           \
    if (1)

#define END_TRY                                                                \
    Except_pop_once(&except_popped);                                           \
    if (except_state == EXCEPT_RAISED)                                          \
        RERAISE;                                                               \
    }                                                                          \
    while (0)                                                                  \
        ;

#endif
