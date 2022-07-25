/*
 * Mark and sweep garbage collector.
 *
 * This is free and unencumbered software released into the public domain.
 * http://unlicense.org/
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 1
#include <assert.h>
#else
/* Cause a crash in the debugger so we can get a stack trace */
#define assert(x) do{if(!(x)) {int a = 1/0;(void)a;}}while(0)
#endif

#include "gc.h"

/* Interval at which gc_collect() will be called from gc_alloc().
 * If it is, for example, 100 then on every hundredth call of gc_alloc()
 * gc_collect() will b called.
 */
#define GC_INTERVAL	10000

#define GC_VERBOSE 0

typedef struct gc_object {
	int mark;
	gc_function marker;
	gc_function dtor;

#ifndef NDEBUG
	/* In debug mode, we can track and display where the
		objects were allocated for troubleshooting.
	*/
	const char *file;
	int line;
#endif

	int retcount;

	/* For the linked list of all objects; for the sweep */
	struct gc_object *next, *prev;

	/* For the list of root obects for the mark */
	struct gc_object *rnext, *rprev;

} GcObj;

/* All objects allocated for sweep */
static GcObj *gc_objs = NULL;

/* Root objects that need to be navigated during mark */
static GcObj *gc_roots = NULL;

static int gc_counter = 0;

#ifdef NDEBUG
void *gc_alloc(size_t size) {
#else
void *gc_alloc_(size_t size, const char *file, int line) {
#endif
	void *data;
	GcObj *r;

	if(gc_counter++ > GC_INTERVAL) {
		gc_counter = 0;
		gc_collect();
	}

	r = malloc((sizeof *r) + size);
	data = (char*)r + sizeof *r;
	r->mark = 0;
	r->marker = NULL;
	r->dtor = NULL;

	r->retcount = 0;

#ifndef NDEBUG
	r->file = file;
	r->line = line;
#endif

	r->next = gc_objs;
	gc_objs = r;
	if(r->next) r->next->prev = r;
	r->prev = NULL;

	r->rnext = NULL;
	r->rprev = NULL;

	return data;
}

void *gc_retain(void *p) {
	GcObj *r;
	assert(p);
	r = (GcObj *)((char *)p - sizeof *r);

	assert(r->retcount >= 0);
	if(r->retcount == 0) {
		r->rnext = gc_roots;
		gc_roots = r;
		if(r->rnext) r->rnext->rprev = r;
		r->rprev = NULL;
	}
	r->retcount++;

	return p;
}

void gc_release(void *p) {
	GcObj *r;
	assert(p);
	r = (GcObj *)((char *)p - sizeof *r);

	assert(r->retcount > 0);
	r->retcount--;
	if(r->retcount == 0) {
		if(r->rnext)  r->rnext->rprev = r->rprev;
		if(r->rprev)  r->rprev->rnext = r->rnext;
		if(gc_roots == r)
			gc_roots = r->rnext;
		r->rnext = NULL;
		r->rprev = NULL;
	}
}

static int gc_markedx;

void gc_mark(void *p) {
	GcObj *r;
	assert(p);
	r = (GcObj *)((char *)p - sizeof *r);
	if(r->mark) return;
	r->mark = 1;
	gc_markedx++;
	if(r->marker)
		r->marker(p);
}

static void gc_mark_all() {
	GcObj *gc;
	gc_markedx = 0;
	for(gc = gc_roots; gc; gc = gc->rnext) {
		void *data = (char*)gc + sizeof *gc;
		gc_mark(data);
	}
#if !defined(NDEBUG) && GC_VERBOSE
	printf("GC mark: %d objects marked\n", gc_markedx);
#endif
}

static void gc_sweep() {
	GcObj *gc = gc_objs;

	int collected = 0;

	while(gc) {
		if(gc->mark == 0) {
			GcObj *col = gc;
			gc = gc->next;

			if(col->next)  col->next->prev = col->prev;
			if(col->prev)  col->prev->next = col->next;
			if(gc_objs == col)
				gc_objs = col->next;

			if(col->dtor) {
				void *data = (char*)col + sizeof *col;
				col->dtor(data);
			}
			free(col);

			collected++;
		} else {
			gc->mark = 0;
			gc = gc->next;
		}
	}
#if !defined(NDEBUG) && GC_VERBOSE
	printf("GC sweep: %d objects collected\n", collected);
#endif
}

void gc_collect() {
	gc_mark_all();
	gc_sweep();
}

void gc_set_dtor(void *p, gc_function dtor) {
	GcObj *r;
	assert(p);
	r = (GcObj *)((char *)p - sizeof *r);
	r->dtor = dtor;
}

void gc_set_marker(void *p, gc_function marker) {
	GcObj *r;
	assert(p);
	r = (GcObj *)((char *)p - sizeof *r);
	r->marker = marker;
}

void gc_dump() {
	GcObj *gc = gc_roots;
	if(gc) {
		printf("Roots:\n");
		while(gc) {
			void *data = (char*)gc + sizeof *gc;
#ifndef NDEBUG
			printf("    - %p (%d) @ %s:%d\n", data, gc->retcount, gc->file, gc->line);
#else
			printf("    - %p (%d)\n", data, gc->retcount);
#endif
			gc = gc->rnext;
		}
	}

	gc = gc_objs;
	if(gc) {
		printf("Garbage:\n");
		while(gc) {
			void *data = (char*)gc + sizeof *gc;
			printf("    - %p (%d)\n", data, gc->mark);
			gc = gc->next;
		}
	}
}

void gc_init() {
#ifndef NDEBUG
	atexit(gc_dump);
#endif
}