/** 
 * # gc.h
 *  
 * Mark and Sweep garbage collector.
 *
 * * Objects are allocated using `gc_alloc()`. It pads the object with 
 *   additional information.
 * * The root objects are registered through `gc_retain()` (and dergistered
 *   through `gc_release()`).
 * * `gc_collect()` iterates through the root objects:
 *   * It calls the marker function registered through `gc_set_marker()` on every
 *     root object. It is the marker function's responsibility to call
 *     `gc_mark()` on any objects that are reachable from the root.  
 *     `gc_mark()` will recursively call any reachable object's marker function,
 *     etc. to ensure that all reachable objects are marked.
 *   * `gc_collect()` then iterates through all the allocated objects and 
 *     reclaims any objects that have not been marked.
 *   * If the collector reclaims an object, it will call a destructor function
 *     that has been registered on the object through `gc_set_dtor()` so that
 *     any additional resources that are being kept by the object can be reclaimed. 
 *
 * `gc_collect()` is called automatically from `gc_alloc()` at regular 
 * intervals.
 * 
 * ### License
 *
 *     Author: Werner Stoop
 *     This is free and unencumbered software released into the public domain.
 *     http://unlicense.org/
 *
 * ## API
 */
 
/**
 * ### Types
 * #### `typedef void (*gc_function)(void *)`
 * Prototype for functions that will be used to
 * mark objects and object finalizers.
 */
typedef void (*gc_function)(void *);

/**
 * ### Functions
 * #### `void gc_init()`
 * Initializes the garbage collector.
 */
void gc_init();

/**
 * #### `void *gc_alloc(size_t size)`
 * Allocates a block of `size` bytes that will be managed by the 
 * collector. The block will be reclaimed if it is no longer reachable.
 * 
 * In debug mode, this function is replaced with a macro to help
 * troubleshoot issues.
 */
#ifdef NDEBUG
void *gc_alloc(size_t size);
#else
void *gc_alloc_(size_t size, const char *file, int line);
#define gc_alloc(size) gc_alloc_(size, __FILE__, __LINE__)
#endif

/**
 * #### `void *gc_retain(void *p)`
 * Adds an object to the list of roots managed by the garbage collector.
 * 
 * `p` must be a pointer allocated previously through `gc_alloc()`
 */
void *gc_retain(void *p);

/**
 * #### `void gc_release(void *p)`
 * Removes an object from the list of roots that have previously been retained
 * by `gc_retain()`
 */
void gc_release(void *p);

/**
 * #### `void gc_set_marker(void *p, gc_function marker)`
 * Registers a marker function `marker` on an object `p`.
 *
 * See `gc_mark()` for more info.
 */
void gc_set_marker(void *p, gc_function marker);

/**
 * #### `void gc_mark(void *p)`
 * Marks an object as reachable during the _mark_ phase of the garbage collector.
 * 
 * It must be called from the marker function registered on 
 * objects through the `gc_set_marker()` marker function.
 * 
 * For example, if object A is a root (that has been retained through
 * `gc_retain(A)`) that has a pointer to object B, then object A should have
 * a marker function registered that will call `gc_mark(B)`.
 * 
 * If object B in turn has pointers to objects C and D then the marker function
 * registered on B must call `gc_mark(C)` and `gc_mark(D)`.
 */
void gc_mark(void *p);

/**
 * #### `void gc_collect()`
 * Runs the garbage collector.
 * 
 * This function is called automatically by `gc_alloc()` on regular
 * intervals (see `GC_INTERVAL` in `gc.c`), but it is exposed for 
 * convenience.
 */
void gc_collect();

/**
 * #### `void gc_set_dtor(void *p, gc_function dtor)`
 * Sets the destructor (aka _finalizer_) of an object to be called when
 * the collector reclaims the memory.
 * 
 * `p` is a pointer previously returned by `gc_alloc()` and `dtor`
 * is a function that will clean-up the contents of `p` when `p` is
 * reclaimed.
 */
void gc_set_dtor(void *p, gc_function dtor);

/**
 * #### `void gc_dump()`
 * Utility function to display roots and uncollected garbage.
 * 
 * You can use this at program termination to ensure that all
 * roots have been released and all garbage collected
 */
void gc_dump();
