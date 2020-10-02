#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

using namespace leptjson;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_BOOL(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_EQ_UL(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%lu")
#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")

#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse(json));\
        EXPECT_EQ_INT(LEPT_NUMBER, v.lept_get_type());\
        EXPECT_EQ_DOUBLE(expect, v.lept_get_number());\
    } while(0)

#define TEST_BOOL(expect, json, type)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse(json));\
        EXPECT_EQ_INT(type, v.lept_get_type());\
        EXPECT_EQ_BOOL(expect, v.lept_get_boolean());\
    } while(0)

#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.lept_set_type(LEPT_NULL);\
        EXPECT_EQ_INT(error, v.lept_parse(json));\
        EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());\
    } while(0)

static void test_parse_null() {
    lept_value v;
    v.lept_set_type(LEPT_FALSE);
    EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse("null"));
    EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());
}

static void test_parse_true() {
    TEST_BOOL(true, "true", LEPT_TRUE);
}

static void test_parse_false() {
    TEST_BOOL(false, "false", LEPT_FALSE);
}

static void test_parse_expect_value() {
    lept_value v;

    v.lept_set_type(LEPT_FALSE);
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, v.lept_parse(""));
    EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());

    v.lept_set_type(LEPT_FALSE);
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, v.lept_parse(" "));
    EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());
}

static void test_parse_invalid_value() {
    lept_value v;
    v.lept_set_type(LEPT_FALSE);
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, v.lept_parse("nul"));
    EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());

    v.lept_set_type(LEPT_FALSE);
    v.lept_set_type(LEPT_FALSE);
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, v.lept_parse("?"));
    EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());

    /* invalid value in array */
#if 1
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[1, null, 123,]");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[\"a\", nul]");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[1, [1, false], 123, [4, null,]");
#endif
}

static void test_parse_object() {
    lept_value v;
    size_t i;

    EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse(" { } "));
    EXPECT_EQ_INT(LEPT_OBJECT, v.lept_get_type());
    EXPECT_EQ_SIZE_T(0, v.lept_get_object_size());
    v.lept_free();

    v.type = LEPT_NULL;
    EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse(
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(LEPT_OBJECT, v.lept_get_type());
    EXPECT_EQ_SIZE_T(7, v.lept_get_object_size());
    EXPECT_EQ_STRING("n", v.lept_get_object_key(0), v.lept_get_object_key_length(0));
    EXPECT_EQ_INT(LEPT_NULL,  v.lept_get_object_value(0)->lept_get_type());
    EXPECT_EQ_STRING("f", v.lept_get_object_key(1), v.lept_get_object_key_length(1));
    EXPECT_EQ_INT(LEPT_FALSE,  v.lept_get_object_value(1)->lept_get_type());
    EXPECT_EQ_STRING("t", v.lept_get_object_key(2), v.lept_get_object_key_length(2));
    EXPECT_EQ_INT(LEPT_TRUE,   v.lept_get_object_value(2)->lept_get_type());
    EXPECT_EQ_STRING("i", v.lept_get_object_key(3), v.lept_get_object_key_length(3));
    EXPECT_EQ_INT(LEPT_NUMBER, v.lept_get_object_value(3)->lept_get_type());
    EXPECT_EQ_DOUBLE(123.0, v.lept_get_object_value(3)->lept_get_number());
    EXPECT_EQ_STRING("s", v.lept_get_object_key(4), v.lept_get_object_key_length(4));
    EXPECT_EQ_INT(LEPT_STRING, v.lept_get_object_value(4)->lept_get_type());
    EXPECT_EQ_STRING("abc", v.lept_get_object_value(4)->lept_get_string(), v.lept_get_object_value(4)->lept_get_string_length());
    EXPECT_EQ_STRING("a", v.lept_get_object_key(5), v.lept_get_object_key_length(5));
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_object_value(5)->lept_get_type());
    EXPECT_EQ_SIZE_T(3, v.lept_get_object_value(5)->lept_get_array_size());
    for (i = 0; i < 3; i++) {
        lept_value* e = v.lept_get_object_value(5)->lept_get_array_element(i);
        EXPECT_EQ_INT(LEPT_NUMBER, e->lept_get_type());
        EXPECT_EQ_DOUBLE(i + 1.0, e->lept_get_number());
    }
    EXPECT_EQ_STRING("o", v.lept_get_object_key(6), v.lept_get_object_key_length(6));
    {
        lept_value* o = v.lept_get_object_value(6);
        EXPECT_EQ_INT(LEPT_OBJECT, o->lept_get_type());
        for (i = 0; i < 3; i++) {
            lept_value* ov = o->lept_get_object_value(i);
            EXPECT_TRUE('1' + i == o->lept_get_object_key(i)[0]);
            EXPECT_EQ_SIZE_T(1, o->lept_get_object_key_length(i));
            EXPECT_EQ_INT(LEPT_NUMBER, ov->lept_get_type());
            EXPECT_EQ_DOUBLE(i + 1.0, ov->lept_get_number());
        }
    }
    v.lept_free();
}

static void test_parse_root_not_singular() {
    lept_value v;
    v.lept_set_type(LEPT_FALSE);
    EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, v.lept_parse("null x"));
    EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());
    EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse("null "));
}

static void test_parse_miss_key() {
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
}

#define TEST_STRING(expect, json)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse(json));\
        EXPECT_EQ_INT(LEPT_STRING, v.lept_get_type());\
        EXPECT_EQ_STRING(expect, v.lept_get_string(), v.lept_get_string_length());\
        v.lept_free();\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
#if 1
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
}

#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.lept_set_type(LEPT_NULL);\
        EXPECT_EQ_INT(error, v.lept_parse(json));\
        EXPECT_EQ_INT(LEPT_NULL, v.lept_get_type());\
    } while(0)

static void test_parse_invalid_number() {
    /* ... */
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
}

static void test_parse_invalid_string_escape() {
#if 1
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char() {
#if 1
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_array() {
    lept_value v;

    EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse("[ ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_type());
    EXPECT_EQ_SIZE_T(0, v.lept_get_array_size());
    v.lept_free();
}

static void test_parse_array2() {
    lept_value v;

    EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse("[ true   , false, 123  , null , \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_type());
    EXPECT_EQ_SIZE_T(5, v.lept_get_array_size());
    EXPECT_EQ_INT(LEPT_TRUE, v.lept_get_array_element(0)->lept_get_type()); 
    EXPECT_EQ_INT(LEPT_FALSE, v.lept_get_array_element(1)->lept_get_type()); 
    EXPECT_EQ_INT(LEPT_NUMBER, v.lept_get_array_element(2)->lept_get_type()); 
    EXPECT_EQ_INT(LEPT_NULL, v.lept_get_array_element(3)->lept_get_type()); 
    EXPECT_EQ_INT(LEPT_STRING, v.lept_get_array_element(4)->lept_get_type()); 
    v.lept_free();
}

static void test_parse_array3() {
    lept_value v;

    EXPECT_EQ_INT(LEPT_PARSE_OK, v.lept_parse("[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_type());
    EXPECT_EQ_SIZE_T(4, v.lept_get_array_size());
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_array_element(0)->lept_get_type()); 
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_array_element(1)->lept_get_type()); 
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_array_element(2)->lept_get_type()); 
    EXPECT_EQ_INT(LEPT_ARRAY, v.lept_get_array_element(3)->lept_get_type()); 
    EXPECT_EQ_UL(0UL, v.lept_get_array_element(0)->lept_get_array_size()); 
    EXPECT_EQ_UL(1UL, v.lept_get_array_element(1)->lept_get_array_size()); 
    EXPECT_EQ_UL(2UL, v.lept_get_array_element(2)->lept_get_array_size()); 
    EXPECT_EQ_UL(3UL, v.lept_get_array_element(3)->lept_get_array_size()); 
    v.lept_free();
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number();
    test_parse_invalid_number();
    test_parse_string();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_array();
    test_parse_array2();
    test_parse_array3();
    test_parse_object();
#if 0
    test_parse_miss_colon();
    test_parse_miss_key();
    test_parse_miss_comma_or_curly_bracket();
#endif
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
