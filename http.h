/**
 * Simple HTTP Client using libcurl
 * https://github.com/mceck/c-stb
 *
 * Dependent on:
 * - libcurl, link with -lcurl (https://curl.se/libcurl/)
 * - ./ds.h
 *
 * Example:
```c
    HttpResponse response = {0};
    // GET url
    response = http(url);
    http_free_response(&response);
    // GET url with headers
    HttpHeaders headers = {0};
    ds_da_append(&headers, "Content-Type: application/json");
    response = http(url, .headers = &headers);
    http_free_response(&response);
    ds_da_free(&headers);
    // POST url with data
    response = http(url, .method = HTTP_POST, .body="data");
    http_free_response(&response);
    // Streaming response (e.g., LLM APIs)
    size_t on_chunk(const char *chunk, size_t len, HttpStreamContext *ctx) {
        (void)ctx;
        fwrite(chunk, 1, len, stdout);
        return len; // return number of bytes processed
    }
    response = http(url, .method = HTTP_POST, .body=json, .stream_callback = on_chunk);
    http_free_response(&response);
```
 */

#ifndef HTTP_H_
#define HTTP_H_

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ds.h"

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_PATCH,
    HTTP_DELETE,
    HTTP_OPTIONS,
    HTTP_HEAD,
} HttpMethod;

ds_da_declare(HttpHeaders, const char *);

typedef struct HttpStreamContext HttpStreamContext;
/**
 * Callback for streaming responses.
 * @param chunk The chunk of data received.
 * @param len The length of the chunk.
 * @param ctx The HTTP stream context.
 * @return The number of bytes processed.
 */
typedef size_t (*HttpStreamCallback)(const char *contents, size_t total, HttpStreamContext *ctx);


typedef struct HttpStreamContext {
    HttpStreamCallback callback;
    void *userdata;
    DsString *body;
} HttpStreamContext;

typedef struct {
    long status_code;
    DsString body;
    CURLcode curl_code;
} HttpResponse;

typedef struct {
    HttpMethod method;
    HttpHeaders *headers;
    const char *body;
    HttpStreamCallback stream_callback;
    void *stream_userdata;
} HttpRequestOpts;

#define http_free_response(resp) ds_da_free(&(resp)->body)
#define http_init() curl_global_init(CURL_GLOBAL_DEFAULT)
#define http_cleanup() curl_global_cleanup()

static CURLcode http_set_method(CURL *curl, const HttpMethod method);

static CURLcode http_set_headers(CURL *curl, const HttpHeaders *headers, struct curl_slist **header_list);

static size_t write_callback(const char *contents, size_t total, HttpStreamContext *ctx);

static size_t stream_write_callback(void *contents, size_t size, size_t nmemb, void *userdata);

/**
 * Sends an HTTP request.
 * @param url The URL to send the request to.
 * @param method The HTTP method to use.
 * @param headers The headers to include in the request, can be NULL.
 * @param body The body of the request, can be NULL.
 * @param stream_callback Callback for streaming responses, can be NULL.
 * @param stream_userdata User-defined data passed to the stream callback, can be NULL.
 * @return An HttpResponse struct containing the response data.
 */
HttpResponse http_request(const char *url, const HttpMethod method, const HttpHeaders *headers, const char *body, HttpStreamCallback stream_callback, void *stream_userdata);

HttpResponse http_request_opts(const char *url, HttpRequestOpts opts);

#define http(url, ...) \
    http_request_opts(url, (HttpRequestOpts){__VA_ARGS__})

#endif // HTTP_H_

#ifdef HTTP_IMPLEMENTATION
static CURLcode http_set_method(CURL *curl, const HttpMethod method) {
    switch (method) {
    case HTTP_GET:
        return curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    case HTTP_POST:
        return curl_easy_setopt(curl, CURLOPT_POST, 1L);
    case HTTP_PUT:
        return curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    case HTTP_PATCH:
        return curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    case HTTP_DELETE:
        return curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    case HTTP_OPTIONS:
        return curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    default:
        return curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    }
    return CURLE_UNSUPPORTED_PROTOCOL;
}

static CURLcode http_set_headers(CURL *curl, const HttpHeaders *headers, struct curl_slist **header_list) {
    for (size_t i = 0; i < headers->length; i++) {
        *header_list = curl_slist_append(*header_list, headers->data[i]);
    }
    CURLcode res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *header_list);
    return res;
}

static size_t write_callback(const char *contents, size_t total, HttpStreamContext *ctx) {
    ds_da_append_many(ctx->body, (char *)contents, total);
    ds_str_append(ctx->body);
    return total;
}

static size_t stream_write_callback(void *contents, size_t size, size_t nmemb, void *userdata) {
    HttpStreamContext *ctx = (HttpStreamContext *)userdata;
    return ctx->callback(contents, size * nmemb, ctx);
}

HttpResponse http_request_opts(const char *url, HttpRequestOpts opts) {
    return http_request(url, opts.method, opts.headers, opts.body, opts.stream_callback, opts.stream_userdata);
}

HttpResponse http_request(const char *url, const HttpMethod method, const HttpHeaders *headers, const char *body, HttpStreamCallback stream_callback, void *stream_userdata) {
    CURL *curl;
    struct curl_slist *header_list = NULL;
    CURLcode res = CURLE_FAILED_INIT;
    HttpResponse response = {0};
    HttpStreamContext stream_ctx = {0};
    stream_ctx.body = &response.body;
    curl = curl_easy_init();
    if (!curl) goto cleanup;
    res = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (res) goto cleanup;
    res = http_set_method(curl, method);
    if (res) goto cleanup;
    if (headers) {
        res = http_set_headers(curl, headers, &header_list);
        if (res) goto cleanup;
    }
    if (body) {
        res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        if (res) goto cleanup;
    }

    if (stream_callback) {
        stream_ctx.callback = stream_callback;
    } else {
        stream_ctx.callback = write_callback;
    }
    stream_ctx.userdata = stream_userdata;
    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_write_callback);
    if (res) goto cleanup;
    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream_ctx);
    if (res) goto cleanup;

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
cleanup:
    if (header_list) curl_slist_free_all(header_list);
    if (curl) curl_easy_cleanup(curl);
    if (res) http_free_response(&response);
    response.curl_code = res;
    return response;
}

#endif // HTTP_IMPLEMENTATION