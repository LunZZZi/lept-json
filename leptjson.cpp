#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod */
#include <cstring>
#include <cstdio>
#include <errno.h>
#include <cmath>

namespace leptjson {

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch) do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} lept_context;


static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char *str, lept_type type) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (c->json[i] != str[i])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += len;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /*  validate number */
    const char* ch = c->json;
    if (!ISDIGIT(*ch) && *ch != '-') 
        return LEPT_PARSE_INVALID_VALUE;
    ch++;
    while (ISDIGIT1TO9(*ch) || *ch == '.') {
        ch++;
    }
    if (*(ch - 1) == '.')
        return LEPT_PARSE_INVALID_VALUE;

    double n = strtod(c->json, &end);
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    c->json = end;
    v->lept_set_number(n);
    return LEPT_PARSE_OK;
}

static const char* lept_parse_hex4(const char* p, unsigned* u) {
    /* \TODO */
    return p;
}

static void lept_encode_utf8(lept_context* c, unsigned u) {
    /* \TODO */
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int lept_parse_string(lept_context* c, lept_value* v) {
    unsigned u;
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        if ('\x01' <= ch && ch <= '\x1F')
            return LEPT_PARSE_INVALID_STRING_CHAR;
        switch (ch) {
            case '\"':
                len = c->top - head;
                v->lept_set_string((const char*)lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
                c->top = head;
                return LEPT_PARSE_MISS_QUOTATION_MARK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    case 'u':
                        if (!(p = lept_parse_hex4(p, &u)))
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        /* \TODO surrogate handling */
                        lept_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            default:
                PUTC(c, ch);
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    // check length
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v);
    }
}

int lept_value::lept_parse(const char* json) {
    lept_context c;
    // assert(v != NULL);
    c.json = json;
    c.stack = NULL;        /* <- */
    c.size = c.top = 0;    /* <- */
    this->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    int result = lept_parse_value(&c, this);

    if (result == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0')
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
    }

    assert(c.top == 0);    /* <- */
    free(c.stack);         /* <- */
    return result;
}

lept_type lept_value::lept_get_type() {
    return type;
}

void lept_value::lept_free() {
    if (this->type == LEPT_STRING)
        free(this->u.s.s);
    this->type = LEPT_NULL;
}

int lept_value::lept_get_boolean() {
    assert((this->type == LEPT_TRUE || this->type == LEPT_FALSE));
    return this->type == LEPT_TRUE ? true : false;
}

void lept_value::lept_set_boolean(int b) {
    this->type = b > 0 ? LEPT_TRUE : LEPT_FALSE;
}

double lept_value::lept_get_number() {
    assert(this->type == LEPT_NUMBER);
    return this->u.n;
}

void lept_value::lept_set_number(double n) {
    this->u.n = n;
    this->type = LEPT_NUMBER;
}

const char* lept_value::lept_get_string() {
    assert(this->type == LEPT_STRING);
    return this->u.s.s;
}

size_t lept_value::lept_get_string_length() {
    assert(this->type == LEPT_STRING);
    return this->u.s.len;
}

void lept_value::lept_set_string(const char* s, size_t len) {
    assert((s != NULL || len == 0));
    this->lept_free();
    this->u.s.s = (char*)malloc(len + 1);
    memcpy(this->u.s.s, s, len);
    this->u.s.s[len] = '\0';
    this->u.s.len = len;
    this->type = LEPT_STRING;
}

}
