/**
 * # `list.h`
 * A generic linked-list implementation.
 *
 * It aims to be a set of comprehensive, reentrant and portable 
 * linked-list functions with a simple API.
 * 
 * The reason for this thing's existence is the that I spent about
 * an hour searching for something like this on Google's Codesearch
 * but couldn't find anything suitable.
 *
 * Implementing a linked list is usually a trivial matter, but I 
 * decided that having a generic implementation may be worthwhile
 * for situations where you have bigger fish to fry.
 *
 * The API centers around the `link_list` structure that represents a
 * linked list that stores `void` pointers. It has these features:
 *
 * * Use `list_create()` to create a new linked list structure.
 * * Use `list_destroy()` to destroy a linked list
 * * Use `list_isempty()` and `list_count()` to get information on the list.
 * * Use `list_append()`, `list_insert()` and `list_prepend()` to insert
 *   data into the list.
 * * Use `list_remove_element()` to remove items from the list.
 * * `list_pop_front()`, `list_pop_back()` and `list_pop()` allows
 *   you to use the list for queue and stack like operations.
 * * `list_iterate()` and `list_iterate_reverse()` allows iteration through
 *   the list.
 * * `list_find()` is used to search the list for a specific item. It is 
 *   an O(n) algorithm unfortunately.
 * * `list_remove()` is used to delete specific  It is 
 *   an O(n) algorithm unfortunately.
 *
 * ### License
 *
 *     Author: Werner Stoop
 *     This software is provided under the terms of the unlicense.
 *     See http://unlicense.org/ for more details.
 *
 * ## API
 * ### Definitions
 */
#ifndef LIST_H
#define LIST_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"
{
#endif

/**
 * #### `struct list_element`
 * Structure stored at each node in the linked list.\n
 * It has two pointers `next` and `prev` tha points to the
 * next and previous elements in the list, respectively.\n
 * It has a void pointer `data` that points to the data stored 
 * at the node
 */
typedef struct list_element {
	struct list_element *next, *prev;
	void *data;
} list_element;

/** 
 * #### `struct link_list`
 * Structure containing a linked list.\n
 * Create it with `list_create()` and destroy it with `list_destroy()`.\n
 * It has a pointer `first` that points to the first `list_element`
 * in the list, and a pointer `last` that points to the last
 * `list_element` in the list.
 */
typedef struct link_list {
	list_element *first, *last;
} link_list;

/**
 * #### `typedef void (*list_dtor)(void *data);`
 * Functions used to deallocate the list nodes in `list_destroy()` should
 * match this prototype. 
 * 
 * The data stored at each node in the list will be passed to this function
 * to be deallocated.
 */
typedef void (*list_dtor)(void *data);

/**
 * ### Functions
 * 
 * #### `link_list *list_create()`
 * Creates a linked list structure.
 * 
 * It returns a newly allocated `link_list` structure, or `NULL`
 * if `malloc()` failed.
 * 
 * Deallocate the returned structure with `list_destroy()`
 */
link_list *list_create();

/**
 * #### `int list_isempty(link_list *list)`
 * Returns non-zero if `list` is empty, zero otherwise.
 */
int list_isempty(link_list *list);

/**
 * #### `int list_count(link_list *list)`
 * Returns the number of elements in `list`.
 * 
 * This is an O(n) operation at the moment.
 */
int list_count(link_list *list);

/**
 * #### `void list_destroy(link_list *list, list_dtor dtor)`
 * Destroys the linked list.
 * 
 * The data stored at each node will be passed to the function `dtor`
 * to be deallocated.
 */
void list_destroy(link_list *list, list_dtor dtor);

/**
 * #### `list_element *list_append(link_list *list, void *data)`
 * Adds an element to the end of the list.
 * 
 * It returns a pointer to the inserted element, or `NULL` if
 * either data is `NULL` or `malloc()` failed.
 */
list_element *list_append(link_list *list, void *data);

/**
 * #### `list_element *list_insert(link_list *list, void *data)`
 * Adds an element to the end of the list.
 * 
 * It's defined as a macro around `list_append()`.
 */
#define list_insert(list, data) (list_append((list), (data)))

/**
 * #### `list_element *list_prepend(link_list *list, void *data)`
 * Adds an element to the front of the list.
 * 
 * It returns a pointer to the inserted element, or `NULL` if
 * either data is `NULL` or `malloc()` failed.
 */
list_element *list_prepend(link_list *list, void *data);

/**
 * #### `void *list_remove_element(link_list *list, list_element *el)`
 * Removes the element pointed to by `el` from the `list` and returns
 * its `data` pointer for manual cleanup.
 */
void *list_remove_element(link_list *list, list_element *el);

/**
 * #### `void *list_pop_front(link_list *list)`
 * Removes the first element from the list, and returns its 
 * data pointer.
 * 
 * It may return `NULL` if the list is empty.
 */
void *list_pop_front(link_list *list);

/**
 * #### `void *list_pop_back(link_list *list)`
 * Removes the last element from the list, and returns its 
 * data pointer.
 * 
 * It may return `NULL` if the list is empty.
 */
void *list_pop_back(link_list *list);

/**
 * #### `void *list_pop(link_list *list)`
 * Removes the first element from the list, and returns its 
 * data pointer.
 * 
 * It may return `NULL` if the list is empty.
 * It is defined as a macro around `list_pop_back()`
 */
#define list_pop(list) (list_pop_back((list)))

/**
 * #### `typedef int (*list_iter)(void *data)`
 * Iteration callback function.
 * 
 * The function should return non-zero on success, or 0 if
 * the iteration should stop.
 */
typedef int (*list_iter)(void *data);

/**
 * #### `void list_iterate(link_list *list, list_iter iter)`
 * Iterates through the `list`, calling the function `iter`
 * for each node.
 * 
 * It returns 1 if it ran through all the nodes successfully, or
 * 0 if `iter` returned 0 at any of the nodes.
 */
int list_iterate(link_list *list, list_iter iter);

/**
 * #### `void list_iterate_reverse(link_list *list, list_iter iter)`
 * Iterates through the `list` in reverse order, calling the function 
 * `iter` for each node.
 * 
 * It returns 1 if it ran through all the nodes successfully, or
 * 0 if `iter` returned 0 at any of the nodes.
 */
int list_iterate_reverse(link_list *list, list_iter iter);

/**
 * #### `typedef int (*list_comp)(void *, void *)`
 * Prototype for the function to be used in list searching functions.
 * 
 * It should return 1 if the data pointed to by the two pointers are
 * _equal_, whatever the definition of _equal_ may be.
 * 
 * It should return 0 otherwise.
 */
typedef int (*list_comp)(void *, void *);

/**
 * #### `list_element *list_find(link_list *list, void *data, list_comp comp)`
 * Searches the `list` for a specific element, using the function
 * `comp` to compare each node to the `data` parameter. It returns the
 * first `list_element` for which `comp` returns non-zero.\n
 * It returns  `NULL` if the node is not in the list.
 */
list_element *list_find(link_list *list, void *data, list_comp comp);

/**
 * #### `void *list_remove(link_list *list, void *data, list_comp comp);`
 * Searches the `list` for a specific element, using the function
 * `comp` to compare each node to the `data` parameter.
 * 
 * It then removes the first element for which `comp` returns non-zero. 
 * It returns the data pointer associated with the removed element, so 
 * that it can be manually cleaned.
 * 
 * It returns `NULL` if the element was not found in the list.
 */
void *list_remove(link_list *list, void *data, list_comp comp);

/**
 * #### `int list_strcmp(void *, void *)`
 * A `list_comp` function that compares the two elements as strings,
 * provided for convenience.
 */
int list_strcmp(void *, void *);

/**
 * #### `int list_stricmp(void *, void *)`
 * A `list_comp` function that compares the two elements as strings,
 * provided for convenience.
 * 
 * This is a case-insensitive version of list_strcmp
 */
int list_stricmp(void *, void *);

/**
 * #### `#define list_for_each(element, in_list)`
 * Macro that wraps around a `for` loop to iterate over each element
 * in a list:
 * 
 *     for(element = in_list->first; element; element = element->next)
 *     {list_for_each(element, my_list) { Blob *b = element->data; ... }
 */
#define list_for_each(element, in_list) for(element = in_list->first; element; element = element->next)

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
#endif /* LIST_H */ 
