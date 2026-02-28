#ifndef JSGEN_MODELS_H
#define JSGEN_MODELS_H

#include <stdbool.h>
#include <stddef.h>
#include "../jsgen/jsgen.h"

// Basic struct with all primitive types
JSON typedef struct {
    int id;
    char *name;
    double score;
    float rating;
    bool active;
    long timestamp;
    size_t count;
} BasicModel;

// Struct with alias annotation
JSON typedef struct {
    int id;
    char *first_name alias("firstName");
    char *last_name alias("lastName");
    bool is_active alias("isActive");
} AliasModel;

// Struct with pointer to another struct (nested object)
JSON typedef struct {
    char *street;
    char *city;
    int zip;
} Address;

JSON typedef struct {
    int id;
    char *name;
    Address *address;
} PersonModel;

// Struct with sized_by array of structs
JSON typedef struct {
    char *key;
    char *value;
} Tag;

JSON typedef struct {
    int id;
    Tag *tags sized_by("tag_count");
    size_t tag_count;
} TaggedModel;

// Non-typedef struct (uses "struct" keyword)
JSON struct color {
    int r;
    int g;
    int b;
};

JSON typedef struct {
    char *name;
    struct color *fg;
    struct color *bg;
} ThemeModel;

// Struct with ignored field
JSON typedef struct {
    int id;
    char *name;
    char *internal_secret jsgen_ignore();
    bool visible;
} IgnoreModel;

// Single field struct
JSON typedef struct {
    int dummy;
} MinimalModel;

// Nested struct (inline, not pointer) - uses typedef
JSON typedef struct {
    int x;
    int y;
} Inner;

JSON typedef struct {
    char *name;
    Inner pos;
} InlineNestedModel;

// JSONS - stringify only
JSONS typedef struct {
    int code;
    char *message;
} StringifyOnlyModel;

// JSONP - parse only
JSONP typedef struct {
    int code;
    char *message;
} ParseOnlyModel;

// Struct with optional (nullable) pointer to another struct
JSON typedef struct {
    char *name;
    Address *home;
    Address *work; // can be null
} DualAddressModel;

// Struct with sized_by array of primitives
JSON typedef struct {
    int *values sized_by("value_count");
    size_t value_count;
} IntArrayModel;

JSON typedef struct {
    double *scores sized_by("score_count");
    size_t score_count;
} DoubleArrayModel;

#endif // JSGEN_MODELS_H
