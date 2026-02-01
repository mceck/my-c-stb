#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#define DS_NO_PREFIX
#define DS_IMPLEMENTATION
#include "ds.h"
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    char name[128];
    char alias[128];
    bool has_alias;
    char type[128];
    char simple_type[128];
    bool is_pointer;
    bool is_array;
    bool has_counter;
    char counter_field[128];
    bool is_counter_field;
    bool is_json_literal;
} Field;

da_declare(Fields, Field);

typedef struct {
    char name[128];
    char simple_name[128];
    bool stringify;
    bool parse;
    Fields fields;
} Model;

da_declare(Models, Model);

typedef struct {
    stb_lexer *lex;
    Models *models;
    Model *current_model;
    Field *current_field;
    int level;
    bool in_typedef;
    bool in_struct;
    bool in_substruct;
    bool is_field;
    bool can_generate;
} Parser;

#define sb_cat_line(sb, indent, ...)           \
    do {                                       \
        for (int i = 0; i < (indent) * 4; ++i) \
            str_append(sb, " ");                \
        str_append(sb, __VA_ARGS__);            \
        str_append(sb, "\n");                   \
    } while (0)

const char *js_getalias(Field *field) {
    return field->has_alias ? field->alias : field->name;
}

const char *get_jsp_type(const char *type) {
    if (strcmp(type, "int") == 0) return "number";
    if (strcmp(type, "float") == 0) return "number";
    if (strcmp(type, "double") == 0) return "number";
    if (strcmp(type, "long") == 0) return "number";
    if (strcmp(type, "size_t") == 0) return "number";
    if (strcmp(type, "bool") == 0) return "boolean";
    if (strcmp(type, "char*") == 0) return "string";
    int len = strlen(type);
    if (len > 1 && type[len - 1] == '*') {
        char base_type[128];
        strncpy(base_type, type, len - 1);
        base_type[len - 1] = 0;
        return get_jsp_type(base_type);
    }
    return NULL;
}

const char *get_jsb_type(const char *type) {
    if (strcmp(type, "int") == 0) return "int";
    if (strcmp(type, "float") == 0) return "number";
    if (strcmp(type, "double") == 0) return "number";
    if (strcmp(type, "long") == 0) return "int";
    if (strcmp(type, "size_t") == 0) return "int";
    if (strcmp(type, "bool") == 0) return "bool";
    if (strcmp(type, "char*") == 0) return "string";
    int len = strlen(type);
    if (len > 1 && type[len - 1] == '*') {
        char base_type[128];
        strncpy(base_type, type, len - 1);
        base_type[len - 1] = 0;
        return get_jsb_type(base_type);
    }
    return NULL;
}

void post_process_model(Model *model) {
    for (size_t i = 0; i < model->fields.length; ++i) {
        Field *field = &model->fields.data[i];
        if (field->has_counter) {
            for (size_t j = 0; j < model->fields.length; ++j) {
                if (strcmp(model->fields.data[j].name, field->counter_field) == 0) {
                    model->fields.data[j].is_counter_field = true;
                    break;
                }
            }
        }
    }
}

int parse_field_annotation(Parser *p) {
    if (!p->is_field) return 0;
    Field *field = da_last(&p->current_model->fields);
    if (strcmp(p->lex->string, "alias") == 0 || strcmp(p->lex->string, "jsgen_alias") == 0) {
        stb_c_lexer_get_token(p->lex);
        stb_c_lexer_get_token(p->lex);
        field->has_alias = true;
        strcpy(field->alias, p->lex->string);
        return 1;
    }
    if (strcmp(p->lex->string, "sized_by") == 0 || strcmp(p->lex->string, "jsgen_sized_by") == 0) {
        stb_c_lexer_get_token(p->lex);
        stb_c_lexer_get_token(p->lex);
        field->has_counter = true;
        field->is_array = true;
        strcpy(field->counter_field, p->lex->string);
        return 1;
    }
    if (strcmp(p->lex->string, "jsgen_ignore") == 0) {
        if (p->current_model->fields.length > 0) p->current_model->fields.length--;
        return 1;
    }
    if (strcmp(p->lex->string, "jsgen_json_literal") == 0 || strcmp(p->lex->string, "json_literal") == 0) {
        field->is_json_literal = true;
        return 1;
    }
    return 0;
}

void parse_field(Parser *p, const char *type_prefix) {
    Field field = {0};
    strcpy(field.type, type_prefix);
    strcat(field.type, p->lex->string);
    strcpy(field.simple_type, p->lex->string);

    stb_c_lexer_get_token(p->lex);
    if (p->lex->token == '*') {
        field.is_pointer = true;
        strcat(field.type, "*");
        stb_c_lexer_get_token(p->lex);
    }
    if (p->lex->token == CLEX_id) {
        strcpy(field.name, p->lex->string);
    } else {
        return;
    }
    da_append(&p->current_model->fields, field);

    char *parse_point = p->lex->parse_point;
    stb_c_lexer_get_token(p->lex);
    if (p->lex->token == '[') {
        Field *last_field = da_last(&p->current_model->fields);
        last_field->is_array = true;
        last_field->is_pointer = true;
        while (p->lex->token != ']')
            stb_c_lexer_get_token(p->lex);
    }
    p->lex->parse_point = parse_point;
}

bool is_json_initializer(const char *s) {
    return strcmp(s, "JSON") == 0 || strcmp(s, "JSGEN_JSON") == 0 || strcmp(s, "JSONS") == 0 || strcmp(s, "JSGEN_JSONS") == 0 || strcmp(s, "JSONP") == 0 || strcmp(s, "JSGEN_JSONP") == 0;
}

void handle_id(Parser *p) {
    if (is_json_initializer(p->lex->string)) {
        p->can_generate = 1;
        p->current_model->stringify = true;
        p->current_model->parse = true;
        if (strcmp(p->lex->string, "JSONP") == 0 || strcmp(p->lex->string, "JSGEN_JSONP") == 0) {
            p->current_model->stringify = false;
        } else if (strcmp(p->lex->string, "JSONS") == 0 || strcmp(p->lex->string, "JSGEN_JSONS") == 0) {
            p->current_model->parse = false;
        }
        return;
    }
    if (!p->can_generate) return;
    if (strcmp(p->lex->string, "const") == 0) {
        stb_c_lexer_get_token(p->lex);
    }
    if (strcmp(p->lex->string, "typedef") == 0)
        p->in_typedef = 1;
    else if (strcmp(p->lex->string, "struct") == 0) {
        if (p->in_struct)
            p->in_substruct = 1;
        else
            p->in_struct = 1;
    } else if (parse_field_annotation(p)) {
    } else if (p->in_struct && p->level == 0) {
        if (p->in_typedef)
            strcpy(p->current_model->name, p->lex->string);
        else
            sprintf(p->current_model->name, "struct %s", p->lex->string);
        strcpy(p->current_model->simple_name, p->lex->string);
    } else if (p->in_struct && p->level == 1) {
        parse_field(p, p->in_substruct ? "struct " : "");
        p->in_substruct = 0;
        p->is_field = 1;
    }
}

void handle_semicolon(Parser *p) {
    p->in_substruct = 0;
    if (p->level == 0 && p->in_struct) {
        post_process_model(p->current_model);
        da_append(p->models, *p->current_model);
        *p->current_model = (Model){0};
        p->in_struct = p->in_typedef = p->can_generate = 0;
    }
}

int parse_file(const char *filename, Models *models) {
    String f = {0};
    if (!read_entire_file(filename, &f)) return -1;

    stb_lexer lex = {0};
    char buffer[512];
    Model m = {0};
    Parser p = {.lex = &lex, .models = models, .current_model = &m};

    stb_c_lexer_init(&lex, f.data, f.data + f.length, buffer, sizeof(buffer));
    while (stb_c_lexer_get_token(&lex)) {
        if (!p.can_generate && lex.token != CLEX_id) continue;

        switch (lex.token) {
        case CLEX_id:
            handle_id(&p);
            break;
        case '{':
            p.level++;
            break;
        case '}':
            p.level--;
            break;
        case ';':
            handle_semicolon(&p);
            break;
        case CLEX_parse_error:
            return -1;
        default:
            break;
        }
    }
    da_free(&f);
    return 0;
}

void gen_parse_field_body(String *sb, Field *field, int indent) {
    const char *jsp_type = get_jsp_type(field->type);

    if (jsp_type) {
        sb_cat_line(sb, indent, "err = jsp_value(jsp);");
        sb_cat_line(sb, indent, "if (err) return err;");
        if (strcmp(jsp_type, "string") == 0) {
            if (field->is_json_literal) {
                sb_cat_line(sb, indent, "size_t start = jsp->off - 1;");
                sb_cat_line(sb, indent, "int brace_count = 1;");
                sb_cat_line(sb, indent, "char ob = (jsp->type == JSP_TYPE_OBJECT ? '{' : '[');");
                sb_cat_line(sb, indent, "char cb = (jsp->type == JSP_TYPE_OBJECT ? '}' : ']');");
                sb_cat_line(sb, indent, "while (jsp->off < jsp->length && brace_count > 0) {");
                sb_cat_line(sb, indent + 1, "if (jsp->buffer[jsp->off] == '\\\\') jsp->off++;");
                sb_cat_line(sb, indent + 1, "else if (jsp->buffer[jsp->off] == ob) brace_count++;");
                sb_cat_line(sb, indent + 1, "else if (jsp->buffer[jsp->off] == cb) brace_count--;");
                sb_cat_line(sb, indent + 1, "jsp->off++;");
                sb_cat_line(sb, indent, "}");
                sb_cat_line(sb, indent, "size_t fldlen = jsp->off - start;");
                sb_cat_line(sb, indent, "out->", field->name, " = jsgen_malloc(a, fldlen + 1);");
                sb_cat_line(sb, indent, "memcpy(out->", field->name, ", &jsp->buffer[start], fldlen);");
                sb_cat_line(sb, indent, "out->", field->name, "[fldlen] = '\\0';");
                sb_cat_line(sb, indent, "jsp_skip_end(jsp);");
            }
            if (field->is_pointer) {
                sb_cat_line(sb, indent, "size_t s_len = jsp->string ? strlen(jsp->string) : 0;");
                if (field->has_counter) sb_cat_line(sb, indent, "out->", field->counter_field, " = s_len;");
                sb_cat_line(sb, indent, "if(s_len > 0) {");
                sb_cat_line(sb, indent + 1, "out->", field->name, " = jsgen_malloc(a, s_len + 1);");
                sb_cat_line(sb, indent + 1, "strcpy(out->", field->name, ", jsp->string);");
                sb_cat_line(sb, indent, "} else {");
                sb_cat_line(sb, indent + 1, "out->", field->name, " = NULL;");
                sb_cat_line(sb, indent, "}");
            } else {
                sb_cat_line(sb, indent, "if(jsp->string) strncpy(out->", field->name, ", jsp->string, sizeof(out->", field->name, ") - 1);");
            }
        } else {
            sb_cat_line(sb, indent, "out->", field->name, " = jsp->", jsp_type, ";");
        }
    } else if (field->is_array) {
        sb_cat_line(sb, indent, "err = jsp_begin_array(jsp);");
        sb_cat_line(sb, indent, "if (err) return err;");
        if (field->has_counter) {
            sb_cat_line(sb, indent, "size_t len = jsp_array_length(jsp);");
            sb_cat_line(sb, indent, "out->", field->counter_field, " = len;");
            sb_cat_line(sb, indent, "out->", field->name, " = jsgen_malloc(a, sizeof(", field->simple_type, ") * len);");
            sb_cat_line(sb, indent, "for (size_t i = 0; i < len; i++) {");
        } else {
            sb_cat_line(sb, indent, "size_t i = 0;");
            sb_cat_line(sb, indent, "while(jsp->offset < jsp->length && jsp->content[jsp->offset] != ']') {");
        }

        const char *arr_jsp_type = get_jsp_type(field->simple_type);
        if (arr_jsp_type) {
            sb_cat_line(sb, indent + 1, "err = jsp_value(jsp);");
            sb_cat_line(sb, indent + 1, "if (err) break;");
            sb_cat_line(sb, indent + 1, "out->", field->name, "[i] = jsp->", arr_jsp_type, ";");
        } else {
            sb_cat_line(sb, indent + 1, "err = _parse_", field->simple_type, "(jsp, &out->", field->name, "[i], a);");
            sb_cat_line(sb, indent + 1, "if (err) break;");
        }
        if (!field->has_counter) sb_cat_line(sb, indent + 1, "i++;");
        sb_cat_line(sb, indent, "}");
        sb_cat_line(sb, indent, "err = jsp_end_array(jsp);");
        sb_cat_line(sb, indent, "if (err) return err;");

    } else if (field->is_pointer) {
        sb_cat_line(sb, indent, "err = jsp_value(jsp);");
        sb_cat_line(sb, indent, "if (!err && jsp->type == JSP_TYPE_NULL) {");
        sb_cat_line(sb, indent + 1, "out->", field->name, " = NULL;");
        sb_cat_line(sb, indent, "} else {");
        sb_cat_line(sb, indent + 1, "out->", field->name, " = jsgen_malloc(a, sizeof(", field->simple_type, "));");
        sb_cat_line(sb, indent + 1, "err = _parse_", field->simple_type, "(jsp, out->", field->name, ", a);");
        sb_cat_line(sb, indent + 1, "if (err) return err;");
        sb_cat_line(sb, indent, "}");
    } else {
        sb_cat_line(sb, indent, "err = _parse_", field->simple_type, "(jsp, &out->", field->name, ", a);");
        sb_cat_line(sb, indent, "if (err) return err;");
    }
}

void gen_stringify_field(String *sb, Field *field, int indent) {
    if (field->is_counter_field) return;
    if (field->is_pointer) {
        sb_cat_line(sb, indent, "if (in->", field->name, " != NULL) {");
        indent++;
    }

    sb_cat_line(sb, indent, "if (jsb_key(jsb, \"", js_getalias(field), "\")) return -1;");
    const char *jsb_type = get_jsb_type(field->type);

    if (jsb_type) {
        if (field->is_json_literal) {
            sb_cat_line(sb, indent, "size_t plen = in->", field->name, " ? strlen(in->", field->name, ") : 0;");
            sb_cat_line(sb, indent, "if (plen > 0) {");
            sb_cat_line(sb, indent + 1, "jsb_srealloc(&jsb->buffer, jsb->buffer.length + plen + 1);");
            sb_cat_line(sb, indent + 1, "memcpy(jsb->buffer.data + jsb->buffer.length, in->", field->name, ", plen);");
            sb_cat_line(sb, indent + 1, "jsb->buffer.length += plen;");
            sb_cat_line(sb, indent + 1, "jsb->buffer.data[jsb->buffer.length] = '\\0';");
            sb_cat_line(sb, indent + 1, "jsb->is_first = false;");
            sb_cat_line(sb, indent + 1, "jsb->is_key = false;");
            sb_cat_line(sb, indent, "} else jsb_null(jsb);");
        } else {
            sb_cat_line(sb, indent, "if (jsb_", jsb_type, "(jsb, in->", field->name, (strcmp(jsb_type, "number") == 0 ? ", 5" : ""), ")) return -1;");
        }
    } else if (field->is_array) {
        sb_cat_line(sb, indent, "if (jsb_begin_array(jsb)) return -1;");
        if (field->has_counter) {
            sb_cat_line(sb, indent, "for (size_t i = 0; i < (size_t)in->", field->counter_field, "; ++i) {");
            const char *arr_jsb_type = get_jsb_type(field->simple_type);
            if (arr_jsb_type)
                sb_cat_line(sb, indent + 1, "if (jsb_", arr_jsb_type, "(jsb, in->", field->name, "[i])) return -1;");
            else
                sb_cat_line(sb, indent + 1, "if (_stringify_", field->simple_type, "(jsb, &in->", field->name, "[i])) return -1;");
            sb_cat_line(sb, indent, "}");
        }
        sb_cat_line(sb, indent, "if (jsb_end_array(jsb)) return -1;");
    } else if (field->is_pointer) {
        sb_cat_line(sb, indent, "if (in->", field->name, " == NULL) jsb_null(jsb);");
        sb_cat_line(sb, indent, "else if (_stringify_", field->simple_type, "(jsb, in->", field->name, ")) return -1;");
    } else {
        sb_cat_line(sb, indent, "if (_stringify_", field->simple_type, "(jsb, &in->", field->name, ")) return -1;");
    }
    if (field->is_pointer) {
        sb_cat_line(sb, --indent, "}");
    }
}

void generate_model_code(String *sb, Model *model) {
    int indent = 0;
    if (model->parse) {

        sb_cat_line(sb, indent, "int _parse_", model->simple_name, "(Jsp *jsp, ", model->name, " *out, JsGenAllocator *a) {");
        indent++;
        sb_cat_line(sb, indent, "(void)a;");
        sb_cat_line(sb, indent, "int err = jsp_begin_object(jsp);");
        sb_cat_line(sb, indent, "if (err) return err;");
        sb_cat_line(sb, indent, "while (jsp_key(jsp) == 0) {");
        indent++;

        for (int i = 0; i < indent * 4; ++i)
            str_append(sb, " ");
        for (size_t i = 0; i < model->fields.length; ++i) {
            Field *field = &model->fields.data[i];
            if (field->is_counter_field) continue;
            if (i > 0) str_append(sb, "} else ");
            str_append(sb, "if (strcmp(jsp->string, \"", js_getalias(field), "\") == 0) {\n");
            gen_parse_field_body(sb, field, indent + 1);
            for (int j = 0; j < indent * 4; ++j)
                str_append(sb, " ");
        }
        if (model->fields.length > 0)
            str_append(sb, "} else {\n");
        else
            str_append(sb, "{\n");

        sb_cat_line(sb, indent + 1, "err = jsp_skip(jsp);");
        sb_cat_line(sb, indent + 1, "if (err) return err;");
        sb_cat_line(sb, indent, "}");

        indent--;
        sb_cat_line(sb, indent, "}");
        sb_cat_line(sb, indent, "err = jsp_end_object(jsp);");
        sb_cat_line(sb, indent, "return err;");
        indent--;
        sb_cat_line(sb, indent, "}");
        str_append(sb, "\n");

        sb_cat_line(sb, indent, "int parse_", model->simple_name, "(const char *json, ", model->name, " *out, JsGenAllocator *a) {");
        indent++;
        sb_cat_line(sb, indent, "Jsp jsp = {0};");
        sb_cat_line(sb, indent, "int err = jsp_init(&jsp, json, strlen(json));");
        sb_cat_line(sb, indent, "if (err) return err;");
        sb_cat_line(sb, indent, "err = _parse_", model->simple_name, "(&jsp, out, a);");
        sb_cat_line(sb, indent, "jsp_free(&jsp);");
        sb_cat_line(sb, indent, "return err;");
        indent--;
        sb_cat_line(sb, indent, "}");
        str_append(sb, "\n");

        sb_cat_line(sb, indent, "int _parse_", model->simple_name, "_list(Jsp *jsp, ", model->name, " **out, size_t *out_count, JsGenAllocator *a) {");
        indent++;
        sb_cat_line(sb, indent, "int err = jsp_begin_array(jsp);");
        sb_cat_line(sb, indent, "if (err) return err;");
        sb_cat_line(sb, indent, "size_t len = jsp_array_length(jsp);");
        sb_cat_line(sb, indent, "*out_count = len;");
        sb_cat_line(sb, indent, "*out = jsgen_malloc(a, sizeof(", model->name, ") * len);");
        sb_cat_line(sb, indent, "for (size_t i = 0; i < len; i++) {");
        sb_cat_line(sb, indent + 1, "err = _parse_", model->simple_name, "(jsp, &(*out)[i], a);");
        sb_cat_line(sb, indent + 1, "if (err) return err;");
        sb_cat_line(sb, indent, "}");
        sb_cat_line(sb, indent, "err = jsp_end_array(jsp);");
        sb_cat_line(sb, indent, "if (err) { *out = NULL; *out_count = 0; }");
        sb_cat_line(sb, indent, "return err;");
        indent--;
        sb_cat_line(sb, indent, "}");

        sb_cat_line(sb, indent, "int parse_", model->simple_name, "_list(const char *json, ", model->name, " **out, size_t *out_count, JsGenAllocator *a) {");
        indent++;
        sb_cat_line(sb, indent, "Jsp jsp = {0};");
        sb_cat_line(sb, indent, "int err = jsp_init(&jsp, json, strlen(json));");
        sb_cat_line(sb, indent, "if (err) return err;");
        sb_cat_line(sb, indent, "err = _parse_", model->simple_name, "_list(&jsp, out, out_count, a);");
        sb_cat_line(sb, indent, "jsp_free(&jsp);");
        sb_cat_line(sb, indent, "return err;");
        indent--;
        sb_cat_line(sb, indent, "}");
        str_append(sb, "\n");
    }
    if (model->stringify) {
        sb_cat_line(sb, indent, "int _stringify_", model->simple_name, "(Jsb *jsb, ", model->name, " *in) {");
        indent++;
        sb_cat_line(sb, indent, "if (jsb_begin_object(jsb)) return -1;");
        sb_cat_line(sb, indent, "{");
        indent++;
        for (size_t i = 0; i < model->fields.length; ++i) {
            gen_stringify_field(sb, &model->fields.data[i], indent);
        }
        indent--;
        sb_cat_line(sb, indent, "}");
        sb_cat_line(sb, indent, "return jsb_end_object(jsb);");
        indent--;
        sb_cat_line(sb, indent, "}");
        str_append(sb, "\n");

        sb_cat_line(sb, indent, "char* stringify_", model->simple_name, "_indent(", model->name, " *in, int indent) {");
        indent++;
        sb_cat_line(sb, indent, "Jsb jsb = {.pp = indent};");
        sb_cat_line(sb, indent, "if(_stringify_", model->simple_name, "(&jsb, in)) {");
        sb_cat_line(sb, indent + 1, "jsb_free(&jsb);");
        sb_cat_line(sb, indent + 1, "return NULL;");
        sb_cat_line(sb, indent, "}");
        sb_cat_line(sb, indent, "return jsb_get(&jsb);");
        indent--;
        sb_cat_line(sb, indent, "}");
        str_append(sb, "\n");

        sb_cat_line(sb, indent, "#define stringify_", model->simple_name, "(in) stringify_", model->simple_name, "_indent((in), 0)");
        str_append(sb, "\n");

        sb_cat_line(sb, indent, "char* stringify_", model->simple_name, "_list_indent(", model->name, " *in, size_t count, int indent) {");
        indent++;
        sb_cat_line(sb, indent, "Jsb jsb = {.pp = indent};");
        sb_cat_line(sb, indent, "if (jsb_begin_array(&jsb)) return NULL;");
        sb_cat_line(sb, indent, "for (size_t i = 0; i < count; i++) {");
        sb_cat_line(sb, indent + 1, "if (_stringify_", model->simple_name, "(&jsb, &in[i])) return NULL;");
        sb_cat_line(sb, indent, "}");
        sb_cat_line(sb, indent, "if (jsb_end_array(&jsb)) return NULL;");
        sb_cat_line(sb, indent, "return jsb_get(&jsb);");
        indent--;
        sb_cat_line(sb, indent, "}");
        str_append(sb, "\n");
        sb_cat_line(sb, indent, "#define stringify_", model->simple_name, "_list(in, count) stringify_", model->simple_name, "_list_indent((in), (count), 0)");
        str_append(sb, "\n");
    }
}

int generate_all_code(const char *out_filename, Models *models) {
    String sb = {0};
    str_append(&sb, "#include \"jsb.h\"\n#include \"jsp.h\"\n\n");
    da_foreach(models, model) {
        generate_model_code(&sb, model);
    }
    write_entire_file(out_filename, &sb);
    da_free(&sb);
    return 0;
}

int main(int argc, char **argv) {
    Models models = {0};
    char out_filename[256] = "models.g.h";
    if (argc < 2) {
        printf("Usage: %s <input_file.h or dir> [-o output_file.h]\n", argv[0]);
        return -1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            strncpy(out_filename, argv[++i], sizeof(out_filename) - 1);
        } else {
            DIR *dir = opendir(argv[i]);
            if(!dir) {
                if (parse_file(argv[i], &models) != 0) {
                    printf("Failed to parse file: %s\n", argv[i]);
                    return -1;
                }
            } else {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_type == DT_REG) {
                        const char *ext = strrchr(entry->d_name, '.');
                        if (ext && strcmp(ext, ".h") == 0) {
                            char filepath[512];
                            snprintf(filepath, sizeof(filepath), "%s/%s", argv[i], entry->d_name);
                            if (parse_file(filepath, &models) != 0) {
                                printf("Failed to parse file: %s\n", filepath);
                            }
                        }
                    }
                }
                closedir(dir);
            }
        }
    }

    generate_all_code(out_filename, &models);
    da_foreach(&models, model) {
        da_free(&model->fields);
    }
    da_free(&models);
    return 0;
}