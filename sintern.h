#ifndef STR_INTERN_H

/* =============================================================
  String Interning

`si_intern()` returns a reference counted copy of its parameter. It looks up
the string in the collection of strings it has seen before. If it hasn't seen
the string before, it creates a copy with a reference count initialised to 1
and adds it to the collection, otherwise it increments the reference count of
the string in the collection and returns it.

The collection the strings are kept in is a Red-Black tree. The implementation
is just a straight forward adaption of the one in the [Wikipedia article][red-black].

The first parameter to `si_intern()` is a pointer to an `si_node_t*` that forms
the root of this tree.

The reference counting works by storing an `unsigned int` in the bytes before
the `char*` returned by `si_intern()`. The `char*` can therefore be used like
a regular null-terminated C string.

Call `si_release()` to decrement the reference count. If the reference count
drops to zero, the memory is freed.

[red-black]: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree

============================================================= */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct si_node_t si_node_t;

void si_release(char *str);

char *si_retain(char *str);

void si_free_tree(si_node_t *node);

char *si_intern(si_node_t **pool, const char *str);

#ifdef EOF
void si_list(si_node_t *root, FILE *f);
#endif

#if defined(STR_INTERN_IMPLEMENTATION) || defined(STR_INTERN_TEST)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BLACK 0
#define RED   1

struct si_node_t {
    char *value;
    int color;
    struct si_node_t *parent, *left, *right;
};

static char *_si_make_str(const char *str) {
    size_t len = strlen(str);
    unsigned int *rc = malloc((sizeof *rc) + len + 1);
    char *data = (char*)rc + sizeof *rc;
    *rc = 1;
    memcpy(data, str, len);
    data[len] = '\0';
    return data;
}

static si_node_t *_si_make_node(si_node_t *parent, const char *str) {
    si_node_t *node = malloc(sizeof *node);
    if(!node)
        return NULL;

    node->value = _si_make_str(str);

    node->parent = parent;
    node->color = RED;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void si_release(char *str) {
    unsigned int *rc = (unsigned int *)(str - sizeof *rc);
    if(--(*rc) == 0)
        free(rc);
}

char *si_retain(char *str) {
    unsigned int *rc = (unsigned int *)(str - sizeof *rc);
    (*rc)++;
    return str;
}

static si_node_t *_si_do_intern(si_node_t *root, const char *str) {
    int i = strcmp(root->value, str);
    if(i == 0) {
        si_retain(root->value);
        return root;
    } else if(i > 0) {
        if(root->left) {
            return _si_do_intern(root->left, str);
        } else {
            root->left = _si_make_node(root, str);
            return root->left;
        }
    } else { // i < 0
        if(root->right) {
            return _si_do_intern(root->right, str);
        } else {
            root->right = _si_make_node(root, str);
            return root->right;
        }
    }
}

static void _si_repair_tree(si_node_t *n);

char *si_intern(si_node_t **pool, const char *str) {
    si_node_t *n;
    if(!*pool) {
        n = _si_make_node(NULL, str);
    } else {
        n = _si_do_intern(*pool, str);
    }
    _si_repair_tree(n);

    si_node_t *root = n;
    while(root->parent)
        root = root->parent;
    *pool = root;
    return n->value;
}

void si_free_tree(si_node_t *node) {
    if(!node)
        return;
    si_free_tree(node->left);
    si_free_tree(node->right);
    // Note that node->value is not freed on purpose...
    free(node);
}

/* Red-Black Tree */
static si_node_t *_si_uncle(si_node_t *n) {
    si_node_t *p = n->parent;
    if(p && p->parent)
        return p == p->parent->left ? p->parent->right : p->parent->left;
    return NULL;
}

static void _si_rotate_left(si_node_t *n) {
    assert(n);
    si_node_t *nnew = n->right;
    si_node_t *p = n->parent;
    assert(nnew);

    n->right = nnew->left;
    nnew->left = n;
    n->parent = nnew;
    if(n->right)
        n->right->parent = n;
    if(p) {
        if(n == p->left)
            p->left = nnew;
        else
            p->right = nnew;
    }
    nnew->parent = p;
}

static void _si_rotate_right(si_node_t *n) {
    assert(n);
    si_node_t *nnew = n->left;
    si_node_t *p = n->parent;
    assert(nnew);

    n->left = nnew->right;
    nnew->right = n;
    n->parent = nnew;
    if(n->left)
        n->left->parent = n;
    if(p) {
        if(n == p->left)
            p->left = nnew;
        else
            p->right = nnew;
    }
    nnew->parent = p;
}

static void _si_repair_tree(si_node_t *n) {
    si_node_t *p = n->parent;
    if(!p) {
        // case 1
        n->color = BLACK;
    } else if(p->color == BLACK) {
        // case 2
        return;
    } else {
        si_node_t *u = _si_uncle(n);
        si_node_t *g = p->parent;
        assert(g);

        if(u && u->color == RED) {
            // case 3
            p->color = BLACK;
            u->color = BLACK;
            g->color = RED;
            _si_repair_tree(g);
        } else {
            // case 4
            if(n == p->right && p == g->left) {
                _si_rotate_left(p);
                n = n->left;
                p = n->parent;
                g = p->parent;
            } else if(n == p->left && p == g->right) {
                _si_rotate_right(p);
                n = n->right;
                p = n->parent;
                g = p->parent;
            }
            // case 4 step 2
            assert(p && g);
            if(n == p->left)
                _si_rotate_right(g);
            else
                _si_rotate_left(g);
            p->color = BLACK;
            g->color = RED;
        }
    }
}

void si_list(si_node_t *node, FILE *f) {
	if(node->left) 
		si_list(node->left, f);
	fprintf(f, "'%s'\n", node->value);
	if(node->right) 
		si_list(node->right, f);
}

#  endif /* STR_INTERN_IMPLEMENTATION */

#ifdef __cplusplus
} /* extern "C" */
#endif

// ===========================================================

#ifdef STR_INTERN_TEST

static void drawNode(si_node_t *node) {
    if(!node)
        return;
    drawNode(node->left);
    if(node->color == RED)
        printf("  %s [color=red,fontcolor=white,style=filled];\n", node->value);
    else
        printf("  %s [color=black,fontcolor=white,style=filled];\n", node->value);
    drawNode(node->right);
}

static void drawLink(si_node_t *node) {
    if(!node)
        return;
    if(node->left) {
        printf("  %s -> %s;\n", node->value, node->left->value);
        drawLink(node->left);
    }
    if(node->right) {
        printf("  %s -> %s;\n", node->value, node->right->value);
        drawLink(node->right);
    }
}

void drawGraph(si_node_t *root) {
    printf("digraph G {\n");
    drawNode(root);
    drawLink(root);
    printf("}\n");
}

/*
dot -Tjpeg -o x.jpg g.dot
*/

int main(int argc, char *argv[]) {
    si_node_t *root = NULL;

    si_intern(&root, "e");
    si_intern(&root, "l");
    si_intern(&root, "f");
    si_intern(&root, "w");
    si_intern(&root, "z");
    si_intern(&root, "m");
    si_intern(&root, "o");
    si_intern(&root, "c");
    si_intern(&root, "b");
    si_intern(&root, "j");
    si_intern(&root, "h");
    si_intern(&root, "s");
    si_intern(&root, "x");
    si_intern(&root, "n");
    si_intern(&root, "u");
    si_intern(&root, "v");
    si_intern(&root, "g");
    si_intern(&root, "d");
    si_intern(&root, "y");
    si_intern(&root, "i");
    si_intern(&root, "p");
    si_intern(&root, "a");
    si_intern(&root, "t");
    si_intern(&root, "q");
    si_intern(&root, "r");
    si_intern(&root, "k");

    si_intern(&root, "ga");
    si_intern(&root, "zd");
    si_intern(&root, "gh");
    si_intern(&root, "ge");
    si_intern(&root, "gk");
    si_intern(&root, "ze");
    si_intern(&root, "da");
    si_intern(&root, "za");
    si_intern(&root, "zb");
    si_intern(&root, "gz");
    si_intern(&root, "zc");
    si_intern(&root, "aa");
    si_intern(&root, "az");
    si_intern(&root, "ac");
    si_intern(&root, "ab");
    si_intern(&root, "de");
    si_intern(&root, "dg");
    si_intern(&root, "zf");
    si_intern(&root, "ma");
    si_intern(&root, "mb");
    si_intern(&root, "ea");
    si_intern(&root, "eaa");
    si_intern(&root, "eab");
    si_intern(&root, "eaba");
    si_intern(&root, "eabb");


    drawGraph(root);

    si_free_tree(root);

    /*
    Note that this program will leak memory, because
    `si_release()` needs to be called on all the return
    values of `si_intern()` but I didn't bother with
    it here
    */

    return 0;
}
#endif /* STR_INTERN_TEST */

#endif /* STR_INTERN_H */