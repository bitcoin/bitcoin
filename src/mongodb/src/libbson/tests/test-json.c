/* required on old Windows for rand_s to be defined */
#define _CRT_RAND_S

#include <bson.h>
#include <bcon.h>
#include <fcntl.h>
#include <stdio.h>
#include <bson-string.h>
#include <bson-private.h>
#include <math.h>

#include "bson-config.h"
#include "bson-tests.h"
#include "TestSuite.h"

#ifndef BINARY_DIR
#define BINARY_DIR "tests/binary"
#endif

#ifndef JSON_DIR
#define JSON_DIR "tests/json"
#endif

static ssize_t
test_bson_json_read_cb_helper (void *string, uint8_t *buf, size_t len)
{
   size_t str_size = strlen ((char *) string);
   if (str_size) {
      memcpy (buf, string, BSON_MIN (str_size, len));
      return str_size;
   } else {
      return 0;
   }
}

static void
test_bson_json_allow_multiple (void)
{
   int32_t one;
   bson_error_t error;
   bson_json_reader_t *reader;
   bson_json_reader_t *reader_multiple;
   bson_t bson = BSON_INITIALIZER;
   bool allow_multiple = true;
   char *multiple_json = "{\"a\": 1}{\"b\": 1}";

   reader = bson_json_reader_new (
      multiple_json, &test_bson_json_read_cb_helper, NULL, !allow_multiple, 0);
   reader_multiple = bson_json_reader_new (
      multiple_json, &test_bson_json_read_cb_helper, NULL, allow_multiple, 0);
   BSON_ASSERT (reader);
   BSON_ASSERT (reader_multiple);

   /* reader with allow_multiple false */
   /* read first json */
   ASSERT_CMPINT (1, ==, bson_json_reader_read (reader, &bson, &error));
   BCON_EXTRACT (&bson, "a", BCONE_INT32 (one));
   ASSERT_CMPINT (1, ==, one);

   /* read second json */
   bson_reinit (&bson);
   ASSERT_CMPINT (1, ==, bson_json_reader_read (reader, &bson, &error));
   BCON_EXTRACT (&bson, "b", BCONE_INT32 (one));
   ASSERT_CMPINT (1, ==, one);

   /* start at the beginning of the string */
   bson_reinit (&bson);
   ASSERT_CMPINT (1, ==, bson_json_reader_read (reader, &bson, &error));
   BCON_EXTRACT (&bson, "a", BCONE_INT32 (one));
   ASSERT_CMPINT (1, ==, one);

   /* reader_multiple with allow_multiple true */
   /* read first json */
   bson_reinit (&bson);
   ASSERT_CMPINT (
      1, ==, bson_json_reader_read (reader_multiple, &bson, &error));
   BCON_EXTRACT (&bson, "a", BCONE_INT32 (one));
   ASSERT_CMPINT (1, ==, one);

   /* read second json */
   bson_reinit (&bson);
   ASSERT_CMPINT (
      1, ==, bson_json_reader_read (reader_multiple, &bson, &error));
   BCON_EXTRACT (&bson, "b", BCONE_INT32 (one));
   ASSERT_CMPINT (1, ==, one);

   /* start at the beginning of the string */
   bson_reinit (&bson);
   ASSERT_CMPINT (
      1, ==, bson_json_reader_read (reader_multiple, &bson, &error));
   BCON_EXTRACT (&bson, "a", BCONE_INT32 (one));
   ASSERT_CMPINT (1, ==, one);

   bson_json_reader_destroy (reader);
   bson_json_reader_destroy (reader_multiple);
   bson_destroy (&bson);
}


static void
test_bson_as_json_x1000 (void)
{
   bson_oid_t oid;
   bson_decimal128_t decimal128;
   bson_t *b;
   bson_t *b2;
   char *str;
   size_t len;
   int i;

   decimal128.high = 0x3040000000000000ULL;
   decimal128.low = 0x000000000000000B;
   bson_oid_init_from_string (&oid, "123412341234abcdabcdabcd");

   b = bson_new ();
   BSON_ASSERT (bson_append_utf8 (b, "utf8", -1, "bar", -1));
   BSON_ASSERT (bson_append_int32 (b, "int32", -1, 1234));
   BSON_ASSERT (bson_append_int64 (b, "int64", -1, 4321));
   BSON_ASSERT (bson_append_double (b, "double", -1, 123.4));
   BSON_ASSERT (bson_append_undefined (b, "undefined", -1));
   BSON_ASSERT (bson_append_null (b, "null", -1));
   BSON_ASSERT (bson_append_oid (b, "oid", -1, &oid));
   BSON_ASSERT (bson_append_bool (b, "true", -1, true));
   BSON_ASSERT (bson_append_bool (b, "false", -1, false));
   BSON_ASSERT (bson_append_time_t (b, "date", -1, time (NULL)));
   BSON_ASSERT (
      bson_append_timestamp (b, "timestamp", -1, (uint32_t) time (NULL), 1234));
   BSON_ASSERT (bson_append_regex (b, "regex", -1, "^abcd", "xi"));
   BSON_ASSERT (
      bson_append_dbpointer (b, "dbpointer", -1, "mycollection", &oid));
   BSON_ASSERT (bson_append_minkey (b, "minkey", -1));
   BSON_ASSERT (bson_append_maxkey (b, "maxkey", -1));
   BSON_ASSERT (bson_append_symbol (b, "symbol", -1, "var a = {};", -1));
   BSON_ASSERT (bson_append_decimal128 (b, "decimal128", -1, &decimal128));

   b2 = bson_new ();
   BSON_ASSERT (bson_append_int32 (b2, "0", -1, 60));
   BSON_ASSERT (bson_append_document (b, "document", -1, b2));
   BSON_ASSERT (bson_append_array (b, "array", -1, b2));

   {
      const uint8_t binary[] = {0, 1, 2, 3, 4};
      BSON_ASSERT (bson_append_binary (
         b, "binary", -1, BSON_SUBTYPE_BINARY, binary, sizeof binary));
   }

   for (i = 0; i < 1000; i++) {
      str = bson_as_json (b, &len);
      bson_free (str);
   }

   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_as_json_multi (void)
{
   bson_t *b;
   char *str;
   size_t len;

   b = bson_new ();

   {
      bson_oid_t oid;
      bson_oid_init_from_string (&oid, "57e193d7a9cc81b4027498b5");
      BSON_ASSERT (bson_append_oid (b, "_id", -1, &oid));
   }

   BSON_ASSERT (bson_append_symbol (b, "Symbol", -1, "symbol", -1));
   BSON_ASSERT (bson_append_utf8 (b, "String", -1, "string", -1));
   BSON_ASSERT (bson_append_int32 (b, "Int32", -1, 42));
   BSON_ASSERT (bson_append_int64 (b, "Int64", -1, 42));
   BSON_ASSERT (bson_append_double (b, "Double", -1, -1.0));

   {
      const uint8_t binary[] = { 0xa3, 0x4c, 0x38, 0xf7, 0xc3, 0xab,
                                 0xed, 0xc8, 0xa3, 0x78, 0x14, 0xa9,
                                 0x92, 0xab, 0x8d, 0xb6 };
      BSON_ASSERT (bson_append_binary (
         b, "Binary", -1, BSON_SUBTYPE_UUID_DEPRECATED, binary, sizeof binary));
   }

   {
      const uint8_t binary[] = { 1, 2, 3, 4, 5 };
      BSON_ASSERT (bson_append_binary (
         b, "BinaryUserDefined", -1, BSON_SUBTYPE_USER, binary, sizeof binary));
   }

   BSON_ASSERT (bson_append_code (b, "Code", -1, "function() {}"));

   {
      bson_t *scope = bson_new ();
      BSON_ASSERT (bson_append_code_with_scope (
         b, "CodeWithScope", -1, "function() {}", scope));
      bson_destroy (scope);
   }

   {
      bson_t *document = bson_new ();
      BSON_ASSERT (bson_append_utf8 (document, "foo", -1, "bar", -1));
      BSON_ASSERT (bson_append_document (b, "Subdocument", -1, document));
      bson_destroy (document);
   }

   {
      bson_t *array = bson_new ();
      BSON_ASSERT (bson_append_int32 (array, "0", -1, 1));
      BSON_ASSERT (bson_append_int32 (array, "1", -1, 2));
      BSON_ASSERT (bson_append_int32 (array, "2", -1, 3));
      BSON_ASSERT (bson_append_int32 (array, "3", -1, 4));
      BSON_ASSERT (bson_append_int32 (array, "4", -1, 5));
      BSON_ASSERT (bson_append_array (b, "Array", -1, array));
      bson_destroy (array);
   }

   BSON_ASSERT (bson_append_timestamp (b, "Timestamp", -1, 42, 1));
   BSON_ASSERT (bson_append_regex (b, "Regex", -1, "pattern", ""));
   BSON_ASSERT (bson_append_date_time (b, "DatetimeEpoch", -1, 0));
   BSON_ASSERT (bson_append_date_time (b, "DatetimePositive", -1, (int64_t) 2147483647LL));
   BSON_ASSERT (bson_append_date_time (b, "DatetimeNegative", -1, (int64_t) -2147483648LL));
   BSON_ASSERT (bson_append_bool (b, "True", -1, true));
   BSON_ASSERT (bson_append_bool (b, "False", -1, false));

   {
      bson_oid_t oid;
      bson_oid_init_from_string (&oid, "57e193d7a9cc81b4027498b1");
      BSON_ASSERT (bson_append_dbpointer (
         b, "DBPointer", -1, "collection", &oid));
   }

   {
      bson_oid_t oid;
      bson_t *dbref = bson_new ();
      bson_oid_init_from_string (&oid, "57fd71e96e32ab4225b723fb");
      BSON_ASSERT (bson_append_utf8 (dbref, "$ref", -1, "collection", -1));
      BSON_ASSERT (bson_append_oid (dbref, "$id", -1, &oid));
      BSON_ASSERT (bson_append_utf8 (dbref, "$db", -1, "database", -1));
      BSON_ASSERT (bson_append_document (b, "DBRef", -1, dbref));
      bson_destroy (dbref);
   }

   BSON_ASSERT (bson_append_minkey (b, "Minkey", -1));
   BSON_ASSERT (bson_append_maxkey (b, "Maxkey", -1));
   BSON_ASSERT (bson_append_null (b, "Null", -1));
   BSON_ASSERT (bson_append_undefined (b, "Undefined", -1));

   {
      bson_decimal128_t decimal128;
      decimal128.high = 0x3040000000000000ULL;
      decimal128.low = 0x000000000000000B;
      BSON_ASSERT (bson_append_decimal128 (b, "Decimal128", -1, &decimal128));
   }

   str = bson_as_json (b, &len);

   /* Based on multi-type-deprecated.json from BSON Corpus Tests. */
   ASSERT_CMPSTR (str,
                  "{"
                  " \"_id\" : { \"$oid\" : \"57e193d7a9cc81b4027498b5\" },"
                  " \"Symbol\" : \"symbol\","
                  " \"String\" : \"string\","
                  " \"Int32\" : 42,"
                  " \"Int64\" : 42,"
                  " \"Double\" : -1.0,"
                  " \"Binary\" : { \"$binary\" : \"o0w498Or7cijeBSpkquNtg==\", \"$type\" : \"03\" },"
                  " \"BinaryUserDefined\" : { \"$binary\" : \"AQIDBAU=\", \"$type\" : \"80\" },"
                  " \"Code\" : { \"$code\" : \"function() {}\" },"
                  " \"CodeWithScope\" : { \"$code\" : \"function() {}\", \"$scope\" : { } },"
                  " \"Subdocument\" : { \"foo\" : \"bar\" },"
                  " \"Array\" : [ 1, 2, 3, 4, 5 ],"
                  " \"Timestamp\" : { \"$timestamp\" : { \"t\" : 42, \"i\" : 1 } },"
                  " \"Regex\" : { \"$regex\" : \"pattern\", \"$options\" : \"\" },"
                  " \"DatetimeEpoch\" : { \"$date\" : 0 },"
                  " \"DatetimePositive\" : { \"$date\" : 2147483647 },"
                  " \"DatetimeNegative\" : { \"$date\" : -2147483648 },"
                  " \"True\" : true,"
                  " \"False\" : false,"
                  " \"DBPointer\" : { \"$ref\" : \"collection\", \"$id\" : \"57e193d7a9cc81b4027498b1\" },"
                  " \"DBRef\" : { \"$ref\" : \"collection\", \"$id\" : { \"$oid\" : \"57fd71e96e32ab4225b723fb\" }, \"$db\" : \"database\" },"
                  " \"Minkey\" : { \"$minKey\" : 1 },"
                  " \"Maxkey\" : { \"$maxKey\" : 1 },"
                  " \"Null\" : null,"
                  " \"Undefined\" : { \"$undefined\" : true },"
                  " \"Decimal128\" : { \"$numberDecimal\" : \"11\" } }");

   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_string (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new ();
   BSON_ASSERT (bson_append_utf8 (b, "foo", -1, "bar", -1));
   str = bson_as_json (b, &len);
   BSON_ASSERT (len == 17);
   BSON_ASSERT (!strcmp ("{ \"foo\" : \"bar\" }", str));
   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_int32 (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new ();
   BSON_ASSERT (bson_append_int32 (b, "foo", -1, 1234));
   str = bson_as_json (b, &len);
   BSON_ASSERT (len == 16);
   BSON_ASSERT (!strcmp ("{ \"foo\" : 1234 }", str));
   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_int64 (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new ();
   BSON_ASSERT (bson_append_int64 (b, "foo", -1, 341234123412341234ULL));
   str = bson_as_json (b, &len);
   BSON_ASSERT (len == 30);
   BSON_ASSERT (!strcmp ("{ \"foo\" : 341234123412341234 }", str));
   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_double (void)
{
   size_t len;
   bson_t *b;
   char *str;
   char *expected;

   b = bson_new ();
   BSON_ASSERT (bson_append_double (b, "foo", -1, 123.5));
   BSON_ASSERT (bson_append_double (b, "bar", -1, 3));
   BSON_ASSERT (bson_append_double (b, "baz", -1, -1));
   BSON_ASSERT (bson_append_double (b, "quux", -1, 0.03125));
   BSON_ASSERT (bson_append_double (b, "huge", -1, 1e99));
   str = bson_as_json (b, &len);

   expected = bson_strdup_printf ("{"
                                  " \"foo\" : 123.5,"
                                  " \"bar\" : 3.0,"
                                  " \"baz\" : -1.0,"
                                  " \"quux\" : 0.03125,"
                                  " \"huge\" : %.20g }",
                                  1e99);

   ASSERT_CMPSTR (str, expected);

   bson_free (expected);
   bson_free (str);
   bson_destroy (b);
}


#if defined(NAN) && defined(INFINITY)
static void
test_bson_as_json_double_nonfinite (void)
{
   size_t len;
   bson_t *b;
   char *str;
   char *expected;

   b = bson_new ();
   BSON_ASSERT (bson_append_double (b, "nan", -1, NAN));
   BSON_ASSERT (bson_append_double (b, "pos_inf", -1, INFINITY));
   BSON_ASSERT (bson_append_double (b, "neg_inf", -1, -INFINITY));
   str = bson_as_json (b, &len);

   expected = bson_strdup_printf ("{"
                                  " \"nan\" : %.20g,"
                                  " \"pos_inf\" : %.20g,"
                                  " \"neg_inf\" : %.20g }",
                                  NAN,
                                  INFINITY,
                                  -INFINITY);

   ASSERT_CMPSTR (str, expected);

   bson_free (expected);
   bson_free (str);
   bson_destroy (b);
}
#endif


static void
test_bson_as_json_decimal128 (void)
{
   size_t len;
   bson_t *b;
   char *str;
   bson_decimal128_t decimal128;
   decimal128.high = 0x3040000000000000ULL;
   decimal128.low = 0x000000000000000B;

   b = bson_new ();
   BSON_ASSERT (bson_append_decimal128 (b, "decimal128", -1, &decimal128));
   str = bson_as_json (b, &len);
   ASSERT_CMPSTR (str,
                  "{ "
                  "\"decimal128\" : { \"$numberDecimal\" : \"11\" }"
                  " }");

   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_code (void)
{
   bson_t code = BSON_INITIALIZER;
   bson_t scope = BSON_INITIALIZER;
   char *str;

   BSON_ASSERT (bson_append_code (&code, "c", -1, "function () {}"));
   str = bson_as_json (&code, NULL);
   ASSERT_CMPSTR (str, "{ \"c\" : { \"$code\" : \"function () {}\" } }");

   bson_free (str);
   bson_reinit (&code);

   /* empty scope */
   BSON_ASSERT (
      BSON_APPEND_CODE_WITH_SCOPE (&code, "c", "function () {}", &scope));
   str = bson_as_json (&code, NULL);
   ASSERT_CMPSTR (
      str, "{ \"c\" : { \"$code\" : \"function () {}\", \"$scope\" : { } } }");

   bson_free (str);
   bson_reinit (&code);

   BSON_APPEND_INT32 (&scope, "x", 1);
   BSON_ASSERT (
      BSON_APPEND_CODE_WITH_SCOPE (&code, "c", "function () {}", &scope));
   str = bson_as_json (&code, NULL);
   ASSERT_CMPSTR (str,
                  "{ \"c\" : { \"$code\" : \"function () {}\", \"$scope\" "
                  ": { \"x\" : 1 } } }");

   bson_free (str);
   bson_reinit (&code);

   /* test that embedded quotes are backslash-escaped */
   BSON_ASSERT (BSON_APPEND_CODE (&code, "c", "return \"a\""));
   str = bson_as_json (&code, NULL);

   /* hard to read, this is { "c" : { "$code" : "return \"a\"" } } */
   ASSERT_CMPSTR (str, "{ \"c\" : { \"$code\" : \"return \\\"a\\\"\" } }");

   bson_free (str);
   bson_destroy (&code);
   bson_destroy (&scope);
}


static void
test_bson_as_json_date_time (void)
{
   bson_t *b;
   char *str;
   size_t len;

   b = bson_new ();
   BSON_ASSERT (bson_append_date_time (b, "epoch", -1, 0));
   BSON_ASSERT (bson_append_date_time (b, "negative", -1, -123456000));
   BSON_ASSERT (bson_append_date_time (b, "positive", -1, 123456000));
   str = bson_as_json (b, &len);

   ASSERT_CMPSTR (str,
                  "{"
                  " \"epoch\" : { \"$date\" : 0 },"
                  " \"negative\" : { \"$date\" : -123456000 },"
                  " \"positive\" : { \"$date\" : 123456000 } }");

   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_regex (void)
{
   bson_t *b;
   char *str;
   size_t len;

   b = bson_new ();
   BSON_ASSERT (bson_append_regex (b, "regex", -1, "^abcd", "xi"));
   BSON_ASSERT (bson_append_regex (b, "escaping", -1, "^\"", ""));
   BSON_ASSERT (bson_append_regex (b, "ordered", -1, "^abcd", "ilmsux"));
   BSON_ASSERT (bson_append_regex (b, "unordered", -1, "^abcd", "xusmli"));
   BSON_ASSERT (bson_append_regex (b, "duplicate", -1, "^abcd", "mmiii"));
   BSON_ASSERT (bson_append_regex (b, "unsupported", -1, "^abcd", "jkmlvz"));
   str = bson_as_json (b, &len);

   ASSERT_CMPSTR (str,
                  "{"
                  " \"regex\" : { \"$regex\" : \"^abcd\", \"$options\" "
                  ": \"ix\" },"
                  " \"escaping\" : { \"$regex\" : \"^\\\"\", \"$options\" "
                  ": \"\" },"
                  " \"ordered\" : { \"$regex\" : \"^abcd\", \"$options\" "
                  ": \"ilmsux\" },"
                  " \"unordered\" : { \"$regex\" : \"^abcd\", \"$options\" "
                  ": \"ilmsux\" },"
                  " \"duplicate\" : { \"$regex\" : \"^abcd\", \"$options\" "
                  ": \"im\" },"
                  " \"unsupported\" : { \"$regex\" : \"^abcd\", \"$options\" "
                  ": \"lm\" } }");

   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_symbol (void)
{
   bson_t *b;
   char *str;
   size_t len;

   b = bson_new ();
   BSON_ASSERT (bson_append_symbol (b, "symbol", -1, "foo", -1));
   BSON_ASSERT (bson_append_symbol (b, "escaping", -1, "\"bar\"", -1));
   str = bson_as_json (b, &len);

   ASSERT_CMPSTR (str,
                  "{"
                  " \"symbol\" : \"foo\","
                  " \"escaping\" : \"\\\"bar\\\"\" }");

   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_utf8 (void)
{
/* euro currency symbol */
#define EU "\xe2\x82\xac"
#define FIVE_EUROS EU EU EU EU EU
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new ();
   BSON_ASSERT (bson_append_utf8 (b, FIVE_EUROS, -1, FIVE_EUROS, -1));
   str = bson_as_json (b, &len);
   BSON_ASSERT (!strcmp (str, "{ \"" FIVE_EUROS "\" : \"" FIVE_EUROS "\" }"));
   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_json_dbpointer (void)
{
   bson_oid_t oid;
   bson_t *b;
   char *str;
   size_t len;

   bson_oid_init_from_string (&oid, "12341234123412abcdababcd");

   b = bson_new ();
   BSON_ASSERT (bson_append_dbpointer (b, "dbpointer", -1, "collection", &oid));
   BSON_ASSERT (bson_append_dbpointer (b, "escaping", -1, "\"coll\"", &oid));
   str = bson_as_json (b, &len);

   ASSERT_CMPSTR (str,
                  "{"
                  " \"dbpointer\" : { \"$ref\" : \"collection\", \"$id\" "
                  ": \"12341234123412abcdababcd\" },"
                  " \"escaping\" : { \"$ref\" : \"\\\"coll\\\"\", \"$id\" "
                  ": \"12341234123412abcdababcd\" } }");

   bson_free (str);
   bson_destroy (b);
}


static void
test_bson_as_canonical_extended_json_dbpointer (void)
{
   bson_oid_t oid;
   bson_t *b;
   size_t len;
   char *str;

   bson_oid_init_from_string (&oid, "12341234123412abcdababcd");
   b = bson_new ();
   BSON_ASSERT (BSON_APPEND_DBPOINTER (b, "p", "coll", &oid));
   str = bson_as_canonical_extended_json (b, &len);
   ASSERT_CMPJSON (str,
                   "{ \"p\" : { \"$dbPointer\" : { \"$ref\" : "
                   "\"coll\", \"$id\" : { \"$oid\" : "
                   "\"12341234123412abcdababcd\" } } } }");

   bson_destroy (b);
}


static void
test_bson_as_json_stack_overflow (void)
{
   uint8_t *buf;
   bson_t b;
   size_t buflen = 1024 * 1024 * 17;
   char *str;
   int fd;
   ssize_t r;

   buf = bson_malloc0 (buflen);

   fd = bson_open (BINARY_DIR "/stackoverflow.bson", O_RDONLY);
   BSON_ASSERT (-1 != fd);

   r = bson_read (fd, buf, buflen);
   BSON_ASSERT (r == 16777220);

   r = bson_init_static (&b, buf, 16777220);
   BSON_ASSERT (r);

   str = bson_as_json (&b, NULL);
   BSON_ASSERT (str);

   r = !!strstr (str, "...");
   BSON_ASSERT (r);

   bson_free (str);
   bson_destroy (&b);
   bson_free (buf);
}


static void
test_bson_corrupt (void)
{
   uint8_t *buf;
   bson_t b;
   size_t buflen = 1024;
   char *str;
   int fd;
   ssize_t r;

   buf = bson_malloc0 (buflen);

   fd = bson_open (BINARY_DIR "/test55.bson", O_RDONLY);
   BSON_ASSERT (-1 != fd);

   r = bson_read (fd, buf, buflen);
   BSON_ASSERT (r == 24);

   r = bson_init_static (&b, buf, (uint32_t) r);
   BSON_ASSERT (r);

   str = bson_as_json (&b, NULL);
   BSON_ASSERT (!str);

   bson_destroy (&b);
   bson_free (buf);
}

static void
test_bson_corrupt_utf8 (void)
{
   uint8_t *buf;
   bson_t b;
   size_t buflen = 1024;
   char *str;
   int fd;
   ssize_t r;

   buf = bson_malloc0 (buflen);

   fd = bson_open (BINARY_DIR "/test56.bson", O_RDONLY);
   BSON_ASSERT (-1 != fd);

   r = bson_read (fd, buf, buflen);
   BSON_ASSERT (r == 42);

   r = bson_init_static (&b, buf, (uint32_t) r);
   BSON_ASSERT (r);

   str = bson_as_json (&b, NULL);
   BSON_ASSERT (!str);

   bson_destroy (&b);
   bson_free (buf);
}

static void
test_bson_corrupt_binary (void)
{
   uint8_t *buf;
   bson_t b;
   size_t buflen = 1024;
   char *str;
   int fd;
   ssize_t r;

   buf = bson_malloc0 (buflen);

   fd = bson_open (BINARY_DIR "/test57.bson", O_RDONLY);
   BSON_ASSERT (-1 != fd);

   r = bson_read (fd, buf, buflen);
   BSON_ASSERT (r == 26);

   r = bson_init_static (&b, buf, (uint32_t) r);
   BSON_ASSERT (r);

   str = bson_as_json (&b, NULL);
   BSON_ASSERT (!str);

   bson_destroy (&b);
   bson_free (buf);
}

#ifdef _WIN32
#define RAND_R rand_s
#else
#define RAND_R rand_r
#endif

static char *
rand_str (size_t maxlen,
          unsigned int *seed /* IN / OUT */,
          char *buf /* IN / OUT */)
{
   size_t len = RAND_R (seed) % (maxlen - 1);
   size_t i;

   for (i = 0; i < len; i++) {
      buf[i] = (char) ('a' + i % 26);
   }

   buf[len] = '\0';
   return buf;
}

/* test with random buffer sizes to ensure we parse whole keys and values when
 * reads pause and resume in the middle of tokens */
static void
test_bson_json_read_buffering (void)
{
   bson_t **bsons;
   char *json_tmp;
   bson_string_t *json;
   bson_error_t error;
   bson_t bson_out = BSON_INITIALIZER;
   int i;
   unsigned int seed = 42;
   int n_docs, docs_idx;
   int n_elems, elem_idx;
   char key[25];
   char val[25];
   bson_json_reader_t *reader;
   int r;

   json = bson_string_new (NULL);

   /* parse between 1 and 10 JSON objects */
   for (n_docs = 1; n_docs < 10; n_docs++) {
      /* do 50 trials */
      for (i = 0; i < 50; i++) {
         bsons = bson_malloc (n_docs * sizeof (bson_t *));
         for (docs_idx = 0; docs_idx < n_docs; docs_idx++) {
            /* a BSON document with up to 10 strings and numbers */
            bsons[docs_idx] = bson_new ();
            n_elems = RAND_R (&seed) % 5;
            for (elem_idx = 0; elem_idx < n_elems; elem_idx++) {
               bson_append_utf8 (bsons[docs_idx],
                                 rand_str (sizeof key, &seed, key),
                                 -1,
                                 rand_str (sizeof val, &seed, val),
                                 -1);

               bson_append_int32 (bsons[docs_idx],
                                  rand_str (sizeof key, &seed, key),
                                  -1,
                                  RAND_R (&seed) % INT32_MAX);
            }

            /* append the BSON document's JSON representation to "json" */
            json_tmp = bson_as_json (bsons[docs_idx], NULL);
            BSON_ASSERT (json_tmp);
            bson_string_append (json, json_tmp);
            bson_free (json_tmp);
         }

         reader = bson_json_data_reader_new (
            true /* "allow_multiple" is unused */,
            (size_t) RAND_R (&seed) % 100 /* bufsize*/);

         bson_json_data_reader_ingest (
            reader, (uint8_t *) json->str, json->len);

         for (docs_idx = 0; docs_idx < n_docs; docs_idx++) {
            bson_reinit (&bson_out);
            r = bson_json_reader_read (reader, &bson_out, &error);
            if (r == -1) {
               fprintf (stderr, "%s\n", error.message);
               abort ();
            }

            BSON_ASSERT (r);
            bson_eq_bson (&bson_out, bsons[docs_idx]);
         }

         /* finished parsing */
         BSON_ASSERT_CMPINT (
            0, ==, bson_json_reader_read (reader, &bson_out, &error));

         bson_json_reader_destroy (reader);
         bson_string_truncate (json, 0);

         for (docs_idx = 0; docs_idx < n_docs; docs_idx++) {
            bson_destroy (bsons[docs_idx]);
         }

         bson_free (bsons);
      }
   }

   bson_string_free (json, true);
   bson_destroy (&bson_out);
}

static void
_test_bson_json_read_compare (const char *json, int size, ...)
{
   bson_error_t error = {0};
   bson_json_reader_t *reader;
   va_list ap;
   int r;
   bson_t *compare;
   bson_t bson = BSON_INITIALIZER;

   reader = bson_json_data_reader_new ((size == 1), size);
   bson_json_data_reader_ingest (reader, (uint8_t *) json, strlen (json));

   va_start (ap, size);

   while ((r = bson_json_reader_read (reader, &bson, &error))) {
      if (r == -1) {
         fprintf (stderr, "%s\n", error.message);
         abort ();
      }

      compare = va_arg (ap, bson_t *);

      BSON_ASSERT (compare);

      bson_eq_bson (&bson, compare);

      bson_destroy (compare);

      bson_reinit (&bson);
   }

   va_end (ap);

   bson_json_reader_destroy (reader);
   bson_destroy (&bson);
}

static void
test_bson_json_read (void)
{
   const char *json = "{ \n\
      \"foo\" : \"bar\", \n\
      \"bar\" : 12341, \n\
      \"baz\" : 123.456, \n\
      \"map\" : { \"a\" : 1 }, \n\
      \"array\" : [ 1, 2, 3, 4 ], \n\
      \"null\" : null, \n\
      \"boolean\" : true, \n\
      \"oid\" : { \n\
        \"$oid\" : \"000000000000000000000000\" \n\
      }, \n\
      \"binary\" : { \n\
        \"$type\" : \"00\", \n\
        \"$binary\" : \"ZGVhZGJlZWY=\" \n\
      }, \n\
      \"regex\" : { \n\
        \"$regularExpression\" : { \n\
          \"pattern\": \"foo|bar\", \"options\" : \"ism\" \n\
        } \n\
      }, \n\
      \"date\" : { \n\
        \"$date\" : \"1970-01-01T00:00:10Z\" \n\
      }, \n\
      \"ref\" : { \n\
        \"$ref\" : \"foo\", \n\
        \"$id\" : {\"$oid\": \"000000000000000000000000\"} \n\
      }, \n\
      \"undefined\" : { \n\
        \"$undefined\" : true \n\
      }, \n\
      \"minkey\" : { \n\
        \"$minKey\" : 1 \n\
      }, \n\
      \"maxkey\" : { \n\
        \"$maxKey\" : 1 \n\
      }, \n\
      \"timestamp\" : { \n\
        \"$timestamp\" : { \n\
           \"t\" : 100, \n\
           \"i\" : 1000 \n\
        } \n\
      } \n\
   } { \"after\": \"b\" } { \"twice\" : true }";

   bson_oid_t oid;
   bson_t *first, *second, *third;

   bson_oid_init_from_string (&oid, "000000000000000000000000");

   first =
      BCON_NEW ("foo",
                "bar",
                "bar",
                BCON_INT32 (12341),
                "baz",
                BCON_DOUBLE (123.456),
                "map",
                "{",
                "a",
                BCON_INT32 (1),
                "}",
                "array",
                "[",
                BCON_INT32 (1),
                BCON_INT32 (2),
                BCON_INT32 (3),
                BCON_INT32 (4),
                "]",
                "null",
                BCON_NULL,
                "boolean",
                BCON_BOOL (true),
                "oid",
                BCON_OID (&oid),
                "binary",
                BCON_BIN (BSON_SUBTYPE_BINARY, (const uint8_t *) "deadbeef", 8),
                "regex",
                BCON_REGEX ("foo|bar", "ism"),
                "date",
                BCON_DATE_TIME (10000),
                "ref",
                "{",
                "$ref",
                BCON_UTF8 ("foo"),
                "$id",
                BCON_OID (&oid),
                "}",
                "undefined",
                BCON_UNDEFINED,
                "minkey",
                BCON_MINKEY,
                "maxkey",
                BCON_MAXKEY,
                "timestamp",
                BCON_TIMESTAMP (100, 1000));

   second = BCON_NEW ("after", "b");
   third = BCON_NEW ("twice", BCON_BOOL (true));

   _test_bson_json_read_compare (json, 5, first, second, third, NULL);
}

static void
test_bson_json_read_raw_utf8 (void)
{
   bson_t *bson;
   bson_iter_t iter;

   bson = bson_new_from_json (
      (const uint8_t *) "{\"" EU "\": \"" EU "\"}", -1, NULL);
   ASSERT (bson);
   ASSERT (bson_iter_init_find (&iter, bson, EU));
   ASSERT_CMPSTR (bson_iter_key (&iter), EU);
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), EU);
   ASSERT (!bson_iter_next (&iter));

   bson_destroy (bson);
}

static void
test_bson_json_read_corrupt_utf8 (void)
{
   const char *bad_key = "{ \"\x80\" : \"a\"}";
   const char *bad_value = "{ \"a\" : \"\x80\"}";
   bson_error_t error = {0};

   BSON_ASSERT (!bson_new_from_json ((uint8_t *) bad_key, -1, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_CORRUPT_JS,
                          "invalid bytes in UTF8 string");

   BSON_ASSERT (!bson_new_from_json ((uint8_t *) bad_value, -1, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_CORRUPT_JS,
                          "invalid bytes in UTF8 string");
}


static void
test_bson_json_read_decimal128 (void)
{
   const char *json = "{ \"decimal\" : { \"$numberDecimal\" : \"123.5\" }}";
   bson_decimal128_t dec;
   bson_t *doc;

   bson_decimal128_from_string ("123.5", &dec);
   doc = BCON_NEW ("decimal", BCON_DECIMAL128 (&dec));

   _test_bson_json_read_compare (json, 5, doc, NULL);
}


static void
test_bson_json_read_dbpointer (void)
{
   bson_t b;
   bson_error_t error;
   bool r;

   /* must have both $ref and $id, $id must be ObjectId */
   const char *invalid[] = {
      "{\"p\": {\"$dbPointer\": {\"$ref\": \"db.collection\"}}",
      "$dbPointer requires both $id and $ref",

      "{\"p\": {\"$dbPointer\": {\"$ref\": \"db.collection\", \"$id\": 1}}",
      "$dbPointer.$id must be like {\"$oid\": ...\"}",

      "{\"p\": {\"$dbPointer\": {\"$id\": {"
      "\"$oid\": \"57e193d7a9cc81b4027498b1\"}}}}",
      "$dbPointer requires both $id and $ref",

      "{\"p\": {\"$dbPointer\": {}}}",
      "Empty $dbPointer",

      NULL};

   const char **p;

   for (p = invalid; *p; p += 2) {
      r = bson_init_from_json (&b, *p, -1, &error);
      BSON_ASSERT (!r);
      ASSERT_ERROR_CONTAINS (
         error, BSON_ERROR_JSON, BSON_JSON_ERROR_READ_INVALID_PARAM, *(p + 1));

      bson_destroy (&b);
   }
}


static void
test_bson_json_read_legacy_regex (void)
{
   bson_t b;
   bson_error_t error;
   bool r;
   const char *pattern;
   const char *flags;

   r = bson_init_from_json (
      &b, "{\"a\": {\"$regex\": \"abc\", \"$options\": \"ix\"}}", -1, &error);
   ASSERT_OR_PRINT (r, error);
   BCON_EXTRACT (&b, "a", BCONE_REGEX (pattern, flags));
   ASSERT_CMPSTR (pattern, "abc");
   ASSERT_CMPSTR (flags, "ix");

   bson_destroy (&b);

   r = bson_init_from_json (&b, "{\"a\": {\"$regex\": \"abc\"}}", -1, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Missing \"$options\" after \"$regex\"");

   r = bson_init_from_json (&b, "{\"a\": {\"$options\": \"ix\"}}", -1, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Missing \"$regex\" after \"$options\"");
}


static void
test_bson_json_read_regex_options_order (void)
{
   bson_t b;
   bson_error_t error;
   bool r;
   const char *pattern;
   const char *flags;

   r = bson_init_from_json (
      &b, "{\"a\": {\"$regex\": \"\", \"$options\": \"ism\"}}", -1, &error);
   ASSERT_OR_PRINT (r, error);
   BCON_EXTRACT (&b, "a", BCONE_REGEX (pattern, flags));
   ASSERT_CMPSTR (flags, "ims");

   bson_destroy (&b);

   r = bson_init_from_json (
      &b, "{\"a\": {\"$regex\": \"\", \"$options\": \"misl\"}}", -1, &error);
   ASSERT_OR_PRINT (r, error);
   BCON_EXTRACT (&b, "a", BCONE_REGEX (pattern, flags));
   ASSERT_CMPSTR (flags, "ilms");

   bson_destroy (&b);
}


static void
test_bson_json_read_binary (void)
{
   bson_error_t error;
   bson_t b;
   bool r;
   bson_subtype_t subtype;
   uint32_t len;
   const uint8_t *binary;

   r = bson_init_from_json (
      &b,
      "{\"b\": {\"$binary\": {\"base64\": \"Zm9v\", \"subType\": \"05\"}}}",
      -1,
      &error);
   ASSERT_OR_PRINT (r, error);

   BCON_EXTRACT (&b, "b", BCONE_BIN (subtype, binary, len));
   ASSERT_CMPINT ((int) subtype, ==, 5);
   ASSERT_CMPUINT32 (len, ==, (uint32_t) 3);
   ASSERT_CMPSTR ((const char *) binary, "foo");

   bson_destroy (&b);

   r = bson_init_from_json (
      &b,
      "{\"b\": {\"$binary\": {\"subType\": \"05\", \"base64\": \"Zm9v\"}}}",
      -1,
      &error);
   ASSERT_OR_PRINT (r, error);
   BCON_EXTRACT (&b, "b", BCONE_BIN (subtype, binary, len));
   ASSERT_CMPINT ((int) subtype, ==, 5);
   ASSERT_CMPUINT32 (len, ==, (uint32_t) 3);
   ASSERT_CMPSTR ((const char *) binary, "foo");

   bson_destroy (&b);

   /* no base64 */
   r = bson_init_from_json (
      &b, "{\"b\": {\"$binary\": {\"subType\": \"5\"}}}", -1, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Missing \"base64\" after \"subType\"");

   /* no subType */
   r = bson_init_from_json (
      &b, "{\"b\": {\"$binary\": {\"base64\": \"Zm9v\"}}}", -1, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Missing \"subType\" after \"base64\"");
}


static void
test_bson_json_read_legacy_binary (void)
{
   const char *jsons[] = {"{\"x\": {\"$binary\": \"Zm9v\", \"$type\": \"05\"}}",
                          "{\"x\": {\"$type\": \"05\", \"$binary\": \"Zm9v\"}}",
                          NULL};

   const char **json;
   bson_error_t error;
   bson_t b;
   bool r;
   bson_subtype_t subtype;
   uint32_t len;
   const uint8_t *binary;

   for (json = jsons; *json; json++) {
      r = bson_init_from_json (&b, *json, -1, &error);

      ASSERT_OR_PRINT (r, error);
      BCON_EXTRACT (&b, "x", BCONE_BIN (subtype, binary, len));
      ASSERT_CMPINT ((int) subtype, ==, 5);
      ASSERT_CMPUINT32 (len, ==, (uint32_t) 3);
      ASSERT_CMPSTR ((const char *) binary, "foo");

      bson_destroy (&b);
   }
}


static void
test_json_reader_new_from_file (void)
{
   const char *path = JSON_DIR "/test.json";
   const char *bar;
   const bson_oid_t *oid;
   bson_oid_t oid_expected;
   int32_t one;
   bson_t bson = BSON_INITIALIZER;
   bson_json_reader_t *reader;
   bson_error_t error;

   reader = bson_json_reader_new_from_file (path, &error);
   BSON_ASSERT (reader);

   /* read two documents */
   ASSERT_CMPINT (1, ==, bson_json_reader_read (reader, &bson, &error));

   BCON_EXTRACT (&bson, "foo", BCONE_UTF8 (bar), "a", BCONE_INT32 (one));
   ASSERT_CMPSTR ("bar", bar);
   ASSERT_CMPINT (1, ==, one);

   bson_reinit (&bson);
   ASSERT_CMPINT (1, ==, bson_json_reader_read (reader, &bson, &error));

   BCON_EXTRACT (&bson, "_id", BCONE_OID (oid));
   bson_oid_init_from_string (&oid_expected, "aabbccddeeff001122334455");
   BSON_ASSERT (bson_oid_equal (&oid_expected, oid));

   bson_destroy (&bson);
   bson_json_reader_destroy (reader);
}

static void
test_json_reader_new_from_bad_path (void)
{
   const char *bad_path = JSON_DIR "/does-not-exist";
   bson_json_reader_t *reader;
   bson_error_t error;

   reader = bson_json_reader_new_from_file (bad_path, &error);
   BSON_ASSERT (!reader);
   ASSERT_CMPINT (BSON_ERROR_READER, ==, error.domain);
   ASSERT_CMPINT (BSON_ERROR_READER_BADFD, ==, error.code);
}

static void
test_bson_json_error (const char *json, int domain, bson_json_error_code_t code)
{
   bson_error_t error;
   bson_t *bson;

   bson = bson_new_from_json ((const uint8_t *) json, strlen (json), &error);

   BSON_ASSERT (!bson);
   BSON_ASSERT (error.domain == domain);
   BSON_ASSERT (error.code == code);
}

static void
test_bson_json_read_empty (void)
{
   bson_error_t error;
   bson_t *bson_ptr;
   bson_t bson;

   bson_ptr = bson_new_from_json ((uint8_t *) "", 0, &error);
   BSON_ASSERT (!bson_ptr);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Empty JSON string");

   memset (&error, 0, sizeof error);
   bson_init_from_json (&bson, "", 0, &error);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Empty JSON string");
}

static void
test_bson_json_read_missing_complex (void)
{
   const char *json = "{ \n\
      \"foo\" : { \n\
         \"$options\" : \"ism\"\n\
      }\n\
   }";

   test_bson_json_error (
      json, BSON_ERROR_JSON, BSON_JSON_ERROR_READ_INVALID_PARAM);
}

static void
test_bson_json_read_invalid_binary (void)
{
   bson_error_t error;
   const char *json =
      "{ "
      " \"bin\" : { \"$binary\" : \"invalid\", \"$type\" : \"80\" } }";
   bson_t b;
   bool r;

   r = bson_init_from_json (&b, json, -1, &error);
   BSON_ASSERT (!r);

   bson_destroy (&b);
}

static void
test_bson_json_read_invalid_json (void)
{
   const char *json = "{ \n\
      \"foo\" : { \n\
   }";
   bson_t *b;

   test_bson_json_error (
      json, BSON_ERROR_JSON, BSON_JSON_ERROR_READ_CORRUPT_JS);

   b = bson_new_from_json ((uint8_t *) "1", 1, NULL);
   BSON_ASSERT (!b);

   b = bson_new_from_json ((uint8_t *) "*", 1, NULL);
   BSON_ASSERT (!b);

   b = bson_new_from_json ((uint8_t *) "", 0, NULL);
   BSON_ASSERT (!b);

   b = bson_new_from_json ((uint8_t *) "asdfasdf", -1, NULL);
   BSON_ASSERT (!b);

   b = bson_new_from_json ((uint8_t *) "{\"a\":*}", -1, NULL);
   BSON_ASSERT (!b);
}

static ssize_t
test_bson_json_read_bad_cb_helper (void *_ctx, uint8_t *buf, size_t len)
{
   return -1;
}

static void
test_bson_json_read_bad_cb (void)
{
   bson_error_t error;
   bson_json_reader_t *reader;
   int r;
   bson_t bson = BSON_INITIALIZER;

   reader = bson_json_reader_new (
      NULL, &test_bson_json_read_bad_cb_helper, NULL, false, 0);

   r = bson_json_reader_read (reader, &bson, &error);

   BSON_ASSERT (r == -1);
   BSON_ASSERT (error.domain == BSON_ERROR_JSON);
   BSON_ASSERT (error.code == BSON_JSON_ERROR_READ_CB_FAILURE);

   bson_json_reader_destroy (reader);
   bson_destroy (&bson);
}

static ssize_t
test_bson_json_read_invalid_helper (void *ctx, uint8_t *buf, size_t len)
{
   BSON_ASSERT (len);
   *buf = 0x80; /* no UTF-8 sequence can start with 0x80 */
   return 1;
}

static void
test_bson_json_read_invalid (void)
{
   bson_error_t error;
   bson_json_reader_t *reader;
   int r;
   bson_t bson = BSON_INITIALIZER;

   reader = bson_json_reader_new (
      NULL, test_bson_json_read_invalid_helper, NULL, false, 0);

   r = bson_json_reader_read (reader, &bson, &error);

   BSON_ASSERT (r == -1);
   BSON_ASSERT (error.domain == BSON_ERROR_JSON);
   BSON_ASSERT (error.code == BSON_JSON_ERROR_READ_CORRUPT_JS);

   bson_json_reader_destroy (reader);
   bson_destroy (&bson);
}

static void
test_bson_json_number_long (void)
{
   bson_error_t error;
   bson_iter_t iter;
   bson_t b;
   bool r;

   r = bson_init_from_json (
      &b, "{\"key\": {\"$numberLong\": \"4611686018427387904\"}}", -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);
   BSON_ASSERT (bson_iter_init (&iter, &b));
   BSON_ASSERT (bson_iter_find (&iter, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   BSON_ASSERT (bson_iter_int64 (&iter) == 4611686018427387904LL);
   bson_destroy (&b);

   BSON_ASSERT (!bson_init_from_json (
      &b, "{\"key\": {\"$numberLong\": \"461168601abcd\"}}", -1, &error));
   BSON_ASSERT (!bson_init_from_json (
      &b, "{\"key\": {\"$numberLong\": \"461168601abcd\"}}", -1, &error));

   /* INT64_MAX */
   r = bson_init_from_json (
      &b, "{\"x\": {\"$numberLong\": \"9223372036854775807\"}}", -1, &error);
   ASSERT_OR_PRINT (r, error);
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT_CMPINT64 (bson_iter_int64 (&iter), ==, (int64_t) INT64_MAX);

   /* INT64_MIN */
   r = bson_init_from_json (
      &b, "{\"x\": {\"$numberLong\": \"-9223372036854775808\"}}", -1, &error);
   ASSERT_OR_PRINT (r, error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT_CMPINT64 (bson_iter_int64 (&iter), ==, (int64_t) INT64_MIN);
   bson_destroy (&b);

   /* INT64_MAX + 1 */
   r = bson_init_from_json (
      &b, "{\"x\": {\"$numberLong\": \"9223372036854775808\"}}", -1, &error);

   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"9223372036854775808\" is out of range");

   /* INT64_MIN - 1 */
   r = bson_init_from_json (
      &b, "{\"x\": {\"$numberLong\": \"-9223372036854775809\"}}", -1, &error);

   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"-9223372036854775809\" is out of range");

   r = bson_init_from_json (
      &b, "{\"x\": {\"$numberLong\": \"10000000000000000000\"}}", -1, &error);

   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"10000000000000000000\" is out of range");

   /* INT64_MIN - 2 */
   r = bson_init_from_json (&b, "{\"x\": -10000000000000000000}", -1, &error);

   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"-10000000000000000000\" is out of range");
}

static void
test_bson_json_number_long_zero (void)
{
   bson_error_t error;
   bson_iter_t iter;
   const char *json = "{ \"key\": { \"$numberLong\": \"0\" }}";
   bson_t b;
   bool r;

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);
   BSON_ASSERT (bson_iter_init (&iter, &b));
   BSON_ASSERT (bson_iter_find (&iter, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   BSON_ASSERT (bson_iter_int64 (&iter) == 0);
   bson_destroy (&b);
}

static void
test_bson_json_code (void)
{
   const char *json_code = "{\"a\": {\"$code\": \"b\"}}";
   bson_t *bson_code = BCON_NEW ("a", BCON_CODE ("b"));

   const char *json_code_w_nulls = "{\"a\": {\"$code\": \"b\\u0000c\\u0000\"}}";
   bson_t *bson_code_w_nulls = BCON_NEW ("a", BCON_CODE ("b\0c\0"));

   const char *json_code_w_scope = "{\"a\": {\"$code\": \"b\", "
                                   "         \"$scope\": {\"var\": 1}}}";
   bson_t *scope1 = BCON_NEW ("var", BCON_INT32 (1));
   bson_t *bson_code_w_scope = BCON_NEW ("a", BCON_CODEWSCOPE ("b", scope1));

   const char *json_code_w_scope_special =
      "{\"a\": {\"$code\": \"b\", "
      "         \"$scope\": {\"var2\": {\"$numberLong\": \"2\"}}}}";
   bson_t *scope2 = BCON_NEW ("var2", BCON_INT64 (2));
   bson_t *bson_code_w_scope_special =
      BCON_NEW ("a", BCON_CODEWSCOPE ("b", scope2));

   const char *json_2_codes_w_scope =
      "{\"a\": {\"$code\": \"b\", "
      "         \"$scope\": {\"var\": 1}},"
      " \"c\": {\"$code\": \"d\", "
      "         \"$scope\": {\"var2\": {\"$numberLong\": \"2\"}}}}";
   bson_t *bson_2_codes_w_scope = BCON_NEW (
      "a", BCON_CODEWSCOPE ("b", scope1), "c", BCON_CODEWSCOPE ("d", scope2));

   const char *json_code_then_regular =
      "{\"a\": {\"$code\": \"b\", "
      "         \"$scope\": {\"var\": 1}},"
      " \"c\": {\"key1\": \"value\", "
      "         \"subdoc\": {\"key2\": \"value2\"}}}";

   bson_t *bson_code_then_regular = BCON_NEW ("a",
                                              BCON_CODEWSCOPE ("b", scope1),
                                              "c",
                                              "{",
                                              "key1",
                                              BCON_UTF8 ("value"),
                                              "subdoc",
                                              "{",
                                              "key2",
                                              BCON_UTF8 ("value2"),
                                              "}",
                                              "}");

   const char *json_code_w_scope_reverse = "{\"a\": {\"$scope\": {\"var\": 1}, "
                                           "         \"$code\": \"b\"}}";
   bson_t *bson_code_w_scope_reverse =
      BCON_NEW ("a", BCON_CODEWSCOPE ("b", scope1));

   const char *json_code_w_scope_nest = "{\"a\": {\"$code\": \"b\", "
                                        "         \"$scope\": {\"var\": {}}}}";
   bson_t *scope3 = BCON_NEW ("var", "{", "}");
   bson_t *bson_code_w_scope_nest =
      BCON_NEW ("a", BCON_CODEWSCOPE ("b", scope3));

   const char *json_code_w_scope_nest_deep =
      "{\"a\": {\"$code\": \"b\", "
      "         \"$scope\": {\"arr\": [1, 2], \"d\": {},"
      "                      \"n\": {\"$numberLong\": \"1\"},"
      "                      \"d2\": {\"x\": 1, \"d3\": {\"z\": 3}}}}}";

   bson_t *scope_deep = BCON_NEW ("arr",
                                  "[",
                                  BCON_INT32 (1),
                                  BCON_INT32 (2),
                                  "]",
                                  "d",
                                  "{",
                                  "}",
                                  "n",
                                  BCON_INT64 (1),
                                  "d2",
                                  "{",
                                  "x",
                                  BCON_INT32 (1),
                                  "d3",
                                  "{",
                                  "z",
                                  BCON_INT32 (3),
                                  "}",
                                  "}");

   bson_t *bson_code_w_scope_nest_deep =
      BCON_NEW ("a", BCON_CODEWSCOPE ("b", scope_deep));

   const char *json_code_w_empty_scope = "{\"a\": {\"$code\": \"b\", "
                                         "         \"$scope\": {}}}";
   bson_t *empty = bson_new ();
   bson_t *bson_code_w_empty_scope =
      BCON_NEW ("a", BCON_CODEWSCOPE ("b", empty));

   const char *json_code_in_scope =
      "{\"a\": {\"$code\": \"b\", "
      "         \"$scope\": {\"x\": {\"$code\": \"c\"}}}}";

   bson_t *code_in_scope = BCON_NEW ("x", "{", "$code", BCON_UTF8 ("c"), "}");
   bson_t *bson_code_in_scope =
      BCON_NEW ("a", BCON_CODEWSCOPE ("b", code_in_scope));

   typedef struct {
      const char *json;
      bson_t *expected_bson;
   } code_test_t;

   code_test_t tests[] = {
      {json_code, bson_code},
      {json_code_w_nulls, bson_code_w_nulls},
      {json_code_w_scope, bson_code_w_scope},
      {json_code_w_scope_special, bson_code_w_scope_special},
      {json_code_then_regular, bson_code_then_regular},
      {json_2_codes_w_scope, bson_2_codes_w_scope},
      {json_code_w_scope_reverse, bson_code_w_scope_reverse},
      {json_code_w_scope_nest, bson_code_w_scope_nest},
      {json_code_w_scope_nest_deep, bson_code_w_scope_nest_deep},
      {json_code_w_empty_scope, bson_code_w_empty_scope},
      {json_code_in_scope, bson_code_in_scope},
   };

   int n_tests = sizeof (tests) / sizeof (code_test_t);
   int i;
   bson_t b;
   bool r;
   bson_error_t error;

   for (i = 0; i < n_tests; i++) {
      r = bson_init_from_json (&b, tests[i].json, -1, &error);
      if (!r) {
         fprintf (stderr, "%s\n", error.message);
      }

      BSON_ASSERT (r);
      bson_eq_bson (&b, tests[i].expected_bson);
      bson_destroy (&b);
   }

   for (i = 0; i < n_tests; i++) {
      bson_destroy (tests[i].expected_bson);
   }

   bson_destroy (scope1);
   bson_destroy (scope2);
   bson_destroy (scope3);
   bson_destroy (scope_deep);
   bson_destroy (code_in_scope);
   bson_destroy (empty);
}

static void
test_bson_json_code_errors (void)
{
   bson_error_t error;
   bson_t b;
   bool r;
   size_t i;

   typedef struct {
      const char *json;
      const char *error_message;
   } code_error_test_t;

   code_error_test_t tests[] = {
      {"{\"a\": {\"$scope\": {}}", "Missing $code after $scope"},
      {"{\"a\": {\"$scope\": {}, \"$x\": 1}", "Invalid key \"$x\""},
      {"{\"a\": {\"$scope\": {\"a\": 1}}", "Missing $code after $scope"},
      {"{\"a\": {\"$code\": \"\", \"$scope\": \"a\"}}",
       "Invalid read of \"a\""},
      {"{\"a\": {\"$code\": \"\", \"$scope\": 1}}",
       "Unexpected integer 1 in state \"IN_BSON_TYPE_SCOPE_STARTMAP\""},
      {"{\"a\": {\"$code\": \"\", \"$scope\": []}}", "Invalid read of \"[\""},
      {"{\"a\": {\"$code\": \"\", \"x\": 1}}",
       "Invalid key \"x\".  Looking for values for type \"code\""},
      {"{\"a\": {\"$code\": \"\", \"$x\": 1}}", "Invalid key \"$x\""},
      {"{\"a\": {\"$code\": \"\", \"$numberLong\": \"1\"}}",
       "Invalid key \"$numberLong\""},
   };

   for (i = 0; i < sizeof (tests) / (sizeof (code_error_test_t)); i++) {
      r = bson_init_from_json (&b, tests[i].json, -1, &error);
      BSON_ASSERT (!r);
      ASSERT_ERROR_CONTAINS (error,
                             BSON_ERROR_JSON,
                             BSON_JSON_ERROR_READ_INVALID_PARAM,
                             tests[i].error_message);
   }
}

static const bson_oid_t *
oid_zero (void)
{
   static bool initialized = false;
   static bson_oid_t oid;

   if (!initialized) {
      bson_oid_init_from_string (&oid, "000000000000000000000000");
      initialized = true;
   }

   return &oid;
}

static void
test_bson_json_dbref (void)
{
   bson_error_t error;

   const char *json_with_objectid =
      "{ \"key\": {"
      "\"$ref\": \"collection\","
      "\"$id\": {\"$oid\": \"000000000000000000000000\"}}}";

   bson_t *bson_with_objectid = BCON_NEW ("key",
                                          "{",
                                          "$ref",
                                          BCON_UTF8 ("collection"),
                                          "$id",
                                          BCON_OID (oid_zero ()),
                                          "}");

   const char *json_with_int_id = "{ \"key\": {"
                                  "\"$ref\": \"collection\","
                                  "\"$id\": 1}}";

   bson_t *bson_with_int_id = BCON_NEW (
      "key", "{", "$ref", BCON_UTF8 ("collection"), "$id", BCON_INT32 (1), "}");

   const char *json_with_subdoc_id = "{ \"key\": {"
                                     "\"$ref\": \"collection\","
                                     "\"$id\": {\"a\": 1}}}";

   bson_t *bson_with_subdoc_id = BCON_NEW ("key",
                                           "{",
                                           "$ref",
                                           BCON_UTF8 ("collection"),
                                           "$id",
                                           "{",
                                           "a",
                                           BCON_INT32 (1),
                                           "}",
                                           "}");

   const char *json_with_metadata = "{ \"key\": {"
                                    "\"$ref\": \"collection\","
                                    "\"$id\": 1,"
                                    "\"meta\": true}}";

   bson_t *bson_with_metadata = BCON_NEW ("key",
                                          "{",
                                          "$ref",
                                          BCON_UTF8 ("collection"),
                                          "$id",
                                          BCON_INT32 (1),
                                          "meta",
                                          BCON_BOOL (true),
                                          "}");

   bson_t b;
   bool r;

   typedef struct {
      const char *json;
      bson_t *expected_bson;
   } dbref_test_t;

   dbref_test_t tests[] = {
      {json_with_objectid, bson_with_objectid},
      {json_with_int_id, bson_with_int_id},
      {json_with_subdoc_id, bson_with_subdoc_id},
      {json_with_metadata, bson_with_metadata},
   };

   int n_tests = sizeof (tests) / sizeof (dbref_test_t);
   int i;

   for (i = 0; i < n_tests; i++) {
      r = bson_init_from_json (&b, tests[i].json, -1, &error);
      if (!r) {
         fprintf (stderr, "%s\n", error.message);
      }

      BSON_ASSERT (r);
      bson_eq_bson (&b, tests[i].expected_bson);
      bson_destroy (&b);
   }

   for (i = 0; i < n_tests; i++) {
      bson_destroy (tests[i].expected_bson);
   }
}

static void
test_bson_json_uescape (void)
{
   bson_error_t error;
   bson_t b;
   bool r;

   const char *euro = "{ \"euro\": \"\\u20AC\"}";
   bson_t *bson_euro = BCON_NEW ("euro", BCON_UTF8 ("\xE2\x82\xAC"));

   const char *crlf = "{ \"crlf\": \"\\r\\n\"}";
   bson_t *bson_crlf = BCON_NEW ("crlf", BCON_UTF8 ("\r\n"));

   const char *quote = "{ \"quote\": \"\\\"\"}";
   bson_t *bson_quote = BCON_NEW ("quote", BCON_UTF8 ("\""));

   const char *backslash = "{ \"backslash\": \"\\\\\"}";
   bson_t *bson_backslash = BCON_NEW ("backslash", BCON_UTF8 ("\\"));

   const char *empty = "{ \"\": \"\"}";
   bson_t *bson_empty = BCON_NEW ("", BCON_UTF8 (""));

   const char *escapes = "{ \"escapes\": \"\\f\\b\\t\"}";
   bson_t *bson_escapes = BCON_NEW ("escapes", BCON_UTF8 ("\f\b\t"));

   const char *nil_byte = "{ \"nil\": \"\\u0000\"}";
   bson_t *bson_nil_byte = bson_new (); /* we'll append "\0" to it, below */

   typedef struct {
      const char *json;
      bson_t *expected_bson;
   } uencode_test_t;

   uencode_test_t tests[] = {
      {euro, bson_euro},
      {crlf, bson_crlf},
      {quote, bson_quote},
      {backslash, bson_backslash},
      {empty, bson_empty},
      {escapes, bson_escapes},
      {nil_byte, bson_nil_byte},
   };

   int n_tests = sizeof (tests) / sizeof (uencode_test_t);
   int i;

   bson_append_utf8 (bson_nil_byte, "nil", -1, "\0", 1);

   for (i = 0; i < n_tests; i++) {
      r = bson_init_from_json (&b, tests[i].json, -1, &error);

      if (!r) {
         fprintf (stderr, "%s\n", error.message);
      }

      BSON_ASSERT (r);
      bson_eq_bson (&b, tests[i].expected_bson);
      bson_destroy (&b);
   }

   for (i = 0; i < n_tests; i++) {
      bson_destroy (tests[i].expected_bson);
   }
}

static void
test_bson_json_uescape_key (void)
{
   bson_error_t error;
   bson_t b;
   bool r;

   bson_t *bson_euro = BCON_NEW ("\xE2\x82\xAC", BCON_UTF8 ("euro"));

   r = bson_init_from_json (&b, "{ \"\\u20AC\": \"euro\"}", -1, &error);
   BSON_ASSERT (r);
   bson_eq_bson (&b, bson_euro);

   bson_destroy (&b);
   bson_destroy (bson_euro);
}

static void
test_bson_json_uescape_bad (void)
{
   bson_error_t error;
   bson_t b;
   bool r;

   r = bson_init_from_json (&b, "{ \"bad\": \"\\u1\"}", -1, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_CORRUPT_JS,
                          "UESCAPE_TOOSHORT");

   bson_destroy (&b);
}


static void
test_bson_json_int32 (void)
{
   bson_t b;
   bson_iter_t iter;
   bson_error_t error;

   /* INT32_MAX */
   ASSERT_OR_PRINT (
      bson_init_from_json (&b, "{ \"x\": 2147483647 }", -1, &error), error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   ASSERT_CMPINT32 (bson_iter_int32 (&iter), ==, (int32_t) 2147483647LL);
   bson_destroy (&b);

   /* INT32_MIN */
   ASSERT_OR_PRINT (
      bson_init_from_json (&b, "{ \"x\": -2147483648 }", -1, &error), error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   ASSERT_CMPINT32 (bson_iter_int32 (&iter), ==, (int32_t) -2147483648LL);
   bson_destroy (&b);

   /* INT32_MAX + 1 */
   ASSERT_OR_PRINT (
      bson_init_from_json (&b, "{ \"x\": 2147483648 }", -1, &error), error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT_CMPINT64 (bson_iter_int64 (&iter), ==, (int64_t) 2147483648LL);
   bson_destroy (&b);

   /* INT32_MIN - 1 */
   ASSERT_OR_PRINT (
      bson_init_from_json (&b, "{ \"x\": -2147483649 }", -1, &error), error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT_CMPINT64 (bson_iter_int64 (&iter), ==, (int64_t) -2147483649LL);
   bson_destroy (&b);
}


static void
test_bson_json_int64 (void)
{
   bson_t b;
   bson_iter_t iter;
   bson_error_t error;

   /* INT64_MAX */
   ASSERT_OR_PRINT (
      bson_init_from_json (&b, "{ \"x\": 9223372036854775807 }", -1, &error),
      error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT_CMPINT64 (
      bson_iter_int64 (&iter), ==, (int64_t) 9223372036854775807LL);
   bson_destroy (&b);

   /* INT64_MIN */
   ASSERT_OR_PRINT (
      bson_init_from_json (&b, "{ \"x\": -9223372036854775808 }", -1, &error),
      error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT_CMPINT64 (bson_iter_int64 (&iter), ==, (int64_t) INT64_MIN);
   bson_destroy (&b);

   /* INT64_MAX + 1 */
   BSON_ASSERT (
      !bson_init_from_json (&b, "{ \"x\": 9223372036854775808 }", -1, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"9223372036854775808\" is out of range");

   /* INT64_MIN - 1 */
   BSON_ASSERT (
      !bson_init_from_json (&b, "{ \"x\": -9223372036854775809 }", -1, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"-9223372036854775809\" is out of range");

   BSON_ASSERT (
      !bson_init_from_json (&b, "{ \"x\": 10000000000000000000 }", -1, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"10000000000000000000\" is out of range");

   /* INT64_MIN - 2 */
   BSON_ASSERT (!bson_init_from_json (
      &b, "{ \"x\": -10000000000000000000 }", -1, &error));

   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_JSON,
                          BSON_JSON_ERROR_READ_INVALID_PARAM,
                          "Number \"-10000000000000000000\" is out of range");
}


static void
test_bson_json_double (void)
{
   bson_t b;
   bson_error_t error;
   bson_iter_t iter;

   ASSERT_OR_PRINT (bson_init_from_json (&b, "{ \"x\": 1 }", -1, &error),
                    error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   ASSERT_CMPINT32 (bson_iter_int32 (&iter), ==, (int32_t) 1);
   bson_destroy (&b);

   ASSERT_OR_PRINT (
      bson_init_from_json (&b, "{ \"x\": 4294967296 }", -1, &error), error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT_CMPINT64 (bson_iter_int64 (&iter), ==, (int64_t) 4294967296);
   bson_destroy (&b);

   ASSERT_OR_PRINT (bson_init_from_json (&b, "{ \"x\": 1.0 }", -1, &error),
                    error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOUBLE (&iter));
   ASSERT_CMPDOUBLE (bson_iter_double (&iter), ==, 1.0);
   bson_destroy (&b);

   ASSERT_OR_PRINT (bson_init_from_json (&b, "{ \"x\": 0.0 }", -1, &error),
                    error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOUBLE (&iter));
   ASSERT_CMPDOUBLE (bson_iter_double (&iter), ==, 0.0);
   bson_destroy (&b);

   ASSERT_OR_PRINT (bson_init_from_json (&b, "{ \"x\": -0.0 }", -1, &error),
                    error);

   BSON_ASSERT (bson_iter_init_find (&iter, &b, "x"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOUBLE (&iter));
   ASSERT_CMPDOUBLE (bson_iter_double (&iter), ==, 0.0);

/* check that "x" is -0.0. signbit not available on Solaris or VS 2010 */
#if !defined(__sun) && (!defined(_MSC_VER) || (_MSC_VER >= 1800))
   BSON_ASSERT (signbit (bson_iter_double (&iter)));
#endif

   bson_destroy (&b);
}


static void
test_bson_json_double_overflow (void)
{
   const char *nums[] = {"2e400", "-2e400", NULL};
   const char **p;
   char *j;
   bson_error_t error;
   bson_t b;

   for (p = nums; *p; p++) {
      j = bson_strdup_printf ("{ \"d\" : %s }", *p);
      BSON_ASSERT (!bson_init_from_json (&b, j, -1, &error));
      ASSERT_ERROR_CONTAINS (error,
                             BSON_ERROR_JSON,
                             BSON_JSON_ERROR_READ_INVALID_PARAM,
                             "out of range");

      bson_free (j);

      /* same test with canonical Extended JSON */
      j = bson_strdup_printf ("{ \"d\" : { \"$numberDouble\" : \"%s\" } }", *p);
      BSON_ASSERT (!bson_init_from_json (&b, j, -1, &error));
      ASSERT_ERROR_CONTAINS (error,
                             BSON_ERROR_JSON,
                             BSON_JSON_ERROR_READ_INVALID_PARAM,
                             "out of range");

      bson_free (j);
   }
}


static void
test_bson_json_nan (void)
{
   bson_error_t error;
   bson_t b;
   double d;

   /* should parse any capitalization of NaN */
   const char *jsons[] = {"{ \"d\": NaN }",
                          "{ \"d\": nAn }",
                          "{ \"d\": {\"$numberDouble\": \"NaN\" } }",
                          "{ \"d\": {\"$numberDouble\": \"nAn\" } }",
                          NULL};

   /* test our patch to JSONSL that updates its state while parsing "n..." */
   const char *bad[] = {"{ \"d\": NaNn }",
                        "{ \"d\": nul }",
                        "{ \"d\": nulll }",
                        "{ \"d\": nulll }",
                        "{ \"d\": foo }",
                        "{ \"d\": NULL }",
                        "{ \"d\": nall }",
                        NULL};

   const char *partial[] = {"{ \"d\": nu", "{ \"d\": na", "{ \"d\": n", NULL};
   const char **j;

   for (j = jsons; *j; j++) {
      BSON_ASSERT (bson_init_from_json (&b, *j, -1, &error));
      BSON_ASSERT (BCON_EXTRACT (&b, "d", BCONE_DOUBLE (d)));
      BSON_ASSERT (d != d);
      bson_destroy (&b);
   }

   for (j = bad; *j; j++) {
      BSON_ASSERT (!bson_init_from_json (&b, *j, -1, &error));
      ASSERT_ERROR_CONTAINS (error,
                             BSON_ERROR_JSON,
                             BSON_JSON_ERROR_READ_CORRUPT_JS,
                             "Got parse error at");
   }

   for (j = partial; *j; j++) {
      BSON_ASSERT (!bson_init_from_json (&b, *j, -1, &error));
      ASSERT_ERROR_CONTAINS (error,
                             BSON_ERROR_JSON,
                             BSON_JSON_ERROR_READ_CORRUPT_JS,
                             "Incomplete JSON");
   }
}

static void
test_bson_json_infinity (void)
{
   bson_error_t error;
   bson_t b;
   double d;

   /* should parse any capitalization of Infinity */
   const char *infs[] = {"{ \"d\": Infinity }",
                         "{ \"d\": infinity }",
                         "{ \"d\": inFINIty }",
                         NULL};

   const char *negs[] = {"{ \"d\": -Infinity }",
                         "{ \"d\": -infinity }",
                         "{ \"d\": -inFINIty }",
                         NULL};

   const char *bad[] = {"{ \"d\": Infinityy }",
                        "{ \"d\": Infinit }",
                        "{ \"d\": -Infinityy }",
                        "{ \"d\": -Infinit }",
                        "{ \"d\": infinityy }",
                        "{ \"d\": infinit }",
                        "{ \"d\": -infinityy }",
                        "{ \"d\": -infinit }",
                        NULL};

   const char *partial[] = {"{ \"d\": In", "{ \"d\": I", "{ \"d\": i", NULL};
   const char **j;

   for (j = infs; *j; j++) {
      BSON_ASSERT (bson_init_from_json (&b, *j, -1, &error));
      BSON_ASSERT (BCON_EXTRACT (&b, "d", BCONE_DOUBLE (d)));
      /* Infinite */
      BSON_ASSERT (d == d && ((d - d) != (d - d)));
      bson_destroy (&b);
   }

   for (j = negs; *j; j++) {
      BSON_ASSERT (bson_init_from_json (&b, *j, -1, &error));
      BSON_ASSERT (BCON_EXTRACT (&b, "d", BCONE_DOUBLE (d)));
      /* Infinite */
      BSON_ASSERT (d == d && ((d - d) != (d - d)));
      BSON_ASSERT (d < 0);
      bson_destroy (&b);
   }

   for (j = bad; *j; j++) {
      BSON_ASSERT (!bson_init_from_json (&b, *j, -1, &error));
      ASSERT_ERROR_CONTAINS (error,
                             BSON_ERROR_JSON,
                             BSON_JSON_ERROR_READ_CORRUPT_JS,
                             "Got parse error at");
   }

   for (j = partial; *j; j++) {
      BSON_ASSERT (!bson_init_from_json (&b, *j, -1, &error));
      ASSERT_ERROR_CONTAINS (error,
                             BSON_ERROR_JSON,
                             BSON_JSON_ERROR_READ_CORRUPT_JS,
                             "Incomplete JSON");
   }
}


static void
test_bson_json_null (void)
{
   bson_error_t error;
   bson_t b;

   const char *json = "{ \"x\": null }";
   BSON_ASSERT (bson_init_from_json (&b, json, -1, &error));
   BSON_ASSERT (BCON_EXTRACT (&b, "x", BCONE_NULL));
   bson_destroy (&b);
}


static void
test_bson_json_empty_final_object (void)
{
   const char *json = "{\"a\": {\"b\": {}}}";
   bson_t *bson = BCON_NEW ("a", "{", "b", "{", "}", "}");
   bson_t b = BSON_INITIALIZER;
   bool r;
   bson_error_t error;

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r) {
      fprintf (stderr, "%s\n", error.message);
   }

   BSON_ASSERT (r);
   bson_eq_bson (&b, bson);

   bson_destroy (&b);
   bson_destroy (bson);
}

static void
test_bson_json_number_decimal (void)
{
   bson_error_t error;
   bson_iter_t iter;
   bson_decimal128_t decimal128;
   const char *json = "{ \"key\" : { \"$numberDecimal\": \"11\" }}";
   bson_t b;
   bool r;

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);
   BSON_ASSERT (bson_iter_init (&iter, &b));
   BSON_ASSERT (bson_iter_find (&iter, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_DECIMAL128 (&iter));
   bson_iter_decimal128 (&iter, &decimal128);
   BSON_ASSERT (decimal128.low == 11);
   BSON_ASSERT (decimal128.high == 0x3040000000000000ULL);
   bson_destroy (&b);
}

static void
test_bson_json_inc (void)
{
   /* test that reproduces a bug with special mode checking.  Specifically,
    * mistaking '$inc' for '$id'
    *
    * From https://github.com/mongodb/mongo-c-driver/issues/62
    */
   bson_error_t error;
   const char *json = "{ \"$inc\" : { \"ref\" : 1 } }";
   bson_t b;
   bool r;

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);
   bson_destroy (&b);
}

static void
test_bson_json_array (void)
{
   bson_error_t error;
   const char *json = "[ 0, 1, 2, 3 ]";
   bson_t b, compare;
   bool r;

   bson_init (&compare);
   bson_append_int32 (&compare, "0", 1, 0);
   bson_append_int32 (&compare, "1", 1, 1);
   bson_append_int32 (&compare, "2", 1, 2);
   bson_append_int32 (&compare, "3", 1, 3);

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);

   bson_eq_bson (&b, &compare);
   bson_destroy (&compare);
   bson_destroy (&b);
}

static void
test_bson_json_array_single (void)
{
   bson_error_t error;
   const char *json = "[ 0 ]";
   bson_t b, compare;
   bool r;

   bson_init (&compare);
   bson_append_int32 (&compare, "0", 1, 0);

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);

   bson_eq_bson (&b, &compare);
   bson_destroy (&compare);
   bson_destroy (&b);
}

static void
test_bson_json_array_int64 (void)
{
   bson_error_t error;
   const char *json = "[ { \"$numberLong\" : \"123\" },"
                      "  { \"$numberLong\" : \"42\" } ]";
   bson_t b, compare;
   bool r;

   bson_init (&compare);
   bson_append_int64 (&compare, "0", 1, 123);
   bson_append_int64 (&compare, "1", 1, 42);

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);

   bson_eq_bson (&b, &compare);
   bson_destroy (&compare);
   bson_destroy (&b);
}

static void
test_bson_json_array_subdoc (void)
{
   bson_error_t error;
   const char *json = "[ { \"a\" : 123 } ]";
   bson_t b, compare, subdoc;
   bool r;

   bson_init (&compare);
   bson_init (&subdoc);
   bson_append_int32 (&subdoc, "a", 1, 123);
   bson_append_document (&compare, "0", 1, &subdoc);

   r = bson_init_from_json (&b, json, -1, &error);
   if (!r)
      fprintf (stderr, "%s\n", error.message);
   BSON_ASSERT (r);

   bson_eq_bson (&b, &compare);
   bson_destroy (&compare);
   bson_destroy (&b);
}

static void
test_bson_json_date_check (const char *json, int64_t value)
{
   bson_error_t error = {0};
   bson_t b, compare;
   bool r;

   bson_init (&compare);

   BSON_APPEND_DATE_TIME (&compare, "dt", value);

   r = bson_init_from_json (&b, json, -1, &error);

   if (!r) {
      fprintf (stderr, "%s\n", error.message);
   }

   BSON_ASSERT (r);

   bson_eq_bson (&b, &compare);
   bson_destroy (&compare);
   bson_destroy (&b);
}


static void
test_bson_json_date_error (const char *json, const char *msg)
{
   bson_error_t error = {0};
   bson_t b;
   bool r;
   r = bson_init_from_json (&b, json, -1, &error);
   if (r) {
      fprintf (stderr, "parsing %s should fail\n", json);
   }
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (
      error, BSON_ERROR_JSON, BSON_JSON_ERROR_READ_INVALID_PARAM, msg);
}

static void
test_bson_json_date (void)
{
   /* to make a timestamp, "python3 -m pip install iso8601" and in Python 3:
    * iso8601.parse_date("2016-12-13T12:34:56.123Z").timestamp() * 1000
    */
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : \"2016-12-13T12:34:56.123Z\" } }",
      1481632496123);
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T00:00:00.000Z\" } }", 0);
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : \"1969-12-31T16:00:00.000-0800\" } }", 0);

   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000+01:00\" } }",
      "Could not parse");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:30:\" } }",
      "reached end of date while looking for seconds");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:+01:00\" } }",
      "seconds is required");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:30:00.\" } }",
      "reached end of date while looking for milliseconds");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.+01:00\" } }",
      "milliseconds is required");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"foo-01-01T00:00:00.000Z\" } }",
      "year must be an integer");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-foo-01T00:00:00.000Z\" } }",
      "month must be an integer");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-fooT00:00:00.000Z\" } }",
      "day must be an integer");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01Tfoo:00:00.000Z\" } }",
      "hour must be an integer");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T00:foo:00.000Z\" } }",
      "minute must be an integer");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T00:00:foo.000Z\" } }",
      "seconds must be an integer");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000\" } }",
      "timezone is required");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000X\" } }",
      "timezone is required");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000+1\" } }",
      "could not parse timezone");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000+xx00\" } }",
      "could not parse timezone");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000+2400\" } }",
      "timezone hour must be at most 23");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000-2400\" } }",
      "timezone hour must be at most 23");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000+0060\" } }",
      "timezone minute must be at most 59");
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : \"1970-01-01T01:00:00.000-0060\" } }",
      "timezone minute must be at most 59");
}


static void
test_bson_json_date_legacy (void)
{
   test_bson_json_date_check ("{ \"dt\" : { \"$date\" : 0 } }", 0);
   test_bson_json_date_check ("{ \"dt\" : { \"$date\" : 1356351330500 } }",
                              1356351330500);
   test_bson_json_date_check ("{ \"dt\" : { \"$date\" : -62135593139000 } }",
                              -62135593139000);

   /* INT64_MAX */
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : 9223372036854775807 } }", INT64_MAX);

   /* INT64_MIN */
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : -9223372036854775808 } }", INT64_MIN);

   /* INT64_MAX + 1 */
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : 9223372036854775808 } }",
      "Number \"9223372036854775808\" is out of range");

   /* INT64_MIN - 1 */
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : -9223372036854775809 } }",
      "Number \"-9223372036854775809\" is out of range");

   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : 10000000000000000000 } }",
      "Number \"10000000000000000000\" is out of range");

   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : -10000000000000000000 } }",
      "Number \"-10000000000000000000\" is out of range");
}


static void
test_bson_json_date_numberlong (void)
{
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : {\"$numberLong\": \"0\" } } }", 0);
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : {\"$numberLong\": \"1356351330500\" } } }",
      1356351330500);
   test_bson_json_date_check (
      "{ \"dt\" : { \"$date\" : { \"$numberLong\" : \"-62135593139000\" } } }",
      -62135593139000);

   /* INT64_MAX */
   test_bson_json_date_check ("{ \"dt\" : { \"$date\" : { \"$numberLong\" "
                              ": \"9223372036854775807\" } } }",
                              INT64_MAX);

   /* INT64_MIN */
   test_bson_json_date_check ("{ \"dt\" : { \"$date\" : { \"$numberLong\" "
                              ": \"-9223372036854775808\" } } }",
                              INT64_MIN);

   /* INT64_MAX + 1 */
   test_bson_json_date_error ("{ \"dt\" : { \"$date\" : { \"$numberLong\" "
                              ": \"9223372036854775808\" } } }",
                              "Number \"9223372036854775808\" is out of range");

   /* INT64_MIN - 1 */
   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : { \"$numberLong\" "
      ": \"-9223372036854775809\" } } }",
      "Number \"-9223372036854775809\" is out of range");

   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : { \"$numberLong\" "
      ": \"10000000000000000000\" } } }",
      "Number \"10000000000000000000\" is out of range");

   test_bson_json_date_error (
      "{ \"dt\" : { \"$date\" : { \"$numberLong\" "
      ": \"-10000000000000000000\" } } }",
      "Number \"-10000000000000000000\" is out of range");
}


static void
test_bson_json_timestamp (void)
{
   bson_error_t error = {0};
   bson_t b, compare;
   bool r;

   bson_init (&compare);

   BSON_APPEND_TIMESTAMP (
      &compare, "ts", (uint32_t) 1486785977, (uint32_t) 1234);

   r = bson_init_from_json (
      &b,
      "{\"ts\": {\"$timestamp\": {\"t\": 1486785977, \"i\": 1234}}}",
      -1,
      &error);

   if (!r) {
      fprintf (stderr, "%s\n", error.message);
   }

   BSON_ASSERT (r);

   bson_eq_bson (&b, &compare);
   bson_destroy (&compare);
   bson_destroy (&b);
}


static void
test_bson_array_as_json (void)
{
   bson_t d = BSON_INITIALIZER;
   size_t len;
   char *str;

   str = bson_array_as_json (&d, &len);
   BSON_ASSERT (0 == strcmp (str, "[ ]"));
   BSON_ASSERT (len == 3);
   bson_free (str);

   BSON_APPEND_INT32 (&d, "0", 1);
   str = bson_array_as_json (&d, &len);
   BSON_ASSERT (0 == strcmp (str, "[ 1 ]"));
   BSON_ASSERT (len == 5);
   bson_free (str);

   bson_destroy (&d);
}


static void
test_bson_as_json_spacing (void)
{
   bson_t d = BSON_INITIALIZER;
   size_t len;
   char *str;

   str = bson_as_json (&d, &len);
   BSON_ASSERT (0 == strcmp (str, "{ }"));
   BSON_ASSERT (len == 3);
   bson_free (str);

   BSON_APPEND_INT32 (&d, "a", 1);
   str = bson_as_json (&d, &len);
   BSON_ASSERT (0 == strcmp (str, "{ \"a\" : 1 }"));
   BSON_ASSERT (len == 11);
   bson_free (str);

   bson_destroy (&d);
}


static void
test_bson_json_errors (void)
{
   typedef const char *test_bson_json_error_t[2];
   test_bson_json_error_t tests[] = {
      {"{\"x\": {\"$numberLong\": 1}}",
       "Invalid state for integer read: INT64"},
      {"{\"x\": {\"$binary\": 1}}", "Unexpected integer 1 in type \"binary\""},
      {"{\"x\": {\"$numberInt\": true}}",
       "Invalid read of boolean in state IN_BSON_TYPE"},
      {"{\"x\": {\"$dbPointer\": true}}",
       "Invalid read of boolean in state IN_BSON_TYPE_DBPOINTER_STARTMAP"},
      {"{\"x\": {\"$numberInt\": \"8589934592\"}}",
       "Invalid input string \"8589934592\", looking for INT32"},
      {0},
   };

   bson_error_t error;
   test_bson_json_error_t *p;

   for (p = tests; *(p[0]); p++) {
      BSON_ASSERT (!bson_new_from_json ((const uint8_t *) (*p)[0], -1, &error));
      ASSERT_ERROR_CONTAINS (
         error, BSON_ERROR_JSON, BSON_JSON_ERROR_READ_INVALID_PARAM, (*p)[1]);
   }
}


static void
test_bson_integer_width (void)
{
   const char *sd = "{\"v\":-1234567890123, \"x\":12345678901234}";
   char *match;
   bson_error_t err;
   bson_t *bs = bson_new_from_json ((const uint8_t *) sd, strlen (sd), &err);

   match = bson_as_json (bs, 0);
   ASSERT_CMPSTR (match, "{ \"v\" : -1234567890123, \"x\" : 12345678901234 }");

   bson_free (match);
   bson_destroy (bs);
}

void
test_json_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/as_json/x1000", test_bson_as_json_x1000);
   TestSuite_Add (suite, "/bson/as_json/multi", test_bson_as_json_multi);
   TestSuite_Add (suite, "/bson/as_json/string", test_bson_as_json_string);
   TestSuite_Add (suite, "/bson/as_json/int32", test_bson_as_json_int32);
   TestSuite_Add (suite, "/bson/as_json/int64", test_bson_as_json_int64);
   TestSuite_Add (suite, "/bson/as_json/double", test_bson_as_json_double);
#if defined(NAN) && defined(INFINITY)
   TestSuite_Add (suite,
                  "/bson/as_json/double/nonfinite",
                  test_bson_as_json_double_nonfinite);
#endif
   TestSuite_Add (suite, "/bson/as_json/code", test_bson_as_json_code);
   TestSuite_Add (suite, "/bson/as_json/date_time", test_bson_as_json_date_time);
   TestSuite_Add (suite, "/bson/as_json/regex", test_bson_as_json_regex);
   TestSuite_Add (suite, "/bson/as_json/symbol", test_bson_as_json_symbol);
   TestSuite_Add (suite, "/bson/as_json/utf8", test_bson_as_json_utf8);
   TestSuite_Add (suite,
                  "/bson/as_json/dbpointer",
                  test_bson_as_json_dbpointer);
   TestSuite_Add (suite,
                  "/bson/as_canonical_extended_json/dbpointer",
                  test_bson_as_canonical_extended_json_dbpointer);
   TestSuite_Add (
      suite, "/bson/as_json/stack_overflow", test_bson_as_json_stack_overflow);
   TestSuite_Add (suite, "/bson/as_json/corrupt", test_bson_corrupt);
   TestSuite_Add (suite, "/bson/as_json/corrupt_utf8", test_bson_corrupt_utf8);
   TestSuite_Add (
      suite, "/bson/as_json/corrupt_binary", test_bson_corrupt_binary);
   TestSuite_Add (suite, "/bson/as_json_spacing", test_bson_as_json_spacing);
   TestSuite_Add (suite, "/bson/array_as_json", test_bson_array_as_json);
   TestSuite_Add (
      suite, "/bson/json/allow_multiple", test_bson_json_allow_multiple);
   TestSuite_Add (
      suite, "/bson/json/read/buffering", test_bson_json_read_buffering);
   TestSuite_Add (suite, "/bson/json/read", test_bson_json_read);
   TestSuite_Add (suite, "/bson/json/inc", test_bson_json_inc);
   TestSuite_Add (suite, "/bson/json/array", test_bson_json_array);
   TestSuite_Add (
      suite, "/bson/json/array/single", test_bson_json_array_single);
   TestSuite_Add (suite, "/bson/json/array/int64", test_bson_json_array_int64);
   TestSuite_Add (
      suite, "/bson/json/array/subdoc", test_bson_json_array_subdoc);
   TestSuite_Add (suite, "/bson/json/date", test_bson_json_date);
   TestSuite_Add (suite, "/bson/json/date/legacy", test_bson_json_date_legacy);
   TestSuite_Add (
      suite, "/bson/json/date/long", test_bson_json_date_numberlong);
   TestSuite_Add (suite, "/bson/json/timestamp", test_bson_json_timestamp);
   TestSuite_Add (suite, "/bson/json/read/empty", test_bson_json_read_empty);
   TestSuite_Add (suite,
                  "/bson/json/read/missing_complex",
                  test_bson_json_read_missing_complex);
   TestSuite_Add (suite,
                  "/bson/json/read/invalid_binary",
                  test_bson_json_read_invalid_binary);
   TestSuite_Add (
      suite, "/bson/json/read/invalid_json", test_bson_json_read_invalid_json);
   TestSuite_Add (suite, "/bson/json/read/bad_cb", test_bson_json_read_bad_cb);
   TestSuite_Add (
      suite, "/bson/json/read/invalid", test_bson_json_read_invalid);
   TestSuite_Add (
      suite, "/bson/json/read/raw_utf8", test_bson_json_read_raw_utf8);
   TestSuite_Add (
      suite, "/bson/json/read/corrupt_utf8", test_bson_json_read_corrupt_utf8);
   TestSuite_Add (
      suite, "/bson/json/read/decimal128", test_bson_json_read_decimal128);
   TestSuite_Add (
      suite, "/bson/json/read/dbpointer", test_bson_json_read_dbpointer);
   TestSuite_Add (
      suite, "/bson/json/read/legacy_regex", test_bson_json_read_legacy_regex);
   TestSuite_Add (suite,
                  "/bson/json/read/regex_options_order",
                  test_bson_json_read_regex_options_order);
   TestSuite_Add (suite, "/bson/json/read/binary", test_bson_json_read_binary);
   TestSuite_Add (suite,
                  "/bson/json/read/legacy_binary",
                  test_bson_json_read_legacy_binary);
   TestSuite_Add (
      suite, "/bson/json/read/file", test_json_reader_new_from_file);
   TestSuite_Add (
      suite, "/bson/json/read/bad_path", test_json_reader_new_from_bad_path);
   TestSuite_Add (
      suite, "/bson/json/read/$numberLong", test_bson_json_number_long);
   TestSuite_Add (suite,
                  "/bson/json/read/$numberLong/zero",
                  test_bson_json_number_long_zero);
   TestSuite_Add (suite, "/bson/json/read/code", test_bson_json_code);
   TestSuite_Add (
      suite, "/bson/json/read/code/errors", test_bson_json_code_errors);
   TestSuite_Add (suite, "/bson/json/read/dbref", test_bson_json_dbref);
   TestSuite_Add (suite, "/bson/json/read/uescape", test_bson_json_uescape);
   TestSuite_Add (
      suite, "/bson/json/read/uescape/key", test_bson_json_uescape_key);
   TestSuite_Add (
      suite, "/bson/json/read/uescape/bad", test_bson_json_uescape_bad);
   TestSuite_Add (suite, "/bson/json/read/int32", test_bson_json_int32);
   TestSuite_Add (suite, "/bson/json/read/int64", test_bson_json_int64);
   TestSuite_Add (suite, "/bson/json/read/double", test_bson_json_double);
   TestSuite_Add (
      suite, "/bson/json/read/double/overflow", test_bson_json_double_overflow);
   TestSuite_Add (suite, "/bson/json/read/double/nan", test_bson_json_nan);
   TestSuite_Add (
      suite, "/bson/json/read/double/infinity", test_bson_json_infinity);
   TestSuite_Add (suite, "/bson/json/read/null", test_bson_json_null);
   TestSuite_Add (
      suite, "/bson/json/read/empty_final", test_bson_json_empty_final_object);
   TestSuite_Add (
      suite, "/bson/as_json/decimal128", test_bson_as_json_decimal128);
   TestSuite_Add (
      suite, "/bson/json/read/$numberDecimal", test_bson_json_number_decimal);
   TestSuite_Add (suite, "/bson/json/errors", test_bson_json_errors);
   TestSuite_Add (suite, "/bson/integer/width", test_bson_integer_width);
}
