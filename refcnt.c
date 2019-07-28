/*
 * Reference counter for C.
 *
 * It works by allocating a hidden header before each object
 * containing the reference count.
 * A `rc_retain()` operation increments this reference count.
 * A `rc_release()` operation decrements the reference count.
 * If the reference count reaches 0 the object is deallocated.
 *
 * The header also has a pointer to a _destructor_ function that
 * will be called (if set) when the object is deallocated so
 * that the object can release any references to other objects
 * it may hold.
 *
 * In _debug_ mode (if `NDEBUG` is not defined) the objects are
 * added and removed from a linked list as they are allocated
 * and deallocated. Each object also tracks where it was
 * allocated, retained and released using some `__FILE__` and
 * `__LINE__` trickery, and will print a report to
 * `stdout` of objects that are still allocated when the program
 * terminates in order to troubleshoot memory leaks.
 *
 * This adds a lot of overhead and is not reentrant because of
 * its reliance on a global linked list.
 *
 * See `refcnt.h` for usage instructions
 *
 * Based on this article:
 * http://www.xs-labs.com/en/archives/articles/c-reference-counting/
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "refcnt.h"

/* Define DEBUG_DOUBLE_DEALLOC to troubleshoot situations
 * where an object might be released more than it is retained.
 *
 * In this case, the object won't actually be `free()`d so that
 * the debugging facilities can track the releases, so the program
 * _will_ leak memory.
 *
 * I don't recommend using it all the time during development either,
 * because it may mask other errors (like segfaults that woudl normaly
 * trigger the debugger)
 */
#if 0
#  define DEBUG_DOUBLE_DEALLOC
#else
#  undef DEBUG_DOUBLE_DEALLOC
#endif

#ifndef NDEBUG

#  ifdef DEBUG_DOUBLE_DEALLOC
/* Can't be unsigned for this to work */
typedef int RefType;
#  else
typedef unsigned int RefType;
#  endif

/* to guard against buffer overruns */
#  define SENTINEL    0xDEADBEEF

typedef struct history_list {
    const char *desc;
    const char *file;
    int line;
    struct history_list *next;
} HistoryList;
#else
/* NDEBUG (Release mode) */
typedef unsigned int RefType;
#  ifdef DEBUG_DOUBLE_DEALLOC
/* This doesn't work in release mode */
#    undef DEBUG_DOUBLE_DEALLOC
#  endif
#endif

typedef struct refobj {
    RefType refcnt;
    ref_dtor dtor;
#ifndef NDEBUG
    int is_str;
    HistoryList *list;
    struct refobj *next, *prev;
    size_t size;
#endif
} RefObj;

#ifndef NDEBUG
static void add_list_item(RefObj *r, const char *file, int line, const char *desc) {
    HistoryList *hl = malloc(sizeof *hl), *it;
    hl->next = NULL;
    if(r->list) {
        for(it = r->list; it->next; it = it->next);
        it->next = hl;
    } else {
        r->list = hl;
    }
    hl->file = file;
    hl->line = line;
    hl->desc = desc;
}

static void free_list(HistoryList *hl) {
    if(hl->next)
        free_list(hl->next);
    free(hl);
}

static int rc_alloc_count = 0;
static int rc_free_count = 0;
static RefObj *rc_root = NULL;

static size_t alloced = 0, max_alloced = 0;

static void exit_fun() {
    RefObj *r;
    if(alloced || 0) {
        /* a = Allocated objects, d = deallocated objects, g = uncollected garbage */
        printf("\n** Reference counter report **\n");
        printf("** Ref Counts..: a:%d d:%d g:%d\n", rc_alloc_count, rc_free_count, rc_alloc_count - rc_free_count);
        printf("** Mem Usage...: max:%lu cur:%lu\n", (unsigned long)max_alloced, (unsigned long)alloced);
        for (r = rc_root; r; r = r->next) {
            HistoryList *it;

#ifdef DEBUG_DOUBLE_DEALLOC
            if(r->refcnt != 0) {
                if(r->is_str) {
                    printf("** - String '%s' with %d references\n", (char*)r + sizeof *r, r->refcnt);
                } else {
                    printf("** - Object %p with %d references\n", (char*)r + sizeof *r, r->refcnt);
                }
                for(it = r->list; it; it = it->next) {
                    printf("      - %s @ %s:%d\n", it->desc, it->file, it->line);
                }
            }
#else
            if(r->is_str) {
                printf("** - String '%s' with %u references\n", (char*)r + sizeof *r, r->refcnt);
            } else {
                printf("** - Object %p with %u references\n", (char*)r + sizeof *r, r->refcnt);
            }
            for(it = r->list; it; it = it->next) {
                printf("      - %s @ %s:%d\n", it->desc, it->file, it->line);
            }
#endif
        }
    }
}
#endif

void rc_init() {
#ifndef NDEBUG
    atexit(exit_fun);
#endif
}

#ifdef NDEBUG
void *rc_alloc(size_t size) {
#else
void *rc_alloc_(size_t size, const char *file, int line) {
#endif
    void *data;

#ifndef NDEBUG
    RefObj *r = malloc((sizeof *r) + size + sizeof(int));
#else
    RefObj *r = malloc((sizeof *r) + size);
#endif
    if(!r)
        return NULL;
    data = (char*)r + sizeof *r;
    r->refcnt = 1;
    r->dtor = NULL;
#ifndef NDEBUG

    *(int *)((char *)data + size) = SENTINEL;

    r->is_str = 0;
    r->list = NULL;
    r->size = size;
    alloced += size;
    if(alloced > max_alloced)
        max_alloced = alloced;
    add_list_item(r, file, line, "rc_alloc");
    r->next = rc_root;
    rc_root = r;
    if(r->next) r->next->prev = r;
    r->prev = NULL;
    rc_alloc_count++;
#endif
    return data;
}

#ifdef NDEBUG
void *rc_realloc(void *p, size_t size) {
#else
void *rc_realloc_(void *p, size_t size, const char *file, int line) {
#endif
    RefObj *r;
    if(!p)
        return NULL;
    r = (RefObj *)((char *)p - sizeof *r);
    assert(r->refcnt == 1);
#ifdef NDEBUG
    r = realloc(r, (sizeof *r) + size);
#else
    r = realloc(r, (sizeof *r) + size + sizeof(int));
    char *data = (char*)r + sizeof *r;
    *(int *)((char *)data + size) = SENTINEL;

    alloced = alloced - r->size + size;
    if(alloced > max_alloced)
        max_alloced = alloced;
    r->size = size;
    if(r->prev)
        r->prev->next = r;
    else
        rc_root = r;
    if(r->next)
        r->next->prev = r;
    add_list_item(r, file, line, "rc_realloc");
#endif
    return (char*)r + sizeof *r;
}

#ifdef NDEBUG
char *rc_strdup(const char *s) {
#else
void *rc_strdup_(const char *s, const char *file, int line) {
    RefObj *r;
#endif
    size_t len = strlen(s);
    char *n = rc_alloc(len + 1);
    if(!n)
        return NULL;
    memcpy(n, s, len + 1);
#ifndef NDEBUG
    r = (RefObj *)((char *)n - sizeof *r);
    assert(r->list);
    r->list->file = file;
    r->list->line = line;
    r->list->desc = "rc_strdup";
    r->is_str = 1;
#endif
    return n;
}


#ifdef NDEBUG
void *rc_memdup(const void *p, size_t size) {
#else
void *rc_memdup_(const void *p, size_t size, const char *file, int line) {
    RefObj *r;
#endif
    char *n = rc_alloc(size);
    if(!n)
        return NULL;
    memcpy(n, p, size);
#ifndef NDEBUG
    r = (RefObj *)((char *)n - sizeof *r);
    assert(r->list);
    r->list->file = file;
    r->list->line = line;
    r->list->desc = "rc_memdup";
    r->is_str = 0;
#endif
    return n;
}


#ifdef NDEBUG
void *rc_retain(void *p) {
#else
void *rc_retain_(void *p, const char *file, int line) {
#endif
    RefObj *r;
    if(!p)
        return NULL;
    r = (RefObj *)((char *)p - sizeof *r);
    r->refcnt++;
#ifndef NDEBUG
    add_list_item(r, file, line, "rc_retain");
#endif
    return p;
}

#ifdef NDEBUG
void rc_release(void *p) {
#else
void rc_release_(void *p, const char *file, int line) {
#endif
    RefObj *r;
    if(!p)
        return;
    r = (RefObj *)((char *)p - sizeof *r);
    r->refcnt--;
#if !defined(NDEBUG) && defined(DEBUG_DOUBLE_DEALLOC)
    if(r->refcnt < 0) {
        add_list_item(r, file, line, "rc_release ***");
    } else
#endif
    if(r->refcnt == 0) {
#ifndef NDEBUG
        char *data = (char*)r + sizeof *r;
        int sentinel = *(int *)(data + r->size);
        if(sentinel != SENTINEL) {
            assert(r->list);
            fprintf(stderr, "** Buffer overrun on object allocated at %s:%d\n", r->list->file, r->list->line);
            fflush(stderr);
        }

#  ifdef DEBUG_DOUBLE_DEALLOC
        add_list_item(r, file, line, "rc_release");
        (void)free_list;
#  else
        assert(alloced >= r->size);
        alloced -= r->size;
        rc_free_count++;
        if(rc_root == r) rc_root = r->next;
        if(r->next) r->next->prev = r->prev;
        if(r->prev) r->prev->next = r->next;
        free_list(r->list);
#  endif

#endif
        if(r->dtor != NULL) {
            r->dtor(p);
        }
#ifndef DEBUG_DOUBLE_DEALLOC
        free(r);
#endif
    }
#ifndef NDEBUG
    else {
        add_list_item(r, file, line, "rc_release");
    }
#endif
}

void rc_set_dtor(void *p, ref_dtor dtor) {
    RefObj *r;
    if(!p) return;
    r = (RefObj *)((char *)p - sizeof *r);
    r->dtor = dtor;
}

#ifdef NDEBUG
void *rc_assign(void **p, void *val) {
#else
void *rc_assign_(void **p, void *val, const char *file, int line) {
#endif
    if(*p) {
#ifdef NDEBUG
        rc_release(*p);
#else
        rc_release_(*p, file, line);
#endif
    }
    *p = val;
    return val;
}
