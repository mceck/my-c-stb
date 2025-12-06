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
    http(url, &response);
    // GET url with headers
    HttpHeaders headers = {0};
    ds_da_append(&headers, "Content-Type: application/json");
    http(url, &response, .headers = &headers);
    ds_da_free(&headers);
    // POST url with data
    http(url, &response, .method = HTTP_POST, .body="data");
```
 */

#ifndef HTTP_H_
#define HTTP_H_

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
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

typedef struct {
    long status_code;
    DsString body;
} HttpResponse;

typedef struct {
    HttpMethod method;
    HttpHeaders *headers;
    const char *body;
} HttpRequestOpts;

#define http_free_response(resp) ds_da_free(&(resp)->body)
#define http_init() curl_global_init(CURL_GLOBAL_DEFAULT)
#define http_cleanup() curl_global_cleanup()

CURLcode http_request(const char *url, HttpMethod method, HttpHeaders *headers, const char *body, HttpResponse *response);

#define http(url, response, ...) \
    http_request_opts(url, response, (HttpRequestOpts){__VA_ARGS__})
#define http_get(url, headers, response) http_request(url, HTTP_GET, headers, NULL, response)
#define http_post(url, headers, body, response) http_request(url, HTTP_POST, headers, body, response)
#define http_put(url, headers, body, response) http_request(url, HTTP_PUT, headers, body, response)
#define http_patch(url, headers, body, response) http_request(url, HTTP_PATCH, headers, body, response)
#define http_delete(url, headers, response) http_request(url, HTTP_DELETE, headers, NULL, response)
#define http_reset_response(resp) (resp)->body.length = 0

static inline CURLcode http_set_method(CURL *curl, HttpMethod method) {
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

static inline CURLcode http_set_headers(CURL *curl, HttpHeaders *headers, struct curl_slist **header_list) {
    for (size_t i = 0; i < headers->length; i++) {
        *header_list = curl_slist_append(*header_list, headers->data[i]);
    }
    CURLcode res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *header_list);
    return res;
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    DsString *sb = (DsString *)userp;
    ds_da_append_many(sb, (char *)contents, total);
    ds_sb_append(sb);
    return total;
}

static inline CURLcode http_request_opts(const char *url, HttpResponse *response, HttpRequestOpts opts) {
    return http_request(url, opts.method, opts.headers, opts.body, response);
}

/**
 * Sends an HTTP request.
 * @param url The URL to send the request to.
 * @param method The HTTP method to use.
 * @param headers The headers to include in the request, can be NULL.
 * @param body The body of the request, can be NULL.
 * @param response The response object to populate.
 * @return CURLcode indicating the result of the request.
 */
CURLcode http_request(const char *url, HttpMethod method, HttpHeaders *headers, const char *body, HttpResponse *response) {
    CURL *curl;
    struct curl_slist *header_list = NULL;
    CURLcode res = CURLE_FAILED_INIT;

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

    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    if (res) goto cleanup;
    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response->body);
    if (res) goto cleanup;

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
cleanup:
    if (header_list) curl_slist_free_all(header_list);
    if (curl) curl_easy_cleanup(curl);
    return res;
}

#endif // HTTP_H_