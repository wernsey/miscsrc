/**
 * # JSON Parser
 *
 * An over-engineered JSON parser and serializer.
 *
 * ## License
 *
 *     Author: Werner Stoop
 *     This is free and unencumbered software released into the public domain.
 *     See http://unlicense.org/ for more details.
 *
 * ## References
 *
 * * https://www.json.org/json-en.html
 * * https://tools.ietf.org/id/draft-ietf-json-rfc4627bis-09.html
 * * Unicode related:
 *   * https://unicodebook.readthedocs.io/index.html
 *   * https://en.wikipedia.org/wiki/UTF-8
 *   * https://en.wikipedia.org/wiki/UTF-16
 *
 */

#ifndef JSON_H
#define JSON_H
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * ## Types
 *
 * ### `typedef enum json_type JSON_Type;`
 *
 * Enumeration for the type a `JSON` entity might have.
 *
 * Values are `j_string`, `j_number`, `j_object`, `j_array`,
 * `j_true`, `j_false` and `j_null`.
 *
 * See `json_get_type()`.
 */

typedef enum json_type {
	j_string,
	j_number,
	j_object,
	j_array,
	j_true,
	j_false,
	j_null
} JSON_Type;

/**
 * ### `typedef struct json JSON;`
 *
 * Structure that encapsulates a JSON entity.
 */
typedef struct json JSON;

/**
 * ## Utility Function Pointers
 *
 * These variables can be changed to point to custom functions to
 * change the behaviour of the parser.
 */

/**
 * ### `extern int (*json_error)(const char *fmt, ...);`
 *
 * Pointer to a function that that will be used internally
 * to display error messages.
 *
 * The default just performs a `fprintf()` to `stderr`.
 */
extern int (*json_error)(const char *fmt, ...);

/**
 * ### `extern char *(*json_readfile)(const char *fname);`
 *
 * Pointer to a function that will be called by `json_read()` to read
 * the entire contents of a file specified by `fname` into memory.
 *
 * The return value is expected to be allocated on the heap, and `free()`
 * will be called on it.
 *
 * The default uses `fopen()` and `fread()`
 */
extern char *(*json_readfile)(const char *fname);

/**
 * ## API Functions
 */

/**
 * ### `JSON *json_read(const char *filename);`
 *
 * Reads and parses a file specified by `filename`
 */
JSON *json_read(const char *filename);

/**
 * ### `JSON *json_parse(const char *text);`
 *
 * Parses a string `text` into a `JSON` entity.
 */
JSON *json_parse(const char *text);

/**
 * ### `JSON *json_retain(JSON *j);`
 *
 * Increments the reference count of a JSON entity.
 */
JSON *json_retain(JSON *j);

/**
 * ### `void json_release(JSON *j)`
 *
 * Decrements the reference count of a JSON entity. If the
 * reference count reaches zero the entity is destroyed and
 * its memory reclaimed.
 */
void json_release(JSON *j);

/**
 * ### `char *json_serialize(JSON *j);`
 *
 * Serializes a JSON entity into a heap-allocated string.
 */
char *json_serialize(JSON *j);

/**
 * ### `char *json_pretty(JSON *j);`
 *
 * Serializes a JSON entity into a pretty-printed heap-allocated string.
 */
char *json_pretty(JSON *j);

/**
 * ### `JSON *json_new_object()`
 *
 * Creates a new JSON entity containing an empty object.
 */
JSON *json_new_object();

/**
 * ### `JSON *json_new_array()`
 *
 * Creates a new JSON entity conatining an empty array.
 */
JSON *json_new_array();

/**
 * ### `JSON *json_new_string(const char *text)`
 *
 * Creates a new JSON string entity containing the given text.
 */
JSON *json_new_string(const char *text);

/**
 * ### `JSON *json_new_number(double n)`
 *
 * Creates a new JSON number entity with the specified value.
 */
JSON *json_new_number(double n);

/**
 * ### `JSON *json_null()`
 *
 * Returns a JSON entity representing the JSON `null` value.
 */
JSON *json_null();

/**
 * ### `JSON *json_true()`
 *
 * Returns a JSON entity representing the JSON `true` value.
 */
JSON *json_true();

/**
 * ### `JSON *json_false()`
 *
 * Returns a JSON entity representing the JSON `false` value.
 */
JSON *json_false();

/**
 * ### `JSON *json_boolean(int value)`
 *
 * Returns a JSON entity representing either the JSON `true`
 * if `value` is non-zero, or `false` otherwise.
 */
JSON *json_boolean(int value);

/**
 * ### `JSON_Type json_get_type(JSON *j)`
 *
 * Returns the type of a JSON entity.
 */
JSON_Type json_get_type(JSON *j);

/**
 * ### `int json_is_null(JSON *j)`
 *
 * Returns non-zero if `j` represents the JSON `null` value
 */
int json_is_null(JSON *j);

/**
 * ### `int json_is_boolean(JSON *j)`
 *
 * Returns non-zero if `j` and only if represents the JSON
 * `true` or `false` values.
 */
int json_is_boolean(JSON *j);

/**
 * ### `int json_is_true(JSON *j)`
 *
 * Returns non-zero if `j` represents the JSON `true` value.
 *
 * It resembles the JavaScript operation `j === true`. See
 * `json_is_truthy()` for the less strict version.
 */
int json_is_true(JSON *j);

/**
 * ### `int json_is_false(JSON *j)`
 *
 * Returns non-zero if `j` represents the JSON `false` value.
 *
 * It resembles the JavaScript operation `j === false`. See
 * `json_is_falsey()` for the less strict version.
 */
int json_is_false(JSON *j);

/**
 * ### `int json_is_truthy(JSON *j)`
 *
 * Returns non-zero if `j` represents a JSON [_truthy_][truthy] value.
 *
 * [truthy]: https://developer.mozilla.org/en-US/docs/Glossary/Truthy
 */
int json_is_truthy(JSON *j);

/**
 * ### `int json_is_falsey(JSON *j)`
 *
 * Returns non-zero if `j` represents a JSON [_falsey_][falsey] value.
 *
 * [falsey]: https://developer.mozilla.org/en-US/docs/Glossary/Falsy
 */
int json_is_falsey(JSON *j);

/**
 * ### `int json_is_number(JSON *j)`
 *
 * Returns non-zero if `j` represents a numeric JSON value
 */
int json_is_number(JSON *j);

/**
 * ### `int json_is_string(JSON *j)`
 *
 * Returns non-zero if `j` represents a JSON string value
 */
int json_is_string(JSON *j);

/**
 * ### `int json_is_object(JSON *j)`
 *
 * Returns non-zero if `j` is a JSON object
 */
int json_is_object(JSON *j);

/**
 * ### `int json_is_array(JSON *j)`
 *
 * Returns non-zero if `j` is a JSON array
 */
int json_is_array(JSON *j);

/**
 * ### `double json_as_number(JSON *j)`
 *
 * Returns a numeric representation of the JSON entity `j`
 */
double json_as_number(JSON *j);

/**
 * ### `const char *json_as_string(JSON *j)`
 *
 * Returns the string representation of the JSON entity `j`
 */
const char *json_as_string(JSON *j);

/**
 * ### `int json_obj_has(JSON *obj, const char *name)`
 *
 * Returns non-zero if `obj` is a JSON object containing the key `name`,
 * zero otherwise.
 */
int json_obj_has(JSON *obj, const char *name);

/**
 * ### `const char *json_obj_next(JSON *j, const char *name);`
 *
 * Returns the next key in a JSON object following `name`.
 *
 * If name is `NULL`, it will return the first key in the object.
 *
 * It will return `NULL` if `name` is the last key in the object.
 *
 * It kan be used for iterating through all the keys in an object,
 * for example:
 *
 * ```
 * const char *name = json_obj_next(obj, NULL);
 * while(name) {
 *   JSON *item = json_obj_get(obj, name);
 *   // --snip--
 *   name = json_obj_next(obj, name);
 * }
 * ```
 */
const char *json_obj_next(JSON *j, const char *name);

/**
 * ### `JSON *json_obj_get(JSON *obj, const char *name)`
 *
 * Retrieves the value associated with `name` in the JSON object `obj`.
 *
 * It returns NULL if `obj` does not contain `name`.
 */
JSON *json_obj_get(JSON *obj, const char *name);

/**
 * ### `double json_obj_get_number(JSON *obj, const char *name)`
 *
 * Returns the JSON entity associated with `name` in the JSON
 * object `obj` as a number.
 *
 * It returns `0.0` if `obj[name]` is not numeric.
 */
double json_obj_get_number(JSON *obj, const char *name);

/**
 * ### `double json_obj_get_number_or(JSON *obj, const char *name, double def)`
 *
 * Returns the JSON entity associated with `name` in the JSON
 * object `obj` as a number, or returns a default value `def` if
 * `obj[name]` is not numeric.
 */
double json_obj_get_number_or(JSON *obj, const char *name, double def);

/**
 * ### `const char *json_obj_get_string(JSON *obj, const char *name)`
 *
 * Returns the JSON entity associated with `name` in the JSON
 * object `obj` as a string.
 *
 * It returns `NULL` if `obj[name]` is not a string.
 */
const char *json_obj_get_string(JSON *obj, const char *name);

/**
 * ### `const char *json_obj_get_string_or(JSON *j, const char *name, const char *def)`
 *
 * Returns the JSON entity associated with `name` in the JSON
 * object `obj` as a string, or returns a default value `def` if
 * `obj[name]` is not a string.
 */
const char *json_obj_get_string_or(JSON *j, const char *name, const char *def);

/**
 * ### `int json_obj_get_bool(JSON *j, const char *name)`
 *
 * Returns the JSON entity associated with `name` in the JSON
 * object `obj` as a boolean value (zero = false, non-zero is true).
 *
 * It returns false if `obj[name]` is not boolean.
 */
int json_obj_get_bool(JSON *j, const char *name);

/**
 * ### `int json_obj_get_bool(JSON *j, const char *name)`
 *
 * Returns the JSON entity associated with `name` in the JSON
 * object `obj` as a boolean value (zero = false, non-zero is true).
 *
 * It returns false if `obj[name]` is not boolean.
 */
int json_obj_get_bool_or(JSON *j, const char *name, int def);

/**
 * ### `JSON *json_obj_set(JSON *obj, char *k, JSON *v)`
 *
 * Sets the value associated with `name` in the JSON object `obj` to `v`.
 *
 * It returns `obj`.
 */
JSON *json_obj_set(JSON *obj, char *k, JSON *v);

/**
 * ### `JSON *json_obj_set_number(JSON *obj, char *k, double n)`
 *
 * Sets the value associated with `name` in the JSON object `obj` to
 * the numeric value `n`.
 *
 * It returns `obj`.
 */
JSON *json_obj_set_number(JSON *obj, char *k, double n);

/**
 * ### `JSON *json_obj_set_string(JSON *obj, char *k, const char *str)`
 *
 * Sets the value associated with `name` in the JSON object `obj` to
 * the string value `str`.
 *
 * It returns `obj`.
 */
JSON *json_obj_set_string(JSON *obj, char *k, const char *str);

/**
 * ### `int json_obj_check_type(JSON *obj, const char *name, JSON_Type type)`
 *
 * Returns non-zero if the value associated with `name` in the JSON object `obj`
 * is of the given type `type`.
 *
 * Returns zero if it is not of the given type, or `name` is not found in `obj`
 */
int json_obj_check_type(JSON *obj, const char *name, JSON_Type type);

/**
 * ### `unsigned int json_array_len(JSON *array)`
 *
 * Returns the length of the JSON array `array`.
 */
unsigned int json_array_len(JSON *array);

/**
 * ### `JSON *json_array_reserve(JSON *array, unsigned int n)`
 *
 * Reserves space for `n` items in the JSON array `array`.
 */
JSON *json_array_reserve(JSON *array, unsigned int n);

/**
 * ### `JSON *json_array_get(JSON *array, int n)`
 *
 * Retrieves the `n`'th element in the JSON array `array`.
 */
JSON *json_array_get(JSON *array, int n);

/**
 * ### `double json_array_get_number(JSON *array, int n)`
 *
 * Retrieves the `n`'th element in the JSON array `array` as a number.
 */
double json_array_get_number(JSON *array, int n);

/**
 * ### `const char *json_array_get_string(JSON *array, int n)`
 *
 * Retrieves the `n`'th element in the JSON array `array` as a string.
 */
const char *json_array_get_string(JSON *array, int n);

/**
 * ### `JSON *json_array_set(JSON *array, int n, JSON *v)`
 *
 * Sets the `n`'th element in the JSON array `array` to the value `v`.
 *
 * Returns `array`.
 */
JSON *json_array_set(JSON *array, int n, JSON *v);

/**
 * ### `JSON *json_array_add(JSON *array, JSON *value)`
 *
 * Appends `value` to the JSON array `array`.
 *
 * Returns `array`.
 */
JSON *json_array_add(JSON *array, JSON *value);

/**
 * ### `JSON *json_array_add_number(JSON *array, double number)`
 *
 * Appends the number `number` to the JSON array `array`.
 *
 * Returns `array`.
 */
JSON *json_array_add_number(JSON *array, double number);

/**
 * ### `JSON *json_array_add_string(JSON *array, const char *str)`
 *
 * Appends the string `str` to the JSON array `array`.
 *
 * Returns `array`.
 */
JSON *json_array_add_string(JSON *array, const char *str);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifdef */