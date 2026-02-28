#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define JSB_IMPLEMENTATION
#include "../jsb.h"
#define JSP_IMPLEMENTATION
#include "../jsp.h"

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
// JSB Tests
// ============================================================================

void test_jsb_empty_object(void) {
    TEST("jsb: empty object");
    Jsb jsb = {0};
    ASSERT_EQ(jsb_begin_object(&jsb), 0, "begin_object failed");
    ASSERT_EQ(jsb_end_object(&jsb), 0, "end_object failed");
    ASSERT_STR(jsb_get(&jsb), "{}", "should be {}");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_empty_array(void) {
    TEST("jsb: empty array");
    Jsb jsb = {0};
    ASSERT_EQ(jsb_begin_array(&jsb), 0, "begin_array failed");
    ASSERT_EQ(jsb_end_array(&jsb), 0, "end_array failed");
    ASSERT_STR(jsb_get(&jsb), "[]", "should be []");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_single_string(void) {
    TEST("jsb: object with single string");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "name");
    jsb_string(&jsb, "John");
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb), "{\"name\": \"John\"}", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_multiple_types(void) {
    TEST("jsb: object with all value types");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "s"); jsb_string(&jsb, "hello");
    jsb_key(&jsb, "i"); jsb_int(&jsb, 42);
    jsb_key(&jsb, "n"); jsb_number(&jsb, 3.14, 2);
    jsb_key(&jsb, "b"); jsb_bool(&jsb, true);
    jsb_key(&jsb, "f"); jsb_bool(&jsb, false);
    jsb_key(&jsb, "z"); jsb_null(&jsb);
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb),
        "{\"s\": \"hello\",\"i\": 42,\"n\": 3.14,\"b\": true,\"f\": false,\"z\": null}",
        "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_nested_objects(void) {
    TEST("jsb: nested objects");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "outer");
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "inner");
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "value");
    jsb_int(&jsb, 1);
    jsb_end_object(&jsb);
    jsb_end_object(&jsb);
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb),
        "{\"outer\": {\"inner\": {\"value\": 1}}}",
        "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_array_values(void) {
    TEST("jsb: array with mixed values");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_string(&jsb, "a");
    jsb_int(&jsb, 1);
    jsb_bool(&jsb, false);
    jsb_null(&jsb);
    jsb_end_array(&jsb);
    ASSERT_STR(jsb_get(&jsb), "[\"a\",1,false,null]", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_array_of_objects(void) {
    TEST("jsb: array of objects");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "id"); jsb_int(&jsb, 1);
    jsb_end_object(&jsb);
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "id"); jsb_int(&jsb, 2);
    jsb_end_object(&jsb);
    jsb_end_array(&jsb);
    ASSERT_STR(jsb_get(&jsb), "[{\"id\": 1},{\"id\": 2}]", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_nested_arrays(void) {
    TEST("jsb: nested arrays");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_begin_array(&jsb);
    jsb_int(&jsb, 1);
    jsb_int(&jsb, 2);
    jsb_end_array(&jsb);
    jsb_begin_array(&jsb);
    jsb_int(&jsb, 3);
    jsb_int(&jsb, 4);
    jsb_end_array(&jsb);
    jsb_end_array(&jsb);
    ASSERT_STR(jsb_get(&jsb), "[[1,2],[3,4]]", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_string_escaping(void) {
    TEST("jsb: string escaping (quotes, backslash, newline, tab)");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "msg");
    jsb_string(&jsb, "hello \"world\"\nnew\tline\\end");
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb),
        "{\"msg\": \"hello \\\"world\\\"\\nnew\\tline\\\\end\"}",
        "escaping failed");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_empty_string(void) {
    TEST("jsb: empty string value");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "empty");
    jsb_string(&jsb, "");
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb), "{\"empty\": \"\"}", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_null_string(void) {
    TEST("jsb: NULL string becomes null");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "v");
    jsb_string(&jsb, NULL);
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb), "{\"v\": null}", "NULL string should produce null");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_negative_int(void) {
    TEST("jsb: negative integer");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_int(&jsb, -42);
    jsb_int(&jsb, 0);
    jsb_int(&jsb, 2147483647);
    jsb_int(&jsb, -2147483648);
    jsb_end_array(&jsb);
    ASSERT_STR(jsb_get(&jsb), "[-42,0,2147483647,-2147483648]", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_number_precision(void) {
    TEST("jsb: number precision");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_number(&jsb, 1.0, 0);
    jsb_number(&jsb, 1.23456789, 4);
    jsb_number(&jsb, 0.0, 2);
    jsb_number(&jsb, -99.5, 1);
    jsb_end_array(&jsb);
    ASSERT_STR(jsb_get(&jsb), "[1,1.2346,0.00,-99.5]", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_pretty_print(void) {
    TEST("jsb: pretty print");
    Jsb jsb = {.pp = 2};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "a");
    jsb_int(&jsb, 1);
    jsb_key(&jsb, "b");
    jsb_begin_array(&jsb);
    jsb_int(&jsb, 2);
    jsb_int(&jsb, 3);
    jsb_end_array(&jsb);
    jsb_end_object(&jsb);
    const char *expected =
        "{\n"
        "  \"a\": 1,\n"
        "  \"b\": [\n"
        "    2,\n"
        "    3\n"
        "  ]\n"
        "}";
    ASSERT_STR(jsb_get(&jsb), expected, "pretty print mismatch");
    jsb_free(&jsb);
    PASS();
}

// -- State validation tests (invalid operations should return -1)

void test_jsb_value_without_key(void) {
    TEST("jsb: value without key in object returns -1");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    ASSERT_EQ(jsb_string(&jsb, "oops"), -1, "string without key should fail");
    ASSERT_EQ(jsb_int(&jsb, 1), -1, "int without key should fail");
    ASSERT_EQ(jsb_bool(&jsb, true), -1, "bool without key should fail");
    ASSERT_EQ(jsb_null(&jsb), -1, "null without key should fail");
    ASSERT_EQ(jsb_number(&jsb, 1.0, 1), -1, "number without key should fail");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_key_in_array(void) {
    TEST("jsb: key in array returns -1");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    ASSERT_EQ(jsb_key(&jsb, "bad"), -1, "key in array should fail");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_double_key(void) {
    TEST("jsb: double key without value returns -1");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    ASSERT_EQ(jsb_key(&jsb, "k1"), 0, "first key should succeed");
    ASSERT_EQ(jsb_key(&jsb, "k2"), -1, "second key without value should fail");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_end_wrong_container(void) {
    TEST("jsb: end wrong container returns -1");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    ASSERT_EQ(jsb_end_array(&jsb), -1, "end_array on object should fail");
    jsb_free(&jsb);

    Jsb jsb2 = {0};
    jsb_begin_array(&jsb2);
    ASSERT_EQ(jsb_end_object(&jsb2), -1, "end_object on array should fail");
    jsb_free(&jsb2);
    PASS();
}

void test_jsb_end_without_begin(void) {
    TEST("jsb: end without begin returns -1");
    Jsb jsb = {0};
    ASSERT_EQ(jsb_end_object(&jsb), -1, "end_object without begin should fail");
    ASSERT_EQ(jsb_end_array(&jsb), -1, "end_array without begin should fail");
    PASS();
}

void test_jsb_reuse_after_free(void) {
    TEST("jsb: reuse after free");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "a"); jsb_int(&jsb, 1);
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb), "{\"a\": 1}", "first build failed");
    jsb_free(&jsb);

    // build a new one on the same struct
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "b"); jsb_int(&jsb, 2);
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb), "{\"b\": 2}", "second build failed");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_free_idempotent(void) {
    TEST("jsb: double free doesn't crash");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_int(&jsb, 1);
    jsb_end_array(&jsb);
    jsb_free(&jsb);
    jsb_free(&jsb);
    ASSERT_EQ(jsb.buffer.items, NULL, "items should be NULL after free");
    PASS();
}

void test_jsb_key_with_special_chars(void) {
    TEST("jsb: key with special characters");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "key\"with\\quotes");
    jsb_int(&jsb, 1);
    jsb_end_object(&jsb);
    ASSERT_STR(jsb_get(&jsb), "{\"key\\\"with\\\\quotes\": 1}", "key escaping failed");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_top_level_array(void) {
    TEST("jsb: top-level array");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_string(&jsb, "one");
    jsb_string(&jsb, "two");
    jsb_end_array(&jsb);
    ASSERT_STR(jsb_get(&jsb), "[\"one\",\"two\"]", "unexpected output");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_nstring(void) {
    TEST("jsb: nstring with explicit length");
    Jsb jsb = {0};
    jsb_begin_array(&jsb);
    jsb_nstring(&jsb, "hello world", 5); // only "hello"
    jsb_end_array(&jsb);
    ASSERT_STR(jsb_get(&jsb), "[\"hello\"]", "nstring should use length");
    jsb_free(&jsb);
    PASS();
}

void test_jsb_deeply_nested(void) {
    TEST("jsb: deeply nested structures");
    Jsb jsb = {0};
    int depth = 30;
    for (int i = 0; i < depth; i++) jsb_begin_object(&jsb), jsb_key(&jsb, "n");
    jsb_int(&jsb, 42);
    for (int i = 0; i < depth; i++) jsb_end_object(&jsb);
    ASSERT_NEQ(jsb_get(&jsb), NULL, "deep nesting should work");
    // verify it starts and ends correctly
    ASSERT(strncmp(jsb_get(&jsb), "{\"n\":", 5) == 0, "should start with {\"n\":");
    size_t len = strlen(jsb_get(&jsb));
    ASSERT(jsb_get(&jsb)[len-1] == '}', "should end with }");
    jsb_free(&jsb);
    PASS();
}

// ============================================================================
// JSP Tests
// ============================================================================

void test_jsp_simple_object(void) {
    TEST("jsp: parse simple object");
    const char *json = "{\"name\": \"John\", \"age\": 30}";
    Jsp jsp = {0};
    ASSERT_EQ(jsp_sinit(&jsp, json), 0, "init failed");
    ASSERT_EQ(jsp_begin_object(&jsp), 0, "begin_object failed");

    ASSERT_EQ(jsp_key(&jsp), 0, "key1 failed");
    ASSERT_STR(jsp.string, "name", "key should be name");
    ASSERT_EQ(jsp_value(&jsp), 0, "value1 failed");
    ASSERT_EQ(jsp.type, JSP_TYPE_STRING, "type should be string");
    ASSERT_STR(jsp.string, "John", "value should be John");

    ASSERT_EQ(jsp_key(&jsp), 0, "key2 failed");
    ASSERT_STR(jsp.string, "age", "key should be age");
    ASSERT_EQ(jsp_value(&jsp), 0, "value2 failed");
    ASSERT_EQ(jsp.type, JSP_TYPE_NUMBER, "type should be number");
    ASSERT(fabs(jsp.number - 30.0) < 0.001, "value should be 30");

    ASSERT_EQ(jsp_key(&jsp), -1, "no more keys");
    ASSERT_EQ(jsp_end_object(&jsp), 0, "end_object failed");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_simple_array(void) {
    TEST("jsp: parse simple array");
    const char *json = "[1, 2, 3]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    ASSERT_EQ(jsp_begin_array(&jsp), 0, "begin_array failed");

    ASSERT_EQ(jsp_value(&jsp), 0, "value1 failed");
    ASSERT(fabs(jsp.number - 1.0) < 0.001, "should be 1");
    ASSERT_EQ(jsp_value(&jsp), 0, "value2 failed");
    ASSERT(fabs(jsp.number - 2.0) < 0.001, "should be 2");
    ASSERT_EQ(jsp_value(&jsp), 0, "value3 failed");
    ASSERT(fabs(jsp.number - 3.0) < 0.001, "should be 3");

    ASSERT_EQ(jsp_value(&jsp), -1, "no more values");
    ASSERT_EQ(jsp_end_array(&jsp), 0, "end_array failed");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_empty_object(void) {
    TEST("jsp: parse empty object");
    const char *json = "{}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    ASSERT_EQ(jsp_begin_object(&jsp), 0, "begin_object failed");
    ASSERT_EQ(jsp_key(&jsp), -1, "should have no keys");
    ASSERT_EQ(jsp_end_object(&jsp), 0, "end_object failed");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_empty_array(void) {
    TEST("jsp: parse empty array");
    const char *json = "[]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    ASSERT_EQ(jsp_begin_array(&jsp), 0, "begin_array failed");
    ASSERT_EQ(jsp_value(&jsp), -1, "should have no values");
    ASSERT_EQ(jsp_end_array(&jsp), 0, "end_array failed");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_all_types(void) {
    TEST("jsp: parse all value types");
    const char *json = "[\"hello\", 42, true, false, null]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);

    ASSERT_EQ(jsp_value(&jsp), 0, "string failed");
    ASSERT_EQ(jsp.type, JSP_TYPE_STRING, "type should be string");
    ASSERT_STR(jsp.string, "hello", "should be hello");

    ASSERT_EQ(jsp_value(&jsp), 0, "number failed");
    ASSERT_EQ(jsp.type, JSP_TYPE_NUMBER, "type should be number");
    ASSERT(fabs(jsp.number - 42.0) < 0.001, "should be 42");

    ASSERT_EQ(jsp_value(&jsp), 0, "true failed");
    ASSERT_EQ(jsp.type, JSP_TYPE_BOOLEAN, "type should be boolean");
    ASSERT_EQ(jsp.boolean, true, "should be true");

    ASSERT_EQ(jsp_value(&jsp), 0, "false failed");
    ASSERT_EQ(jsp.type, JSP_TYPE_BOOLEAN, "type should be boolean");
    ASSERT_EQ(jsp.boolean, false, "should be false");

    ASSERT_EQ(jsp_value(&jsp), 0, "null failed");
    ASSERT_EQ(jsp.type, JSP_TYPE_NULL, "type should be null");

    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_nested_object(void) {
    TEST("jsp: parse nested objects");
    const char *json = "{\"a\": {\"b\": {\"c\": 42}}}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_object(&jsp);
    ASSERT_EQ(jsp_key(&jsp), 0, "key a");
    ASSERT_STR(jsp.string, "a", "should be a");
    jsp_begin_object(&jsp);
    ASSERT_EQ(jsp_key(&jsp), 0, "key b");
    ASSERT_STR(jsp.string, "b", "should be b");
    jsp_begin_object(&jsp);
    ASSERT_EQ(jsp_key(&jsp), 0, "key c");
    ASSERT_STR(jsp.string, "c", "should be c");
    ASSERT_EQ(jsp_value(&jsp), 0, "value c");
    ASSERT(fabs(jsp.number - 42.0) < 0.001, "should be 42");
    ASSERT_EQ(jsp_key(&jsp), -1, "no more in c");
    jsp_end_object(&jsp);
    ASSERT_EQ(jsp_key(&jsp), -1, "no more in b");
    jsp_end_object(&jsp);
    ASSERT_EQ(jsp_key(&jsp), -1, "no more in a");
    jsp_end_object(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_nested_arrays(void) {
    TEST("jsp: parse nested arrays");
    const char *json = "[[1, 2], [3, 4]]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);

    ASSERT_EQ(jsp_begin_array(&jsp), 0, "inner array 1");
    ASSERT_EQ(jsp_value(&jsp), 0, "v1"); ASSERT(fabs(jsp.number - 1.0) < 0.001, "=1");
    ASSERT_EQ(jsp_value(&jsp), 0, "v2"); ASSERT(fabs(jsp.number - 2.0) < 0.001, "=2");
    ASSERT_EQ(jsp_value(&jsp), -1, "end inner 1");
    jsp_end_array(&jsp);

    ASSERT_EQ(jsp_begin_array(&jsp), 0, "inner array 2");
    ASSERT_EQ(jsp_value(&jsp), 0, "v3"); ASSERT(fabs(jsp.number - 3.0) < 0.001, "=3");
    ASSERT_EQ(jsp_value(&jsp), 0, "v4"); ASSERT(fabs(jsp.number - 4.0) < 0.001, "=4");
    ASSERT_EQ(jsp_value(&jsp), -1, "end inner 2");
    jsp_end_array(&jsp);

    ASSERT_EQ(jsp_begin_array(&jsp), -1, "no more arrays");
    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_string_escapes(void) {
    TEST("jsp: parse escaped strings");
    const char *json = "[\"hello\\nworld\", \"tab\\there\", \"quote\\\"end\", \"back\\\\slash\"]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);

    jsp_value(&jsp);
    ASSERT_STR(jsp.string, "hello\nworld", "newline escape");

    jsp_value(&jsp);
    ASSERT_STR(jsp.string, "tab\there", "tab escape");

    jsp_value(&jsp);
    ASSERT_STR(jsp.string, "quote\"end", "quote escape");

    jsp_value(&jsp);
    ASSERT_STR(jsp.string, "back\\slash", "backslash escape");

    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_unicode_escape(void) {
    TEST("jsp: parse unicode escapes");
    const char *json = "[\"\\u0041\", \"\\u00E9\"]";  // A, e-acute
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);

    jsp_value(&jsp);
    ASSERT_STR(jsp.string, "A", "\\u0041 should be A");

    jsp_value(&jsp);
    // e-acute is U+00E9 = 0xC3 0xA9 in UTF-8
    ASSERT_EQ((unsigned char)jsp.string[0], 0xC3, "utf8 byte 1");
    ASSERT_EQ((unsigned char)jsp.string[1], 0xA9, "utf8 byte 2");
    ASSERT_EQ(jsp.string[2], '\0', "null terminated");

    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_negative_numbers(void) {
    TEST("jsp: parse negative and decimal numbers");
    const char *json = "[-1, -0.5, 1e2, 1.5e-3, 0]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);

    jsp_value(&jsp); ASSERT(fabs(jsp.number - (-1.0)) < 0.001, "-1");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - (-0.5)) < 0.001, "-0.5");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 100.0) < 0.001, "1e2");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 0.0015) < 0.0001, "1.5e-3");
    jsp_value(&jsp); ASSERT(fabs(jsp.number) < 0.001, "0");

    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_skip_value(void) {
    TEST("jsp: skip simple values");
    const char *json = "{\"a\": 1, \"b\": \"skip_me\", \"c\": 3}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_object(&jsp);

    jsp_key(&jsp);
    ASSERT_STR(jsp.string, "a", "key a");
    jsp_value(&jsp);
    ASSERT(fabs(jsp.number - 1.0) < 0.001, "a=1");

    jsp_key(&jsp);
    ASSERT_STR(jsp.string, "b", "key b");
    ASSERT_EQ(jsp_skip(&jsp), 0, "skip b");

    jsp_key(&jsp);
    ASSERT_STR(jsp.string, "c", "key c");
    jsp_value(&jsp);
    ASSERT(fabs(jsp.number - 3.0) < 0.001, "c=3");

    jsp_end_object(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_skip_nested(void) {
    TEST("jsp: skip nested object/array");
    const char *json = "{\"a\": {\"nested\": [1,2,{\"deep\":true}]}, \"b\": 42}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_object(&jsp);

    jsp_key(&jsp);
    ASSERT_STR(jsp.string, "a", "key a");
    ASSERT_EQ(jsp_skip(&jsp), 0, "skip nested");

    jsp_key(&jsp);
    ASSERT_STR(jsp.string, "b", "key b");
    jsp_value(&jsp);
    ASSERT(fabs(jsp.number - 42.0) < 0.001, "b=42");

    jsp_end_object(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_array_length(void) {
    TEST("jsp: array_length");
    const char *json = "[1, 2, 3, 4, 5]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);
    ASSERT_EQ(jsp_array_length(&jsp), 5, "length should be 5");
    // after array_length, offset should be restored
    jsp_value(&jsp);
    ASSERT(fabs(jsp.number - 1.0) < 0.001, "first value should still be 1");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_array_length_empty(void) {
    TEST("jsp: array_length on empty array");
    const char *json = "[]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);
    ASSERT_EQ(jsp_array_length(&jsp), 0, "length should be 0");
    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_array_length_nested(void) {
    TEST("jsp: array_length with nested structures");
    const char *json = "[{\"a\":1}, [1,2], \"str\"]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);
    ASSERT_EQ(jsp_array_length(&jsp), 3, "length should be 3");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_whitespace_handling(void) {
    TEST("jsp: handles extra whitespace");
    const char *json = "  {  \"key\"  :  \"value\"  }  ";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    ASSERT_EQ(jsp_begin_object(&jsp), 0, "begin_object");
    ASSERT_EQ(jsp_key(&jsp), 0, "key");
    ASSERT_STR(jsp.string, "key", "key name");
    ASSERT_EQ(jsp_value(&jsp), 0, "value");
    ASSERT_STR(jsp.string, "value", "value");
    jsp_end_object(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_init_null_buffer(void) {
    TEST("jsp: init with NULL buffer returns -1");
    Jsp jsp = {0};
    ASSERT_EQ(jsp_init(&jsp, NULL, 10), -1, "NULL buffer should fail");
    PASS();
}

void test_jsp_init_zero_length(void) {
    TEST("jsp: init with zero length returns -1");
    Jsp jsp = {0};
    ASSERT_EQ(jsp_init(&jsp, "{}", 0), -1, "zero length should fail");
    PASS();
}

void test_jsp_wrong_container(void) {
    TEST("jsp: end wrong container returns -1");
    const char *json = "{\"a\": 1}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_object(&jsp);
    ASSERT_EQ(jsp_end_array(&jsp), -1, "end_array on object should fail");
    jsp_free(&jsp);

    const char *json2 = "[1]";
    Jsp jsp2 = {0};
    jsp_sinit(&jsp2, json2);
    jsp_begin_array(&jsp2);
    ASSERT_EQ(jsp_end_object(&jsp2), -1, "end_object on array should fail");
    jsp_free(&jsp2);
    PASS();
}

void test_jsp_key_in_array(void) {
    TEST("jsp: key in array context returns -1");
    const char *json = "[1, 2]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);
    ASSERT_EQ(jsp_key(&jsp), -1, "key in array should fail");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_value_in_object_without_key(void) {
    TEST("jsp: value in object without key returns -1");
    const char *json = "{\"a\": 1}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_object(&jsp);
    // try to read value without reading key first
    ASSERT_EQ(jsp_value(&jsp), -1, "value without key should fail");
    jsp_free(&jsp);
    PASS();
}

void test_jsp_empty_string_value(void) {
    TEST("jsp: parse empty string value");
    const char *json = "{\"k\": \"\"}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_object(&jsp);
    jsp_key(&jsp);
    jsp_value(&jsp);
    ASSERT_EQ(jsp.type, JSP_TYPE_STRING, "type should be string");
    ASSERT_STR(jsp.string, "", "should be empty string");
    jsp_end_object(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_object_in_array(void) {
    TEST("jsp: objects inside array");
    const char *json = "[{\"a\":1},{\"b\":2}]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);

    ASSERT_EQ(jsp_begin_object(&jsp), 0, "obj1 begin");
    jsp_key(&jsp); ASSERT_STR(jsp.string, "a", "key a");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 1.0) < 0.001, "a=1");
    jsp_end_object(&jsp);

    ASSERT_EQ(jsp_begin_object(&jsp), 0, "obj2 begin");
    jsp_key(&jsp); ASSERT_STR(jsp.string, "b", "key b");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 2.0) < 0.001, "b=2");
    jsp_end_object(&jsp);

    ASSERT_EQ(jsp_begin_object(&jsp), -1, "no more objects");
    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_large_number(void) {
    TEST("jsp: parse large numbers");
    const char *json = "[999999999999, -999999999999, 1.7976931348623157e308]";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_array(&jsp);
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 999999999999.0) < 1.0, "large positive");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - (-999999999999.0)) < 1.0, "large negative");
    jsp_value(&jsp); ASSERT(jsp.number > 1e307, "very large");
    jsp_end_array(&jsp);
    jsp_free(&jsp);
    PASS();
}

void test_jsp_skip_entire_array(void) {
    TEST("jsp: skip array value");
    const char *json = "{\"arr\": [1,2,3], \"after\": true}";
    Jsp jsp = {0};
    jsp_sinit(&jsp, json);
    jsp_begin_object(&jsp);

    jsp_key(&jsp); ASSERT_STR(jsp.string, "arr", "key arr");
    ASSERT_EQ(jsp_skip(&jsp), 0, "skip array");

    jsp_key(&jsp); ASSERT_STR(jsp.string, "after", "key after");
    jsp_value(&jsp);
    ASSERT_EQ(jsp.type, JSP_TYPE_BOOLEAN, "should be boolean");
    ASSERT_EQ(jsp.boolean, true, "should be true");

    jsp_end_object(&jsp);
    jsp_free(&jsp);
    PASS();
}

// ============================================================================
// JSB -> JSP roundtrip tests
// ============================================================================

void test_roundtrip_simple(void) {
    TEST("roundtrip: build then parse simple object");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "name"); jsb_string(&jsb, "Alice");
    jsb_key(&jsb, "age"); jsb_int(&jsb, 25);
    jsb_key(&jsb, "active"); jsb_bool(&jsb, true);
    jsb_end_object(&jsb);

    Jsp jsp = {0};
    jsp_sinit(&jsp, jsb_get(&jsb));
    jsp_begin_object(&jsp);

    jsp_key(&jsp); ASSERT_STR(jsp.string, "name", "key name");
    jsp_value(&jsp); ASSERT_STR(jsp.string, "Alice", "name=Alice");

    jsp_key(&jsp); ASSERT_STR(jsp.string, "age", "key age");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 25.0) < 0.001, "age=25");

    jsp_key(&jsp); ASSERT_STR(jsp.string, "active", "key active");
    jsp_value(&jsp); ASSERT_EQ(jsp.boolean, true, "active=true");

    ASSERT_EQ(jsp_key(&jsp), -1, "no more keys");
    jsp_end_object(&jsp);
    jsp_free(&jsp);
    jsb_free(&jsb);
    PASS();
}

void test_roundtrip_nested(void) {
    TEST("roundtrip: nested object and array");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "items");
    jsb_begin_array(&jsb);
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "id"); jsb_int(&jsb, 1);
    jsb_key(&jsb, "tags");
    jsb_begin_array(&jsb);
    jsb_string(&jsb, "a");
    jsb_string(&jsb, "b");
    jsb_end_array(&jsb);
    jsb_end_object(&jsb);
    jsb_end_array(&jsb);
    jsb_key(&jsb, "count"); jsb_int(&jsb, 1);
    jsb_end_object(&jsb);

    Jsp jsp = {0};
    jsp_sinit(&jsp, jsb_get(&jsb));
    jsp_begin_object(&jsp);

    jsp_key(&jsp); ASSERT_STR(jsp.string, "items", "key items");
    jsp_begin_array(&jsp);
    jsp_begin_object(&jsp);
    jsp_key(&jsp); ASSERT_STR(jsp.string, "id", "key id");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 1.0) < 0.001, "id=1");
    jsp_key(&jsp); ASSERT_STR(jsp.string, "tags", "key tags");
    jsp_begin_array(&jsp);
    jsp_value(&jsp); ASSERT_STR(jsp.string, "a", "tag a");
    jsp_value(&jsp); ASSERT_STR(jsp.string, "b", "tag b");
    jsp_end_array(&jsp);
    jsp_end_object(&jsp);
    jsp_end_array(&jsp);

    jsp_key(&jsp); ASSERT_STR(jsp.string, "count", "key count");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 1.0) < 0.001, "count=1");

    jsp_end_object(&jsp);
    jsp_free(&jsp);
    jsb_free(&jsb);
    PASS();
}

void test_roundtrip_special_strings(void) {
    TEST("roundtrip: strings with escapes survive build+parse");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "msg");
    jsb_string(&jsb, "line1\nline2\ttab\"quote\\back");
    jsb_end_object(&jsb);

    Jsp jsp = {0};
    jsp_sinit(&jsp, jsb_get(&jsb));
    jsp_begin_object(&jsp);
    jsp_key(&jsp);
    jsp_value(&jsp);
    ASSERT_STR(jsp.string, "line1\nline2\ttab\"quote\\back", "escaped roundtrip");
    jsp_end_object(&jsp);
    jsp_free(&jsp);
    jsb_free(&jsb);
    PASS();
}

void test_roundtrip_null_values(void) {
    TEST("roundtrip: null values");
    Jsb jsb = {0};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "a"); jsb_null(&jsb);
    jsb_key(&jsb, "b"); jsb_string(&jsb, NULL);
    jsb_end_object(&jsb);

    Jsp jsp = {0};
    jsp_sinit(&jsp, jsb_get(&jsb));
    jsp_begin_object(&jsp);
    jsp_key(&jsp); jsp_value(&jsp);
    ASSERT_EQ(jsp.type, JSP_TYPE_NULL, "a should be null");
    jsp_key(&jsp); jsp_value(&jsp);
    ASSERT_EQ(jsp.type, JSP_TYPE_NULL, "b (NULL string) should be null");
    jsp_end_object(&jsp);
    jsp_free(&jsp);
    jsb_free(&jsb);
    PASS();
}

void test_roundtrip_pretty_print(void) {
    TEST("roundtrip: pretty-printed JSON is parseable");
    Jsb jsb = {.pp = 4};
    jsb_begin_object(&jsb);
    jsb_key(&jsb, "x"); jsb_int(&jsb, 1);
    jsb_key(&jsb, "arr");
    jsb_begin_array(&jsb);
    jsb_string(&jsb, "a");
    jsb_string(&jsb, "b");
    jsb_end_array(&jsb);
    jsb_end_object(&jsb);

    Jsp jsp = {0};
    jsp_sinit(&jsp, jsb_get(&jsb));
    jsp_begin_object(&jsp);
    jsp_key(&jsp); ASSERT_STR(jsp.string, "x", "key x");
    jsp_value(&jsp); ASSERT(fabs(jsp.number - 1.0) < 0.001, "x=1");
    jsp_key(&jsp); ASSERT_STR(jsp.string, "arr", "key arr");
    jsp_begin_array(&jsp);
    jsp_value(&jsp); ASSERT_STR(jsp.string, "a", "arr[0]=a");
    jsp_value(&jsp); ASSERT_STR(jsp.string, "b", "arr[1]=b");
    jsp_end_array(&jsp);
    jsp_end_object(&jsp);
    jsp_free(&jsp);
    jsb_free(&jsb);
    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    // JSB
    SECTION("JSB: Basic building");
    test_jsb_empty_object();
    test_jsb_empty_array();
    test_jsb_single_string();
    test_jsb_multiple_types();
    test_jsb_nested_objects();
    test_jsb_array_values();
    test_jsb_array_of_objects();
    test_jsb_nested_arrays();
    test_jsb_top_level_array();
    test_jsb_nstring();

    SECTION("JSB: Values & escaping");
    test_jsb_string_escaping();
    test_jsb_empty_string();
    test_jsb_null_string();
    test_jsb_negative_int();
    test_jsb_number_precision();
    test_jsb_key_with_special_chars();

    SECTION("JSB: Pretty print");
    test_jsb_pretty_print();

    SECTION("JSB: State validation");
    test_jsb_value_without_key();
    test_jsb_key_in_array();
    test_jsb_double_key();
    test_jsb_end_wrong_container();
    test_jsb_end_without_begin();

    SECTION("JSB: Lifecycle");
    test_jsb_reuse_after_free();
    test_jsb_free_idempotent();
    test_jsb_deeply_nested();

    // JSP
    SECTION("JSP: Basic parsing");
    test_jsp_simple_object();
    test_jsp_simple_array();
    test_jsp_empty_object();
    test_jsp_empty_array();
    test_jsp_all_types();

    SECTION("JSP: Nested structures");
    test_jsp_nested_object();
    test_jsp_nested_arrays();
    test_jsp_object_in_array();

    SECTION("JSP: Values & types");
    test_jsp_string_escapes();
    test_jsp_unicode_escape();
    test_jsp_negative_numbers();
    test_jsp_large_number();
    test_jsp_empty_string_value();
    test_jsp_whitespace_handling();

    SECTION("JSP: Skip");
    test_jsp_skip_value();
    test_jsp_skip_nested();
    test_jsp_skip_entire_array();

    SECTION("JSP: Array length");
    test_jsp_array_length();
    test_jsp_array_length_empty();
    test_jsp_array_length_nested();

    SECTION("JSP: Error handling");
    test_jsp_init_null_buffer();
    test_jsp_init_zero_length();
    test_jsp_wrong_container();
    test_jsp_key_in_array();
    test_jsp_value_in_object_without_key();

    // Roundtrip
    SECTION("Roundtrip: JSB -> JSP");
    test_roundtrip_simple();
    test_roundtrip_nested();
    test_roundtrip_special_strings();
    test_roundtrip_null_values();
    test_roundtrip_pretty_print();

    // Summary
    printf("\n=== Results ===\n");
    printf("Total: %d | \033[32mPassed: %d\033[0m | \033[31mFailed: %d\033[0m\n",
           tests_run, tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
