/**
 * Simple JSON builder
 * https://github.com/mceck/c-stb
 *
 * Example:
```c
#define JSB_IMPLEMENTATION
#include "jsb.h"
...
    Jsb jsb = {.pp = 4}; // pretty print with 4 spaces
    jsb_begin_object(&jsb);
    {
        jsb_key(&jsb, "message");
        jsb_string(&jsb, "Hello, World!");
        jsb_key(&jsb, "data");
        jsb_begin_array(&jsb);
        {
            jsb_string(&jsb, "item1");
            jsb_int(&jsb, 2);
            jsb_number(&jsb, 2.432, 2);
            jsb_bool(&jsb, true);
            jsb_date(&jsb, time(NULL));
            jsb_null(&jsb);
            jsb_begin_object(&jsb);
            {
                jsb_key(&jsb, "key1");
                jsb_string(&jsb, "value1");
            }
            jsb_end_object(&jsb);
        }
        jsb_end_array(&jsb);
    }
    jsb_end_object(&jsb);
    printf("jsb: %s\n", jsb_get(&jsb));
    jsb_free(&jsb);
```
 *
 * Will print:
```json
{
    "message": "Hello, World!",
    "data": [
        "item1",
        2,
        2.432,
        true,
        {
            "key1": "value1"
        }
   ]
}
```
 */

#ifndef JSB_H_
#define JSB_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef JSB_REALLOC
#define JSB_REALLOC realloc
#endif
#ifndef JSB_FREE
#define JSB_FREE free
#endif

#ifndef JSB_MAX_NESTING
#define JSB_MAX_NESTING 64
#endif

#define JSB_SMIN_CAPACITY 32

typedef enum {
    JSB_STATE_START,
    JSB_STATE_ARRAY,
    JSB_STATE_OBJECT,
    JSB_STATE_OBJECT_KEY,
    JSB_STATE_END
} JsbState;

struct jsb_string {
    char *items;
    size_t count;
    size_t capacity;
};

typedef struct {
    struct jsb_string buffer;
    JsbState state[JSB_MAX_NESTING];
    int level;
    bool is_first;
    bool is_key;
    int pp;
} Jsb;

#define jsb_free(jsb)                      \
    do {                                   \
        if ((jsb)->buffer.items) {         \
            JSB_FREE((jsb)->buffer.items); \
            (jsb)->buffer.items = NULL;    \
            (jsb)->buffer.count = 0;       \
            (jsb)->buffer.capacity = 0;    \
        }                                  \
    } while (0)

/**
 * Begin a JSON object.
 * Returns 0 on success, -1 on failure.
 */
int jsb_begin_object(Jsb *jsb);
/**
 * End a JSON object.
 * Returns 0 on success, -1 on failure.
 */
int jsb_end_object(Jsb *jsb);
/**
 * Begin a JSON array.
 * Returns 0 on success, -1 on failure.
 */
int jsb_begin_array(Jsb *jsb);
/**
 * End a JSON array.
 * Returns 0 on success, -1 on failure.
 */
int jsb_end_array(Jsb *jsb);
/**
 * Add a key to the current object.
 * Returns 0 on success, -1 on failure.
 */
int jsb_key(Jsb *jsb, const char *key);
/**
 * Add a string value.
 * Returns 0 on success, -1 on failure.
 */
int jsb_nstring(Jsb *jsb, const char *str, size_t len);
#define jsb_string(jsb, str) jsb_nstring(jsb, str, str ? strlen(str) : 0)
/**
 * Add an integer value.
 * Returns 0 on success, -1 on failure.
 */
int jsb_int(Jsb *jsb, int value);
/**
 * Add a number value.
 * Returns 0 on success, -1 on failure.
 */
int jsb_number(Jsb *jsb, double value, int precision);
/**
 * Add a boolean value.
 * Returns 0 on success, -1 on failure.
 */
int jsb_bool(Jsb *jsb, bool value);
/**
 * Add a formatted date value, accept strftime format string.
 * Returns 0 on success, -1 on failure.
 */
int jsb_date_fmt(Jsb *jsb, time_t timestamp, const char *fmt);
#define jsb_date(jsb, timestamp) jsb_date_fmt(jsb, timestamp, "%Y-%m-%d")
#define jsb_datetime(jsb, timestamp) jsb_date_fmt(jsb, timestamp, "%Y-%m-%dT%H:%M:%SZ")
/**
 * Add a null value.
 * Returns 0 on success, -1 on failure.
 */
int jsb_null(Jsb *jsb);

#define jsb_get(jsb) (jsb)->buffer.items
#endif // JSB_H_

#ifdef JSB_IMPLEMENTATION

static void jsb_srealloc(struct jsb_string *sb, size_t new_capacity) {
    if (new_capacity <= sb->capacity) return;
    if (new_capacity < JSB_SMIN_CAPACITY) new_capacity = JSB_SMIN_CAPACITY;
    size_t cap = sb->capacity ? sb->capacity : JSB_SMIN_CAPACITY;
    while (cap < new_capacity)
        cap *= 2;
    sb->items = JSB_REALLOC(sb->items, cap);
    assert(sb->items != NULL);
    sb->capacity = cap;
}
static void jsb_sappend(struct jsb_string *sb, char c) {
    jsb_srealloc(sb, sb->count + 2);
    sb->items[sb->count] = c;
    if (c != '\0') sb->count++;
}
static void jsb_sappends(struct jsb_string *sb, char *c) {
    size_t len = strlen(c);
    jsb_srealloc(sb, sb->count + len + 1);
    if (len) {
        memcpy(&sb->items[sb->count], c, len);
        sb->count += len;
    }
    sb->items[sb->count] = '\0';
}

/**
 * Appends a string to the JSON buffer.
 * It will escape the string and wrap it in quotes.
 */
static void jsb_escaped_nstring(struct jsb_string *sb, const char *str, size_t len) {
    jsb_sappend(sb, '"');
    // escape
    for (size_t i = 0; i < len; ++i) {
        switch (str[i]) {
        case '\n':
            jsb_sappend(sb, '\\');
            jsb_sappend(sb, 'n');
            break;
        case '\t':
            jsb_sappend(sb, '\\');
            jsb_sappend(sb, 't');
            break;
        case '"':
        case '\\':
            jsb_sappend(sb, '\\');
            jsb_sappend(sb, str[i]);
            break;
        case '\b':
        case '\r':
            break;
        default:
            jsb_sappend(sb, str[i]);
        }
    }
    jsb_sappend(sb, '"');
}
static void jsb_escaped_string(struct jsb_string *sb, const char *str) {
    jsb_escaped_nstring(sb, str, strlen(str));
}

/**
 * Values are valid only in:
 * beginning of the document
 * object context after a key
 * array context
 */
static int jsb_check_val(Jsb *jsb) {
    JsbState state = jsb->state[jsb->level];
    if (state == JSB_STATE_ARRAY) return 0;
    if (state == JSB_STATE_OBJECT && jsb->is_key) return 0;
    if (state == JSB_STATE_START && jsb->is_first) return 0;
    return -1;
}

/**
 * Adds new line and indentation for pretty printing.
 */
static void jsb_pretty_print_ch(Jsb *jsb) {
    if (jsb->pp && !jsb->is_key) {
        jsb_sappend(&jsb->buffer, '\n');
        for (int i = 0; i < jsb->level * jsb->pp; ++i) {
            jsb_sappend(&jsb->buffer, ' ');
        }
    }
}

static void _jsb_init(Jsb *jsb) {
    jsb->buffer.count = 0;
    jsb->level = 0;
    jsb->state[0] = JSB_STATE_START;
    jsb->is_first = true;
}

static int _jsb_end(Jsb *jsb) {
    if (jsb->level != 0) return -1;
    jsb_sappends(&jsb->buffer, "");
    jsb->state[0] = JSB_STATE_END;
    return 0;
}

int jsb_begin_object(Jsb *jsb) {
    if (jsb->level == 0) _jsb_init(jsb);
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    jsb_sappend(&jsb->buffer, '{');
    jsb->state[++jsb->level] = JSB_STATE_OBJECT;
    jsb->is_first = true;
    jsb->is_key = false;
    return 0;
}

int jsb_end_object(Jsb *jsb) {
    if (jsb->level < 1 || jsb->state[jsb->level] != JSB_STATE_OBJECT) return -1;
    jsb->level--;
    jsb_pretty_print_ch(jsb);
    jsb_sappend(&jsb->buffer, '}');
    if (jsb->level == 0) _jsb_end(jsb);
    jsb->is_first = false;
    return 0;
}

int jsb_begin_array(Jsb *jsb) {
    if (jsb->level == 0) _jsb_init(jsb);
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    jsb_sappend(&jsb->buffer, '[');
    jsb->state[++jsb->level] = JSB_STATE_ARRAY;
    jsb->is_first = true;
    jsb->is_key = false;
    return 0;
}

int jsb_end_array(Jsb *jsb) {
    if (jsb->state[jsb->level] != JSB_STATE_ARRAY) return -1;
    jsb->level--;
    jsb_pretty_print_ch(jsb);
    jsb_sappend(&jsb->buffer, ']');
    if (jsb->level == 0) _jsb_end(jsb);
    jsb->is_first = false;
    return 0;
}

int jsb_key(Jsb *jsb, const char *key) {
    if (jsb->state[jsb->level] != JSB_STATE_OBJECT || jsb->is_key) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    jsb_escaped_string(&jsb->buffer, key);
    jsb_sappends(&jsb->buffer, ": ");
    jsb->is_first = true;
    jsb->is_key = true;
    return 0;
}

int jsb_nstring(Jsb *jsb, const char *str, size_t len) {
    if (!str) return jsb_null(jsb);
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    jsb_escaped_nstring(&jsb->buffer, str, len);
    jsb_sappend(&jsb->buffer, '\0');
    jsb->is_first = false;
    jsb->is_key = false;
    return 0;
}

int jsb_int(Jsb *jsb, int value) {
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    char numbuf[16];
    int n = snprintf(numbuf, sizeof(numbuf), "%d", value);
    if (n < 0) return -1;
    jsb_sappends(&jsb->buffer, numbuf);
    jsb->is_first = false;
    jsb->is_key = false;
    return 0;
}

int jsb_number(Jsb *jsb, double value, int precision) {
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    char numbuf[64];
    int n = snprintf(numbuf, sizeof(numbuf), "%.*f", precision, value);
    if (n < 0) return -1;
    jsb_sappends(&jsb->buffer, numbuf);
    jsb->is_first = false;
    jsb->is_key = false;
    return 0;
}

int jsb_bool(Jsb *jsb, bool value) {
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    jsb_sappends(&jsb->buffer, value ? "true" : "false");
    jsb->is_first = false;
    jsb->is_key = false;
    return 0;
}

int jsb_null(Jsb *jsb) {
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    jsb_sappends(&jsb->buffer, "null");
    jsb->is_first = false;
    jsb->is_key = false;
    return 0;
}

int jsb_date_fmt(Jsb *jsb, time_t timestamp, const char *fmt) {
    if (jsb_check_val(jsb)) return -1;
    if (!jsb->is_first) jsb_sappend(&jsb->buffer, ',');
    jsb_pretty_print_ch(jsb);
    char datebuf[32];
    struct tm *tm_info = localtime((time_t *)&timestamp);
    strftime(datebuf, sizeof(datebuf), fmt, tm_info);
    jsb_escaped_string(&jsb->buffer, datebuf);
    jsb->is_first = false;
    jsb->is_key = false;
    return 0;
}
#endif // JSB_IMPLEMENTATION