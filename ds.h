/**
 * ds - A simple and lightweight stb-style data structures and utilities library in C.
 * https://github.com/mceck/c-stb
 *
 * - Dynamic arrays
 * - String builder
 * - Linked lists
 * - Hash maps
 * - Logging
 * - File utilities
 *
 * #define DS_NO_PREFIX to disable the `ds_` prefix for all functions and types.
 */
#ifndef DS_H_
#define DS_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef DS_ALLOC
#define DS_ALLOC malloc
#endif // DS_ALLOC
#ifndef DS_REALLOC
#define DS_REALLOC realloc
#endif // DS_REALLOC
#ifndef DS_FREE
#define DS_FREE free
#endif // DS_FREE

typedef enum {
    DS_DEBUG,
    DS_INFO,
    DS_WARN,
    DS_ERROR
} DsLogLevel;

#ifndef DS_LOG_LEVEL
#define DS_LOG_LEVEL DS_INFO
#endif // DS_LOG_LEVEL

static const char *DsLogLevelStrings[] = {
    [DS_DEBUG] = "DEBUG",
    [DS_INFO] = "INFO",
    [DS_WARN] = "WARN",
    [DS_ERROR] = "ERROR"};

#define ds_log(LVL, FMT, ...)                                                                  \
    do {                                                                                       \
        if (DS_LOG_LEVEL <= LVL) {                                                             \
            FILE *log_file = LVL >= DS_ERROR ? stderr : stdout;                                \
            fprintf(log_file, "[%s] " FMT, DsLogLevelStrings[LVL] __VA_OPT__(, ) __VA_ARGS__); \
            fflush(log_file);                                                                  \
        }                                                                                      \
    } while (0)
#define ds_log_info(FMT, ...) ds_log(DS_INFO, FMT, __VA_ARGS__)
#define ds_log_debug(FMT, ...) ds_log(DS_DEBUG, FMT, __VA_ARGS__)
#define ds_log_warn(FMT, ...) ds_log(DS_WARN, FMT, __VA_ARGS__)
#define ds_log_error(FMT, ...) ds_log(DS_ERROR, FMT, __VA_ARGS__)

#define DS_TODO(msg)                                                                     \
    do {                                                                                 \
        ds_log(DS_WARN, "TODO: %s\nat %s::%s::%d\n", msg, __FILE__, __func__, __LINE__); \
        abort();                                                                         \
    } while (0)
#define DS_UNREACHABLE()                                                                  \
    do {                                                                                  \
        ds_log(DS_ERROR, "UNREACHABLE CODE: %s::%s::%d\n", __FILE__, __func__, __LINE__); \
        abort();                                                                          \
    } while (0)
#define DS_UNUSED(x) (void)(x)

#ifndef DS_DA_INIT_CAPACITY
#define DS_DA_INIT_CAPACITY 1024
#endif // DS_DA_INIT_CAPACITY

/**
 * Dynamic array declaration
 * It will create a dynamic array of the specified type.
 * Example:
 *   `ds_da_declare(my_array, int);`
 *
 * This will create a dynamic array of integers like this:
```c
typedef struct {
    int *data;
    size_t length;
    size_t capacity;
} my_array;
```
 */
#define ds_da_declare(name, type) \
    typedef struct {              \
        type *data;               \
        size_t length;            \
        size_t capacity;          \
    } name

/**
 * Reserve space in a dynamic array.
 */
#define ds_da_reserve(da, expected_capacity)                                           \
    do {                                                                               \
        if ((size_t)(expected_capacity) > (da)->capacity) {                            \
            if ((da)->capacity == 0) {                                                 \
                (da)->capacity = DS_DA_INIT_CAPACITY;                                  \
            }                                                                          \
            while ((size_t)(expected_capacity) > (da)->capacity) {                     \
                (da)->capacity = (da)->capacity + ((da)->capacity >> 1);               \
            }                                                                          \
            (da)->data = DS_REALLOC((da)->data, (da)->capacity * sizeof(*(da)->data)); \
            assert((da)->data != NULL);                                                \
        }                                                                              \
    } while (0)

/**
 * Append an item to a dynamic array.
 * Example:
 ```c
 ds_da_declare(my_array, int);
 ...
    my_array a = {0};
    ds_da_append(&a, 42);
 ```
 */
#define ds_da_append(da, item)                 \
    do {                                       \
        ds_da_reserve((da), (da)->length + 1); \
        (da)->data[(da)->length++] = (item);   \
    } while (0)

/**
 * Append multiple items to a dynamic array.
 * It will copy the items from the source array to the destination array.
 * Example:
 ```c
 ds_da_declare(my_array, int);
 ...
    my_array a = {0};
    ds_da_append_many(&a, (int[]){1, 2, 3}, 3);
 ```
 */
#define ds_da_append_many(da, items, size)          \
    do {                                            \
        if ((size) == 0) break;                     \
        ds_da_reserve((da), (da)->length + (size)); \
        memcpy(&(da)->data[(da)->length], (items),  \
               (size) * sizeof(*(da)->data));       \
        (da)->length += (size);                     \
    } while (0)

/**
 * Pop an item from a dynamic array.
 */
#define ds_da_pop(da)               \
    ({                              \
        assert((da)->data != NULL); \
        (da)->data[--(da)->length]; \
    })

/**
 * Remove a range of data from a dynamic array.
 * Example:
 *   `ds_da_remove(&a, 1, 2); // [1,2,3,4,5] -> [1,4,5]`
 */
#define ds_da_remove(da, idx, del)                                             \
    do {                                                                       \
        if ((da)->data && (idx) < (da)->length) {                              \
            memmove(&(da)->data[(idx)], &(da)->data[(idx) + (del)],            \
                    ((da)->length - (idx) - (del)) * sizeof(*(da)->data));     \
            (da)->length = (da)->length > (del) ? (da)->length - (del) : 0;    \
            memset(&(da)->data[(da)->length], 0, (del) * sizeof(*(da)->data)); \
        }                                                                      \
    } while (0)

/**
 * Insert an item at a specific index in a dynamic array.
 * Example:
 *   `ds_da_insert(&a, 1, 42); // [1,2,3] -> [1,42,2,3]`
 */
#define ds_da_insert(da, idx, item)                            \
    do {                                                       \
        if ((idx) > (da)->length) (idx) = (da)->length;        \
        ds_da_reserve((da), (da)->length + 1);                 \
        memmove(&(da)->data[(idx) + 1], &(da)->data[(idx)],    \
                ((da)->length - (idx)) * sizeof(*(da)->data)); \
        (da)->data[(idx)] = (item);                            \
        (da)->length++;                                        \
    } while (0)

/**
 * Prepend an item to a dynamic array.
 * Example:
 ```c
 ds_da_declare(my_array, int);
 ...
    my_array a = {0};
    ds_da_prepend(&a, 42);
 ```
 */
#define ds_da_prepend(da, item) ds_da_insert((da), 0, (item))
/**
 * Get a pointer to the last item of a dynamic array, or NULL if the array is empty.
 */
#define ds_da_last(da) ((da)->length > 0 ? &(da)->data[(da)->length - 1] : NULL)

/**
 * Get a pointer to the first item of a dynamic array, or NULL if the array is empty.
 */
#define ds_da_first(da) ((da)->length > 0 ? &(da)->data[0] : NULL)

/**
 * Reset a dynamic array. It will not free the underlying memory.
 */
#define ds_da_zero(da)      \
    do {                    \
        (da)->length = 0;   \
        (da)->capacity = 0; \
        (da)->data = NULL;  \
    } while (0)

/**
 * Free a dynamic array.
 */
#define ds_da_free(da)                       \
    do {                                     \
        if ((da)->data) DS_FREE((da)->data); \
        ds_da_zero(da);                      \
    } while (0)

/**
 * Iterate over a dynamic array.
 * Example:
 *   `ds_da_foreach(&a, item) { printf("%d\n", item); }`
 */
#define ds_da_foreach(da, var)  \
    __typeof__((da)->data) var; \
    for (size_t _i = 0; _i < (da)->length && (var = &(da)->data[_i]); _i++)

/**
 * Find an item in a dynamic array. It returns a pointer to the item, or NULL if not found.
 * Example:
 *   `int *x = ds_da_find(&a, e == 0);`
 */
#define ds_da_find(da, expr)                              \
    ({                                                    \
        __typeof__(*(da)->data) *_r = NULL;               \
        for (size_t _i = 0; _i < (da)->length; _i++) {    \
            __typeof__(*(da)->data) *e = &(da)->data[_i]; \
            if (expr) {                                   \
                _r = &(da)->data[_i];                     \
                break;                                    \
            }                                             \
        }                                                 \
        _r;                                               \
    })

/**
 * Get the index of an item in a dynamic array.
 * It returns the index of the item, or -1 if not found.
 */
#define ds_da_index_of(da, expr)                          \
    ({                                                    \
        int _r = -1;                                      \
        for (size_t _i = 0; _i < (da)->length; _i++) {    \
            __typeof__(*(da)->data) *e = &(da)->data[_i]; \
            if (expr) {                                   \
                _r = (int)_i;                             \
                break;                                    \
            }                                             \
        }                                                 \
        _r;                                               \
    })

/**
 * Dynamic string builder.
 */
ds_da_declare(DsString, char);

void _ds_sb_append(DsString *sb, ...) {
    va_list args;
    va_start(args, sb);
    const char *str;
    while ((str = va_arg(args, const char *))) {
        size_t len = strlen(str);
        ds_da_reserve(sb, sb->length + len + 1);
        memcpy(sb->data + sb->length, str, len);
        sb->length += len;
    }
    va_end(args);
    ds_da_reserve(sb, sb->length + 1);
    sb->data[sb->length] = '\0';
}

/**
 * Append formatted string to a dynamic string builder.
 * Example:
 *   `ds_sb_appendf(&sb, "Hello, %s!", "World");`
 */
void ds_sb_appendf(DsString *sb, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (n > 0) {
        ds_da_reserve(sb, sb->length + n + 1);
        va_start(args, fmt);
        vsnprintf(sb->data + sb->length, n + 1, fmt, args);
        va_end(args);
        sb->length += n;
    }
}

/**
 * Prepend formatted string to a dynamic string builder.
 * Example:
 *   `ds_sb_prependf(&sb, "Hello, %s!", "World");`
 */
void ds_sb_prependf(DsString *sb, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (n > 0) {
        ds_da_reserve(sb, sb->length + n + 1);
        char c0 = sb->data[0];
        memmove(sb->data + n, sb->data, sb->length);
        va_start(args, fmt);
        vsnprintf(sb->data, n + 1, fmt, args);
        sb->data[n] = c0;
        va_end(args);
        sb->length += n;
    }
}

/**
 * Append strings to a dynamic string builder.
 * Example:
 *   `ds_sb_append(&sb, "Hello, ", "World", ...);`
 */
#define ds_sb_append(sb, ...) _ds_sb_append(sb __VA_OPT__(, ) __VA_ARGS__, NULL)

/**
 * Insert a string at a specific index in the string builder.
 */
void ds_sb_insert(DsString *sb, const char *str, size_t index) {
    if (!sb || !str || index > sb->length) return;
    size_t len = strlen(str);
    ds_da_reserve(sb, sb->length + len + 1);
    memmove(sb->data + index + len, sb->data + index, sb->length - index);
    memcpy(sb->data + index, str, len);
    sb->length += len;
}

/**
 * Prepend a string to the string builder.
 */
#define ds_sb_prepend(sb, str) ds_sb_insert((sb), (str), 0)

/**
 * Check if a substring is included in the string builder.
 */
bool ds_sb_include(const DsString *sb, const char *substr) {
    if (!sb || !substr || sb->length == 0 || strlen(substr) == 0) return false;
    return strstr(sb->data, substr) != NULL;
}

/**
 * Trim leading whitespace from the string builder.
 */
DsString *ds_sb_ltrim(DsString *sb) {
    if (sb && sb->length > 0) {
        size_t i = 0;
        while (i < sb->length && (sb->data[i] == ' ' || sb->data[i] == '\t' || sb->data[i] == '\n' || sb->data[i] == '\r')) {
            i++;
        }
        if (i > 0) {
            memmove(sb->data, sb->data + i, sb->length - i + 1);
            sb->length -= i;
        }
    }
    return sb;
}

/**
 * Trim trailing whitespace from the string builder.
 */
DsString *ds_sb_rtrim(DsString *sb) {
    if (sb && sb->length > 0) {
        size_t i = sb->length;
        while (i > 0 && (sb->data[i - 1] == ' ' || sb->data[i - 1] == '\t' || sb->data[i - 1] == '\n' || sb->data[i - 1] == '\r')) {
            i--;
        }
        if (i < sb->length) {
            sb->data[i] = '\0';
            sb->length = i;
        }
    }
    return sb;
}

/**
 * Trim whitespace from the string builder.
 */
inline DsString *ds_sb_trim(DsString *sb) {
    return ds_sb_rtrim(ds_sb_ltrim(sb));
}

#ifndef DS_HM_LOAD_FACTOR
#define DS_HM_LOAD_FACTOR 0.75f
#endif

size_t _ds_hash_pointer(const void *key) {
    return (size_t)key;
}

size_t _ds_hash_int(int key) {
    uint32_t k = (uint32_t)key;
    k = (k ^ 61) ^ (k >> 16);
    k = k + (k << 3);
    k = k ^ (k >> 4);
    k = k * 0x27d4eb2d;
    k = k ^ (k >> 15);
    return (size_t)k;
}
size_t _ds_hash_long(long key) {
    uint64_t k = (uint64_t)key;
    k = (~k) + (k << 21);
    k = k ^ (k >> 24);
    k = (k + (k << 3)) + (k << 8);
    k = k ^ (k >> 14);
    k = (k + (k << 2)) + (k << 4);
    k = k ^ (k >> 28);
    k = k + (k << 31);
    return (size_t)k;
}

size_t _ds_hash_float(double key) {
    union {
        double f;
        uint64_t u;
    } tmp;
    tmp.f = key;
    return _ds_hash_long(tmp.u);
}

size_t _ds_hash_string(const char *key) {
    if (!key) return 0;
    size_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int _ds_eq_int(int a, int b) {
    return a == b;
}

int _ds_eq_long(long a, long b) {
    return a == b;
}

int _ds_eq_float(double a, double b) {
    return a == b;
}

int _ds_eq_str(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

int _ds_eq_ptr(const void *a, const void *b) {
    return a == b;
}

#define _ds_eq_fn(a, b) _Generic((a), \
    char *: _ds_eq_str,               \
    const char *: _ds_eq_str,         \
    int: _ds_eq_int,                  \
    long: _ds_eq_long,                \
    float: _ds_eq_float,              \
    double: _ds_eq_float,             \
    default: _ds_eq_ptr)(a, b)

#define _ds_hash_fn(key) _Generic((key), \
    char *: _ds_hash_string,             \
    const char *: _ds_hash_string,       \
    int: _ds_hash_int,                   \
    long: _ds_hash_long,                 \
    float: _ds_hash_float,               \
    double: _ds_hash_float,              \
    default: _ds_hash_pointer)(key)

#define _ds_hm_hfn(hm, key_v) ((hm)->hfn ? (hm)->hfn(key_v) : _ds_hash_fn(key_v))
#define _ds_hm_eqfn(hm, a, b) ((hm)->eqfn ? (hm)->eqfn(a, b) : _ds_eq_fn(a, b))

/**
 * Declare a hash map.
 * Example:
 *   `ds_hm_declare(my_map, int, const char *);`
 *
```c
This will create a hash map of int keys and const char values like this:
// Key-Value pair structure
typedef struct {
    int key;
    const char *value;
} my_map_Kv;
// Dynamic array of key-value pairs
typedef struct {
    my_map_Kv *data;
    size_t length;
    size_t capacity;
} my_map_Da;
// Hash map structure
typedef struct {
    my_map_Da *table;
    size_t (*hfn)(int);
    int (*eqfn)(int, int);
    size_t size;
} my_map;
```
 * You can customize the hash function and equality function for your specific key type.
 * `my_map hm = {.hfn = my_hash_function, .eqfn = my_eq_function};`
 * The hash function should follow the signature `size_t hash_function(key_t key);`
 * The equality function should follow the signature `int eq_function(key_t a, key_t b);`
 */
#define ds_hm_declare(name, key_t, val_t)   \
    typedef struct {                        \
        key_t key;                          \
        val_t value;                        \
    } name##_Kv;                            \
    ds_da_declare(name##_Da, name##_Kv);    \
    ds_da_declare(name##_Table, name##_Da); \
    typedef struct {                        \
        name##_Table table;                 \
        size_t (*hfn)(key_t);               \
        int (*eqfn)(key_t, key_t);          \
        size_t size;                        \
    } name

#define _ds_hm_resize(hm)                                                                     \
    do {                                                                                      \
        size_t new_capacity = (hm)->table.length == 0 ? 1 : (hm)->table.length * 2;           \
        __typeof__((hm)->table) new_table = {0};                                              \
        ds_da_reserve(&new_table, new_capacity);                                              \
        new_capacity = new_table.length = new_table.capacity;                                 \
        memset(new_table.data, 0, new_capacity * sizeof(*new_table.data));                    \
        for (size_t _i = 0; _i < (hm)->table.length; _i++) {                                  \
            for (size_t _j = 0; _j < (hm)->table.data[_i].length; _j++) {                     \
                __typeof__((hm)->table.data[_i].data[_j]) kv = (hm)->table.data[_i].data[_j]; \
                size_t _h = _ds_hm_hfn((hm), kv.key) % new_capacity;                          \
                ds_da_append(&new_table.data[_h], kv);                                        \
            }                                                                                 \
        }                                                                                     \
        ds_da_free(&(hm)->table);                                                             \
        (hm)->table = new_table;                                                              \
    } while (0)

/**
 * Set a key-value pair in the hash map.
 * Example:
```c
ds_hm_declare(my_map, int, const char *);
...
    my_map hm = {0};
    ds_hm_set(&hm, 42, "Hello");
```
 */
#define ds_hm_set(hm, key_v, val_v)                                                            \
    do {                                                                                       \
        if ((hm)->table.length == 0 || (hm)->size >= (hm)->table.length * DS_HM_LOAD_FACTOR) { \
            _ds_hm_resize(hm);                                                                 \
        }                                                                                      \
        __typeof__(*(hm)->table.data[0].data) kv = {.key = key_v, .value = val_v};             \
        size_t _hash = _ds_hm_hfn((hm), (kv).key) % (hm)->table.length;                        \
        size_t _i;                                                                             \
        for (_i = 0; _i < (hm)->table.data[_hash].length; _i++) {                              \
            if (_ds_hm_eqfn((hm), (hm)->table.data[_hash].data[_i].key, (kv).key)) {           \
                (hm)->table.data[_hash].data[_i].value = (kv).value;                           \
                break;                                                                         \
            }                                                                                  \
        }                                                                                      \
        if (_i == (hm)->table.data[_hash].length) {                                            \
            ds_da_append(&(hm)->table.data[_hash], kv);                                        \
            (hm)->size++;                                                                      \
        }                                                                                      \
    } while (0)

/**
 * Try to get a value from the hash map.
 * Returns NULL if the key is not found else it returns a pointer to the value.
 * Example:
 ```c
 ds_hm_declare(my_map, int, const char *);
 ...
    const char **value = ds_hm_try(&hm, 42);
    printf("%s\n", *value);
 ```
 */
#define ds_hm_try(hm, key_v)                                                            \
    ({                                                                                  \
        __typeof__(&(hm)->table.data[0].data[0].value) _val = NULL;                     \
        if ((hm)->table.length) {                                                       \
            size_t _hash = _ds_hm_hfn((hm), key_v) % (hm)->table.length;                \
            for (size_t _i = 0; _i < (hm)->table.data[_hash].length; _i++) {            \
                if (_ds_hm_eqfn((hm), (hm)->table.data[_hash].data[_i].key, (key_v))) { \
                    _val = &(hm)->table.data[_hash].data[_i].value;                     \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
        }                                                                               \
        _val;                                                                           \
    })

/**
 * Get a value from the hash map. It will return the value associated with the key
 * The result will be {0} if the key is not found
 * You should use ds_hm_try if you are not really sure the key is present
 * Example:
 ```c
 const char *value = ds_hm_get(&hm, 42);
 ```
 */
#define ds_hm_get(hm, key_v)                                                        \
    ({                                                                              \
        __typeof__((hm)->table.data[0].data[0].value) _val = {0};                   \
        size_t _hash = _ds_hm_hfn((hm), key_v) % (hm)->table.length;                \
        for (size_t _i = 0; _i < (hm)->table.data[_hash].length; _i++) {            \
            if (_ds_hm_eqfn((hm), (hm)->table.data[_hash].data[_i].key, (key_v))) { \
                _val = (hm)->table.data[_hash].data[_i].value;                      \
                break;                                                              \
            }                                                                       \
        }                                                                           \
        _val;                                                                       \
    })

/**
 * Remove a value from the hash map and return a pointer to it, or NULL if not found.
 * Example:
 ```c
 const char **value = ds_hm_remove(&hm, 42);
 ```
 */
#define ds_hm_remove(hm, key_v)                                                   \
    ({                                                                            \
        __typeof__(&(hm)->table.data[0].data[0].value) _val = NULL;               \
        size_t _hash = _ds_hm_hfn((hm), key_v) % (hm)->table.length;              \
        for (size_t _i = 0; _i < (hm)->table.data[_hash].length; _i++) {          \
            if (_ds_hm_eqfn((hm), (hm)->table.data[_hash].data[_i].key, key_v)) { \
                _val = &(hm)->table.data[_hash].data[_i].value;                   \
                ds_da_remove(&(hm)->table.data[_hash], _i, 1);                    \
                (hm)->size--;                                                     \
            }                                                                     \
        }                                                                         \
        _val;                                                                     \
    })

/**
 * Loop over all key-value pairs in the hash map.
 * It will define `key` and `value` as pointers to the current key-value pair.
 * Example:
```c
ds_hm_foreach(&hm, key, value) {
    printf("Key: %d, Value: %s\n", *key, *value);
}
```
 */
#define ds_hm_foreach(hm, key_var, val_var)                                       \
    __typeof__(&(hm)->table.data[0].data[0].key) key_var;                         \
    __typeof__(&(hm)->table.data[0].data[0].value) val_var;                       \
    for (size_t _i = 0; _i < (hm)->table.length; _i++)                            \
        for (size_t _j = 0; _j < (hm)->table.data[_i].length &&                   \
                            ((key_var) = &(hm)->table.data[_i].data[_j].key,      \
                            (val_var) = &(hm)->table.data[_i].data[_j].value, 1); \
             _j++)

/**
 * Free the hash map.
 * It will not free the keys or values themselves.
 * You should free the keys and values separately if needed.
 */
#define ds_hm_free(hm)                                       \
    do {                                                     \
        for (size_t _i = 0; _i < (hm)->table.length; _i++) { \
            ds_da_free(&(hm)->table.data[_i]);               \
        }                                                    \
        ds_da_free(&(hm)->table);                            \
    } while (0)

/**
 * Declare a linked list.
 * The linked list will store elements of the specified type.
 * Example:
 * `ds_ll_declare(my_list, int);`
 *
 * This will create a linked list type called `my_list` that stores `int` values like this:
```c
typedef struct my_list_Node {
    int val;
    struct my_list_Node *next;
} my_list_Node;

typedef struct {
    my_list_Node *head;
    my_list_Node *tail;
    size_t size;
} my_list;
```
 */
#define ds_ll_declare(name, type) \
    typedef struct name##_Node {  \
        type val;                 \
        struct name##_Node *next; \
    } name##_Node;                \
    typedef struct {              \
        name##_Node *head;        \
        name##_Node *tail;        \
        size_t size;              \
    } name

/**
 * Push a new value onto the front of the linked list.
 * Example:
 ```c
 ds_ll_declare(my_list, int);
 ...
     my_list l = {0};
     ds_ll_push(&l, 42);
 ```
 */
#define ds_ll_push(ll, value)                       \
    do {                                            \
        __typeof__((ll)->head) _n = (ll)->head;     \
        (ll)->head = DS_ALLOC(sizeof(*(ll)->head)); \
        assert((ll)->head != NULL);                 \
        (ll)->head->val = (value);                  \
        (ll)->head->next = _n;                      \
        if (!(ll)->tail) {                          \
            (ll)->tail = (ll)->head;                \
        }                                           \
        (ll)->size++;                               \
    } while (0)

/**
 * Append a new value to the end of the linked list.
 * Example:
 ```c
 ds_ll_declare(my_list, int);
 ...
    my_list l = {0};
    ds_ll_append(&l, 42);
 ```
 */
#define ds_ll_append(ll, v)                                        \
    do {                                                           \
        __typeof__((ll)->head) _n = DS_ALLOC(sizeof(*(ll)->head)); \
        assert(_n != NULL);                                        \
        if (!(ll)->tail) {                                         \
            (ll)->head = (ll)->tail = _n;                          \
        } else {                                                   \
            (ll)->tail->next = _n;                                 \
            (ll)->tail = _n;                                       \
        }                                                          \
        (ll)->tail->val = (v);                                     \
        (ll)->size++;                                              \
    } while (0)

/**
 * Pop the front value from the linked list. Returns the popped value.
 * Example:
 * ```
 * int value = ds_ll_pop(&my_list);
 * ```
 */
#define ds_ll_pop(ll)                             \
    ({                                            \
        assert((ll)->head != NULL);               \
        __typeof__((ll)->head) _old = (ll)->head; \
        (ll)->head = (ll)->head->next;            \
        if (!(ll)->head) {                        \
            (ll)->tail = NULL;                    \
        }                                         \
        (ll)->size--;                             \
        _old;                                     \
    })

/**
 * Free the linked list.
 */
#define ds_ll_free(ll)          \
    while ((ll)->head) {        \
        DS_FREE(ds_ll_pop(ll)); \
    }

/**
 * Read the entire contents of a file into a string builder.
 * Example:
```c
DsString sb = {0};
ds_read_entire_file("path/to/file.txt", &sb);
```
 */
bool ds_read_entire_file(const char *path, DsString *sb) {
    bool result = false;

    FILE *f = fopen(path, "rb");
    if (f == NULL) goto cleanup;
    if (fseek(f, 0, SEEK_END) < 0) goto cleanup;
    long s = ftell(f);
    if (s < 0) goto cleanup;
    if (fseek(f, 0, SEEK_SET) < 0) goto cleanup;

    size_t size = sb->length + s;
    ds_da_reserve(sb, size);
    fread(sb->data + sb->length, s, 1, f);
    if (ferror(f)) goto cleanup;
    sb->length = size;
    result = true;
cleanup:
    if (!result)
        ds_log(DS_ERROR, "Could not read file %s: %s", path, strerror(errno));
    if (f)
        fclose(f);
    return result;
}

/**
 * Write the entire contents of a string builder to a file.
 * Example:
```c
DsString sb = {0};
ds_read_entire_file("path/to/file.txt", &sb);
ds_write_entire_file("path/to/output.txt", &sb);
```
 */
bool ds_write_entire_file(const char *path, const DsString *sb) {
    bool result = false;
    FILE *f = fopen(path, "wb");
    if (f == NULL) goto cleanup;

    const char *buf = sb->data;
    size_t size = sb->length;
    while (size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if (ferror(f)) goto cleanup;
        size -= n;
        buf += n;
    }
    result = true;
cleanup:
    if (!result) ds_log(DS_ERROR, "Could not write file %s: %s\n", path, strerror(errno));
    if (f) fclose(f);
    return result;
}

typedef struct {
    const char *data;
    size_t length;
} DsStringIterator;

DsStringIterator ds_sb_iter(const DsString *sb) {
    return (DsStringIterator){.data = sb->data, .length = sb->length};
}
DsStringIterator ds_cstr_iter(const char *data) {
    return (DsStringIterator){.data = data, .length = strlen(data)};
}

/**
 * Move a string iterator by a delimiter.
 * Returns the next part of the string and updates the iterator.
 * It will not allocate any memory or modify the original string.
 * The part will not be null-terminated, so you should use the length to access it.
 * Example:
 ```c
 DsStringIterator it = ds_cstr_iter("path/to/file.txt");
 while (it.length > 0) {
    // it will be updated to point to the next part
    DsStringIterator part = ds_str_split(&it, '/');
    // part will be {data: "path/to/file.txt", length: 4} -> {data: "to/file.txt", length: 2} -> {data: "file.txt", length: 8}
    // Do something with part
 }
```
 */
DsStringIterator ds_str_split(DsStringIterator *it, char sep) {
    DsStringIterator part = {0};
    if (it->length == 0) return part;
    size_t i = 0;
    while (i < it->length && it->data[i] != sep)
        i++;
    part.data = it->data;
    part.length = i;
    if (i < it->length) {
        it->data += i + 1;
        it->length -= i + 1;
    } else {
        it->data += i;
        it->length -= i;
    }
    return part;
}

/**
 * Create a directory and all parent directories if they don't exist.
 * Example:
 * `ds_mkdir_p("path/to/directory");`
 */
bool ds_mkdir_p(const char *path) {
    DsStringIterator iter = ds_cstr_iter(path);
    DsString tmp_path = {0};
    if (iter.length && iter.data[0] == '/') ds_da_append(&tmp_path, '/');

    while (iter.length) {
        DsStringIterator part = ds_str_split(&iter, '/');
        if (part.length == 0) continue;
        if (part.length == 1 && part.data[0] == '.') continue;
        ds_da_append_many(&tmp_path, part.data, part.length);
        ds_sb_append(&tmp_path, "/");
        if (mkdir(tmp_path.data, 0755) < 0) {
            if (errno != EEXIST) {
                ds_log(DS_ERROR, "Could not create directory `%s`: %s", tmp_path.data,
                       strerror(errno));
                ds_da_free(&tmp_path);
                return false;
            }
        }
    }
    ds_da_free(&tmp_path);
    return true;
}

#ifdef DS_NO_PREFIX
// Strip prefixes
#define UNREACHABLE DS_UNREACHABLE
#define TODO DS_TODO
#define UNUSED DS_UNUSED
#define log ds_log
#define log_info ds_log_info
#define log_debug ds_log_debug
#define log_warn ds_log_warn
#define log_error ds_log_error
#define DEBUG DS_DEBUG
#define INFO DS_INFO
#define WARN DS_WARN
#define ERROR DS_ERROR
#define da_declare ds_da_declare
#define da_reserve ds_da_reserve
#define da_append ds_da_append
#define da_remove ds_da_remove
#define da_insert ds_da_insert
#define da_prepend ds_da_prepend
#define da_pop ds_da_pop
#define da_append_many ds_da_append_many
#define da_last ds_da_last
#define da_first ds_da_first
#define da_zero ds_da_zero
#define da_free ds_da_free
#define da_foreach ds_da_foreach
#define da_find ds_da_find
#define da_index_of ds_da_index_of
#define sb_append ds_sb_append
#define sb_appendf ds_sb_appendf
#define sb_prependf ds_sb_prependf
#define sb_insert ds_sb_insert
#define sb_prepend ds_sb_prepend
#define sb_include ds_sb_include
#define sb_ltrim ds_sb_ltrim
#define sb_rtrim ds_sb_rtrim
#define sb_trim ds_sb_trim
#define hm_declare ds_hm_declare
#define hm_get ds_hm_get
#define hm_try ds_hm_try
#define hm_set ds_hm_set
#define hm_remove ds_hm_remove
#define hm_foreach ds_hm_foreach
#define hm_free ds_hm_free
#define ll_declare ds_ll_declare
#define ll_push ds_ll_push
#define ll_append ds_ll_append
#define ll_pop ds_ll_pop
#define ll_free ds_ll_free
#define String DsString
#define read_entire_file ds_read_entire_file
#define write_entire_file ds_write_entire_file
#define StringIterator DsStringIterator
#define str_split ds_str_split
#define sb_iter ds_sb_iter
#define cstr_iter ds_cstr_iter
#define mkdir_p ds_mkdir_p
#endif

#endif // DS_H_
