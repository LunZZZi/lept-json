#ifndef LEPTJSON_H__
#define LEPTJSON_H__

namespace leptjson {

typedef enum { 
  LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT
} lept_type ;

typedef struct {
    double n;
    lept_type type;
} lept_value;

enum {
    LEPT_PARSE_OK = 100,
    LEPT_PARSE_EXPECT_VALUE = 200,
    LEPT_PARSE_INVALID_VALUE = 300,
    LEPT_PARSE_ROOT_NOT_SINGULAR = 400
};

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

double lept_get_number(lept_value* v);

}

#endif /* LEPTJSON_H__ */
