#define DS_IMPLEMENTATION
#include "../ds.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                          \
    do {                                                    \
        tests_run++;                                        \
        printf("  %-60s", name);                            \
    } while (0)

#define PASS()                                              \
    do {                                                    \
        tests_passed++;                                     \
        printf("\033[32mPASS\033[0m\n");                    \
    } while (0)

#define FAIL(msg)                                           \
    do {                                                    \
        tests_failed++;                                     \
        printf("\033[31mFAIL\033[0m: %s\n", msg);           \
    } while (0)

#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) ASSERT((a) == (b), msg)
#define ASSERT_NEQ(a, b, msg) ASSERT((a) != (b), msg)
#define ASSERT_STR(a, b, msg) ASSERT(strcmp((a), (b)) == 0, msg)

#define SECTION(name) printf("\n\033[1m[%s]\033[0m\n", name)

// ============================================================================
// Dynamic Array Tests
// ============================================================================

ds_da_declare(IntArray, int);
ds_da_declare(PtrArray, char*);

void test_da_init_zero(void) {
    TEST("da: zero-initialized state");
    IntArray a = {0};
    ASSERT_EQ(a.length, 0, "length should be 0");
    ASSERT_EQ(a.capacity, 0, "capacity should be 0");
    ASSERT_EQ(a.data, NULL, "data should be NULL");
    PASS();
}

void test_da_append_single(void) {
    TEST("da: append single element");
    IntArray a = {0};
    ds_da_append(&a, 42);
    ASSERT_EQ(a.length, 1, "length should be 1");
    ASSERT_EQ(a.data[0], 42, "element should be 42");
    ASSERT(a.capacity >= 1, "capacity should be >= 1");
    ds_da_free(&a);
    PASS();
}

void test_da_append_many_elements(void) {
    TEST("da: append triggers multiple resizes");
    IntArray a = {0};
    for (int i = 0; i < 1000; i++) {
        ds_da_append(&a, i);
    }
    ASSERT_EQ(a.length, 1000, "length should be 1000");
    for (int i = 0; i < 1000; i++) {
        ASSERT_EQ(a.data[i], i, "element mismatch");
    }
    ds_da_free(&a);
    PASS();
}

void test_da_append_many_batch(void) {
    TEST("da: append_many copies multiple items");
    IntArray a = {0};
    int items[] = {10, 20, 30, 40, 50};
    ds_da_append_many(&a, items, 5);
    ASSERT_EQ(a.length, 5, "length should be 5");
    ASSERT_EQ(a.data[0], 10, "first element");
    ASSERT_EQ(a.data[4], 50, "last element");
    ds_da_free(&a);
    PASS();
}

void test_da_append_many_zero_size(void) {
    TEST("da: append_many with size 0 is no-op");
    IntArray a = {0};
    ds_da_append(&a, 1);
    ds_da_append_many(&a, (int[]){99}, 0);
    ASSERT_EQ(a.length, 1, "length should still be 1");
    ds_da_free(&a);
    PASS();
}

void test_da_pop(void) {
    TEST("da: pop returns last element");
    IntArray a = {0};
    ds_da_append(&a, 1);
    ds_da_append(&a, 2);
    ds_da_append(&a, 3);
    int v = ds_da_pop(&a);
    ASSERT_EQ(v, 3, "popped value should be 3");
    ASSERT_EQ(a.length, 2, "length should be 2");
    v = ds_da_pop(&a);
    ASSERT_EQ(v, 2, "popped value should be 2");
    ds_da_free(&a);
    PASS();
}

void test_da_remove_middle(void) {
    TEST("da: remove from middle preserves order");
    IntArray a = {0};
    for (int i = 1; i <= 5; i++) ds_da_append(&a, i);
    // [1,2,3,4,5] -> remove idx=1 count=2 -> [1,4,5]
    ds_da_remove(&a, 1, 2);
    ASSERT_EQ(a.length, 3, "length should be 3");
    ASSERT_EQ(a.data[0], 1, "first");
    ASSERT_EQ(a.data[1], 4, "second");
    ASSERT_EQ(a.data[2], 5, "third");
    ds_da_free(&a);
    PASS();
}

void test_da_remove_first(void) {
    TEST("da: remove first element");
    IntArray a = {0};
    for (int i = 1; i <= 3; i++) ds_da_append(&a, i);
    ds_da_remove(&a, 0, 1);
    ASSERT_EQ(a.length, 2, "length should be 2");
    ASSERT_EQ(a.data[0], 2, "first should be 2");
    ds_da_free(&a);
    PASS();
}

void test_da_remove_last(void) {
    TEST("da: remove last element");
    IntArray a = {0};
    for (int i = 1; i <= 3; i++) ds_da_append(&a, i);
    ds_da_remove(&a, 2, 1);
    ASSERT_EQ(a.length, 2, "length should be 2");
    ASSERT_EQ(a.data[1], 2, "last should be 2");
    ds_da_free(&a);
    PASS();
}

void test_da_remove_all(void) {
    TEST("da: remove all elements at once");
    IntArray a = {0};
    for (int i = 1; i <= 5; i++) ds_da_append(&a, i);
    ds_da_remove(&a, 0, 5);
    ASSERT_EQ(a.length, 0, "length should be 0");
    ds_da_free(&a);
    PASS();
}

void test_da_remove_unordered(void) {
    TEST("da: remove_unordered swaps with last");
    IntArray a = {0};
    for (int i = 1; i <= 5; i++) ds_da_append(&a, i);
    // [1,2,3,4,5] -> remove idx=1 -> [1,5,3,4]
    ds_da_remove_unordered(&a, 1);
    ASSERT_EQ(a.length, 4, "length should be 4");
    ASSERT_EQ(a.data[0], 1, "first should be 1");
    ASSERT_EQ(a.data[1], 5, "swapped element should be 5");
    ds_da_free(&a);
    PASS();
}

void test_da_insert_middle(void) {
    TEST("da: insert in middle");
    IntArray a = {0};
    for (int i = 1; i <= 3; i++) ds_da_append(&a, i);
    size_t idx = 1;
    ds_da_insert(&a, idx, 99);
    ASSERT_EQ(a.length, 4, "length should be 4");
    ASSERT_EQ(a.data[0], 1, "first");
    ASSERT_EQ(a.data[1], 99, "inserted");
    ASSERT_EQ(a.data[2], 2, "shifted");
    ASSERT_EQ(a.data[3], 3, "last");
    ds_da_free(&a);
    PASS();
}

void test_da_insert_at_start(void) {
    TEST("da: insert at index 0 (prepend)");
    IntArray a = {0};
    ds_da_append(&a, 2);
    ds_da_append(&a, 3);
    size_t idx = 0;
    ds_da_insert(&a, idx, 1);
    ASSERT_EQ(a.data[0], 1, "first should be 1");
    ASSERT_EQ(a.data[1], 2, "second");
    ASSERT_EQ(a.data[2], 3, "third");
    ds_da_free(&a);
    PASS();
}

void test_da_insert_beyond_length(void) {
    TEST("da: insert beyond length clamps to end");
    IntArray a = {0};
    ds_da_append(&a, 1);
    size_t idx = 100;
    ds_da_insert(&a, idx, 99);
    ASSERT_EQ(a.length, 2, "length should be 2");
    ASSERT_EQ(a.data[1], 99, "should be at end");
    ds_da_free(&a);
    PASS();
}

void test_da_prepend(void) {
    TEST("da: prepend adds to front");
    IntArray a = {0};
    ds_da_append(&a, 2);
    ds_da_append(&a, 3);
    ds_da_prepend(&a, 1);
    ASSERT_EQ(a.data[0], 1, "first should be 1");
    ASSERT_EQ(a.length, 3, "length should be 3");
    ds_da_free(&a);
    PASS();
}

void test_da_first_last(void) {
    TEST("da: first/last pointers");
    IntArray a = {0};
    ASSERT_EQ(ds_da_first(&a), NULL, "first on empty should be NULL");
    ASSERT_EQ(ds_da_last(&a), NULL, "last on empty should be NULL");
    ds_da_append(&a, 10);
    ds_da_append(&a, 20);
    ds_da_append(&a, 30);
    ASSERT_EQ(*ds_da_first(&a), 10, "first should be 10");
    ASSERT_EQ(*ds_da_last(&a), 30, "last should be 30");
    ds_da_free(&a);
    PASS();
}

void test_da_find(void) {
    TEST("da: find element by expression");
    IntArray a = {0};
    for (int i = 0; i < 10; i++) ds_da_append(&a, i * 10);
    int *found = ds_da_find(&a, *e == 50);
    ASSERT_NEQ(found, NULL, "should find 50");
    ASSERT_EQ(*found, 50, "found value should be 50");
    int *not_found = ds_da_find(&a, *e == 999);
    ASSERT_EQ(not_found, NULL, "should not find 999");
    ds_da_free(&a);
    PASS();
}

void test_da_index_of(void) {
    TEST("da: index_of returns correct index or -1");
    IntArray a = {0};
    for (int i = 0; i < 5; i++) ds_da_append(&a, i * 10);
    int idx = ds_da_index_of(&a, *e == 30);
    ASSERT_EQ(idx, 3, "index of 30 should be 3");
    idx = ds_da_index_of(&a, *e == 999);
    ASSERT_EQ(idx, -1, "not found should be -1");
    ds_da_free(&a);
    PASS();
}

void test_da_foreach(void) {
    TEST("da: foreach iterates all elements");
    IntArray a = {0};
    for (int i = 0; i < 5; i++) ds_da_append(&a, i);
    int sum = 0;
    ds_da_foreach(&a, item) { sum += *item; }
    ASSERT_EQ(sum, 10, "sum should be 0+1+2+3+4=10");
    ds_da_free(&a);
    PASS();
}

void test_da_foreach_idx(void) {
    TEST("da: foreach_idx iterates with index");
    IntArray a = {0};
    for (int i = 0; i < 5; i++) ds_da_append(&a, i * 2);
    int sum = 0;
    ds_da_foreach_idx(&a, i) { sum += a.data[i]; }
    ASSERT_EQ(sum, 20, "sum should be 0+2+4+6+8=20");
    ds_da_free(&a);
    PASS();
}

void test_da_reserve(void) {
    TEST("da: reserve allocates ahead without changing length");
    IntArray a = {0};
    ds_da_reserve(&a, 100);
    ASSERT(a.capacity >= 100, "capacity should be >= 100");
    ASSERT_EQ(a.length, 0, "length should still be 0");
    ds_da_free(&a);
    PASS();
}

void test_da_zero_vs_free(void) {
    TEST("da: zero resets without freeing, free frees");
    IntArray a = {0};
    for (int i = 0; i < 10; i++) ds_da_append(&a, i);
    int *old_data = a.data;
    ds_da_zero(&a);
    ASSERT_EQ(a.length, 0, "length should be 0");
    ASSERT_EQ(a.data, NULL, "data should be NULL");
    // Note: old_data is leaked here intentionally for testing
    free(old_data);
    PASS();
}

void test_da_free_idempotent(void) {
    TEST("da: free on empty array is safe");
    IntArray a = {0};
    ds_da_free(&a); // should not crash
    ds_da_free(&a); // double free on empty should be safe
    PASS();
}

// ============================================================================
// String Builder Tests
// ============================================================================

void test_str_append_basic(void) {
    TEST("str: append basic strings");
    DsString s = {0};
    ds_str_append(&s, "hello", " ", "world");
    ASSERT_EQ(s.length, 11, "length should be 11");
    ASSERT_STR(s.data, "hello world", "content mismatch");
    ds_da_free(&s);
    PASS();
}

void test_str_append_empty(void) {
    TEST("str: append empty string");
    DsString s = {0};
    ds_str_append(&s, "");
    ASSERT_EQ(s.length, 0, "length should be 0");
    ds_da_free(&s);
    PASS();
}

void test_str_append_incremental(void) {
    TEST("str: multiple incremental appends");
    DsString s = {0};
    for (int i = 0; i < 100; i++) {
        ds_str_append(&s, "a");
    }
    ASSERT_EQ(s.length, 100, "length should be 100");
    for (size_t i = 0; i < s.length; i++) {
        ASSERT_EQ(s.data[i], 'a', "all chars should be 'a'");
    }
    ds_da_free(&s);
    PASS();
}

void test_str_appendf(void) {
    TEST("str: appendf formatted string");
    DsString s = {0};
    ds_str_appendf(&s, "num=%d str=%s", 42, "test");
    ASSERT_STR(s.data, "num=42 str=test", "formatted content mismatch");
    ds_da_free(&s);
    PASS();
}

void test_str_appendf_multiple(void) {
    TEST("str: appendf multiple times concatenates");
    DsString s = {0};
    ds_str_appendf(&s, "a=%d", 1);
    ds_str_appendf(&s, " b=%d", 2);
    ASSERT_STR(s.data, "a=1 b=2", "content mismatch");
    ds_da_free(&s);
    PASS();
}

void test_str_prependf(void) {
    TEST("str: prependf inserts at beginning");
    DsString s = {0};
    ds_str_append(&s, "world");
    ds_str_prependf(&s, "hello ");
    ASSERT_EQ(s.length, 11, "length should be 11");
    ASSERT_STR(s.data, "hello world", "content mismatch");
    ds_da_free(&s);
    PASS();
}

void test_str_prependf_to_empty(void) {
    TEST("str: prependf to empty string");
    DsString s = {0};
    ds_str_prependf(&s, "test=%d", 42);
    ASSERT_EQ(s.length, 7, "length should be 7");
    ASSERT_STR(s.data, "test=42", "content mismatch");
    ds_da_free(&s);
    PASS();
}

void test_str_insert_middle(void) {
    TEST("str: insert in the middle");
    DsString s = {0};
    ds_str_append(&s, "helo");
    ds_str_insert(&s, "l", 3);
    ASSERT_STR(s.data, "hello", "content mismatch");
    ASSERT_EQ(s.length, 5, "length should be 5");
    ds_da_free(&s);
    PASS();
}

void test_str_insert_at_start(void) {
    TEST("str: insert at index 0 (prepend)");
    DsString s = {0};
    ds_str_append(&s, "world");
    ds_str_insert(&s, "hello ", 0);
    ASSERT_STR(s.data, "hello world", "content mismatch");
    ds_da_free(&s);
    PASS();
}

void test_str_insert_at_end(void) {
    TEST("str: insert at end");
    DsString s = {0};
    ds_str_append(&s, "hello");
    ds_str_insert(&s, " world", s.length);
    ASSERT_STR(s.data, "hello world", "content mismatch");
    ds_da_free(&s);
    PASS();
}

void test_str_insert_beyond_length(void) {
    TEST("str: insert beyond length is ignored");
    DsString s = {0};
    ds_str_append(&s, "abc");
    ds_str_insert(&s, "x", 100);
    ASSERT_EQ(s.length, 3, "length should stay 3");
    ASSERT_STR(s.data, "abc", "content unchanged");
    ds_da_free(&s);
    PASS();
}

void test_str_include(void) {
    TEST("str: include finds substring");
    DsString s = {0};
    ds_str_append(&s, "hello world");
    ASSERT(ds_str_include(&s, "world"), "should find 'world'");
    ASSERT(ds_str_include(&s, "hello"), "should find 'hello'");
    ASSERT(!ds_str_include(&s, "xyz"), "should not find 'xyz'");
    ASSERT(!ds_str_include(&s, ""), "empty substr returns false");
    ds_da_free(&s);
    PASS();
}

void test_str_include_empty_string(void) {
    TEST("str: include on empty string");
    DsString s = {0};
    ASSERT(!ds_str_include(&s, "x"), "should return false on empty");
    PASS();
}

void test_str_ltrim(void) {
    TEST("str: ltrim removes leading whitespace");
    DsString s = {0};
    ds_str_append(&s, "  \t\nhello");
    ds_str_ltrim(&s);
    ASSERT_STR(s.data, "hello", "leading whitespace removed");
    ASSERT_EQ(s.length, 5, "length should be 5");
    ds_da_free(&s);
    PASS();
}

void test_str_rtrim(void) {
    TEST("str: rtrim removes trailing whitespace");
    DsString s = {0};
    ds_str_append(&s, "hello  \t\n");
    ds_str_rtrim(&s);
    ASSERT_STR(s.data, "hello", "trailing whitespace removed");
    ASSERT_EQ(s.length, 5, "length should be 5");
    ds_da_free(&s);
    PASS();
}

void test_str_trim(void) {
    TEST("str: trim removes both sides");
    DsString s = {0};
    ds_str_append(&s, " \t hello \n ");
    ds_str_trim(&s);
    ASSERT_STR(s.data, "hello", "both sides trimmed");
    ds_da_free(&s);
    PASS();
}

void test_str_trim_all_whitespace(void) {
    TEST("str: trim on all-whitespace string");
    DsString s = {0};
    ds_str_append(&s, "   \t\n\r  ");
    ds_str_trim(&s);
    ASSERT_EQ(s.length, 0, "length should be 0");
    ds_da_free(&s);
    PASS();
}

void test_str_trim_empty(void) {
    TEST("str: trim on empty string is safe");
    DsString s = {0};
    ds_str_ltrim(&s);
    ds_str_rtrim(&s);
    ASSERT_EQ(s.length, 0, "still empty");
    PASS();
}

// ============================================================================
// Hash Map Tests
// ============================================================================

ds_hm_declare(StrIntMap, const char*, int);
ds_hm_declare(IntIntMap, int, int);
ds_hm_declare(IntStrMap, int, const char*);

void test_hm_set_get_basic(void) {
    TEST("hm: set and get basic key-value");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    ds_hm_set(&hm, 2, 200);
    ds_hm_set(&hm, 3, 300);
    ASSERT_EQ(ds_hm_get(&hm, 1), 100, "key 1 -> 100");
    ASSERT_EQ(ds_hm_get(&hm, 2), 200, "key 2 -> 200");
    ASSERT_EQ(ds_hm_get(&hm, 3), 300, "key 3 -> 300");
    ASSERT_EQ(hm.length, 3, "length should be 3");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_overwrite(void) {
    TEST("hm: overwrite existing key");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    ds_hm_set(&hm, 1, 999);
    ASSERT_EQ(ds_hm_get(&hm, 1), 999, "value should be overwritten");
    ASSERT_EQ(hm.length, 1, "length should stay 1");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_try_found(void) {
    TEST("hm: try returns pointer when found");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 42, 123);
    int *val = ds_hm_try(&hm, 42);
    ASSERT_NEQ(val, NULL, "should find key");
    ASSERT_EQ(*val, 123, "value should be 123");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_try_not_found(void) {
    TEST("hm: try returns NULL when not found");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    int *val = ds_hm_try(&hm, 999);
    ASSERT_EQ(val, NULL, "should return NULL");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_try_empty(void) {
    TEST("hm: try on empty map returns NULL");
    IntIntMap hm = {0};
    int *val = ds_hm_try(&hm, 1);
    ASSERT_EQ(val, NULL, "should return NULL on empty");
    PASS();
}

void test_hm_has(void) {
    TEST("hm: has returns correct boolean");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    ASSERT(ds_hm_has(&hm, 1), "should have key 1");
    ASSERT(!ds_hm_has(&hm, 2), "should not have key 2");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_get_missing(void) {
    TEST("hm: get missing key returns zero");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    int val = ds_hm_get(&hm, 999);
    ASSERT_EQ(val, 0, "missing key should return 0");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_string_keys(void) {
    TEST("hm: string keys work correctly");
    StrIntMap hm = {0};
    ds_hm_set(&hm, "apple", 1);
    ds_hm_set(&hm, "banana", 2);
    ds_hm_set(&hm, "cherry", 3);
    ASSERT_EQ(ds_hm_get(&hm, "apple"), 1, "apple -> 1");
    ASSERT_EQ(ds_hm_get(&hm, "banana"), 2, "banana -> 2");
    ASSERT_EQ(ds_hm_get(&hm, "cherry"), 3, "cherry -> 3");
    ASSERT(!ds_hm_has(&hm, "grape"), "grape should not exist");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_many_entries_triggers_resize(void) {
    TEST("hm: many entries trigger resize correctly");
    IntIntMap hm = {0};
    for (int i = 0; i < 200; i++) {
        ds_hm_set(&hm, i, i * 10);
    }
    ASSERT_EQ(hm.length, 200, "length should be 200");
    for (int i = 0; i < 200; i++) {
        int val = ds_hm_get(&hm, i);
        ASSERT_EQ(val, i * 10, "value mismatch after resize");
    }
    ds_hm_free(&hm);
    PASS();
}

void test_hm_remove_basic(void) {
    TEST("hm: remove existing key");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    ds_hm_set(&hm, 2, 200);
    ds_hm_set(&hm, 3, 300);
    int *removed = ds_hm_remove(&hm, 2);
    ASSERT_NEQ(removed, NULL, "should return removed value");
    ASSERT_EQ(*removed, 200, "removed value should be 200");
    ASSERT_EQ(hm.length, 2, "length should be 2");
    ASSERT(!ds_hm_has(&hm, 2), "key 2 should be gone");
    // Remaining keys should still work
    ASSERT_EQ(ds_hm_get(&hm, 1), 100, "key 1 intact");
    ASSERT_EQ(ds_hm_get(&hm, 3), 300, "key 3 intact");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_remove_missing(void) {
    TEST("hm: remove missing key returns NULL");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    int *removed = ds_hm_remove(&hm, 999);
    ASSERT_EQ(removed, NULL, "should return NULL");
    ASSERT_EQ(hm.length, 1, "length unchanged");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_remove_and_reinsert(void) {
    TEST("hm: remove then reinsert same key");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 100);
    ds_hm_remove(&hm, 1);
    ASSERT(!ds_hm_has(&hm, 1), "key removed");
    ds_hm_set(&hm, 1, 999);
    ASSERT_EQ(ds_hm_get(&hm, 1), 999, "reinserted value");
    ASSERT_EQ(hm.length, 1, "length should be 1");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_remove_all_entries(void) {
    TEST("hm: remove all entries one by one");
    IntIntMap hm = {0};
    for (int i = 0; i < 50; i++) {
        ds_hm_set(&hm, i, i * 10);
    }
    for (int i = 0; i < 50; i++) {
        int *v = ds_hm_remove(&hm, i);
        ASSERT_NEQ(v, NULL, "should find key to remove");
    }
    ASSERT_EQ(hm.length, 0, "map should be empty");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_remove_stress_and_lookup(void) {
    TEST("hm: stress remove/insert maintains consistency");
    IntIntMap hm = {0};
    for (int i = 0; i < 100; i++) {
        ds_hm_set(&hm, i, i);
    }
    // Remove even entries
    for (int i = 0; i < 100; i += 2) {
        ds_hm_remove(&hm, i);
    }
    ASSERT_EQ(hm.length, 50, "50 entries left");
    for (int i = 0; i < 100; i++) {
        if (i % 2 == 0) {
            ASSERT(!ds_hm_has(&hm, i), "even key should be gone");
        } else {
            ASSERT(ds_hm_has(&hm, i), "odd key should exist");
            ASSERT_EQ(ds_hm_get(&hm, i), i, "odd value correct");
        }
    }
    ds_hm_free(&hm);
    PASS();
}

void test_hm_foreach(void) {
    TEST("hm: foreach iterates all pairs");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 1, 10);
    ds_hm_set(&hm, 2, 20);
    ds_hm_set(&hm, 3, 30);
    int sum_k = 0, sum_v = 0;
    ds_hm_foreach(&hm, kv) {
        sum_k += kv->key;
        sum_v += kv->value;
    }
    ASSERT_EQ(sum_k, 6, "sum of keys should be 6");
    ASSERT_EQ(sum_v, 60, "sum of values should be 60");
    ds_hm_free(&hm);
    PASS();
}

void test_hm_free_idempotent(void) {
    TEST("hm: free on empty map is safe");
    IntIntMap hm = {0};
    ds_hm_free(&hm);
    ds_hm_free(&hm);
    PASS();
}

void test_hm_negative_and_zero_keys(void) {
    TEST("hm: negative and zero integer keys");
    IntIntMap hm = {0};
    ds_hm_set(&hm, 0, 100);
    ds_hm_set(&hm, -1, 200);
    ds_hm_set(&hm, -999, 300);
    ASSERT_EQ(ds_hm_get(&hm, 0), 100, "key 0");
    ASSERT_EQ(ds_hm_get(&hm, -1), 200, "key -1");
    ASSERT_EQ(ds_hm_get(&hm, -999), 300, "key -999");
    ds_hm_free(&hm);
    PASS();
}

// ============================================================================
// Hash Set Tests
// ============================================================================

ds_hs_declare(IntSet, int);
ds_hs_declare(StrSet, const char*);

void test_hs_add_has(void) {
    TEST("hs: add and has");
    IntSet s = {0};
    ds_hs_add(&s, 1);
    ds_hs_add(&s, 2);
    ds_hs_add(&s, 3);
    ASSERT(ds_hs_has(&s, 1), "should have 1");
    ASSERT(ds_hs_has(&s, 2), "should have 2");
    ASSERT(ds_hs_has(&s, 3), "should have 3");
    ASSERT(!ds_hs_has(&s, 4), "should not have 4");
    ASSERT_EQ(s.length, 3, "length should be 3");
    ds_hs_free(&s);
    PASS();
}

void test_hs_add_duplicate(void) {
    TEST("hs: add duplicate does not increase length");
    IntSet s = {0};
    ds_hs_add(&s, 42);
    ds_hs_add(&s, 42);
    ds_hs_add(&s, 42);
    ASSERT_EQ(s.length, 1, "length should be 1");
    ds_hs_free(&s);
    PASS();
}

void test_hs_remove(void) {
    TEST("hs: remove existing element");
    IntSet s = {0};
    ds_hs_add(&s, 1);
    ds_hs_add(&s, 2);
    ds_hs_add(&s, 3);
    bool removed = ds_hs_remove(&s, 2);
    ASSERT(removed, "should return true");
    ASSERT(!ds_hs_has(&s, 2), "2 should be gone");
    ASSERT(ds_hs_has(&s, 1), "1 still there");
    ASSERT(ds_hs_has(&s, 3), "3 still there");
    ASSERT_EQ(s.length, 2, "length should be 2");
    ds_hs_free(&s);
    PASS();
}

void test_hs_many_elements(void) {
    TEST("hs: many elements triggers resize");
    IntSet s = {0};
    for (int i = 0; i < 200; i++) {
        ds_hs_add(&s, i);
    }
    ASSERT_EQ(s.length, 200, "length should be 200");
    for (int i = 0; i < 200; i++) {
        ASSERT(ds_hs_has(&s, i), "should have all elements");
    }
    ds_hs_free(&s);
    PASS();
}

void test_hs_string_values(void) {
    TEST("hs: string set");
    StrSet s = {0};
    ds_hs_add(&s, "alpha");
    ds_hs_add(&s, "beta");
    ds_hs_add(&s, "gamma");
    ASSERT(ds_hs_has(&s, "alpha"), "has alpha");
    ASSERT(ds_hs_has(&s, "beta"), "has beta");
    ASSERT(!ds_hs_has(&s, "delta"), "no delta");
    ds_hs_free(&s);
    PASS();
}

void test_hs_cat(void) {
    TEST("hs: cat merges two sets");
    IntSet a = {0}, b = {0};
    ds_hs_add(&a, 1);
    ds_hs_add(&a, 2);
    ds_hs_add(&b, 2);
    ds_hs_add(&b, 3);
    ds_hs_cat(&a, &b);
    ASSERT(ds_hs_has(&a, 1), "has 1");
    ASSERT(ds_hs_has(&a, 2), "has 2");
    ASSERT(ds_hs_has(&a, 3), "has 3");
    ASSERT_EQ(a.length, 3, "no duplicates");
    ds_hs_free(&a);
    ds_hs_free(&b);
    PASS();
}

void test_hs_sub(void) {
    TEST("hs: sub removes elements of second set");
    IntSet a = {0}, b = {0};
    ds_hs_add(&a, 1);
    ds_hs_add(&a, 2);
    ds_hs_add(&a, 3);
    ds_hs_add(&b, 2);
    ds_hs_add(&b, 3);
    ds_hs_sub(&a, &b);
    ASSERT(ds_hs_has(&a, 1), "has 1");
    ASSERT(!ds_hs_has(&a, 2), "no 2");
    ASSERT(!ds_hs_has(&a, 3), "no 3");
    ASSERT_EQ(a.length, 1, "length 1");
    ds_hs_free(&a);
    ds_hs_free(&b);
    PASS();
}

void test_hs_remove_stress(void) {
    TEST("hs: stress remove and verify consistency");
    IntSet s = {0};
    for (int i = 0; i < 100; i++) ds_hs_add(&s, i);
    // Remove first 50
    for (int i = 0; i < 50; i++) ds_hs_remove(&s, i);
    ASSERT_EQ(s.length, 50, "50 left");
    for (int i = 0; i < 100; i++) {
        if (i < 50) ASSERT(!ds_hs_has(&s, i), "removed element gone");
        else ASSERT(ds_hs_has(&s, i), "remaining element present");
    }
    ds_hs_free(&s);
    PASS();
}

void test_hs_has_empty(void) {
    TEST("hs: has on empty set returns false");
    IntSet s = {0};
    ASSERT(!ds_hs_has(&s, 42), "empty set has nothing");
    PASS();
}

void test_hs_to_da_and_back(void) {
    TEST("hs: convert to da and back");
    IntSet s = {0};
    IntArray a = {0};
    ds_hs_add(&s, 10);
    ds_hs_add(&s, 20);
    ds_hs_add(&s, 30);
    ds_hs_to_da(&s, &a);
    ASSERT_EQ(a.length, 3, "da length 3");
    IntSet s2 = {0};
    ds_da_to_hs(&a, &s2);
    ASSERT(ds_hs_has(&s2, 10), "has 10");
    ASSERT(ds_hs_has(&s2, 20), "has 20");
    ASSERT(ds_hs_has(&s2, 30), "has 30");
    ASSERT_EQ(s2.length, 3, "set length 3");
    ds_hs_free(&s);
    ds_hs_free(&s2);
    ds_da_free(&a);
    PASS();
}

// ============================================================================
// Linked List Tests
// ============================================================================

ds_ll_declare(IntList, int);

void test_ll_push(void) {
    TEST("ll: push adds to front");
    IntList ll = {0};
    ds_ll_push(&ll, 3);
    ds_ll_push(&ll, 2);
    ds_ll_push(&ll, 1);
    ASSERT_EQ(ll.size, 3, "size should be 3");
    ASSERT_EQ(ll.head->val, 1, "head should be 1");
    ASSERT_EQ(ll.tail->val, 3, "tail should be 3");
    ds_ll_free(&ll);
    PASS();
}

void test_ll_append(void) {
    TEST("ll: append adds to end");
    IntList ll = {0};
    ds_ll_append(&ll, 1);
    ds_ll_append(&ll, 2);
    ds_ll_append(&ll, 3);
    ASSERT_EQ(ll.size, 3, "size should be 3");
    ASSERT_EQ(ll.head->val, 1, "head should be 1");
    ASSERT_EQ(ll.tail->val, 3, "tail should be 3");
    // Verify linked chain is traversable
    ASSERT_EQ(ll.head->next->val, 2, "middle node");
    ASSERT_EQ(ll.head->next->next->val, 3, "tail via traversal");
    ASSERT_EQ(ll.head->next->next->next, NULL, "terminates with NULL");
    ds_ll_free(&ll);
    PASS();
}

void test_ll_pop(void) {
    TEST("ll: pop removes from front");
    IntList ll = {0};
    ds_ll_push(&ll, 3);
    ds_ll_push(&ll, 2);
    ds_ll_push(&ll, 1);
    IntList_Node *n = ds_ll_pop(&ll);
    ASSERT_EQ(n->val, 1, "first pop should be 1");
    free(n);
    n = ds_ll_pop(&ll);
    ASSERT_EQ(n->val, 2, "second pop should be 2");
    free(n);
    ASSERT_EQ(ll.size, 1, "size should be 1");
    ASSERT_EQ(ll.head->val, 3, "head = tail = 3");
    ASSERT_EQ(ll.tail->val, 3, "tail = 3");
    ds_ll_free(&ll);
    PASS();
}

void test_ll_pop_last_clears_tail(void) {
    TEST("ll: pop last element clears tail");
    IntList ll = {0};
    ds_ll_push(&ll, 1);
    IntList_Node *n = ds_ll_pop(&ll);
    ASSERT_EQ(n->val, 1, "popped value");
    free(n);
    ASSERT_EQ(ll.head, NULL, "head should be NULL");
    ASSERT_EQ(ll.tail, NULL, "tail should be NULL");
    ASSERT_EQ(ll.size, 0, "size should be 0");
    PASS();
}

void test_ll_mixed_push_append(void) {
    TEST("ll: mixed push and append");
    IntList ll = {0};
    ds_ll_append(&ll, 2); // [2]
    ds_ll_push(&ll, 1);   // [1,2]
    ds_ll_append(&ll, 3); // [1,2,3]
    ASSERT_EQ(ll.head->val, 1, "head");
    ASSERT_EQ(ll.head->next->val, 2, "middle");
    ASSERT_EQ(ll.tail->val, 3, "tail");
    ASSERT_EQ(ll.size, 3, "size");
    ds_ll_free(&ll);
    PASS();
}

void test_ll_free_empties(void) {
    TEST("ll: free empties the list");
    IntList ll = {0};
    for (int i = 0; i < 10; i++) ds_ll_append(&ll, i);
    ds_ll_free(&ll);
    ASSERT_EQ(ll.head, NULL, "head NULL after free");
    ASSERT_EQ(ll.tail, NULL, "tail NULL after free");
    ASSERT_EQ(ll.size, 0, "size 0 after free");
    PASS();
}

// ============================================================================
// String Iterator Tests
// ============================================================================

void test_s_split_basic(void) {
    TEST("s_split: basic path splitting");
    DsStringIterator it = ds_cstr_iter("path/to/file.txt");
    DsStringIterator part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 4, "first part length");
    ASSERT(strncmp(part.data, "path", 4) == 0, "first part");
    part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 2, "second part length");
    ASSERT(strncmp(part.data, "to", 2) == 0, "second part");
    part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 8, "third part length");
    ASSERT(strncmp(part.data, "file.txt", 8) == 0, "third part");
    ASSERT_EQ(it.length, 0, "iterator exhausted");
    PASS();
}

void test_s_split_single(void) {
    TEST("s_split: no separator found");
    DsStringIterator it = ds_cstr_iter("nosep");
    DsStringIterator part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 5, "whole string");
    ASSERT_EQ(it.length, 0, "exhausted");
    PASS();
}

void test_s_split_empty(void) {
    TEST("s_split: empty iterator");
    DsStringIterator it = ds_str_iter_empty;
    DsStringIterator part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 0, "empty part");
    ASSERT_EQ(part.data, NULL, "null data");
    PASS();
}

void test_s_split_leading_sep(void) {
    TEST("s_split: leading separator produces empty first part");
    DsStringIterator it = ds_cstr_iter("/root/file");
    DsStringIterator part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 0, "first part is empty");
    part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 4, "root");
    part = ds_s_split(&it, '/');
    ASSERT_EQ(part.length, 4, "file");
    PASS();
}

void test_s_ltrim(void) {
    TEST("s_ltrim: trims leading whitespace");
    DsStringIterator it = ds_cstr_iter("  \thello");
    ds_s_ltrim(&it);
    ASSERT_EQ(it.length, 5, "trimmed length");
    ASSERT(strncmp(it.data, "hello", 5) == 0, "trimmed content");
    PASS();
}

void test_s_rtrim(void) {
    TEST("s_rtrim: trims trailing whitespace");
    DsStringIterator it = ds_cstr_iter("hello  \t");
    ds_s_rtrim(&it);
    ASSERT_EQ(it.length, 5, "trimmed length");
    ASSERT(strncmp(it.data, "hello", 5) == 0, "trimmed content");
    PASS();
}

void test_s_trim(void) {
    TEST("s_trim: trims both sides");
    DsStringIterator it = ds_cstr_iter(" \t hello \n ");
    ds_s_trim(&it);
    ASSERT_EQ(it.length, 5, "trimmed length");
    ASSERT(strncmp(it.data, "hello", 5) == 0, "trimmed content");
    PASS();
}

// ============================================================================
// starts_with / ends_with Tests
// ============================================================================

void test_starts_with(void) {
    TEST("starts_with: basic checks");
    ASSERT(ds_starts_with("hello world", "hello"), "starts with hello");
    ASSERT(!ds_starts_with("hello world", "world"), "does not start with world");
    ASSERT(ds_starts_with("hello", "hello"), "exact match");
    ASSERT(ds_starts_with("hello", ""), "empty prefix always matches");
    PASS();
}

void test_ends_with(void) {
    TEST("ends_with: basic checks");
    ASSERT(ds_ends_with("hello world", "world"), "ends with world");
    ASSERT(!ds_ends_with("hello world", "hello"), "does not end with hello");
    ASSERT(ds_ends_with("hello", "hello"), "exact match");
    ASSERT(ds_ends_with("hello", ""), "empty suffix always matches");
    ASSERT(!ds_ends_with("hi", "hello"), "suffix longer than string");
    PASS();
}

// ============================================================================
// Arena Allocator Tests
// ============================================================================

void test_arena_basic(void) {
    TEST("arena: basic alloc and free");
    DsArena arena = {0};
    int *arr = ds_a_malloc(&arena, 10 * sizeof(int));
    ASSERT_NEQ(arr, NULL, "allocation should succeed");
    for (int i = 0; i < 10; i++) arr[i] = i;
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(arr[i], i, "data intact");
    }
    ds_a_free(&arena);
    ASSERT_EQ(arena.start, NULL, "start NULL after free");
    ASSERT_EQ(arena.end, NULL, "end NULL after free");
    PASS();
}

void test_arena_zero_alloc(void) {
    TEST("arena: zero-size alloc returns NULL");
    DsArena arena = {0};
    void *p = ds_a_malloc(&arena, 0);
    ASSERT_EQ(p, NULL, "should return NULL");
    ds_a_free(&arena);
    PASS();
}

void test_arena_many_allocs(void) {
    TEST("arena: many small allocations");
    DsArena arena = {0};
    for (int i = 0; i < 1000; i++) {
        int *p = ds_a_malloc(&arena, sizeof(int));
        ASSERT_NEQ(p, NULL, "alloc should succeed");
        *p = i;
    }
    ds_a_free(&arena);
    PASS();
}

void test_arena_large_alloc(void) {
    TEST("arena: single large allocation > region size");
    DsArena arena = {0};
    size_t big = DS_MIN_ALLOC_REGION * 4;
    char *p = ds_a_malloc(&arena, big);
    ASSERT_NEQ(p, NULL, "large alloc should succeed");
    memset(p, 'A', big);
    ASSERT_EQ(p[0], 'A', "first byte");
    ASSERT_EQ(p[big - 1], 'A', "last byte");
    ds_a_free(&arena);
    PASS();
}

void test_arena_realloc(void) {
    TEST("arena: realloc copies data");
    DsArena arena = {0};
    int *arr = ds_a_malloc(&arena, 5 * sizeof(int));
    for (int i = 0; i < 5; i++) arr[i] = i * 10;
    int *arr2 = ds_a_realloc(&arena, arr, 5 * sizeof(int), 10 * sizeof(int));
    ASSERT_NEQ(arr2, NULL, "realloc should succeed");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(arr2[i], i * 10, "old data preserved");
    }
    ds_a_free(&arena);
    PASS();
}

void test_arena_realloc_shrink(void) {
    TEST("arena: realloc with smaller size returns NULL");
    DsArena arena = {0};
    int *arr = ds_a_malloc(&arena, 10 * sizeof(int));
    int *arr2 = ds_a_realloc(&arena, arr, 10 * sizeof(int), 5 * sizeof(int));
    ASSERT_EQ(arr2, NULL, "shrink realloc should return NULL");
    ds_a_free(&arena);
    PASS();
}

void test_arena_snapshot_restore(void) {
    TEST("arena: snapshot and restore");
    DsArena arena = {0};
    int *a1 = ds_a_malloc(&arena, sizeof(int));
    *a1 = 42;
    DsArenaSnapshot snap = ds_a_snapshot(&arena);
    int *a2 = ds_a_malloc(&arena, sizeof(int));
    *a2 = 99;
    (void)a2;
    ds_a_restore(&arena, snap);
    // After restore, a2 area is reclaimed; a1 should still be accessible
    // (The memory for a1 is still in the same region)
    ASSERT_EQ(*a1, 42, "data before snapshot intact");
    ds_a_free(&arena);
    PASS();
}

void test_arena_snapshot_restore_empty(void) {
    TEST("arena: restore to empty state frees all");
    DsArena arena = {0};
    DsArenaSnapshot snap = ds_a_snapshot(&arena);
    ds_a_malloc(&arena, 100);
    ds_a_malloc(&arena, 200);
    ds_a_restore(&arena, snap);
    ASSERT_EQ(arena.start, NULL, "should be fully freed");
    PASS();
}

// ============================================================================
// RArena Tests
// ============================================================================

void test_rarena_basic(void) {
    TEST("rarena: basic alloc and free");
    DsRArena arena = {0};
    int *p = ds_a_rmalloc(&arena, sizeof(int));
    ASSERT_NEQ(p, NULL, "alloc should succeed");
    *p = 42;
    ASSERT_EQ(*p, 42, "data intact");
    ds_a_rfree(&arena);
    ASSERT_EQ(arena.start, NULL, "freed");
    PASS();
}

void test_rarena_rrealloc(void) {
    TEST("rarena: rrealloc grows data");
    DsRArena arena = {0};
    int *p = ds_a_rmalloc(&arena, 5 * sizeof(int));
    for (int i = 0; i < 5; i++) p[i] = i;
    p = ds_a_rrealloc(&arena, p, 10 * sizeof(int));
    ASSERT_NEQ(p, NULL, "realloc should succeed");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(p[i], i, "old data preserved");
    }
    ds_a_rfree(&arena);
    PASS();
}

void test_rarena_rrealloc_null(void) {
    TEST("rarena: rrealloc with NULL ptr acts as malloc");
    DsRArena arena = {0};
    int *p = ds_a_rrealloc(&arena, NULL, sizeof(int));
    ASSERT_NEQ(p, NULL, "should allocate");
    *p = 99;
    ds_a_rfree(&arena);
    PASS();
}

void test_rarena_rfree_one(void) {
    TEST("rarena: rfree_one frees single allocation");
    DsRArena arena = {0};
    int *a = ds_a_rmalloc(&arena, sizeof(int));
    int *b = ds_a_rmalloc(&arena, sizeof(int));
    int *c = ds_a_rmalloc(&arena, sizeof(int));
    *a = 1; *b = 2; *c = 3;
    ds_a_rfree_one(&arena, b);
    // a and c should still be valid
    ASSERT_EQ(*a, 1, "a still valid");
    ASSERT_EQ(*c, 3, "c still valid");
    ds_a_rfree(&arena);
    PASS();
}

void test_rarena_rfree_one_first(void) {
    TEST("rarena: rfree_one frees first allocation");
    DsRArena arena = {0};
    int *a = ds_a_rmalloc(&arena, sizeof(int));
    int *b = ds_a_rmalloc(&arena, sizeof(int));
    *a = 1; *b = 2;
    ds_a_rfree_one(&arena, a);
    ASSERT_EQ(*b, 2, "b still valid");
    ds_a_rfree(&arena);
    PASS();
}

void test_rarena_rfree_one_last(void) {
    TEST("rarena: rfree_one frees last allocation");
    DsRArena arena = {0};
    int *a = ds_a_rmalloc(&arena, sizeof(int));
    int *b = ds_a_rmalloc(&arena, sizeof(int));
    *a = 1; *b = 2;
    ds_a_rfree_one(&arena, b);
    ASSERT_EQ(*a, 1, "a still valid");
    ASSERT_EQ(arena.end, (DsRRegion *)a - 1, "end updated to a's region");
    ds_a_rfree(&arena);
    PASS();
}

// ============================================================================
// Tmp Allocator Tests
// ============================================================================

void test_tmp_alloc_basic(void) {
    TEST("tmp: basic alloc and free");
    char *s = ds_tmp_alloc(100);
    ASSERT_NEQ(s, NULL, "alloc should succeed");
    memset(s, 'X', 100);
    ds_tmp_free();
    PASS();
}

void test_tmp_strdup(void) {
    TEST("tmp: strdup duplicates string");
    char *s = ds_tmp_strdup("hello world");
    ASSERT_STR(s, "hello world", "duplicated string");
    ds_tmp_free();
    PASS();
}

void test_tmp_strndup(void) {
    TEST("tmp: strndup duplicates n chars");
    char *s = ds_tmp_strndup("hello world", 5);
    ASSERT_STR(s, "hello", "truncated string");
    ds_tmp_free();
    PASS();
}

void test_tmp_sprintf(void) {
    TEST("tmp: sprintf formats string");
    char *s = ds_tmp_sprintf("num=%d str=%s", 42, "test");
    ASSERT_STR(s, "num=42 str=test", "formatted string");
    ds_tmp_free();
    PASS();
}

void test_tmp_snapshot_restore(void) {
    TEST("tmp: snapshot and restore");
    char *s1 = ds_tmp_strdup("persistent");
    DsArenaSnapshot snap = ds_tmp_snapshot();
    ds_tmp_strdup("temporary");
    ds_tmp_strdup("also temporary");
    ds_tmp_restore(snap);
    ASSERT_STR(s1, "persistent", "s1 still valid after restore");
    ds_tmp_free();
    PASS();
}

// ============================================================================
// File I/O Tests
// ============================================================================

void test_file_read_write(void) {
    TEST("file: write and read back");
    DsString out = {0};
    ds_str_append(&out, "hello file test\nline 2\n");
    bool ok = ds_write_entire_file("/tmp/ds_test_file.txt", &out);
    ASSERT(ok, "write should succeed");

    DsString in = {0};
    ok = ds_read_entire_file("/tmp/ds_test_file.txt", &in);
    ASSERT(ok, "read should succeed");
    ASSERT_EQ(in.length, out.length, "lengths match");
    ASSERT(memcmp(in.data, out.data, in.length) == 0, "content matches");
    ds_da_free(&out);
    ds_da_free(&in);
    remove("/tmp/ds_test_file.txt");
    PASS();
}

void test_file_read_nonexistent(void) {
    TEST("file: read nonexistent file returns false");
    DsString s = {0};
    // Suppress log output for this test
    ds_set_log_level(DS_LOG_ERROR + 1);
    bool ok = ds_read_entire_file("/tmp/ds_test_nonexistent_12345.txt", &s);
    ds_set_log_level(DS_LOG_INFO);
    ASSERT(!ok, "should return false");
    PASS();
}

void test_file_read_append(void) {
    TEST("file: read appends to existing string");
    DsString out = {0};
    ds_str_append(&out, "content");
    ds_write_entire_file("/tmp/ds_test_append.txt", &out);

    DsString s = {0};
    ds_str_append(&s, "prefix:");
    bool ok = ds_read_entire_file("/tmp/ds_test_append.txt", &s);
    ASSERT(ok, "read should succeed");
    ASSERT_EQ(s.length, 14, "prefix + content");
    ASSERT(strncmp(s.data, "prefix:content", 14) == 0, "concatenated");
    ds_da_free(&out);
    ds_da_free(&s);
    remove("/tmp/ds_test_append.txt");
    PASS();
}

// ============================================================================
// Logging Tests
// ============================================================================

static int _test_log_called = 0;
static void _test_log_handler(int level, const char *fmt, ...) {
    (void)level; (void)fmt;
    _test_log_called = 1;
}

void test_log_set_handler(void) {
    TEST("log: set custom handler");
    _test_log_called = 0;
    ds_set_log_handler(_test_log_handler);
    ds_log(DS_LOG_INFO, "test");
    ASSERT_EQ(_test_log_called, 1, "custom handler should be called");
    ds_set_log_handler(ds_simple_log_handler);
    PASS();
}

void test_log_set_level(void) {
    TEST("log: set level filters messages");
    // This is a basic sanity check; full verification would need output capture
    ds_set_log_level(DS_LOG_ERROR);
    // INFO should not print (we can't easily check, but no crash)
    ds_log(DS_LOG_INFO, "should be suppressed");
    ds_set_log_level(DS_LOG_INFO);
    PASS();
}

// ============================================================================
// mkdir_p Tests
// ============================================================================

void test_mkdir_p(void) {
    TEST("mkdir_p: creates nested directories");
    bool ok = ds_mkdir_p("/tmp/ds_test_mkdir/a/b/c");
    ASSERT(ok, "should succeed");
    // Verify by opening the directory
    DIR *d = opendir("/tmp/ds_test_mkdir/a/b/c");
    ASSERT_NEQ(d, NULL, "directory should exist");
    closedir(d);
    // Cleanup
    rmdir("/tmp/ds_test_mkdir/a/b/c");
    rmdir("/tmp/ds_test_mkdir/a/b");
    rmdir("/tmp/ds_test_mkdir/a");
    rmdir("/tmp/ds_test_mkdir");
    PASS();
}

void test_mkdir_p_existing(void) {
    TEST("mkdir_p: existing directory is ok");
    ds_mkdir_p("/tmp/ds_test_mkdir2");
    bool ok = ds_mkdir_p("/tmp/ds_test_mkdir2");
    ASSERT(ok, "should succeed on existing dir");
    rmdir("/tmp/ds_test_mkdir2");
    PASS();
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

void test_da_large_struct(void) {
    TEST("da: works with large struct elements");
    typedef struct { int a; double b; char c[64]; } BigStruct;
    ds_da_declare(BigArray, BigStruct);
    BigArray arr = {0};
    for (int i = 0; i < 50; i++) {
        BigStruct item = {.a = i, .b = i * 1.5, .c = "test"};
        ds_da_append(&arr, item);
    }
    ASSERT_EQ(arr.length, 50, "length 50");
    ASSERT_EQ(arr.data[25].a, 25, "element 25");
    ds_da_free(&arr);
    PASS();
}

void test_hm_collision_handling(void) {
    TEST("hm: handles hash collisions gracefully");
    IntIntMap hm = {0};
    for (int i = 0; i < 500; i++) {
        ds_hm_set(&hm, i, i * 3);
    }
    for (int i = 0; i < 500; i++) {
        ASSERT_EQ(ds_hm_get(&hm, i), i * 3, "collision-proof lookup");
    }
    // Remove every 3rd and verify rest
    for (int i = 0; i < 500; i += 3) {
        ds_hm_remove(&hm, i);
    }
    for (int i = 0; i < 500; i++) {
        if (i % 3 == 0) {
            ASSERT(!ds_hm_has(&hm, i), "removed key gone");
        } else {
            ASSERT_EQ(ds_hm_get(&hm, i), i * 3, "surviving key intact");
        }
    }
    ds_hm_free(&hm);
    PASS();
}

void test_str_prependf_preserves_content(void) {
    TEST("str: prependf preserves existing multichar content");
    DsString s = {0};
    ds_str_append(&s, "ABCDEF");
    ds_str_prependf(&s, "123");
    ASSERT_EQ(s.length, 9, "correct length");
    ASSERT_STR(s.data, "123ABCDEF", "prepended correctly");
    ds_da_free(&s);
    PASS();
}

void test_hm_string_key_overwrite(void) {
    TEST("hm: string key overwrite preserves correct value");
    StrIntMap hm = {0};
    ds_hm_set(&hm, "key", 1);
    ds_hm_set(&hm, "key", 2);
    ds_hm_set(&hm, "key", 3);
    ASSERT_EQ(ds_hm_get(&hm, "key"), 3, "last value wins");
    ASSERT_EQ(hm.length, 1, "no duplicates");
    ds_hm_free(&hm);
    PASS();
}

void test_da_remove_edge_empty_array(void) {
    TEST("da: remove on empty array is safe");
    IntArray a = {0};
    ds_da_remove(&a, 0, 1); // should not crash
    ASSERT_EQ(a.length, 0, "still empty");
    PASS();
}

void test_arena_alignment(void) {
    TEST("arena: allocations are pointer-aligned");
    DsArena arena = {0};
    for (int i = 1; i <= 20; i++) {
        void *p = ds_a_malloc(&arena, i);
        ASSERT_EQ((uintptr_t)p % sizeof(uintptr_t), 0, "alignment check");
    }
    ds_a_free(&arena);
    PASS();
}

void test_ds_array_len(void) {
    TEST("DS_ARRAY_LEN: correct static array length");
    int arr[] = {1, 2, 3, 4, 5};
    ASSERT_EQ(DS_ARRAY_LEN(arr), 5, "length should be 5");
    char str[] = "hello";
    ASSERT_EQ(DS_ARRAY_LEN(str), 6, "includes null terminator");
    PASS();
}

// NOTE: ds_hs_cat_da is correct (uses *_v), but ds_hs_sub_da has the same
// pointer-dereference pattern and works correctly too. Testing both.
void test_hs_cat_da(void) {
    TEST("hs: cat_da merges dynamic array into set");
    IntSet s = {0};
    IntArray a = {0};
    ds_hs_add(&s, 1);
    ds_da_append(&a, 2);
    ds_da_append(&a, 3);
    ds_da_append(&a, 1); // duplicate
    ds_hs_cat_da(&s, &a);
    ASSERT_EQ(s.length, 3, "no duplicates");
    ASSERT(ds_hs_has(&s, 1), "has 1");
    ASSERT(ds_hs_has(&s, 2), "has 2");
    ASSERT(ds_hs_has(&s, 3), "has 3");
    ds_hs_free(&s);
    ds_da_free(&a);
    PASS();
}

void test_hs_sub_da(void) {
    TEST("hs: sub_da removes da elements from set");
    IntSet s = {0};
    IntArray a = {0};
    ds_hs_add(&s, 1);
    ds_hs_add(&s, 2);
    ds_hs_add(&s, 3);
    ds_da_append(&a, 1);
    ds_da_append(&a, 3);
    ds_hs_sub_da(&s, &a);
    ASSERT_EQ(s.length, 1, "one left");
    ASSERT(ds_hs_has(&s, 2), "only 2 remains");
    ds_hs_free(&s);
    ds_da_free(&a);
    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    setbuf(stdout, NULL); // disable buffering for test output
    printf("=== ds.h Test Suite ===\n");

    // Dynamic Array
    SECTION("Dynamic Array");
    test_da_init_zero();
    test_da_append_single();
    test_da_append_many_elements();
    test_da_append_many_batch();
    test_da_append_many_zero_size();
    test_da_pop();
    test_da_remove_middle();
    test_da_remove_first();
    test_da_remove_last();
    test_da_remove_all();
    test_da_remove_unordered();
    test_da_insert_middle();
    test_da_insert_at_start();
    test_da_insert_beyond_length();
    test_da_prepend();
    test_da_first_last();
    test_da_find();
    test_da_index_of();
    test_da_foreach();
    test_da_foreach_idx();
    test_da_reserve();
    test_da_zero_vs_free();
    test_da_free_idempotent();
    test_da_remove_edge_empty_array();
    test_da_large_struct();

    // String Builder
    SECTION("String Builder");
    test_str_append_basic();
    test_str_append_empty();
    test_str_append_incremental();
    test_str_appendf();
    test_str_appendf_multiple();
    test_str_prependf();
    test_str_prependf_to_empty();
    test_str_insert_middle();
    test_str_insert_at_start();
    test_str_insert_at_end();
    test_str_insert_beyond_length();
    test_str_include();
    test_str_include_empty_string();
    test_str_ltrim();
    test_str_rtrim();
    test_str_trim();
    test_str_trim_all_whitespace();
    test_str_trim_empty();
    test_str_prependf_preserves_content();

    // Hash Map
    SECTION("Hash Map");
    test_hm_set_get_basic();
    test_hm_overwrite();
    test_hm_try_found();
    test_hm_try_not_found();
    test_hm_try_empty();
    test_hm_has();
    test_hm_get_missing();
    test_hm_string_keys();
    test_hm_many_entries_triggers_resize();
    test_hm_remove_basic();
    test_hm_remove_missing();
    test_hm_remove_and_reinsert();
    test_hm_remove_all_entries();
    test_hm_remove_stress_and_lookup();
    test_hm_foreach();
    test_hm_free_idempotent();
    test_hm_negative_and_zero_keys();
    test_hm_string_key_overwrite();
    test_hm_collision_handling();

    // Hash Set
    SECTION("Hash Set");
    test_hs_add_has();
    test_hs_add_duplicate();
    test_hs_remove();
    test_hs_many_elements();
    test_hs_string_values();
    test_hs_cat();
    test_hs_sub();
    test_hs_remove_stress();
    test_hs_has_empty();
    test_hs_to_da_and_back();
    test_hs_cat_da();
    test_hs_sub_da();

    // Linked List
    SECTION("Linked List");
    test_ll_push();
    test_ll_append();
    test_ll_pop();
    test_ll_pop_last_clears_tail();
    test_ll_mixed_push_append();
    test_ll_free_empties();

    // String Iterator
    SECTION("String Iterator");
    test_s_split_basic();
    test_s_split_single();
    test_s_split_empty();
    test_s_split_leading_sep();
    test_s_ltrim();
    test_s_rtrim();
    test_s_trim();

    // starts_with / ends_with
    SECTION("starts_with / ends_with");
    test_starts_with();
    test_ends_with();

    // Arena Allocator
    SECTION("Arena Allocator");
    test_arena_basic();
    test_arena_zero_alloc();
    test_arena_many_allocs();
    test_arena_large_alloc();
    test_arena_realloc();
    test_arena_realloc_shrink();
    test_arena_snapshot_restore();
    test_arena_snapshot_restore_empty();
    test_arena_alignment();

    // RArena
    SECTION("RArena Allocator");
    test_rarena_basic();
    test_rarena_rrealloc();
    test_rarena_rrealloc_null();
    test_rarena_rfree_one();
    test_rarena_rfree_one_first();
    test_rarena_rfree_one_last();

    // Tmp Allocator
    SECTION("Tmp Allocator");
    test_tmp_alloc_basic();
    test_tmp_strdup();
    test_tmp_strndup();
    test_tmp_sprintf();
    test_tmp_snapshot_restore();

    // File I/O
    SECTION("File I/O");
    test_file_read_write();
    test_file_read_nonexistent();
    test_file_read_append();

    // Logging
    SECTION("Logging");
    test_log_set_handler();
    test_log_set_level();

    // mkdir_p
    SECTION("mkdir_p");
    test_mkdir_p();
    test_mkdir_p_existing();

    // Misc
    SECTION("Misc");
    test_ds_array_len();

    // Summary
    printf("\n=== Results ===\n");
    printf("Total: %d | \033[32mPassed: %d\033[0m | \033[31mFailed: %d\033[0m\n",
           tests_run, tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
