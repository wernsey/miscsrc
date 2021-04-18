/*
 * JSON Parser and serializer.
 *
 * * [www.json.org][json.org]
 * * [RFC 7159][rfc7159]
 * * [RFC 4627][rfc4627]
 *
 * See `json.h` for more info
 *
 * [json.org]: https://www.json.org/json-en.html
 * [rfc7159]: https://tools.ietf.org/html/rfc7159
 * [rfc4627]: https://tools.ietf.org/id/draft-ietf-json-rfc4627bis-09.html
 *
 *
 * This is free and unencumbered software released into the public domain.
 * http://unlicense.org/
 *
 * TODO: I came across this test suite for JSON parsers:
 * [Parsing JSON is a Minefield](http://seriot.ch/parsing_json.php);
 * I should maybe try running this parser through it one day.
 *
 * TODO: I'm not against extending this to [JSON5](https://json5.org/)
 * in principle (See my comments about why I allow comments below).
 * I'm thinking I should parse JSON5 and only emit strict JSON, or
 * perhaps have separate `json_parse()` and `json_parse5()` functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>

#include <math.h>

#include "json.h"

typedef struct HashTable HashTable;
typedef struct Array Array;

/*
 * I am aware of the reasons for [the JSON spec not supporting comments][no-comments],
 * but I also subscribe to [Postel's Law][postel], so the parser has support for
 * comments by default.
 *
 * The parser can be forced to be strict about not allowing comments by defining
 * `JSON_COMMENTS` as 0.
 *
 * Serialization does not ever emit comments, though.
 *
 * [no-comments]: https://stackoverflow.com/a/10976934/115589
 * [postel]: https://en.wikipedia.org/wiki/Robustness_principle
 */
#ifndef JSON_COMMENTS
#  define JSON_COMMENTS 1
#endif

/*
 * If `JSON_REENTRANT` is defined as 1 then each call to the
 * functions `json_null()`, `json_true()` and `json_false()`
 * returns a newly allocated object on the heap, making them
 * reentrant (or at least as reentrant as your system's `malloc()`
 * implementation).
 *
 * If it is defined as 0 then those functions return a reference
 * to a global object of the value.
 *
 * The former is reentrant and thus thread safe. The latter
 * saves memory and avoids some of the overheads of allocating
 * the objects.
 *
 * Since the first call to one of the functions initialises the global
 * variables and they are never modified thereafter,
 * it is sufficient to call `json_release(json_null());` before
 * spawning any threads to still have thread safety in the
 * non-reentrant use case.
 */
#ifndef JSON_REENTRANT
#  define JSON_REENTRANT 1
#endif

/*
 * If you have a lot of duplicate strings in the JSON, then
 * interning those strings may save memory and eliminate
 * unnecessary allocations, though interning itself has some
 * overheads.
 *
 * With string interning enabled, parsing this JSON example
 *  `[{"x":1,"y":2},{"x":3,"y":4},...]`
 * will cause only a single `"x"` and a single `"y"` string
 * object to be allocated.
 */
#ifndef JSON_INTERN_STRINGS
#  define JSON_INTERN_STRINGS 1
#endif

/*
 * The string interning functionality can use a red-black
 * tree to keep the binary tree where the strings are stored
 * for lookup balanced. A balanced tree may speed up lookup of
 * strings a bit, but keeping the tree balanced introduces
 * some other overheads.
 */
#ifndef JSON_USE_RED_BLACK
#  define JSON_USE_RED_BLACK 1
#endif

/*
 * Per section 6 of [rfc4627][]:
 * _"Numeric values that cannot be represented in the grammar below
 * (such as Infinity and NaN) are not permitted"_
 *
 * If `JSON_BAD_NUMBERS_AS_STRINGS` is zero then NaN, Infinity and
 * -Infinity will be emitted as `null` by the serializer. This seems
 * to correspond to how other JSON libraries handle the situation
 * and is the default.
 *
 * If `JSON_BAD_NUMBERS_AS_STRINGS` is non-zero then NaN, Infinity and
 * -Infinity will be emitted as the string values "NaN", "Infinity"
 * and "-Infinity" respectively by the serializer.
 */
#ifndef JSON_BAD_NUMBERS_AS_STRINGS
#  define JSON_BAD_NUMBERS_AS_STRINGS 0
#endif

/* =========================================================== */

struct json {
	JSON_Type type;
	union {
        double number;
        char *string;
        HashTable *object;
        Array *array;
    } value;

    size_t refcount;
};

/* =========================================================== */

#if JSON_INTERN_STRINGS
typedef struct internTreeNode ITNode;
typedef struct internTreeNodes TreeNodes;
static char *str_intern(TreeNodes *nodes, int *n, const char *str);
static char *str_make(const char *str);
static char *str_retain(char *str);
static void str_release(char *str);
#else
#  define str_make(str) strdup(str)
#  define str_release(str) free(str)
#endif

static char *_json_readfile(const char *fname);
static int _json_error(const char *fmt, ...);

int (*json_error)(const char *fmt, ...) = _json_error;
char *(*json_readfile)(const char *fname) = _json_readfile;

/* =============================================================
  Hash Table
============================================================= */

#define HASH_SIZE   8

typedef struct HashElement {
    char *name;
    JSON *value;
} HashElement;

struct HashTable {
    unsigned int allocated, count;
    HashElement *table;
};

static HashTable *ht_create() {
    HashTable *ht = malloc(sizeof *ht);
    if(!ht)
        return NULL;
    ht->allocated = HASH_SIZE;
    ht->table = calloc(ht->allocated, sizeof *ht->table);
    if(!ht->table) {
        free(ht);
        return NULL;
    }
    ht->count = 0;
    return ht;
}

static void ht_destroy(HashTable *ht) {
    int i;
    for(i = 0; i < ht->allocated; i++) {
        if(ht->table[i].name) {
            HashElement* v = &ht->table[i];
            str_release(v->name);
            json_release(v->value);
        }
    }
    free(ht->table);
    free(ht);
}

/*
FNV-1a hash, 32-bit version
https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
*/
static unsigned int hash(const char *s) {
    unsigned int h = 0x811c9dc5;
    for(;s[0];s++) {
        h ^= (unsigned char)s[0];
        h *= 0x01000193;
    }
    return h;
}

static HashElement *find_entry(HashElement *elements, unsigned int mask, const char *name) {
    unsigned int h = hash(name) & mask;
    for(;;) {
        if(!elements[h].name || !strcmp(elements[h].name, name))
            return &elements[h];
        h = (h + 1) & mask;
    }
    return NULL;
}

static JSON *ht_put(HashTable *ht, char *name, JSON *j) {
    assert(ht);

    HashElement *f = find_entry(ht->table, ht->allocated - 1, name);
    if(f->name) {
        /* Replacing an existing entry */
        str_release(f->name);
        json_release(f->value);
    } else {
        /* new entry */
        if(ht->count >= ht->allocated * 3 / 4) {
            /* grow the table */
            int new_size = (ht->allocated) << 1;

            HashElement *new_table = calloc(new_size, sizeof *new_table);
            if(!new_table)
                return NULL;
            unsigned int i;
            for(i = 0; i <= ht->allocated - 1; i++) {
                HashElement *from = &ht->table[i];
                if(from->name) {
                    HashElement *to = find_entry(new_table, new_size-1, from->name);
                    to->name = from->name;
                    to->value = from->value;
                }
            }
            free(ht->table);

            ht->table = new_table;
            ht->allocated = new_size;

            f = find_entry(ht->table, ht->allocated - 1, name);
        }
        ht->count++;
    }
    f->name = name;
    f->value = j;

    return j;
}

static JSON *ht_get(HashTable *ht, const char *name) {
    HashElement *v = find_entry(ht->table, ht->allocated - 1, name);
    if(v->name)
        return v->value;
    return NULL;
}

static const char *ht_next(HashTable *ht, const char *name) {
    unsigned int h = 0;
    assert(ht->count < ht->allocated);
    if(name) {
        int oh;
        oh = hash(name) & (ht->allocated - 1);
        h = oh;
        while(strcmp(ht->table[h].name, name)) {
            if(!ht->table[h].name)
                return NULL;
            h = (h + 1) & (ht->allocated - 1);
        }
        if(++h >= ht->allocated)
            return NULL;
    }
    while(!ht->table[h].name) {
        if(++h >= ht->allocated)
            return NULL;
    }
    return ht->table[h].name;
}

/* =============================================================
  Arrays
============================================================= */

#define ARRAY_INITIAL_SIZE  8

struct Array {
    JSON **elements;
    size_t n, a;
};

static Array *ar_create() {
    Array *a = malloc(sizeof *a);
    if(!a)
        return NULL;
    a->n = 0;
    a->a = ARRAY_INITIAL_SIZE;
    a->elements = calloc(ARRAY_INITIAL_SIZE, sizeof *a->elements);
    if(!a->elements) {
        free(a);
        return NULL;
    }
    return a;
}

static void ar_destroy(Array *a) {
    int i;
    for(i = 0; i < a->n; i++)
        json_release(a->elements[i]);
    free(a->elements);
    free(a);
}

static JSON *ar_append(Array *a, JSON *v) {
    if(a->n == a->a) {
        a->a += a->a >> 1;
        JSON **old = a->elements;
        a->elements = realloc(a->elements, a->a * sizeof *a->elements);
        if(!a->elements) {
            a->elements = old;
            return NULL;
        }
    }
    a->elements[a->n++] = v;
    return v;
}

/* =============================================================
  String Interning

`str_intern()` returns a reference counted copy of its parameter. It looks up
the string in the collection of strings it has seen before. If it hasn't seen
the string before, it creates a copy with a reference count initialised to 1
and adds it to the collection, otherwise it increments the reference count of
the string in the collection and returns it.

The collection the strings are kept in is a Red-Black tree. The implementation
is just a straight forward adaption of the one in the [Wikipedia article][red-black].

The first parameter to `str_intern()` is a pointer to an `ITNode*` that forms
the root of this tree.

The reference counting works by storing an `unsigned int` in the bytes before
the `char*` returned by `str_intern()`. The `char*` can therefore be used like
a regular null-terminated C string.

Call `str_release()` to decrement the reference count. If the reference count
drops to zero, the memory is freed.

TODO: You might want to look into an [AA-Tree][aa-tree], but I'm not doing
that right now.

[red-black]: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Insertion
[aa-tree]: https://en.wikipedia.org/wiki/AA_tree

============================================================= */

#if JSON_INTERN_STRINGS

static char *str_make(const char *str) {
    size_t len = strlen(str);
    unsigned int *rc = malloc((sizeof *rc) + len + 1);
    char *data = (char*)rc + sizeof *rc;
    *rc = 1;
    memcpy(data, str, len);
    data[len] = '\0';
    return data;
}

static void str_release(char *str) {
    unsigned int *rc = (unsigned int *)(str - sizeof *rc);
    if(--(*rc) == 0)
        free(rc);
}

static char *str_retain(char *str) {
    unsigned int *rc = (unsigned int *)(str - sizeof *rc);
    (*rc)++;
    return str;
}

#define BLACK 0
#define RED   1

struct internTreeNode {
    char *value;

#if JSON_USE_RED_BLACK
    int color;
    int parent;
#endif
    int left, right;
};

struct internTreeNodes {
    int a, n;
    ITNode *array;
};

static int make_intern_node(TreeNodes *nodes, int parent, const char *str) {

    if(nodes->n == nodes->a) {
        nodes->a = nodes->a << 1;
        ITNode *old = nodes->array;
        nodes->array = realloc(nodes->array, nodes->a * sizeof *nodes->array);
        if(!nodes->array) {
            nodes->array = old;
            return -1;
        }
    }

    int n = nodes->n++;
    ITNode *node = &nodes->array[n];

    node->value = str_make(str);

#if JSON_USE_RED_BLACK
    node->parent = parent;
    node->color = RED;
#endif

    node->left = -1;
    node->right = -1;
    return n;
}

static int do_str_intern(TreeNodes *nodes, int n, const char *str) {
    assert(n >= 0 && n < nodes->n);
    ITNode *array = nodes->array;

    int i = strcmp(array[n].value, str);
    if(i == 0) {
        str_retain(array[n].value);
        return n;
    } else if(i > 0) {
        int l = array[n].left;
        if(l >= 0) {
            return do_str_intern(nodes, l, str);
        } else {
            l = make_intern_node(nodes, n, str);
            return (nodes->array[n].left = l);
        }
    } else { /* i < 0 */
        int r = array[n].right;
        if(r >= 0) {
            return do_str_intern(nodes, r, str);
        } else {
            r = make_intern_node(nodes, n, str);
            return (nodes->array[n].right = r);
        }
    }
}

#if JSON_USE_RED_BLACK
static void repair_tree(TreeNodes *nodes, int n);
#endif

static char *str_intern(TreeNodes *nodes, int *rootIndex, const char *str) {
    int n;
    if(*rootIndex < 0) {
        n = make_intern_node(nodes, -1, str);
    } else {
        n = do_str_intern(nodes, *rootIndex, str);
    }
    if(n < 0)
        return NULL;

#if JSON_USE_RED_BLACK
    repair_tree(nodes, n);
    int i = n;
    while(nodes->array[i].parent >= 0) {
        assert(i < nodes->n && i >= 0);
        i = nodes->array[i].parent;
    }
    *rootIndex = i;
#else
    if(*rootIndex < 0)
        *rootIndex = n;
#endif

    return nodes->array[n].value;
}

#if JSON_USE_RED_BLACK
/* Red-Black Tree */
static int uncle(TreeNodes *nodes, int n) {
    assert(n >= 0 && n < nodes->n);
    ITNode *array = nodes->array;
    int p = array[n].parent;
    if(p >= 0) {
        int g = array[p].parent;
        if(g >= 0)
            return p == array[g].left ? array[g].right : array[g].left;
    }
    return -1;
}

static void rotate_left(TreeNodes *nodes, int n) {
    assert(n >= 0 && n < nodes->n);
    ITNode *array = nodes->array;
    int nnew = array[n].right;
    int p = array[n].parent;
    assert(nnew >= 0);

    array[n].right = array[nnew].left;
    array[nnew].left = n;
    array[n].parent = nnew;
    if(array[n].right >= 0) {
        int r = array[n].right;
        array[r].parent = n;
    }

    if(p >= 0) {
        if(n == array[p].left)
            array[p].left = nnew;
        else
            array[p].right = nnew;
    }
    array[nnew].parent = p;
}

static void rotate_right(TreeNodes *nodes, int n) {
    assert(n >= 0 && n < nodes->n);
    ITNode *array = nodes->array;
    int nnew = array[n].left;
    int p = array[n].parent;
    assert(nnew >= 0);

    array[n].left = array[nnew].right;
    array[nnew].right = n;
    array[n].parent = nnew;
    if(array[n].left >= 0) {
        int l = array[n].left;
        array[l].parent = n;
    }

    if(p >= 0) {
        if(n == nodes->array[p].left)
            array[p].left = nnew;
        else
            array[p].right = nnew;
    }
    array[nnew].parent = p;
}

static void repair_tree(TreeNodes *nodes, int n) {
    assert(n >= 0 && n < nodes->n);
    ITNode *array = nodes->array;

    int p = array[n].parent;
    if(p < 0) {
        /* case 1 */
        array[n].color = BLACK;
    } else if(array[p].color == BLACK) {
        /* case 2 */
        return;
    } else {
        int u = uncle(nodes, n);
        int g = array[p].parent;
        assert(g >= 0);

        if(u >= 0 && array[u].color == RED) {
            /* case 3 */
            array[p].color = BLACK;
            array[u].color = BLACK;
            array[g].color = RED;
            repair_tree(nodes, g);
        } else {
            /* case 4 */
            if(n == array[p].right && p == array[g].left) {
                rotate_left(nodes, p);
                n = array[n].left;
                p = array[n].parent;
                g = array[p].parent;
            } else if(n == array[p].left && p == array[g].right) {
                rotate_right(nodes, p);
                n = array[n].right;
                p = array[n].parent;
                g = array[p].parent;
            }
            /* case 4 step 2 */
            assert(p >= 0 && g >= 0);
            if(n == array[p].left)
                rotate_right(nodes, g);
            else
                rotate_left(nodes, g);
            array[p].color = BLACK;
            array[g].color = RED;
        }
    }
}
#endif /* JSON_USE_RED_BLACK */

#endif /* JSON_INTERN_STRINGS */

/* =============================================================
  Character buffer
============================================================= */

typedef struct {
    size_t n, a;
    char *buffer;
} Emitter;

static int emit(Emitter *e, char c) {
    if(e->n + 1 == e->a) {
       assert(e->a > 1);
       e->a += e->a >> 1;
       char *old = e->buffer;
       e->buffer = realloc(e->buffer, e->a);
       if(!e->buffer) {
           e->buffer = old;
           return 0;
       }
    }
    e->buffer[e->n++] = c;
    e->buffer[e->n] = '\0';

    assert(e->n < e->a);
    return 1;
}

static int emit_text(Emitter *e, const char *t) {
    size_t len = strlen(t);
    if(e->a < e->n + len + 1) {
        assert(e->a > 1);
        while(e->a < e->n + len + 1)
            e->a += e->a >> 1;
        char *old = e->buffer;
        e->buffer = realloc(e->buffer, e->a);
        if(!e->buffer) {
            e->buffer = old;
            return 0;
        }
    }
    memcpy(e->buffer + e->n, t, len);
    e->n += len;
    e->buffer[e->n] = '\0';
    assert(e->n < e->a);
    return 1;
}

static int init_emitter(Emitter *e, size_t initial_size) {

    e->a = initial_size;
    e->n = 0;
    e->buffer = malloc(e->a);
    if(!e->buffer) {
        json_error("out of memory");
        return 0;
    }
    e->buffer[0] = '\0';

    return 1;
}

/* =============================================================
  Lexical Analyzer
============================================================= */

#define P_ERROR     -1
#define P_END       0
#define P_NUMBER    1
#define P_STRING    2
#define P_NULL      3
#define P_TRUE      4
#define P_FALSE     5
#define P_BOM       99

typedef struct  {
    const char *in;
	int sym;
	int lineno;

	Emitter e;
#if JSON_INTERN_STRINGS
    TreeNodes internNodes;
    int rootIndex;
#endif

} ParserContext;

static int getsym(ParserContext *pc);

static int init_parser(ParserContext *pc, const char *text) {
    pc->in = text;
    pc->sym = 0;
    pc->lineno = 1;

    if(!init_emitter(&pc->e, 32))
        return 0;

#if JSON_INTERN_STRINGS
    pc->internNodes.a = 32;
    pc->internNodes.array = malloc(pc->internNodes.a * sizeof *pc->internNodes.array);
    if(!pc->internNodes.array) {
        json_error("out of memory");
        free(pc->e.buffer);
        return 0;
    }
    pc->internNodes.n = 0;
    pc->rootIndex = -1;
#endif

    /* load the first symbol */
    if(getsym(pc) == P_ERROR) {
        json_error("line %d: %s", pc->lineno, pc->e.buffer);
        free(pc->e.buffer);
        return 0;
    }

    return 1;
}

static void start_text(ParserContext *pc) {
    pc->e.n = 0;
    pc->e.buffer[0] = '\0';
}

static int append_char(ParserContext *pc, char c) {
    return emit(&pc->e, c);
}

static void set_textf(ParserContext *pc, const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
    char buffer[64];
	vsnprintf(buffer, sizeof buffer, fmt, arg);
	va_end(arg);
    pc->e.n = 0;
    pc->e.buffer[0] = '\0';
    emit_text(&pc->e, buffer);
}

static void destroy_parser(ParserContext *pc) {
#if JSON_INTERN_STRINGS
    if(pc->internNodes.array)
        free(pc->internNodes.array);
#endif
    if(pc->e.buffer)
        free(pc->e.buffer);
}

static void codepoint_to_utf8(ParserContext *pc, uint32_t cp) {
    if(cp <= 0x7F) {
        append_char(pc, cp);
    } else if(cp <= 0x07FF) {
        append_char(pc, 0xC0 | (cp >> 6));
        append_char(pc, 0x80 | (cp & 0x3F));
    } else if(cp <= 0xFFFF) {
        append_char(pc, 0xE0 | (cp >> 12));
        append_char(pc, 0x80 | ((cp >> 6) & 0x3F));
        append_char(pc, 0x80 | (cp & 0x3F));
    } else if(cp <= 0x10FFFF) {
        append_char(pc, 0xF0 | (cp >> 18));
        append_char(pc, 0x80 | ((cp >> 12) & 0x3F));
        append_char(pc, 0x80 | ((cp >> 6) & 0x3F));
        append_char(pc, 0x80 | (cp & 0x3F));
    } else {
        append_char(pc, '?');
    }
}

static int check_4_hex_digits(ParserContext *pc) {
    return isxdigit(pc->in[0]) && isxdigit(pc->in[1]) && isxdigit(pc->in[2]) && isxdigit(pc->in[3]);
}

static uint32_t read_4_hex_digits(ParserContext *pc) {
    uint32_t u = 0, i;
    for(i = 0; i < 4; i++) {
        if(pc->in[0] <= '9')
            u = (u << 4) + pc->in[0] - '0';
        else
            u = (u << 4) + tolower(pc->in[0]) - 'a' + 0x0A;
        pc->in++;
    }
    return u;
}

static int getsym(ParserContext *pc) {

#if JSON_COMMENTS
start:
#endif

    /* if(pc->sym == P_ERROR) return P_ERROR; */

    if(pc->in[0] == '\0') {
        return (pc->sym = P_END);
    }
	while(isspace(pc->in[0])) {
		if(pc->in[0] == '\n')
			pc->lineno++;
		pc->in++;
	}

#if JSON_COMMENTS
	if(pc->in[0] == '/' && pc->in[1] == '/') {
		pc->in += 2;
        while(pc->in[0] != '\n' && pc->in[0] != '\0')
			pc->in++;
		goto start;
	} else if(pc->in[0] == '/' && pc->in[1] == '*') {
        pc->in += 2;
        while(pc->in[0] != '*' && pc->in[1] != '/') {
            if(pc->in[0] == '\0') {
                set_textf(pc, "unexpected end of file");
				return (pc->sym = P_ERROR);
            } else if(pc->in[0] == '\n')
                pc->lineno++;
			pc->in++;
        }
        pc->in+=2;
		goto start;
    }
#else
    if(pc->in[0] == '/' && (pc->in[1] == '/' || pc->in[1] == '*')) {
        set_textf(pc, "comments are not supported");
        return (pc->sym = P_ERROR);
	}
#endif

    start_text(pc);

    if(isalpha(pc->in[0])) {
		while(isalpha(pc->in[0]))
			append_char(pc, *(pc->in++));
        if(!strcmp(pc->e.buffer, "null"))
            return (pc->sym = P_NULL);
        else if(!strcmp(pc->e.buffer, "true"))
            return (pc->sym = P_TRUE);
        else if(!strcmp(pc->e.buffer, "false"))
            return (pc->sym = P_FALSE);

        set_textf(pc, "unknown keyword '%s'", pc->e.buffer);
        return (pc->sym = P_ERROR);

	} else if(isdigit(pc->in[0]) || pc->in[0] == '-') {
		pc->sym = P_NUMBER;
        if(pc->in[0] == '-')
			append_char(pc, *(pc->in++));
		while(isdigit(pc->in[0]))
			append_char(pc, *(pc->in++));
		if(pc->in[0] == '.') {
			append_char(pc, *(pc->in++));
			while(isdigit(pc->in[0]))
				append_char(pc, *(pc->in++));
		}
		if(tolower(pc->in[0]) == 'e') {
			append_char(pc, *(pc->in++));
			if(strchr("+-", pc->in[0]))
				append_char(pc, *(pc->in++));
			while(isdigit(pc->in[0]))
				append_char(pc, *(pc->in++));
		}
        return (pc->sym = P_NUMBER);
	} else if(pc->in[0] == '"') {
		pc->in++;
		while(pc->in[0] != '"') {
			switch(pc->in[0]) {
				case '\0' :
				case '\n' : {
						set_textf(pc, "unterminated string literal");
						return (pc->sym = P_ERROR);
					}
				case '\\' : {
						pc->in++;
						switch(pc->in[0]) {
							case '\0' : {
									set_textf(pc, "unterminated string literal");
									return (pc->sym = P_ERROR);
								}
							case '"' : append_char(pc, '"'); pc->in++; break;
							case '\\' : append_char(pc, '\\'); pc->in++; break;
							case '/' : append_char(pc, '/'); pc->in++; break;
							case 'b' : append_char(pc, '\b'); pc->in++; break;
							case 'f' : append_char(pc, '\f'); pc->in++; break;
							case 'n' : append_char(pc, '\n'); pc->in++; break;
							case 'r' : append_char(pc, '\r'); pc->in++; break;
							case 't' : append_char(pc, '\t'); pc->in++; break;
                            case 'u' : {
                                pc->in++;
                                if(check_4_hex_digits(pc)) {
                                    uint32_t u = read_4_hex_digits(pc);
                                    if(u >= 0xD800 && u <= 0xDFFF) {
                                        /* surrogate pair */
                                        if(pc->in[0] != '\\' && pc->in[1]!='u') {
                                            set_textf(pc, "expected a surrogate pair with \\u%04X", u);
									        return (pc->sym = P_ERROR);
                                        }
                                        pc->in += 2;
                                        if(!check_4_hex_digits(pc)) {
                                            set_textf(pc, "bad '\\uXXXX' sequence");
                                            return (pc->sym = P_ERROR);
                                        }
                                        uint32_t hs = u, ls = read_4_hex_digits(pc);
                                        u = (hs - 0xD800) * 0x400 + (ls - 0xDC00) + 0x10000;
                                    }
                                    codepoint_to_utf8(pc, u);
                                } else {
                                    set_textf(pc, "bad '\\uXXXX' sequence");
									return (pc->sym = P_ERROR);
                                }
                            } break;
							default: {
									set_textf(pc, "bad escape sequence '\\%c'", pc->in[0]);
									return (pc->sym = P_ERROR);
								}
						}
					} break;
				default : {
						append_char(pc, *(pc->in++));
					} break;
			}
		}
		pc->in++;
        return (pc->sym = P_STRING);
	} else if(strchr("{}[]:,", pc->in[0])) {
		return (pc->sym = *(pc->in++));
	} else {
		set_textf(pc, "Unexpected token '%c'", pc->in[0]);
	}
    return (pc->sym = P_ERROR);
}

/* =============================================================
  Parser
============================================================= */

JSON *json_retain(JSON *j) {
    j->refcount++;
    return j;
}

void json_release(JSON *j) {
    if(!j)
        return;
    if(--j->refcount == 0) {
        switch(j->type) {
            case j_string: str_release(j->value.string); break;
            case j_object: ht_destroy(j->value.object); break;
            case j_array : ar_destroy(j->value.array); break;
            default: break;
        }
        free(j);
    }
}

static int accept(ParserContext *pc, int sym) {
	if(pc->sym == sym) {
		getsym(pc);
	    if(pc->sym == P_ERROR) {
            json_error("line %d: %s", pc->lineno, pc->e.buffer);
            return 0;
    	}
        return 1;
	}
	return 0;
}

static JSON *json_parse_object(ParserContext *pc);
static JSON *json_parse_array(ParserContext *pc);
static JSON *json_parse_value(ParserContext *pc);

static JSON *json_parse_object(ParserContext *pc) {

	JSON *v = json_new_object();
    if(!v) {
        json_error("out of memory");
        return NULL;
    }

    accept(pc, '{');
	if(pc->sym != '}') {
		do {
			char *key;
			JSON *value;
			if(pc->sym != P_STRING) {
                json_error("line %d: string expected", pc->lineno);
                goto error;
            }
#if JSON_INTERN_STRINGS
            key = str_intern(&pc->internNodes, &pc->rootIndex, pc->e.buffer);
#else
            key = str_make(pc->e.buffer);
#endif
            getsym(pc);
            if(pc->sym == P_ERROR) {
                json_error("line %d: %s", pc->lineno, pc->e.buffer);
                str_release(key);
                goto error;
            }

			if(!accept(pc, ':')) {
                if(pc->sym != P_ERROR)
                    json_error("line %d: ':' expected", pc->lineno);
				str_release(key);
                goto error;
            }

			value = json_parse_value(pc);
			if(!value) {
				str_release(key);
                goto error;
			}

			ht_put (v->value.object, key, value);

		} while(accept(pc, ','));
	}

	if(!accept(pc, '}')) {
		json_error("line %d: '}' expected", pc->lineno);
		goto error;
	}

	return v;

error:
    ht_destroy(v->value.object);
    free(v);
    return NULL;
}

static JSON *json_parse_array(ParserContext *pc) {

    JSON *v = json_new_array();
    if(!v) {
        json_error("out of memory");
        return NULL;
    }

	accept(pc, '[');
	if(pc->sym != ']') {
		do {
			JSON *value = json_parse_value(pc);
			if(!value)
                goto error;
            ar_append(v->value.array, value);
		} while(accept(pc, ','));
	}
	if(!accept(pc, ']')) {
		json_error("line %d: ']' expected", pc->lineno);
		goto error;
	}

	return v;

error:
    ar_destroy(v->value.array);
    free(v);
    return NULL;
}

static JSON *json_parse_value(ParserContext *pc) {
	if(pc->sym == '{')
		return json_parse_object(pc);
	else if(pc->sym == '[')
		return json_parse_array(pc);
	else {
        JSON *v = NULL;
		if(pc->sym == P_NUMBER) {
			v = json_new_number(atof(pc->e.buffer));
		} else if(pc->sym == P_STRING) {
            v = malloc(sizeof *v);
            if(v) {
                v->refcount = 1;
                v->type = j_string;
#if JSON_INTERN_STRINGS
                v->value.string = str_intern(&pc->internNodes, &pc->rootIndex, pc->e.buffer);
#else
                v->value.string = str_make(pc->e.buffer);
#endif
            }
		} else if(pc->sym == P_TRUE) {
			v = json_true();
		} else if(pc->sym == P_FALSE) {
			v = json_false();
		}else if(pc->sym == P_NULL) {
			v = json_null();
		} else {
			json_error("line %d: %s", pc->lineno, pc->e.buffer);
            return NULL;
		}
        if(!v) {
            json_error("out of memory");
            return NULL;
        }
		getsym(pc);
        if(pc->sym == P_ERROR) {
            json_release(v);
            return NULL;
    	}
		return v;
	}
}

JSON *json_parse(const char *text) {
    ParserContext pc;

    /* Skip a BOM, if present */
    if(!memcmp("\xEF\xBB\xBF",text,3))
        text += 3;

    if(!init_parser(&pc, text)) {
        return NULL;
    }

    JSON *j = json_parse_value(&pc);

    destroy_parser(&pc);
    return j;
}

/* =============================================================
  Utility Functions
============================================================= */

/* Default error handler just prints to `stderr` */
static int _json_error(const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	fputs("error: ", stderr);
	int res = vfprintf(stderr, fmt, arg);
	fputc('\n', stderr);
	va_end(arg);
    fflush(stderr);
    return res;
}

/* Reads an entire file into a dynamically allocated memory buffer.
 * The returned buffer needs to be free()d afterwards
 */
static char *_json_readfile(const char *fname) {
	FILE *f;
	long len,r;
	char *str;

	if(!(f = fopen(fname, "rb")))
		return NULL;

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);

	if(!(str = malloc(len+2)))
		return NULL;
	r = fread(str, 1, len, f);

	if(r != len) {
		free(str);
		return NULL;
	}

	fclose(f);
	str[len] = '\0';
	return str;
}

JSON *json_read(const char *filename) {
	JSON *v = NULL;
	char *text = json_readfile(filename);
	if(!text) {
        json_error("unable to read %s", filename);
		return NULL;
    }
	v = json_parse(text);
	free(text);
	return v;
}

#define EMIT(e,c) do{if(!emit(e,c)) return 0;}while(0);

static int serialize_string(Emitter *e, const char *str) {
    assert(str);
    EMIT(e,'"');
    while(*str) {
        if(strchr("\"\\/\b\f\n\r\t",*str)) {
            EMIT(e, '\\');
            switch(*str) {
                case '\"': EMIT(e, '"'); break;
                case '\\': EMIT(e, '\\'); break;
                case '/': EMIT(e, '/'); break;
                case '\b': EMIT(e, 'b'); break;
                case '\f': EMIT(e, 'f'); break;
                case '\n': EMIT(e, 'n'); break;
                case '\r': EMIT(e, 'r'); break;
                case '\t': EMIT(e, 't'); break;
            }
            /* Note: \uXXXX cases for unicode characters aren't implemented */
        }
        else
            EMIT(e, *str);
        str++;
    }
    EMIT(e,'"');
    return 1;
}

static int serialize_value(Emitter *e, JSON *j, int pretty, int indent) {
    char buffer[32];
    int x;
    if(!j) {
        emit_text(e, "null");
        return 1;
    }

    switch(j->type) {
		case j_string: {
			if(!serialize_string(e, j->value.string))
                return 0;
		} break;
		case j_number:
#if defined(isnan) && defined(INFINITY)
            if(isnan(j->value.number) || j->value.number == INFINITY || j->value.number == -INFINITY) {
#if JSON_BAD_NUMBERS_AS_STRINGS
                if(isnan(j->value.number)) emit_text(e, "\"NaN\"");
                else if(j->value.number == INFINITY) emit_text(e, "\"Infinity\"");
                else if(j->value.number == -INFINITY) emit_text(e, "\"-Infinity\"");
#else
                emit_text(e, "null");
#endif
                return 1;
            }
#endif
            snprintf(buffer, sizeof buffer, "%g", j->value.number);
            if(!emit_text(e, buffer))
                return 0;
            break;
		case j_object: {
			HashTable *h = j->value.object;
			const char *key = ht_next (h, NULL);
			EMIT(e, '{');
            if(key) {
                if(pretty) EMIT(e, '\n');
                while(key) {
                    if(pretty) for(x=0;x<indent*2;x++) EMIT(e, ' ');

                    if(!serialize_string(e, key))
                        return 0;
                    if(pretty) EMIT(e, ' ');
                    EMIT(e, ':');
                    if(pretty) EMIT(e, ' ');

                    if(!serialize_value(e, ht_get(h, key), pretty, indent+1))
                        return 0;
                    key = ht_next(h, key);
                    if(key)
                        EMIT(e, ',');
                    if(pretty) EMIT(e, '\n');
                }
                if(pretty) for(x=0;x<(indent - 1)*2;x++) EMIT(e, ' ');
            }
			EMIT(e, '}');
		} break;
		case j_array: {
			int i;
            Array *a = j->value.array;
			EMIT(e, '[');
            if(a->n) {
                if(pretty) EMIT(e, '\n');
                for(i = 0; i < a->n; i++) {

                    if(pretty) for(x=0;x<indent*2;x++) EMIT(e, ' ');

                    if(!serialize_value(e, a->elements[i], pretty, indent+1))
                        return 0;
                    if(i < a->n - 1)
                        EMIT(e, ',');

                    if(pretty) EMIT(e, '\n');
                }
                if(pretty) for(x=0;x<(indent - 1)*2;x++) EMIT(e, ' ');
            }
			EMIT(e, ']');
		} break;
		case j_true: {
			if(!emit_text(e, "true"))
                return 0;
		} break;
		case j_false: {
			if(!emit_text(e, "false"))
                return 0;
		} break;
		case j_null: {
			if(!emit_text(e, "null"))
                return 0;
		} break;
		default: break;
	}
    return 1;
}

char *json_serialize(JSON *j) {
    Emitter e;
    if(!init_emitter(&e, 256))
        return NULL;
    if(!serialize_value(&e, j, 0, 1)) {
        free(e.buffer);
        return NULL;
    }
    return e.buffer;
}

char *json_pretty(JSON *j) {
    Emitter e;
    if(!init_emitter(&e, 256))
        return NULL;
    if(!serialize_value(&e, j, 1, 1)) {
        free(e.buffer);
        return NULL;
    }
    return e.buffer;
}

/* =============================================================
  Accessors
============================================================= */

static JSON *new_value(JSON_Type type) {
    JSON *j = malloc(sizeof *j);
    if(!j)
        return NULL;
    j->type = type;
    j->refcount = 1;
    return j;
}

JSON *json_new_object() {
    JSON *j = new_value(j_object);
    if(!j)
        return NULL;
    j->value.object = ht_create();
    if(!j->value.object) {
        free(j);
        return NULL;
    }
    return j;
}

JSON *json_new_array() {
    JSON *j = new_value(j_array);
    if(!j)
        return NULL;
    j->value.array = ar_create();
    if(!j->value.array) {
        free(j);
        return NULL;
    }
    return j;
}

JSON *json_new_string(const char *text) {
    if(!text)
        return json_null();
    JSON *v = new_value(j_string);
    if(!v)
        return NULL;
    v->value.string = str_make(text);
    return v;
}

JSON *json_new_number(double n) {
    JSON *v = new_value(j_number);
    if(!v)
        return NULL;
    v->type = j_number;
    v->value.number = n;
    return v;
}

#if !JSON_REENTRANT
static JSON *G_null = NULL, *G_true = NULL, *G_false = NULL;
static void free_globals() {
    json_release(G_null);
    json_release(G_true);
    json_release(G_false);
}
static void init_globals() {
    G_null = new_value(j_null);
    G_true = new_value(j_true);
    G_false = new_value(j_false);
    if(!G_null || !G_true || !G_false)
        json_error("unable to allocate globals"); /* Nothing more we can do :( */
    atexit(free_globals);
}
#endif

JSON *json_null() {
#if JSON_REENTRANT
    return new_value(j_null);
#else
    if(!G_null)
        init_globals();
    return json_retain(G_null);
#endif
}

JSON *json_true() {
#if JSON_REENTRANT
    return new_value(j_true);
#else
    if(!G_true)
        init_globals();
    return json_retain(G_true);
#endif
}

JSON *json_false() {
#if JSON_REENTRANT
    return new_value(j_false);
#else
    if(!G_false)
        init_globals();
    return json_retain(G_false);
#endif
}

JSON *json_boolean(int value) {
    if(value)
        return json_true();
    else
        return json_false();
}

JSON_Type json_get_type(JSON *j) {
	return j->type;
}

int json_is_null(JSON *j) {
	return j->type == j_null;
}

int json_is_boolean(JSON *j) {
	return j->type == j_true || j->type == j_false;
}

int json_is_true(JSON *j) {
	return j->type == j_true;
}

int json_is_false(JSON *j) {
	return j->type == j_false;
}

int json_is_truthy(JSON *j) {
    return !json_is_falsey(j);
}

int json_is_falsey(JSON *j) {
    if(!j || j->type == j_false || j->type == j_null)
        return 1;
    if(j->type == j_string && strlen(j->value.string) == 0)
        return 1;
#ifdef isnan
    /* Technically, NaN is not part of JSON, but I included it here for
    completeness */
    if(j->type == j_number && (j->value.number == 0 || isnan(j->value.number)))
        return 1;
#else
    if(j->type == j_number && j->value.number == 0)
        return 1;
#endif
    return 0;
}

int json_is_number(JSON *j) {
	return j->type == j_number;
}

int json_is_string(JSON *j) {
	return j->type == j_string;
}

int json_is_object(JSON *j) {
	return j->type == j_object;
}

int json_is_array(JSON *j) {
	return j->type == j_array;
}

double json_as_number(JSON *j) {
	if(j->type == j_number)
		return j->value.number;
	return 0.0;
}

const char *json_as_string(JSON *j) {
	if(j->type == j_string)
		return j->value.string;
	return NULL;
}

int json_obj_has(JSON *j, const char *name) {
	assert(j->type == j_object);
	HashTable *h = j->value.object;
	return ht_get(h, name) != NULL;
}

const char *json_obj_next(JSON *j, const char *name) {
	assert(j->type == j_object);
	HashTable *h = j->value.object;
	return ht_next(h, name);
}

JSON *json_obj_get(JSON *j, const char *name) {
	assert(j->type == j_object);
	HashTable *h = j->value.object;
	return ht_get(h, name);
}

double json_obj_get_number(JSON *j, const char *name) {
	return json_obj_get_number_or(j, name, 0.0);
}

double json_obj_get_number_or(JSON *j, const char *name, double def) {
	assert(j->type == j_object);
	JSON *v = json_obj_get(j, name);
	if(v)
		return json_as_number(v);
	return def;
}

const char *json_obj_get_string(JSON *j, const char *name) {
    return json_obj_get_string_or(j, name, NULL);
}

const char *json_obj_get_string_or(JSON *j, const char *name, const char *def) {
	assert(j->type == j_object);
	JSON *v = json_obj_get(j, name);
	if(v)
		return json_as_string(v);
	return def;
}

int json_obj_get_bool(JSON *j, const char *name) {
	return json_obj_get_bool_or(j, name, 0);
}

int json_obj_get_bool_or(JSON *j, const char *name, int def) {
	assert(j->type == j_object);
	JSON *v = json_obj_get(j, name);
	if(v) {
		return json_is_true(v);
    }
	return def;
}

/*
 * `json_obj_set()` does not call `json_retain()` on `v`
 * on purpose to enable a fluent API.
 *
 * You can call `JSON *j = json_obj_set(j, "key", json_new_string("value"))`
 * without any memory leaks on the `"value"`.
 *
 * If it retained `v` the above would've had to be written as
 *
 *     JSON *v = json_new_string("value");
 *     JSON *j = json_obj_set(j, "key",v);
 *     json_release(v);
 *
 * to prevent memory leaks.
 *
 * This has these side effects for users of this library:
 *
 * * You have to call `json_retain()` yourself if you
 *   need to keep the value.
 * * If `v` was not created through one of the `json_new_*`
 *   functions (or `json_null()`, `json_true()` and `json_false()`)
 *   then you _must_ call `json_retain()` on it before calling
 *   this function.
 */
JSON *json_obj_set(JSON *obj, char *k, JSON *v) {
    assert(obj->type == j_object);
    k = str_make(k);
    if(!v) v = json_null();
    ht_put(obj->value.object, k, v);
    return obj;
}

JSON *json_obj_set_number(JSON *obj, char *k, double n) {
    return json_obj_set(obj, k, json_new_number(n));
}

JSON *json_obj_set_string(JSON *obj, char *k, const char *str) {
    return json_obj_set(obj, k, json_new_string(str));
}

int json_obj_check_type(JSON *j, const char *name, JSON_Type type) {
    j = json_obj_get(j, name);
    return j && j->type == type;
}

unsigned int json_array_len(JSON *j) {
	assert(j->type == j_array);
	return (unsigned int)j->value.array->n;
}

JSON *json_array_get(JSON *array, int n) {
	assert(array->type == j_array);
	if(n < array->value.array->n)
        return array->value.array->elements[n];
	return NULL;
}

double json_array_get_number(JSON *array, int n) {
    JSON *v = json_array_get(array, n);
    if(v)
        return json_as_number(v);
    return 0.0;
}

const char *json_array_get_string(JSON *array, int n) {
    JSON *v = json_array_get(array, n);
    if(v)
        return json_as_string(v);
    return NULL;
}

JSON *json_array_set(JSON *j, int n, JSON *v) {
	assert(j->type == j_array);
	assert(n < j->value.array->n);
    JSON *old = j->value.array->elements[n];
    j->value.array->elements[n] = v;
    json_release(old);
    return j;
}

JSON *json_array_reserve(JSON *j, unsigned int n) {
	assert(j->type == j_array);
    while(j->value.array->n < n)
        ar_append(j->value.array, json_null());
    return j;
}

JSON *json_array_add(JSON *array, JSON *value) {
    assert(array->type == j_array);
    if(!value) value = json_null();
    ar_append(array->value.array, value);
    return array;
}

JSON *json_array_add_number(JSON *array, double number) {
    return json_array_add(array, json_new_number(number));
}

JSON *json_array_add_string(JSON *array, const char *str) {
    return json_array_add(array, json_new_string(str));
}

