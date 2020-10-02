#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <cstring>
#include <cstdio>

namespace leptjson {

typedef enum { 
  LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT
} lept_type ;

enum {
    LEPT_PARSE_OK = 100,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET
};

struct lept_member;

class lept_value {
public:
    lept_value(): type(LEPT_NULL) {}
    
    void lept_free();

    int lept_parse(const char* json);

    lept_type lept_get_type();
    void lept_set_type(lept_type t) { type = t; }
    
    void lept_set_null() { lept_free(); }
    
    int lept_get_boolean();
    void lept_set_boolean(int b);

    double lept_get_number();
    void lept_set_number(double n);

    const char* lept_get_string();
    size_t lept_get_string_length();
    void lept_set_string(const char* s, size_t len);

    size_t lept_get_array_size();
    lept_value* lept_get_array_element(size_t index);

    // size_t lept_get_object_size();
    // const char* lept_get_object_key(size_t index);
    // size_t lept_get_object_key_length(size_t index);
    // lept_value* lept_get_object_value(size_t index);

public:
    union {
        struct { lept_member* m; size_t size; }o ;
        struct { lept_value* e; size_t size; } a; /* array */
        struct { char* s; size_t len; } s;  /* string */
        double n;                          /* number */
    } u;
public:
    lept_type type;
};

struct lept_member {
    char* k; size_t klen;   /* member key string, key string length */
    lept_value v;           /* member value */
};

}

#endif /* LEPTJSON_H__ */
