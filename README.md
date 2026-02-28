# c-stb

A collection of single-header C libraries in the style of [stb](https://github.com/nothings/stb), built for fun and personal development purposes.

All libraries follow the same convention: include the header normally for declarations, and define `<LIB>_IMPLEMENTATION` in **one** translation unit before including to get the implementation.

```c
#define DS_IMPLEMENTATION
#include "ds.h"
```

Heavily inspired by [Tsoding](https://github.com/tsoding)'s work, especially for the dynamic arrays and JSON parser/builder design.

## Libraries

### ds.h — Data Structures & Utilities

General-purpose library providing:

- **Dynamic arrays** — type-safe, macro-based generic arrays (`ds_da_declare`, `ds_da_append`, `ds_da_remove`, `ds_da_foreach`, ...)
- **Hash maps** — open-addressing with configurable load factor (`ds_hm_declare`, `ds_hm_set`, `ds_hm_get`, `ds_hm_remove`, ...)
- **Hash sets** — with set operations like union and difference (`ds_hs_declare`, `ds_hs_add`, `ds_hs_has`, `ds_hs_cat`, `ds_hs_sub`, ...)
- **Linked lists** — singly-linked (`ds_ll_declare`, `ds_ll_push`, `ds_ll_pop`, ...)
- **String builder** — `DsString` with append, prepend, format, trim
- **Arena allocators** — fixed-size and region-based, with snapshot/restore
- **Logging** — leveled logging with pluggable handlers (plain and colored built-in)
- **File I/O** — `ds_read_entire_file`, `ds_write_entire_file`, `ds_mkdir_p`
- **String utilities** — splitting, trimming, prefix/suffix matching

---

### http.h — HTTP Client

A simple HTTP client built on top of libcurl.

```c
HttpResponse r = http("https://example.com", .method = HTTP_POST, .body = data);
printf("%ld %s\n", r.status_code, r.body.items);
http_free_response(&r);
```

- All standard methods (GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD)
- Custom headers
- Streaming support via callbacks

**Dependencies:** libcurl, ds.h

---

### jsb.h — JSON Builder

Build JSON strings programmatically, with optional pretty-printing.

```c
Jsb jsb = {.pp = 2};           // 2-space indent, 0 for compact
jsb_begin_object(&jsb);
  jsb_key(&jsb, "name");
  jsb_string(&jsb, "Alice");
  jsb_key(&jsb, "age");
  jsb_int(&jsb, 30);
jsb_end_object(&jsb);

printf("%s\n", jsb_get(&jsb)); // {"name":"Alice","age":30}
jsb_free(&jsb);
```

Supports strings, numbers (with configurable precision), booleans, nulls, nested objects/arrays, and date formatting.

No external dependencies.

---

### jsp.h — JSON Stream Parser

Streaming JSON parser for selective field extraction — no DOM, no full parse required.

```c
Jsp jsp = {0};
jsp_sinit(&jsp, "{\"name\":\"Alice\",\"age\":30}");
jsp_begin_object(&jsp);
while (jsp_key(&jsp) == 0) {
    if (strcmp(jsp.string, "name") == 0) {
        jsp_value(&jsp);
        printf("name: %s\n", jsp.string);
    } else {
        jsp_skip(&jsp);
    }
}
jsp_end_object(&jsp);
jsp_free(&jsp);
```

- Type inference (`jsp_infer_type` — string, number, boolean, null, array, object)
- UTF-8 and `\uXXXX` escape support
- Minimal allocations

No external dependencies.

---

### jsgen — JSON Code Generator

Generates C serialization/deserialization code from annotated struct definitions.

#### Building

jsgen is a standalone CLI tool. Compile `jsgen/jsgen.c` and run it on your header files:

```bash
cc -o jsgen jsgen/jsgen.c
./jsgen models.h -o models.g.h
```

It accepts individual `.h` files or entire directories. Use `-o` to set the output file (defaults to `models.g.h`) and `-i` to override the default includes in the generated header.

```bash
./jsgen src/models/ -o src/models.g.h -i '#include "my_includes.h"'
```

#### Usage

Annotate your structs:

```c
JSON typedef struct {
    int id;
    char *name;
    bool is_active alias("active");
} User;

#include "models.g.h"  // generated
```

Then run the generator to produce `models.g.h`, which gives you:

```c
User user = {0};
parse_User(json_string, &user);
char *json = stringify_User(&user);
free(json);
jsgen_free();  // free all jsgen internal allocations
```

#### Memory management

All allocations made during parsing (strings, nested structs, arrays) go through a single `JSGEN_MALLOC` function pointer. By default this is `jsgen_basic_alloc`, a simple bump allocator backed by a static 8 MB buffer. Calling `jsgen_free()` resets the bump pointer, effectively freeing everything at once.

You can change the arena size:

```c
#define JSGEN_BASIC_ALLOC_SIZE (16 * 1024 * 1024)  // 16 MB
```

Or replace the allocator entirely by defining `JSGEN_MALLOC` before including `jsgen.h`:

```c
#define JSGEN_MALLOC my_alloc
#include "jsgen.h"
```

Your custom allocator must match the signature `void* fn(size_t size)`.

#### Annotations

| Annotation | Effect |
|---|---|
| `JSON` | Generate parser + stringifier |
| `JSONS` | Stringifier only |
| `JSONP` | Parser only |
| `alias("key")` | Map field to a different JSON key |
| `sized_by("n")` | Dynamic array sized by another field |
| `json_literal` | Field contains raw JSON |
| `jsgen_ignore()` | Exclude field |

**Dependencies:** jsb and jsp for the generated code.

## License

Public domain / MIT — use however you like.
