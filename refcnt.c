/*
Reference counter for C. 
Based on this article:
http://www.xs-labs.com/en/archives/articles/c-reference-counting/
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "refcnt.h"

#ifndef NDEBUG
typedef struct history_list {
	const char *desc;
	const char *file;
	int line;
	struct history_list *next;
} HistoryList;
#endif

typedef struct refobj {
	unsigned int refcnt;
	ref_dtor dtor;
#ifndef NDEBUG
	int is_str;
	HistoryList *list;
	struct refobj *next, *prev;
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

static void exit_fun() {
	RefObj *r;
	/* a = Allocated objects, d = deallocated objects, g = uncollected garbage */
	printf("Ref Counts: a:%d d:%d g:%d\n", rc_alloc_count, rc_free_count, rc_alloc_count - rc_free_count);
	for (r = rc_root; r; r = r->next) {
		HistoryList *it;
		if(r->is_str) {
			printf("** String '%s' with %u references\n", (char*)r + sizeof *r, r->refcnt);
		} else {
			printf("** Object %p with %u references\n", (char*)r + sizeof *r, r->refcnt);
		}
		for(it = r->list; it; it = it->next) {
			printf("    - %s @ %s:%d\n", it->desc, it->file, it->line);
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
	RefObj *r = malloc((sizeof *r) + size);
	if(!r) 
		return NULL;
	data = (char*)r + sizeof *r;
	r->refcnt = 1;
	r->dtor = NULL;
#ifndef NDEBUG
	r->is_str = 0;
	r->list = NULL;
	add_list_item(r, file, line, "rc_alloc");
	r->next = NULL;
	r->next = rc_root;
	rc_root = r;
	if(r->next) r->next->prev = r;
	r->prev = NULL;	
	rc_alloc_count++;
#endif	
	return data;
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
	if(r->refcnt == 0) {
#ifndef NDEBUG
		rc_free_count++;
		if(rc_root == r) rc_root = r->next; 
		if(r->next) r->next->prev = r->prev;
		if(r->prev) r->prev->next = r->next;
		free_list(r->list);
#endif
		if(r->dtor != NULL) {
			r->dtor(p);
		}
		free(r);
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
