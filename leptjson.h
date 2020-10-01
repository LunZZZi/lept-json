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
    LEPT_PARSE_INVALID_STRING_CHAR
};

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
private:
    union {
        struct { char* s; size_t len; } s;  /* string */
        double n;                          /* number */
    } u;
public:
    lept_type type;
};

}

#endif /* LEPTJSON_H__ */
