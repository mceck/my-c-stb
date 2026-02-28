#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "../jsb.h"
#include "../jsp.h"
#include "../jsgen/jsgen.h"

#include "jsgen_models.h"
#include "jsgen_models.g.h"

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
#define ASSERT_NEAR(a, b, eps, msg) ASSERT(fabs((a) - (b)) < (eps), msg)

#define SECTION(name) printf("\n\033[1m[%s]\033[0m\n", name)

// ============================================================================
// BasicModel tests
// ============================================================================

void test_basic_parse(void) {
    TEST("basic: parse all primitive types");
    const char *json = "{\"id\": 42, \"name\": \"Alice\", \"score\": 9.5, \"rating\": 4.2, \"active\": true, \"timestamp\": 1000000, \"count\": 7}";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 42, "id");
    ASSERT_STR(m.name, "Alice", "name");
    ASSERT_NEAR(m.score, 9.5, 0.01, "score");
    ASSERT_NEAR(m.rating, 4.2, 0.1, "rating");
    ASSERT_EQ(m.active, true, "active");
    ASSERT_EQ(m.timestamp, 1000000, "timestamp");
    ASSERT_EQ(m.count, 7, "count");
    jsgen_free();
    PASS();
}

void test_basic_stringify(void) {
    TEST("basic: stringify all primitive types");
    BasicModel m = {.id = 1, .name = "Bob", .score = 3.14, .rating = 2.5, .active = false, .timestamp = 999, .count = 3};
    char *json = stringify_BasicModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");

    // re-parse to verify
    BasicModel m2 = {0};
    ASSERT_EQ(parse_BasicModel(json, &m2), 0, "re-parse failed");
    ASSERT_EQ(m2.id, 1, "id roundtrip");
    ASSERT_STR(m2.name, "Bob", "name roundtrip");
    ASSERT_EQ(m2.active, false, "active roundtrip");
    ASSERT_EQ(m2.count, 3, "count roundtrip");

    free(json);
    jsgen_free();
    PASS();
}

void test_basic_null_name(void) {
    TEST("basic: parse null string field");
    const char *json = "{\"id\": 1, \"name\": null, \"score\": 0, \"rating\": 0, \"active\": false, \"timestamp\": 0, \"count\": 0}";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 1, "id");
    ASSERT_EQ(m.name, NULL, "name should be NULL");
    jsgen_free();
    PASS();
}

void test_basic_empty_string(void) {
    TEST("basic: parse empty string field");
    const char *json = "{\"id\": 1, \"name\": \"\", \"score\": 0, \"rating\": 0, \"active\": false, \"timestamp\": 0, \"count\": 0}";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "parse failed");
    // empty string => NULL per il codice generato (s_len == 0)
    ASSERT_EQ(m.name, NULL, "empty string becomes NULL");
    jsgen_free();
    PASS();
}

void test_basic_extra_fields(void) {
    TEST("basic: parse ignores unknown fields");
    const char *json = "{\"id\": 1, \"unknown_field\": \"ignored\", \"name\": \"Test\", \"score\": 0, \"rating\": 0, \"active\": false, \"timestamp\": 0, \"count\": 0}";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 1, "id");
    ASSERT_STR(m.name, "Test", "name");
    jsgen_free();
    PASS();
}

void test_basic_partial_fields(void) {
    TEST("basic: parse with missing fields keeps defaults");
    const char *json = "{\"id\": 99}";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 99, "id");
    ASSERT_EQ(m.name, NULL, "name default NULL");
    ASSERT_EQ(m.active, false, "active default false");
    jsgen_free();
    PASS();
}

void test_basic_negative_values(void) {
    TEST("basic: parse negative numbers");
    const char *json = "{\"id\": -5, \"name\": \"neg\", \"score\": -1.5, \"rating\": -0.1, \"active\": false, \"timestamp\": -100, \"count\": 0}";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, -5, "id");
    ASSERT_NEAR(m.score, -1.5, 0.01, "score");
    ASSERT_NEAR(m.rating, -0.1, 0.01, "rating");
    ASSERT_EQ(m.timestamp, -100, "timestamp");
    jsgen_free();
    PASS();
}

void test_basic_stringify_null_name(void) {
    TEST("basic: stringify with NULL name pointer");
    BasicModel m = {.id = 1, .name = NULL, .score = 0, .rating = 0, .active = false, .timestamp = 0, .count = 0};
    char *json = stringify_BasicModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");
    // should contain "name" key with some value (null or omitted depending on impl)
    // re-parse to check roundtrip
    BasicModel m2 = {0};
    ASSERT_EQ(parse_BasicModel(json, &m2), 0, "re-parse failed");
    ASSERT_EQ(m2.name, NULL, "name should remain NULL");
    free(json);
    jsgen_free();
    PASS();
}

void test_basic_list_parse(void) {
    TEST("basic: parse list of BasicModel");
    const char *json = "[{\"id\": 1, \"name\": \"A\"}, {\"id\": 2, \"name\": \"B\"}]";
    BasicModel *list = NULL;
    size_t count = 0;
    ASSERT_EQ(parse_BasicModel_list(json, &list, &count), 0, "parse list failed");
    ASSERT_EQ(count, 2, "count should be 2");
    ASSERT_EQ(list[0].id, 1, "list[0].id");
    ASSERT_STR(list[0].name, "A", "list[0].name");
    ASSERT_EQ(list[1].id, 2, "list[1].id");
    ASSERT_STR(list[1].name, "B", "list[1].name");
    jsgen_free();
    PASS();
}

void test_basic_list_empty(void) {
    TEST("basic: parse empty list");
    const char *json = "[]";
    BasicModel *list = NULL;
    size_t count = 0;
    ASSERT_EQ(parse_BasicModel_list(json, &list, &count), 0, "parse empty list failed");
    ASSERT_EQ(count, 0, "count should be 0");
    jsgen_free();
    PASS();
}

void test_basic_list_stringify(void) {
    TEST("basic: stringify list roundtrip");
    BasicModel items[2] = {
        {.id = 10, .name = "X", .score = 1.0},
        {.id = 20, .name = "Y", .score = 2.0},
    };
    char *json = stringify_BasicModel_list(items, 2);
    ASSERT_NEQ(json, NULL, "stringify list returned NULL");

    BasicModel *out = NULL;
    size_t count = 0;
    ASSERT_EQ(parse_BasicModel_list(json, &out, &count), 0, "re-parse list failed");
    ASSERT_EQ(count, 2, "count");
    ASSERT_EQ(out[0].id, 10, "out[0].id");
    ASSERT_STR(out[0].name, "X", "out[0].name");
    ASSERT_EQ(out[1].id, 20, "out[1].id");

    free(json);
    jsgen_free();
    PASS();
}

// ============================================================================
// AliasModel tests
// ============================================================================

void test_alias_parse(void) {
    TEST("alias: parse with aliased field names");
    const char *json = "{\"id\": 1, \"firstName\": \"John\", \"lastName\": \"Doe\", \"isActive\": true}";
    AliasModel m = {0};
    ASSERT_EQ(parse_AliasModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 1, "id");
    ASSERT_STR(m.first_name, "John", "firstName -> first_name");
    ASSERT_STR(m.last_name, "Doe", "lastName -> last_name");
    ASSERT_EQ(m.is_active, true, "isActive -> is_active");
    jsgen_free();
    PASS();
}

void test_alias_stringify(void) {
    TEST("alias: stringify uses alias names");
    AliasModel m = {.id = 2, .first_name = "Jane", .last_name = "Smith", .is_active = false};
    char *json = stringify_AliasModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");
    // JSON should contain aliased names
    ASSERT(strstr(json, "\"firstName\"") != NULL, "should contain firstName");
    ASSERT(strstr(json, "\"lastName\"") != NULL, "should contain lastName");
    ASSERT(strstr(json, "\"isActive\"") != NULL, "should contain isActive");
    // should NOT contain C field names
    ASSERT(strstr(json, "\"first_name\"") == NULL, "should not contain first_name");

    free(json);
    jsgen_free();
    PASS();
}

void test_alias_roundtrip(void) {
    TEST("alias: stringify -> parse roundtrip");
    AliasModel m = {.id = 3, .first_name = "Bob", .last_name = "Brown", .is_active = true};
    char *json = stringify_AliasModel(&m);
    AliasModel m2 = {0};
    ASSERT_EQ(parse_AliasModel(json, &m2), 0, "roundtrip parse failed");
    ASSERT_EQ(m2.id, 3, "id");
    ASSERT_STR(m2.first_name, "Bob", "first_name");
    ASSERT_STR(m2.last_name, "Brown", "last_name");
    ASSERT_EQ(m2.is_active, true, "is_active");
    free(json);
    jsgen_free();
    PASS();
}

// ============================================================================
// PersonModel (nested struct pointer) tests
// ============================================================================

void test_person_parse_with_address(void) {
    TEST("person: parse with nested address object");
    const char *json = "{\"id\": 1, \"name\": \"Alice\", \"address\": {\"street\": \"123 Main St\", \"city\": \"NY\", \"zip\": 10001}}";
    PersonModel m = {0};
    ASSERT_EQ(parse_PersonModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 1, "id");
    ASSERT_STR(m.name, "Alice", "name");
    ASSERT_NEQ(m.address, NULL, "address should not be NULL");
    ASSERT_STR(m.address->street, "123 Main St", "street");
    ASSERT_STR(m.address->city, "NY", "city");
    ASSERT_EQ(m.address->zip, 10001, "zip");
    jsgen_free();
    PASS();
}

void test_person_parse_null_address(void) {
    TEST("person: parse with null address");
    const char *json = "{\"id\": 2, \"name\": \"Bob\", \"address\": null}";
    PersonModel m = {0};
    ASSERT_EQ(parse_PersonModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 2, "id");
    ASSERT_STR(m.name, "Bob", "name");
    ASSERT_EQ(m.address, NULL, "address should be NULL");
    jsgen_free();
    PASS();
}

void test_person_stringify_roundtrip(void) {
    TEST("person: stringify -> parse roundtrip with address");
    Address addr = {.street = "456 Oak Ave", .city = "LA", .zip = 90001};
    PersonModel m = {.id = 3, .name = "Charlie", .address = &addr};
    char *json = stringify_PersonModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");

    PersonModel m2 = {0};
    ASSERT_EQ(parse_PersonModel(json, &m2), 0, "re-parse failed");
    ASSERT_EQ(m2.id, 3, "id");
    ASSERT_STR(m2.name, "Charlie", "name");
    ASSERT_NEQ(m2.address, NULL, "address");
    ASSERT_STR(m2.address->street, "456 Oak Ave", "street");
    ASSERT_EQ(m2.address->zip, 90001, "zip");
    free(json);
    jsgen_free();
    PASS();
}

void test_person_stringify_null_address(void) {
    TEST("person: stringify with NULL address pointer");
    PersonModel m = {.id = 4, .name = "Dave", .address = NULL};
    char *json = stringify_PersonModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");
    // re-parse
    PersonModel m2 = {0};
    ASSERT_EQ(parse_PersonModel(json, &m2), 0, "re-parse failed");
    ASSERT_EQ(m2.address, NULL, "address should remain NULL");
    free(json);
    jsgen_free();
    PASS();
}

void test_person_missing_address(void) {
    TEST("person: parse without address field keeps default");
    const char *json = "{\"id\": 5, \"name\": \"Eve\"}";
    PersonModel m = {0};
    ASSERT_EQ(parse_PersonModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 5, "id");
    ASSERT_EQ(m.address, NULL, "address default NULL");
    jsgen_free();
    PASS();
}

// ============================================================================
// TaggedModel (sized_by array of structs) tests
// ============================================================================

void test_tagged_parse(void) {
    TEST("tagged: parse array of struct tags");
    const char *json = "{\"id\": 1, \"tags\": [{\"key\": \"env\", \"value\": \"prod\"}, {\"key\": \"ver\", \"value\": \"1.0\"}]}";
    TaggedModel m = {0};
    ASSERT_EQ(parse_TaggedModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 1, "id");
    ASSERT_EQ(m.tag_count, 2, "tag_count");
    ASSERT_STR(m.tags[0].key, "env", "tags[0].key");
    ASSERT_STR(m.tags[0].value, "prod", "tags[0].value");
    ASSERT_STR(m.tags[1].key, "ver", "tags[1].key");
    ASSERT_STR(m.tags[1].value, "1.0", "tags[1].value");
    jsgen_free();
    PASS();
}

void test_tagged_empty_tags(void) {
    TEST("tagged: parse with empty tags array");
    const char *json = "{\"id\": 2, \"tags\": []}";
    TaggedModel m = {0};
    ASSERT_EQ(parse_TaggedModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 2, "id");
    ASSERT_EQ(m.tag_count, 0, "tag_count should be 0");
    jsgen_free();
    PASS();
}

void test_tagged_roundtrip(void) {
    TEST("tagged: stringify -> parse roundtrip");
    Tag tags[] = {{.key = "a", .value = "1"}, {.key = "b", .value = "2"}, {.key = "c", .value = "3"}};
    TaggedModel m = {.id = 10, .tags = tags, .tag_count = 3};
    char *json = stringify_TaggedModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");

    TaggedModel m2 = {0};
    ASSERT_EQ(parse_TaggedModel(json, &m2), 0, "re-parse failed");
    ASSERT_EQ(m2.id, 10, "id");
    ASSERT_EQ(m2.tag_count, 3, "tag_count");
    ASSERT_STR(m2.tags[2].key, "c", "tags[2].key");
    ASSERT_STR(m2.tags[2].value, "3", "tags[2].value");
    free(json);
    jsgen_free();
    PASS();
}

// ============================================================================
// IgnoreModel tests
// ============================================================================

void test_ignore_stringify(void) {
    TEST("ignore: ignored field not in JSON output");
    IgnoreModel m = {.id = 1, .name = "Test", .visible = true};
    char *json = stringify_IgnoreModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");
    ASSERT(strstr(json, "\"id\"") != NULL, "should contain id");
    ASSERT(strstr(json, "\"name\"") != NULL, "should contain name");
    ASSERT(strstr(json, "\"visible\"") != NULL, "should contain visible");
    ASSERT(strstr(json, "internal_secret") == NULL, "should not contain internal_secret");
    free(json);
    jsgen_free();
    PASS();
}

void test_ignore_parse(void) {
    TEST("ignore: parse ignores extra fields gracefully");
    const char *json = "{\"id\": 1, \"name\": \"X\", \"internal_secret\": \"leaked\", \"visible\": true}";
    IgnoreModel m = {0};
    ASSERT_EQ(parse_IgnoreModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 1, "id");
    ASSERT_STR(m.name, "X", "name");
    ASSERT_EQ(m.visible, true, "visible");
    jsgen_free();
    PASS();
}

// ============================================================================
// MinimalModel tests
// ============================================================================

void test_minimal_parse(void) {
    TEST("minimal: parse single field struct");
    const char *json = "{\"dummy\": 99}";
    MinimalModel m = {0};
    ASSERT_EQ(parse_MinimalModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.dummy, 99, "dummy");
    jsgen_free();
    PASS();
}

void test_minimal_roundtrip(void) {
    TEST("minimal: stringify -> parse roundtrip");
    MinimalModel m = {.dummy = 7};
    char *json = stringify_MinimalModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");
    MinimalModel m2 = {0};
    ASSERT_EQ(parse_MinimalModel(json, &m2), 0, "re-parse failed");
    ASSERT_EQ(m2.dummy, 7, "dummy");
    free(json);
    jsgen_free();
    PASS();
}

// ============================================================================
// InlineNestedModel (non-pointer nested struct) tests
// ============================================================================

void test_inline_nested_parse(void) {
    TEST("inline nested: parse struct with inline nested struct");
    const char *json = "{\"name\": \"point\", \"pos\": {\"x\": 10, \"y\": 20}}";
    InlineNestedModel m = {0};
    ASSERT_EQ(parse_InlineNestedModel(json, &m), 0, "parse failed");
    ASSERT_STR(m.name, "point", "name");
    ASSERT_EQ(m.pos.x, 10, "pos.x");
    ASSERT_EQ(m.pos.y, 20, "pos.y");
    jsgen_free();
    PASS();
}

void test_inline_nested_roundtrip(void) {
    TEST("inline nested: stringify -> parse roundtrip");
    InlineNestedModel m = {.name = "origin", .pos = {.x = 0, .y = 0}};
    char *json = stringify_InlineNestedModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");

    InlineNestedModel m2 = {0};
    ASSERT_EQ(parse_InlineNestedModel(json, &m2), 0, "re-parse failed");
    ASSERT_STR(m2.name, "origin", "name");
    ASSERT_EQ(m2.pos.x, 0, "pos.x");
    ASSERT_EQ(m2.pos.y, 0, "pos.y");
    free(json);
    jsgen_free();
    PASS();
}

// ============================================================================
// JSONS / JSONP (stringify only / parse only) tests
// ============================================================================

void test_stringify_only(void) {
    TEST("JSONS: stringify-only model produces valid JSON");
    StringifyOnlyModel m = {.code = 200, .message = "OK"};
    char *json = stringify_StringifyOnlyModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");
    ASSERT(strstr(json, "200") != NULL, "should contain 200");
    ASSERT(strstr(json, "\"OK\"") != NULL, "should contain OK");
    free(json);
    PASS();
}

void test_parse_only(void) {
    TEST("JSONP: parse-only model parses correctly");
    const char *json = "{\"code\": 404, \"message\": \"Not Found\"}";
    ParseOnlyModel m = {0};
    ASSERT_EQ(parse_ParseOnlyModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.code, 404, "code");
    ASSERT_STR(m.message, "Not Found", "message");
    jsgen_free();
    PASS();
}

// ============================================================================
// DualAddressModel (multiple nullable pointers) tests
// ============================================================================

void test_dual_both_present(void) {
    TEST("dual address: both home and work present");
    const char *json = "{\"name\": \"A\", \"home\": {\"street\": \"h\", \"city\": \"hc\", \"zip\": 1}, \"work\": {\"street\": \"w\", \"city\": \"wc\", \"zip\": 2}}";
    DualAddressModel m = {0};
    ASSERT_EQ(parse_DualAddressModel(json, &m), 0, "parse failed");
    ASSERT_STR(m.name, "A", "name");
    ASSERT_NEQ(m.home, NULL, "home not NULL");
    ASSERT_STR(m.home->street, "h", "home.street");
    ASSERT_NEQ(m.work, NULL, "work not NULL");
    ASSERT_STR(m.work->street, "w", "work.street");
    jsgen_free();
    PASS();
}

void test_dual_work_null(void) {
    TEST("dual address: work is null");
    const char *json = "{\"name\": \"B\", \"home\": {\"street\": \"h\", \"city\": \"hc\", \"zip\": 1}, \"work\": null}";
    DualAddressModel m = {0};
    ASSERT_EQ(parse_DualAddressModel(json, &m), 0, "parse failed");
    ASSERT_NEQ(m.home, NULL, "home not NULL");
    ASSERT_EQ(m.work, NULL, "work should be NULL");
    jsgen_free();
    PASS();
}

void test_dual_stringify_roundtrip(void) {
    TEST("dual address: stringify -> parse roundtrip");
    Address home = {.street = "Home St", .city = "HC", .zip = 100};
    Address work = {.street = "Work St", .city = "WC", .zip = 200};
    DualAddressModel m = {.name = "C", .home = &home, .work = &work};
    char *json = stringify_DualAddressModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");

    DualAddressModel m2 = {0};
    ASSERT_EQ(parse_DualAddressModel(json, &m2), 0, "re-parse failed");
    ASSERT_STR(m2.name, "C", "name");
    ASSERT_NEQ(m2.home, NULL, "home");
    ASSERT_STR(m2.home->city, "HC", "home.city");
    ASSERT_NEQ(m2.work, NULL, "work");
    ASSERT_STR(m2.work->city, "WC", "work.city");
    free(json);
    jsgen_free();
    PASS();
}

// ============================================================================
// Edge cases and error handling
// ============================================================================

void test_invalid_json(void) {
    TEST("error: parse invalid JSON returns error");
    const char *json = "{invalid json}";
    BasicModel m = {0};
    ASSERT_NEQ(parse_BasicModel(json, &m), 0, "should fail on invalid JSON");
    jsgen_free();
    PASS();
}

void test_empty_object(void) {
    TEST("error: parse empty object");
    const char *json = "{}";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "empty object should parse ok");
    ASSERT_EQ(m.id, 0, "defaults to 0");
    jsgen_free();
    PASS();
}

void test_string_with_escapes(void) {
    TEST("edge: strings with escape characters roundtrip");
    AliasModel m = {.id = 1, .first_name = "John \"Johnny\" Doe", .last_name = "O'Brien\\Smith", .is_active = true};
    char *json = stringify_AliasModel(&m);
    ASSERT_NEQ(json, NULL, "stringify returned NULL");

    AliasModel m2 = {0};
    ASSERT_EQ(parse_AliasModel(json, &m2), 0, "re-parse failed");
    ASSERT_STR(m2.first_name, "John \"Johnny\" Doe", "first_name with quotes");
    ASSERT_STR(m2.last_name, "O'Brien\\Smith", "last_name with backslash");
    free(json);
    jsgen_free();
    PASS();
}

void test_list_stringify_roundtrip(void) {
    TEST("edge: list stringify -> parse roundtrip");
    Tag tags1[] = {{.key = "a", .value = "1"}};
    Tag tags2[] = {{.key = "b", .value = "2"}, {.key = "c", .value = "3"}};
    TaggedModel items[2] = {
        {.id = 1, .tags = tags1, .tag_count = 1},
        {.id = 2, .tags = tags2, .tag_count = 2},
    };
    char *json = stringify_TaggedModel_list(items, 2);
    ASSERT_NEQ(json, NULL, "stringify list returned NULL");

    TaggedModel *out = NULL;
    size_t count = 0;
    ASSERT_EQ(parse_TaggedModel_list(json, &out, &count), 0, "re-parse list failed");
    ASSERT_EQ(count, 2, "count");
    ASSERT_EQ(out[0].tag_count, 1, "out[0].tag_count");
    ASSERT_STR(out[0].tags[0].key, "a", "out[0].tags[0].key");
    ASSERT_EQ(out[1].tag_count, 2, "out[1].tag_count");
    ASSERT_STR(out[1].tags[1].key, "c", "out[1].tags[1].key");
    free(json);
    jsgen_free();
    PASS();
}

void test_stringify_indent(void) {
    TEST("edge: stringify with indentation produces parseable JSON");
    BasicModel m = {.id = 1, .name = "Indented"};
    char *json = stringify_BasicModel_indent(&m, 4);
    ASSERT_NEQ(json, NULL, "stringify indented returned NULL");
    ASSERT(strstr(json, "\n") != NULL, "should contain newlines");

    BasicModel m2 = {0};
    ASSERT_EQ(parse_BasicModel(json, &m2), 0, "re-parse indented failed");
    ASSERT_EQ(m2.id, 1, "id");
    ASSERT_STR(m2.name, "Indented", "name");
    free(json);
    jsgen_free();
    PASS();
}

void test_jsgen_free_reuse(void) {
    TEST("edge: jsgen_free allows reuse of allocator");
    const char *json = "{\"id\": 1, \"name\": \"first\"}";
    BasicModel m1 = {0};
    ASSERT_EQ(parse_BasicModel(json, &m1), 0, "first parse failed");
    ASSERT_STR(m1.name, "first", "first name");
    jsgen_free();

    // after free, allocator should work again
    const char *json2 = "{\"id\": 2, \"name\": \"second\"}";
    BasicModel m2 = {0};
    ASSERT_EQ(parse_BasicModel(json2, &m2), 0, "second parse failed");
    ASSERT_STR(m2.name, "second", "second name");
    jsgen_free();
    PASS();
}

void test_nested_list_parse(void) {
    TEST("edge: parse list of models with nested structs");
    const char *json = "[{\"id\": 1, \"name\": \"A\", \"address\": {\"street\": \"s1\", \"city\": \"c1\", \"zip\": 1}},"
                       "{\"id\": 2, \"name\": \"B\", \"address\": null}]";
    PersonModel *list = NULL;
    size_t count = 0;
    ASSERT_EQ(parse_PersonModel_list(json, &list, &count), 0, "parse list failed");
    ASSERT_EQ(count, 2, "count");
    ASSERT_NEQ(list[0].address, NULL, "list[0].address not NULL");
    ASSERT_STR(list[0].address->street, "s1", "list[0].address.street");
    ASSERT_EQ(list[1].address, NULL, "list[1].address is NULL");
    jsgen_free();
    PASS();
}

void test_whitespace_json(void) {
    TEST("edge: parse JSON with lots of whitespace");
    const char *json = "  {  \n  \"id\"  :  42  ,  \n  \"name\"  :  \"spaced\"  \n  }  ";
    BasicModel m = {0};
    ASSERT_EQ(parse_BasicModel(json, &m), 0, "parse failed");
    ASSERT_EQ(m.id, 42, "id");
    ASSERT_STR(m.name, "spaced", "name");
    jsgen_free();
    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    SECTION("BasicModel");
    test_basic_parse();
    test_basic_stringify();
    test_basic_null_name();
    test_basic_empty_string();
    test_basic_extra_fields();
    test_basic_partial_fields();
    test_basic_negative_values();
    test_basic_stringify_null_name();
    test_basic_list_parse();
    test_basic_list_empty();
    test_basic_list_stringify();

    SECTION("AliasModel");
    test_alias_parse();
    test_alias_stringify();
    test_alias_roundtrip();

    SECTION("PersonModel (nested struct pointer)");
    test_person_parse_with_address();
    test_person_parse_null_address();
    test_person_stringify_roundtrip();
    test_person_stringify_null_address();
    test_person_missing_address();

    SECTION("TaggedModel (sized_by struct array)");
    test_tagged_parse();
    test_tagged_empty_tags();
    test_tagged_roundtrip();

    SECTION("IgnoreModel");
    test_ignore_stringify();
    test_ignore_parse();

    SECTION("MinimalModel");
    test_minimal_parse();
    test_minimal_roundtrip();

    SECTION("InlineNestedModel");
    test_inline_nested_parse();
    test_inline_nested_roundtrip();

    SECTION("JSONS / JSONP");
    test_stringify_only();
    test_parse_only();

    SECTION("DualAddressModel");
    test_dual_both_present();
    test_dual_work_null();
    test_dual_stringify_roundtrip();

    SECTION("Edge cases & errors");
    test_invalid_json();
    test_empty_object();
    test_string_with_escapes();
    test_list_stringify_roundtrip();
    test_stringify_indent();
    test_jsgen_free_reuse();
    test_nested_list_parse();
    test_whitespace_json();

    printf("\n=== Results ===\n");
    printf("Total: %d | \033[32mPassed: %d\033[0m | \033[31mFailed: %d\033[0m\n",
           tests_run, tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}

#define JSB_IMPLEMENTATION
#include "../jsb.h"
#define JSP_IMPLEMENTATION
#include "../jsp.h"
#define JSGEN_IMPLEMENTATION
#include "../jsgen/jsgen.h"
