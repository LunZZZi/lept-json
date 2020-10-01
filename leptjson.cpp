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
#define ISATOFLOW(ch)       ((ch) >= 'a' && (ch) <= 'f')
#define ISATOFHIGH(ch)      ((ch) >= 'A' && (ch) <= 'F')
#define ISHEX(ch)           (ISDIGIT(ch) || ISATOFLOW(ch) || ISATOFHIGH(ch))
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

/**
 * parse unicode hex4
 * @param p pointer to next character
 * @param u codepoint
 * @return pointer to next character or NULL
 **/
static const char* lept_parse_hex4(const char* p, unsigned* u) {
    for (int i = 0; i < 4; i++) {
        if (!ISHEX(*p))
            return NULL;
        unsigned d = *p - '0';
        *u <<= 4;
        *u |= d;
        p++;
    }
    return p;
}

static void lept_encode_utf8(lept_context* c, unsigned u) {
    if (u >= 0x0000 && u <= 0x007F) {
        PUTC(c, u);
    }

    if (u >= 0x0080 && u <= 0x07FF) {
        PUTC(c, 0xC0 | ((u >>  6) & 0xFF)); /* 0xC0 = 11000000 */
        PUTC(c, 0x80 | ( u        & 0x3F)); /* 0x3F = 00011111 */
    }

    if (u >= 0x0800 && u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF)); /* 0xE0 = 11100000 */
        PUTC(c, 0x80 | ((u >>  6) & 0x3F)); /* 0x80 = 10000000 */
        PUTC(c, 0x80 | ( u        & 0x3F)); /* 0x3F = 00111111 */
    }

    if (u >= 0x10000 && u <= 0x10FFFF) {
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF)); /* 0xF0 = 11110000 */
        PUTC(c, 0x80 | ((u >> 12) & 0x3F)); /* 0xE0 = 11100000 */
        PUTC(c, 0x80 | ((u >>  6) & 0x3F)); /* 0x80 = 10000000 */
        PUTC(c, 0x80 | ( u        & 0x3F)); /* 0x3F = 00111111 */
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int lept_parse_string(lept_context* c, lept_value* v) {
    unsigned u, u2;
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
                        /* surrogate handling */
                        if (*(p + 1) != '\\' || *(p + 2) != 'u')
                           STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE); 
                        p += 2;
                        if (u >= 0xD800 && u <= 0xDBFF) {
                            if (!(p = lept_parse_hex4(p, &u2)))
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(u2 >= 0xDC00 && u2 <= 0xDFFF))
                               STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE); 
                            int high = u;
                            int low = u2;
                            u = 0x10000 + (high - 0xD800) * 0x400 + (low - 0xDC00);
                        }
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

static int lept_parse_value(lept_context* c, lept_value* v);

static int lept_parse_array(lept_context* c, lept_value* v) {
    size_t size = 0;
    int ret;
    EXPECT(c, '[');
    lept_parse_whitespace(c);
    if (*c->json == ']') {
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
    }
    for (;;) {
        lept_value e;

        lept_parse_whitespace(c);
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) {
            for (size_t i = 0; i < size; i++) {
                lept_value* value = (lept_value*)(lept_context_pop(c, sizeof(lept_value)));
                value->lept_free();
            }
            return ret;
        }
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        size++;

        lept_parse_whitespace(c);
        if (*c->json == ',')
            c->json++;
        else if (*c->json == ']') {
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            size *= sizeof(lept_value);
            memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
            return LEPT_PARSE_OK;
        }
        else
            return LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
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
        case '[':  return lept_parse_array(c, v);
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
    if (this->type == LEPT_STRING) {
        free(this->u.s.s);
    }
    if (this->type == LEPT_ARRAY) {
        for (size_t i = 0; i < this->u.a.size; i++) {
            this->u.a.e[i].lept_free();
        }
        free(this->u.a.e);
    }
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

size_t lept_value::lept_get_array_size() {
    assert(this->type == LEPT_ARRAY);
    return this->u.a.size;
}

lept_value* lept_value::lept_get_array_element(size_t index) {
    assert(this->type == LEPT_ARRAY);
    assert(index < this->u.a.size);
    return &this->u.a.e[index];
}

}
