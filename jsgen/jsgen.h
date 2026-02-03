/**
 * jsgen - JSON serialization/deserialization code generator for C
 * Example usage:
```c
#include "jsgen.h"

JSON struct role {
    int id;
    char *name;
    float *values sized_by("value_count");
    size_t value_count;
};

JSON typedef struct {
    int id;
    char *name;
    struct role *role;
    size_t role_count;
    bool is_active alias("active");
} User;

#include "models.g.h" // include the generated code
...
    User user = {0};
    parse_User("{...}", &user);
    char *json_str = stringify_User(&user);
    printf("User as JSON: %s\n", json_str);
    free(json_str);
    jsgen_free(); // free jsgen internal allocations
```
 */
#ifndef JSGEN_H
#define JSGEN_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define JSGEN_BASIC_ALLOC_SIZE (8 * 1024 * 1024)
// Basic allocator for jsgen parsing
void *jsgen_basic_alloc(size_t size);
void jsgen_free();

#ifndef JSGEN_MALLOC
#define JSGEN_MALLOC jsgen_basic_alloc
#endif

typedef void* (*JsGenMalloc)(size_t size);

// Generate JSON serialization/deserialization code for C struct.
#define JSGEN_JSON
// Generate only JSON stringification code for C struct.
#define JSGEN_JSONS
// Generate only JSON parsing code for C struct.
#define JSGEN_JSONP
// Ignore this field in JSON serialization/deserialization.
#define jsgen_ignore()
// Transform the field name in JSON to the indicated alias.
#define jsgen_alias(attr_alias)
// Define an array field that is sized by indicated field.
#define jsgen_sized_by(counter_field)
// Define a field as a JSON literal string, type must be char*.
#define jsgen_json_literal

#ifndef JSGEN_NO_STRIP
// Generate JSON serialization/deserialization code for C struct. Alias for JSGEN_JSON
#define JSON JSGEN_JSON
// Generate only JSON stringification code for C struct. Alias for JSGEN_JSONS
#define JSONS JSGEN_JSONS
// Generate only JSON parsing code for C struct. Alias for JSGEN_JSONP
#define JSONP JSGEN_JSONP
// Define an array field that is sized by indicated field. Alias for jsgen_sized_by
#define sized_by jsgen_sized_by
// Transform the field name in JSON to the indicated alias. Alias for jsgen_alias
#define alias jsgen_alias
// Region allocator for JSON parsing/stringifying. Alias for JsGenAllocator
#define json_literal jsgen_json_literal
#endif // JSGEN_NO_STRIP

#endif // JSGEN_H

#ifdef JSGEN_IMPLEMENTATION
static unsigned char jsgen__basic_alloc[JSGEN_BASIC_ALLOC_SIZE] = {0};
static size_t jsgen__basic_alloc_size;

void *jsgen_basic_alloc(size_t size) {
    if(size > JSGEN_BASIC_ALLOC_SIZE - jsgen__basic_alloc_size) {
        return NULL;
    }
    void *ptr = jsgen__basic_alloc + jsgen__basic_alloc_size;
    jsgen__basic_alloc_size += size;
    return ptr;
}

void jsgen_free() {
    jsgen__basic_alloc_size = 0;
}
#endif // JSGEN_IMPLEMENTATION