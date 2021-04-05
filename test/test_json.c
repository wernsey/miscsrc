#include <stdio.h>
#include <stdlib.h>

#include "../json.h"

int main(int argc, char *argv[]) {
    JSON *j;

    /* Doing this ensures that the globals used by
    json_null(), json_true() and json_false() are
    initialised.
    See the comments about JSON_REENTRANT above */
    json_release(json_null());

    if(argc > 1) {
        j = json_read(argv[1]);
        if(!j) {
            json_error("Unable to parse %s", argv[1]);
            return 1;
        }
    } else {
        j = json_new_object();
        JSON *a = json_new_array();
        json_array_add_string(a, "first element");
        json_array_add_number(a, 10);
        json_array_add(a, json_new_number(20));
        json_array_add_string(a, "some string");
        json_array_add(a, NULL);
        json_array_add_string(a, "another string");
        json_array_set(a, 0, json_new_string("FIRST ELEMENT"));

        json_array_reserve(a, 10);
        json_array_set(a, 8, json_new_string("foo"));

        /* Remember that json_array_set does not retain the objects by itself */
        json_array_set(a, 8, json_retain(json_array_get(a, 8)));

        json_array_set(a, 9, json_new_string("10th element"));

        json_obj_set(j, "array", a);

        /* Note that `j` takes ownership of `a`, so you shouldn't
        call `json_release(a)` */

        json_obj_set(j, "null-value", json_null());
        json_obj_set(j, "true-value", json_true());
        json_obj_set(j, "2nd-true-value", json_true());

        json_obj_set_number(j, "a-number", 123.456);
        json_obj_set_string(j, "a-string", "A string value");
        /* Existing values will get replaced: */
        json_obj_set_string(j, "a-string", "A replacement string value");

        json_obj_set_string(j, "b-string", "BBBB");

        /* Remember that json_obj_set does not retain the objects by itself */
        json_obj_set(j, "b-string", json_retain(json_obj_get(j, "b-string")));

        json_obj_set_string(j, "non-string", json_obj_get_string(j, "key-that-doesn't exist"));
        json_obj_set(j, "null-value", NULL);
        json_obj_set_string(j, "null-string-value", NULL);
    }

    /* char *s = json_serialize(j); */
    char *s = json_pretty(j);
    puts(s);
    free(s);

	json_release(j);

#if 0
    j = json_new_array();
    json_array_add(j,json_null());
    json_array_add(j,json_true());
    json_array_add(j,json_false());
    s = json_serialize(j);
    puts(s);
    free(s);
    json_release(j);
#endif

	return 0;
}
