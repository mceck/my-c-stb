#define DS_IMPLEMENTATION
#include "../ds.h"
#undef DS_IMPLEMENTATION

#define HTTP_IMPLEMENTATION
#include "../http.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

// ============================================================================
// Test Framework (same as test_ds.c)
// ============================================================================

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
// Test Server Management
// ============================================================================

#define TEST_PORT "18234"
#define BASE "http://127.0.0.1:18234"

static pid_t server_pid = 0;

static void stop_server(void) {
    if (server_pid > 0) {
        kill(server_pid, SIGTERM);
        waitpid(server_pid, NULL, 0);
        server_pid = 0;
    }
}

static int start_server(void) {
    int pipefd[2];
    if (pipe(pipefd) != 0) return 0;

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }

    if (pid == 0) {
        // Child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        freopen("/dev/null", "w", stderr);
        execlp("python3", "python3", "tests/http_test_server.py", TEST_PORT, NULL);
        _exit(1);
    }

    // Parent
    server_pid = pid;
    atexit(stop_server);
    close(pipefd[1]);

    // Wait for READY signal
    char buf[64];
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return strstr(buf, "READY") != NULL;
}

// ============================================================================
// Initialization & Cleanup Tests
// ============================================================================

void test_response_zero_init(void) {
    TEST("response: zero-initialized state");
    HttpResponse r = {0};
    ASSERT_EQ(r.status_code, 0, "status should be 0");
    ASSERT_EQ(r.body.length, 0, "body length should be 0");
    ASSERT_EQ(r.body.data, NULL, "body data should be NULL");
    ASSERT_EQ(r.curl_code, 0, "curl_code should be CURLE_OK");
    PASS();
}

void test_free_response_zero(void) {
    TEST("free_response: safe on zero-initialized");
    HttpResponse r = {0};
    http_free_response(&r);
    ASSERT_EQ(r.body.data, NULL, "still NULL after free");
    PASS();
}

void test_free_response_double_free(void) {
    TEST("free_response: double free is safe");
    HttpResponse r = http(BASE "/echo");
    ASSERT_EQ(r.curl_code, CURLE_OK, "request should succeed");
    http_free_response(&r);
    http_free_response(&r); // should not crash
    ASSERT_EQ(r.body.data, NULL, "data NULL after double free");
    PASS();
}

// ============================================================================
// HTTP Method Tests
// ============================================================================

void test_get_basic(void) {
    TEST("GET: basic request returns 200 with body");
    HttpResponse r = http(BASE "/echo");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 200, "status should be 200");
    ASSERT_NEQ(r.body.data, NULL, "body should not be NULL");
    ASSERT(r.body.length > 0, "body should not be empty");
    http_free_response(&r);
    PASS();
}

void test_get_method_verified(void) {
    TEST("GET: server receives GET method");
    HttpResponse r = http(BASE "/method");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "GET", "method should be GET");
    http_free_response(&r);
    PASS();
}

void test_post_method_verified(void) {
    TEST("POST: server receives POST method");
    HttpResponse r = http(BASE "/method", .method = HTTP_POST);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "POST", "method should be POST");
    http_free_response(&r);
    PASS();
}

void test_put_method_verified(void) {
    TEST("PUT: server receives PUT method");
    HttpResponse r = http(BASE "/method", .method = HTTP_PUT);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "PUT", "method should be PUT");
    http_free_response(&r);
    PASS();
}

void test_patch_method_verified(void) {
    TEST("PATCH: server receives PATCH method");
    HttpResponse r = http(BASE "/method", .method = HTTP_PATCH);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "PATCH", "method should be PATCH");
    http_free_response(&r);
    PASS();
}

void test_delete_method_verified(void) {
    TEST("DELETE: server receives DELETE method");
    HttpResponse r = http(BASE "/method", .method = HTTP_DELETE);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "DELETE", "method should be DELETE");
    http_free_response(&r);
    PASS();
}

void test_options_method_verified(void) {
    TEST("OPTIONS: server receives OPTIONS method");
    HttpResponse r = http(BASE "/method", .method = HTTP_OPTIONS);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "OPTIONS", "method should be OPTIONS");
    http_free_response(&r);
    PASS();
}

void test_head_no_body(void) {
    TEST("HEAD: returns status but no body");
    HttpResponse r = http(BASE "/method", .method = HTTP_HEAD);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 200, "status should be 200");
    ASSERT_EQ(r.body.length, 0, "body should be empty for HEAD");
    http_free_response(&r);
    PASS();
}

// ============================================================================
// Request Body Tests
// ============================================================================

void test_post_with_body(void) {
    TEST("POST: body is sent and echoed correctly");
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_POST, .body = "hello world");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 200, "status 200");
    ASSERT_STR(r.body.data, "hello world", "body echoed back");
    http_free_response(&r);
    PASS();
}

void test_post_with_json_body(void) {
    TEST("POST: JSON body with custom Content-Type");
    const char *json = "{\"key\":\"value\",\"num\":42}";
    HttpHeaders h = {0};
    ds_da_append(&h, "Content-Type: application/json");
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_POST, .headers = &h, .body = json);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, json, "JSON body echoed back");
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

void test_post_empty_body_string(void) {
    TEST("POST: empty body string");
    HttpResponse r = http(BASE "/method", .method = HTTP_POST, .body = "");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "POST", "method is still POST");
    http_free_response(&r);
    PASS();
}

void test_post_null_body(void) {
    TEST("POST: NULL body");
    HttpResponse r = http(BASE "/method", .method = HTTP_POST);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "POST", "method is POST");
    http_free_response(&r);
    PASS();
}

void test_put_with_body(void) {
    TEST("PUT: body is sent correctly");
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_PUT, .body = "put data");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "put data", "body echoed back");
    http_free_response(&r);
    PASS();
}

void test_patch_with_body(void) {
    TEST("PATCH: body is sent correctly");
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_PATCH, .body = "patch data");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "patch data", "body echoed back");
    http_free_response(&r);
    PASS();
}

void test_large_request_body(void) {
    TEST("POST: large body (10KB)");
    char *big = malloc(10001);
    ASSERT_NEQ(big, NULL, "malloc should succeed");
    memset(big, 'A', 10000);
    big[10000] = '\0';
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_POST, .body = big);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.body.length, 10000, "body length matches");
    ASSERT_EQ(r.body.data[0], 'A', "first byte");
    ASSERT_EQ(r.body.data[9999], 'A', "last byte");
    free(big);
    http_free_response(&r);
    PASS();
}

void test_body_with_newlines(void) {
    TEST("POST: body with newlines and tabs");
    const char *body = "line1\nline2\ttab\r\nwindows";
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_POST, .body = body);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, body, "body matches including special chars");
    http_free_response(&r);
    PASS();
}

// ============================================================================
// Headers Tests
// ============================================================================

void test_single_custom_header(void) {
    TEST("headers: single custom header received by server");
    HttpHeaders h = {0};
    ds_da_append(&h, "X-Custom-Test: hello123");
    HttpResponse r = http(BASE "/headers", .headers = &h);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT(strstr(r.body.data, "hello123") != NULL, "custom header value present");
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

void test_multiple_custom_headers(void) {
    TEST("headers: multiple custom headers all received");
    HttpHeaders h = {0};
    ds_da_append(&h, "X-First: aaa");
    ds_da_append(&h, "X-Second: bbb");
    ds_da_append(&h, "X-Third: ccc");
    HttpResponse r = http(BASE "/headers", .headers = &h);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT(strstr(r.body.data, "aaa") != NULL, "first header present");
    ASSERT(strstr(r.body.data, "bbb") != NULL, "second header present");
    ASSERT(strstr(r.body.data, "ccc") != NULL, "third header present");
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

void test_empty_headers_array(void) {
    TEST("headers: empty HttpHeaders doesn't break request");
    HttpHeaders h = {0};
    HttpResponse r = http(BASE "/echo", .headers = &h);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 200, "status 200");
    http_free_response(&r);
    PASS();
}

void test_content_type_header(void) {
    TEST("headers: Content-Type header is sent");
    HttpHeaders h = {0};
    ds_da_append(&h, "Content-Type: application/json");
    HttpResponse r = http(BASE "/headers", .headers = &h);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT(strstr(r.body.data, "application/json") != NULL, "Content-Type present");
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

void test_headers_not_leaked_between_requests(void) {
    TEST("headers: custom headers don't leak to next request");
    HttpHeaders h = {0};
    ds_da_append(&h, "X-Leak-Test: should-not-leak");
    HttpResponse r1 = http(BASE "/headers", .headers = &h);
    ASSERT_EQ(r1.curl_code, CURLE_OK, "first request succeeds");
    ASSERT(strstr(r1.body.data, "should-not-leak") != NULL, "header in first");
    http_free_response(&r1);
    ds_da_free(&h);

    // Second request without custom headers
    HttpResponse r2 = http(BASE "/headers");
    ASSERT_EQ(r2.curl_code, CURLE_OK, "second request succeeds");
    ASSERT(strstr(r2.body.data, "should-not-leak") == NULL, "header not in second");
    http_free_response(&r2);
    PASS();
}

// ============================================================================
// Response Tests
// ============================================================================

void test_status_200(void) {
    TEST("response: status code 200");
    HttpResponse r = http(BASE "/status/200");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 200, "status 200");
    http_free_response(&r);
    PASS();
}

void test_status_201(void) {
    TEST("response: status code 201 Created");
    HttpResponse r = http(BASE "/status/201");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 201, "status 201");
    http_free_response(&r);
    PASS();
}

void test_status_204(void) {
    TEST("response: status code 204 No Content");
    HttpResponse r = http(BASE "/status/204");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 204, "status 204");
    http_free_response(&r);
    PASS();
}

void test_status_400(void) {
    TEST("response: status code 400 Bad Request");
    HttpResponse r = http(BASE "/status/400");
    ASSERT_EQ(r.curl_code, CURLE_OK, "curl succeeds (HTTP error != curl error)");
    ASSERT_EQ(r.status_code, 400, "status 400");
    http_free_response(&r);
    PASS();
}

void test_status_404(void) {
    TEST("response: status code 404 Not Found");
    HttpResponse r = http(BASE "/status/404");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 404, "status 404");
    http_free_response(&r);
    PASS();
}

void test_status_500(void) {
    TEST("response: status code 500 Internal Server Error");
    HttpResponse r = http(BASE "/status/500");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 500, "status 500");
    http_free_response(&r);
    PASS();
}

void test_many_status_codes(void) {
    TEST("response: various HTTP status codes");
    int codes[] = {200, 201, 301, 400, 401, 403, 404, 500, 502, 503};
    for (size_t i = 0; i < sizeof(codes)/sizeof(codes[0]); i++) {
        char url[128];
        snprintf(url, sizeof(url), BASE "/status/%d", codes[i]);
        HttpResponse r = http(url);
        ASSERT_EQ(r.curl_code, CURLE_OK, "curl should succeed for HTTP errors");
        ASSERT_EQ(r.status_code, codes[i], "status code mismatch");
        http_free_response(&r);
    }
    PASS();
}

void test_response_body_null_terminated(void) {
    TEST("response: body is null-terminated C string");
    HttpResponse r = http(BASE "/echo");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_NEQ(r.body.data, NULL, "body not NULL");
    ASSERT_EQ(r.body.data[r.body.length], '\0', "null terminated");
    ASSERT_EQ(strlen(r.body.data), r.body.length, "strlen matches length");
    http_free_response(&r);
    PASS();
}

void test_large_response_body(void) {
    TEST("response: large body (100KB) received intact");
    HttpResponse r = http(BASE "/large");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.body.length, 100000, "body length 100KB");
    ASSERT_EQ(r.body.data[0], 'X', "first byte");
    ASSERT_EQ(r.body.data[99999], 'X', "last byte");
    ASSERT_EQ(r.body.data[100000], '\0', "null terminated");
    http_free_response(&r);
    PASS();
}

void test_empty_response_body(void) {
    TEST("response: empty body still has valid C string");
    HttpResponse r = http(BASE "/empty");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 200, "status 200");
    ASSERT_EQ(r.body.length, 0, "body length 0");
    // body.data should be a valid empty string, not NULL
    ASSERT_NEQ(r.body.data, NULL, "body.data is not NULL");
    ASSERT_STR(r.body.data, "", "body.data is empty string");
    http_free_response(&r);
    PASS();
}

void test_404_still_has_body(void) {
    TEST("response: 404 still returns response body");
    HttpResponse r = http(BASE "/nonexistent");
    ASSERT_EQ(r.curl_code, CURLE_OK, "curl succeeds for HTTP 404");
    ASSERT_EQ(r.status_code, 404, "status 404");
    ASSERT(r.body.length > 0, "404 response has body");
    http_free_response(&r);
    PASS();
}

// ============================================================================
// Streaming Tests
// ============================================================================

static size_t _stream_counter;
static size_t _stream_total_bytes;

static size_t stream_cb_count(const char *chunk, size_t len, HttpStreamContext *ctx) {
    (void)ctx; (void)chunk;
    _stream_counter++;
    _stream_total_bytes += len;
    return len;
}

void test_stream_callback_called(void) {
    TEST("stream: callback is invoked");
    _stream_counter = 0;
    _stream_total_bytes = 0;
    HttpResponse r = http(BASE "/echo", .stream_callback = stream_cb_count);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT(r.status_code == 200, "status 200");
    ASSERT(_stream_counter > 0, "callback was called");
    ASSERT(_stream_total_bytes > 0, "received bytes via callback");
    http_free_response(&r);
    PASS();
}

void test_stream_body_not_auto_populated(void) {
    TEST("stream: body NOT populated when custom callback doesn't write it");
    _stream_counter = 0;
    _stream_total_bytes = 0;
    HttpResponse r = http(BASE "/large", .stream_callback = stream_cb_count);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(_stream_total_bytes, 100000, "callback received all 100KB");
    // Body should NOT be populated - our callback doesn't write to ctx->body
    ASSERT_EQ(r.body.length, 0, "body not auto-populated");
    http_free_response(&r);
    PASS();
}

typedef struct {
    int call_count;
} StreamTestData;

static size_t stream_cb_userdata(const char *chunk, size_t len, HttpStreamContext *ctx) {
    (void)chunk;
    StreamTestData *td = (StreamTestData *)ctx->userdata;
    if (td) td->call_count++;
    return len;
}

void test_stream_userdata_passed(void) {
    TEST("stream: userdata is accessible in callback");
    StreamTestData td = {0};
    HttpResponse r = http(BASE "/echo",
        .stream_callback = stream_cb_userdata,
        .stream_userdata = &td);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT(td.call_count > 0, "callback received userdata and used it");
    http_free_response(&r);
    PASS();
}

static size_t stream_cb_abort(const char *chunk, size_t len, HttpStreamContext *ctx) {
    (void)chunk; (void)len; (void)ctx;
    return 0; // abort transfer
}

void test_stream_callback_abort(void) {
    TEST("stream: returning 0 aborts the transfer");
    HttpResponse r = http(BASE "/large", .stream_callback = stream_cb_abort);
    ASSERT_NEQ(r.curl_code, CURLE_OK, "should fail");
    ASSERT_EQ(r.curl_code, CURLE_WRITE_ERROR, "curl reports write error");
    http_free_response(&r);
    PASS();
}

static size_t stream_cb_populate_body(const char *chunk, size_t len, HttpStreamContext *ctx) {
    ds_da_append_many(ctx->body, (char *)chunk, len);
    ds_str_append(ctx->body);
    return len;
}

void test_stream_callback_manual_body_population(void) {
    TEST("stream: callback can manually populate response body");
    HttpResponse r = http(BASE "/echo-body",
        .method = HTTP_POST,
        .body = "streamed",
        .stream_callback = stream_cb_populate_body);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_NEQ(r.body.data, NULL, "body populated by callback");
    ASSERT_STR(r.body.data, "streamed", "body content matches");
    http_free_response(&r);
    PASS();
}

static size_t stream_cb_partial_return(const char *chunk, size_t len, HttpStreamContext *ctx) {
    (void)chunk; (void)ctx;
    // Return less than len to signal an error to curl
    if (len > 1) return len - 1;
    return len;
}

void test_stream_callback_partial_return(void) {
    TEST("stream: returning less than len aborts transfer");
    HttpResponse r = http(BASE "/large", .stream_callback = stream_cb_partial_return);
    // curl should detect the mismatch and abort
    ASSERT_NEQ(r.curl_code, CURLE_OK, "should fail");
    http_free_response(&r);
    PASS();
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void test_connection_refused(void) {
    TEST("error: connection refused returns curl error");
    HttpResponse r = http("http://127.0.0.1:19999/echo");
    ASSERT_NEQ(r.curl_code, CURLE_OK, "should fail");
    ASSERT_EQ(r.status_code, 0, "no status code on connection failure");
    http_free_response(&r);
    PASS();
}

void test_unresolvable_host(void) {
    TEST("error: unresolvable hostname");
    HttpResponse r = http("http://this-host-does-not-exist-xyz.invalid/echo");
    ASSERT_NEQ(r.curl_code, CURLE_OK, "should fail");
    http_free_response(&r);
    PASS();
}

void test_error_response_body_cleaned(void) {
    TEST("error: body is freed on error, safe to use");
    HttpResponse r = http("http://127.0.0.1:19999/echo");
    ASSERT_NEQ(r.curl_code, CURLE_OK, "should fail");
    // Body should be cleaned up internally by http_request
    ASSERT_EQ(r.body.data, NULL, "body data NULL on error");
    ASSERT_EQ(r.body.length, 0, "body length 0 on error");
    // Double free should be safe
    http_free_response(&r);
    PASS();
}

void test_error_curl_code_correct(void) {
    TEST("error: curl_code indicates specific error");
    HttpResponse r = http("http://127.0.0.1:19999/echo");
    ASSERT_NEQ(r.curl_code, CURLE_OK, "should fail");
    // Should be a connection error
    ASSERT(r.curl_code == CURLE_COULDNT_CONNECT,
           "should be CURLE_COULDNT_CONNECT");
    http_free_response(&r);
    PASS();
}

// ============================================================================
// Edge Cases & Potential Bugs
// ============================================================================

void test_default_method_is_get(void) {
    TEST("edge: default method (zero struct) is GET");
    HttpResponse r = http(BASE "/method");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "GET", "default method is GET");
    http_free_response(&r);
    PASS();
}

void test_get_with_body_stays_get(void) {
    TEST("fixed: GET with body correctly sends GET (not POST)");
    // Previously, setting CURLOPT_POSTFIELDS before CURLOPT_HTTPGET
    // caused libcurl to implicitly switch to POST. Now http_set_method
    // is called after POSTFIELDS and uses CUSTOMREQUEST for GET+body.
    HttpResponse r = http(BASE "/method", .body = "data");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "GET", "GET with body stays GET");
    http_free_response(&r);
    PASS();
}

void test_multiple_sequential_requests(void) {
    TEST("edge: 20 sequential requests don't leak resources");
    for (int i = 0; i < 20; i++) {
        HttpResponse r = http(BASE "/echo");
        ASSERT_EQ(r.curl_code, CURLE_OK, "request should succeed");
        ASSERT_EQ(r.status_code, 200, "status 200");
        http_free_response(&r);
    }
    PASS();
}

void test_http_request_opts_explicit(void) {
    TEST("edge: http_request_opts with explicit struct");
    HttpRequestOpts opts = {0};
    opts.method = HTTP_POST;
    opts.body = "test body";
    HttpResponse r = http_request_opts(BASE "/echo-body", opts);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "test body", "body echoed");
    http_free_response(&r);
    PASS();
}

void test_http_request_direct_call(void) {
    TEST("edge: http_request with all explicit parameters");
    HttpHeaders h = {0};
    ds_da_append(&h, "X-Direct: test");
    HttpResponse r = http_request(BASE "/method", HTTP_PUT, &h, "body", NULL, NULL);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "PUT", "method is PUT");
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

void test_http_request_all_nulls(void) {
    TEST("edge: http_request with all NULL optional params");
    HttpResponse r = http_request(BASE "/method", HTTP_GET, NULL, NULL, NULL, NULL);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "GET", "method is GET");
    http_free_response(&r);
    PASS();
}

void test_post_then_get_method_not_sticky(void) {
    TEST("edge: POST then GET uses correct methods (no stickiness)");
    // Each http() call creates a new CURL handle, so methods shouldn't leak
    HttpResponse r1 = http(BASE "/method", .method = HTTP_POST);
    ASSERT_EQ(r1.curl_code, CURLE_OK, "POST succeeds");
    ASSERT_STR(r1.body.data, "POST", "first is POST");
    http_free_response(&r1);

    HttpResponse r2 = http(BASE "/method");
    ASSERT_EQ(r2.curl_code, CURLE_OK, "GET succeeds");
    ASSERT_STR(r2.body.data, "GET", "second is GET");
    http_free_response(&r2);
    PASS();
}

void test_response_body_multi_chunk(void) {
    TEST("edge: large response body assembled from multiple chunks");
    // curl receives data in chunks; verify the write_callback
    // correctly assembles them with proper null termination
    HttpResponse r = http(BASE "/large");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.body.length, 100000, "full body received");
    ASSERT_EQ(r.body.data[r.body.length], '\0', "null terminated");
    ASSERT_EQ(strlen(r.body.data), r.body.length, "strlen consistent");
    // Verify no corruption in the middle
    ASSERT_EQ(r.body.data[50000], 'X', "middle byte intact");
    http_free_response(&r);
    PASS();
}

void test_all_methods_work(void) {
    TEST("edge: all HttpMethod enum values are handled correctly");
    // Verifies the switch in http_set_method covers all cases.
    HttpMethod methods[] = {HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH,
                            HTTP_DELETE, HTTP_OPTIONS, HTTP_HEAD};
    const char *expected[] = {"GET", "POST", "PUT", "PATCH",
                              "DELETE", "OPTIONS", NULL};
    for (size_t i = 0; i < sizeof(methods)/sizeof(methods[0]); i++) {
        HttpResponse r = http(BASE "/method", .method = methods[i]);
        ASSERT_EQ(r.curl_code, CURLE_OK, "method should work");
        if (expected[i] != NULL) {
            ASSERT_STR(r.body.data, expected[i], "method verified");
        }
        http_free_response(&r);
    }
    PASS();
}

void test_delete_with_body(void) {
    TEST("edge: DELETE with body");
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_DELETE, .body = "delete payload");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "delete payload", "body sent with DELETE");
    http_free_response(&r);
    PASS();
}

void test_options_with_body(void) {
    TEST("edge: OPTIONS with body");
    HttpResponse r = http(BASE "/echo-body", .method = HTTP_OPTIONS, .body = "options body");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_STR(r.body.data, "options body", "body sent with OPTIONS");
    http_free_response(&r);
    PASS();
}

void test_header_colon_only_removes_it(void) {
    TEST("edge: 'Header:' (no value) removes header in curl");
    // curl treats "Header:" as a directive to REMOVE that header.
    // This is a curl quirk that http.h users should be aware of.
    // To send a header with an empty value, use "Header; " instead.
    HttpHeaders h = {0};
    ds_da_append(&h, "Accept:");  // removes default Accept header
    HttpResponse r = http(BASE "/headers", .headers = &h);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    // Accept header should be absent since curl removes it
    ASSERT(strstr(r.body.data, "accept") == NULL, "Accept removed by curl");
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

void test_header_semicolon_sends_empty(void) {
    TEST("edge: 'Header;' sends header with no value in curl");
    HttpHeaders h = {0};
    ds_da_append(&h, "X-Empty-Test;");
    HttpResponse r = http(BASE "/headers", .headers = &h);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT(strstr(r.body.data, "x-empty-test") != NULL, "empty header present");
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

void test_redirect_followed(void) {
    TEST("edge: 302 redirect is followed automatically");
    HttpResponse r = http(BASE "/redirect");
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT_EQ(r.status_code, 200, "final status after redirect is 200");
    ASSERT_STR(r.body.data, "GET", "redirected to /method");
    http_free_response(&r);
    PASS();
}

void test_many_headers(void) {
    TEST("edge: many headers (50)");
    HttpHeaders h = {0};
    char buf[64];
    for (int i = 0; i < 50; i++) {
        snprintf(buf, sizeof(buf), "X-H%d: value%d", i, i);
        // Must duplicate because buf is overwritten each iteration
        ds_da_append(&h, strdup(buf));
    }
    HttpResponse r = http(BASE "/headers", .headers = &h);
    ASSERT_EQ(r.curl_code, CURLE_OK, "should succeed");
    ASSERT(strstr(r.body.data, "value0") != NULL, "first header");
    ASSERT(strstr(r.body.data, "value49") != NULL, "last header");
    for (size_t i = 0; i < h.length; i++) free((void *)h.data[i]);
    ds_da_free(&h);
    http_free_response(&r);
    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    setbuf(stdout, NULL);
    printf("=== http.h Test Suite ===\n");

    http_init();

    printf("\nStarting test server...\n");
    if (!start_server()) {
        fprintf(stderr, "Failed to start test HTTP server.\n");
        fprintf(stderr, "Ensure python3 is installed and available.\n");
        http_cleanup();
        return 1;
    }
    usleep(200000); // 200ms settle time

    // Initialization & Cleanup
    SECTION("Initialization & Cleanup");
    test_response_zero_init();
    test_free_response_zero();
    test_free_response_double_free();

    // HTTP Methods
    SECTION("HTTP Methods");
    test_get_basic();
    test_get_method_verified();
    test_post_method_verified();
    test_put_method_verified();
    test_patch_method_verified();
    test_delete_method_verified();
    test_options_method_verified();
    test_head_no_body();

    // Request Body
    SECTION("Request Body");
    test_post_with_body();
    test_post_with_json_body();
    test_post_empty_body_string();
    test_post_null_body();
    test_put_with_body();
    test_patch_with_body();
    test_large_request_body();
    test_body_with_newlines();

    // Headers
    SECTION("Headers");
    test_single_custom_header();
    test_multiple_custom_headers();
    test_empty_headers_array();
    test_content_type_header();
    test_headers_not_leaked_between_requests();

    // Response
    SECTION("Response");
    test_status_200();
    test_status_201();
    test_status_204();
    test_status_400();
    test_status_404();
    test_status_500();
    test_many_status_codes();
    test_response_body_null_terminated();
    test_large_response_body();
    test_empty_response_body();
    test_404_still_has_body();

    // Streaming
    SECTION("Streaming");
    test_stream_callback_called();
    test_stream_body_not_auto_populated();
    test_stream_userdata_passed();
    test_stream_callback_abort();
    test_stream_callback_manual_body_population();
    test_stream_callback_partial_return();

    // Error Handling
    SECTION("Error Handling");
    test_connection_refused();
    test_unresolvable_host();
    test_error_response_body_cleaned();
    test_error_curl_code_correct();

    // Edge Cases & Potential Bugs
    SECTION("Edge Cases & Potential Bugs");
    test_default_method_is_get();
    test_get_with_body_stays_get();
    test_multiple_sequential_requests();
    test_http_request_opts_explicit();
    test_http_request_direct_call();
    test_http_request_all_nulls();
    test_post_then_get_method_not_sticky();
    test_response_body_multi_chunk();
    test_all_methods_work();
    test_delete_with_body();
    test_options_with_body();
    test_header_colon_only_removes_it();
    test_header_semicolon_sends_empty();
    test_redirect_followed();
    test_many_headers();

    // Summary
    printf("\n=== Results ===\n");
    printf("Total: %d | \033[32mPassed: %d\033[0m | \033[31mFailed: %d\033[0m\n",
           tests_run, tests_passed, tests_failed);

    stop_server();
    http_cleanup();

    return tests_failed > 0 ? 1 : 0;
}
