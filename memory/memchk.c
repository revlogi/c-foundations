#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../exceptions/assert.h"
#include "../exceptions/except.h"
#include "mem.h"

const Except_T Mem_Failed = {"Allocation failed"};

// checking types
union align {
    int i;
    long l;
    long *lp;
    void *p;
    void (*fp)(void);
    float f;
    double d;
    long double ld;
};

// checking macros
#define hash(p, t) ((unsigned long)(p) >> 3) & (sizeof(t) / sizeof((t)[0]) - 1)
#define NDESCRIPTORS 512

#define NALLOC ((4096 + sizeof(union align) - 1) / (sizeof(union align))) * (sizeof(union align))

// checking data
static struct descriptor {
    struct descriptor *free;
    struct descriptor *link;

    const void *ptr;
    long size;
    const char *file;
    int line;
} *htab[2048];

static struct descriptor freelist = {
    .free = &freelist,
};

static void insert_free(struct descriptor *bp);

// checking functions
static struct descriptor *find(const void *ptr) {
    struct descriptor *bp = htab[hash(ptr, htab)];

    while (bp && bp->ptr != ptr) {
        bp = bp->link;
    }
    return bp;
}

static struct descriptor *dalloc(void *ptr, long size, const char *file, int line) {
    static struct descriptor *avail;
    static int nleft;

    if (nleft <= 0) {
        avail = malloc(NDESCRIPTORS * sizeof(*avail));
        if (avail == NULL) return NULL;

        nleft = NDESCRIPTORS;
    }

    avail->ptr = ptr;
    avail->size = size;
    avail->file = file;
    avail->line = line;
    avail->free = avail->link = NULL;
    nleft--;

    return avail++;
}

void *Mem_alloc(long nbytes, const char *file, int line) {
    struct descriptor *bp;
    struct descriptor *prev;
    void *ptr;

    assert(nbytes > 0);
    nbytes = ((nbytes + sizeof(union align) - 1) / (sizeof(union align))) * (sizeof(union align));
    for (bp = freelist.free, prev = &freelist; bp; prev = bp, bp = bp->free) {
        if (bp->size > nbytes) {
            bp->size -= nbytes;
            /* Discard a fragment that can never satisfy a request. */
            if (bp->size == sizeof(union align)) {
                prev->free = bp->free;
            }

            ptr = (char *)bp->ptr + bp->size;
            if ((bp = dalloc(ptr, nbytes, file, line)) != NULL) {
                unsigned h = hash(ptr, htab);
                bp->link = htab[h];
                htab[h] = bp;
                memset(ptr, 0xA5, nbytes);
                return ptr;
            } else {
                if (file == NULL)
                    RAISE(Mem_Failed);
                else
                    Except_raise(&Mem_Failed, file, line);
            }
        }

        if (bp == &freelist) {
            struct descriptor *newptr;
            if ((ptr = malloc(nbytes + NALLOC)) == NULL ||
                (newptr = dalloc(ptr, nbytes + NALLOC, __FILE__, __LINE__)) == NULL) {
                if (file == NULL)
                    RAISE(Mem_Failed);
                else
                    Except_raise(&Mem_Failed, file, line);
            }
            insert_free(newptr);
        }
    }
    assert(0);
    return NULL;
}

void *Mem_calloc(long count, long nbytes, const char *file, int line) {
    void *ptr;

    assert(count > 0);
    assert(nbytes > 0);

    ptr = Mem_alloc(count * nbytes, file, line);
    // set each byte to zero
    memset(ptr, '\0', count * nbytes);
    return ptr;
}

static void insert_free(struct descriptor *bp) {
    struct descriptor *next = freelist.free;
    struct descriptor *prev = &freelist;

    while (next != &freelist &&
           (uintptr_t)next->ptr < (uintptr_t)bp->ptr) {
        prev = next;
        next = next->free;
    }

    bp->free = next;
    prev->free = bp;

    if (next != &freelist &&
        (const char *)bp->ptr + bp->size == (const char *)next->ptr) {
        bp->size += next->size;
        bp->free = next->free;
    }

    if (prev != &freelist &&
        (const char *)prev->ptr + prev->size == (const char *)bp->ptr) {
        prev->size += bp->size;
        prev->free = bp->free;
    }
}

void Mem_free(void *ptr, const char *file, int line) {
    if (ptr) {
        struct descriptor *bp;
        if (((unsigned long)ptr) % (sizeof(union align)) != 0 || (bp = find(ptr)) == NULL ||
            bp->free) {
            Except_raise(&Assert_Failed, file, line);
        }

        insert_free(bp);
    }
}

void *Mem_resize(void *ptr, long nbytes, const char *file, int line) {
    struct descriptor *bp;
    void *newptr;

    assert(ptr);
    assert(nbytes > 0);

    if (((unsigned long)ptr) % (sizeof(union align)) != 0 || (bp = find(ptr)) == NULL || bp->free) {
        Except_raise(&Assert_Failed, file, line);
    }
    newptr = Mem_alloc(nbytes, file, line);
    memcpy(newptr, ptr, nbytes < bp->size ? nbytes : bp->size);
    Mem_free(ptr, file, line);
    return newptr;
}
