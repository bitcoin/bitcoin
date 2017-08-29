#include "bson-tests.h"
#include "TestSuite.h"

#include <bcon.h>

static void
test_utf8 (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_utf8 (&expected, "hello", -1, "world", -1);
   BCON_APPEND (&bcon, "hello", "world");

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}

static void
test_double (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_double (&expected, "foo", -1, 1.1);
   BCON_APPEND (&bcon, "foo", BCON_DOUBLE (1.1));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}

static void
test_decimal128 (void)
{
   bson_t bcon, expected;
   bson_decimal128_t dec;
   bson_decimal128_from_string ("120E20", &dec);

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_decimal128 (&expected, "foo", -1, &dec);
   BCON_APPEND (&bcon, "foo", BCON_DECIMAL128 (&dec));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}

static void
test_binary (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_binary (
      &expected, "foo", -1, BSON_SUBTYPE_BINARY, (uint8_t *) "deadbeef", 8);

   BCON_APPEND (&bcon,
                "foo",
                BCON_BIN (BSON_SUBTYPE_BINARY, (const uint8_t *) "deadbeef", 8),
                NULL);

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_undefined (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_undefined (&expected, "foo", -1);

   BCON_APPEND (&bcon, "foo", BCON_UNDEFINED);

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_oid (void)
{
   bson_t bcon, expected;
   bson_oid_t oid;

   bson_init (&bcon);
   bson_init (&expected);

   bson_oid_init (&oid, NULL);

   bson_append_oid (&expected, "foo", -1, &oid);
   BCON_APPEND (&bcon, "foo", BCON_OID (&oid));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_bool (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_bool (&expected, "foo", -1, 1);

   BCON_APPEND (&bcon, "foo", BCON_BOOL (1));

   bson_eq_bson (&bcon, &expected);

   bson_reinit (&bcon);
   bson_reinit (&expected);

   bson_append_bool (&expected, "foo", -1, 0);

   BCON_APPEND (&bcon, "foo", BCON_BOOL (0));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_date_time (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_date_time (&expected, "foo", -1, 10000);

   BCON_APPEND (&bcon, "foo", BCON_DATE_TIME (10000));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_null (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_null (&expected, "foo", -1);

   BCON_APPEND (&bcon, "foo", BCON_NULL);

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_regex (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   /* option flags are sorted */
   bson_append_regex (&expected, "foo", -1, "^foo|bar$", "mis");
   BCON_APPEND (&bcon, "foo", BCON_REGEX ("^foo|bar$", "msi"));
   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_dbpointer (void)
{
   bson_t bcon, expected;
   bson_oid_t oid;

   bson_init (&bcon);
   bson_init (&expected);

   bson_oid_init (&oid, NULL);

   bson_append_dbpointer (&expected, "foo", -1, "collection", &oid);

   BCON_APPEND (&bcon, "foo", BCON_DBPOINTER ("collection", &oid));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_code (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_code (&expected, "foo", -1, "var a = {};");

   BCON_APPEND (&bcon, "foo", BCON_CODE ("var a = {};"));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_symbol (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_symbol (&expected, "foo", -1, "deadbeef", -1);

   BCON_APPEND (&bcon, "foo", BCON_SYMBOL ("deadbeef"));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_codewscope (void)
{
   bson_t bcon, expected, scope;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&scope);

   bson_append_int32 (&scope, "b", -1, 10);

   bson_append_code_with_scope (&expected, "foo", -1, "var a = b;", &scope);

   BCON_APPEND (&bcon, "foo", BCON_CODEWSCOPE ("var a = b;", &scope));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
   bson_destroy (&scope);
}


static void
test_int32 (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_int32 (&expected, "foo", -1, 100);

   BCON_APPEND (&bcon, "foo", BCON_INT32 (100));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_timestamp (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_timestamp (&expected, "foo", -1, 100, 1000);

   BCON_APPEND (&bcon, "foo", BCON_TIMESTAMP (100, 1000));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_int64 (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_int64 (&expected, "foo", -1, 100);

   BCON_APPEND (&bcon, "foo", BCON_INT64 (100));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_maxkey (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_maxkey (&expected, "foo", -1);
   BCON_APPEND (&bcon, "foo", BCON_MAXKEY);

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_minkey (void)
{
   bson_t bcon, expected;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_minkey (&expected, "foo", -1);
   BCON_APPEND (&bcon, "foo", BCON_MINKEY);

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_bson_document (void)
{
   bson_t bcon, expected, child;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&child);

   bson_append_utf8 (&child, "bar", -1, "baz", -1);
   bson_append_document (&expected, "foo", -1, &child);

   BCON_APPEND (&bcon, "foo", BCON_DOCUMENT (&child));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
   bson_destroy (&child);
}


static void
test_bson_array (void)
{
   bson_t bcon, expected, child;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&child);

   bson_append_utf8 (&child, "0", -1, "baz", -1);
   bson_append_array (&expected, "foo", -1, &child);

   BCON_APPEND (&bcon, "foo", BCON_ARRAY (&child));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
   bson_destroy (&child);
}


static void
test_inline_array (void)
{
   bson_t bcon, expected, child;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&child);

   bson_append_utf8 (&child, "0", -1, "baz", -1);
   bson_append_array (&expected, "foo", -1, &child);

   BCON_APPEND (&bcon, "foo", "[", "baz", "]");

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&child);
   bson_destroy (&expected);
}


static void
test_inline_doc (void)
{
   bson_t bcon, expected, child;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&child);

   bson_append_utf8 (&child, "bar", -1, "baz", -1);
   bson_append_document (&expected, "foo", -1, &child);

   BCON_APPEND (&bcon, "foo", "{", "bar", "baz", "}");

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
   bson_destroy (&child);
}


static void
test_inline_nested (void)
{
   bson_t bcon, expected, foo, bar, third;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&foo);
   bson_init (&bar);
   bson_init (&third);

   bson_append_utf8 (&third, "hello", -1, "world", -1);
   bson_append_int32 (&bar, "0", -1, 1);
   bson_append_int32 (&bar, "1", -1, 2);
   bson_append_document (&bar, "2", -1, &third);
   bson_append_array (&foo, "bar", -1, &bar);
   bson_append_document (&expected, "foo", -1, &foo);

   BCON_APPEND (&bcon,
                "foo",
                "{",
                "bar",
                "[",
                BCON_INT32 (1),
                BCON_INT32 (2),
                "{",
                "hello",
                "world",
                "}",
                "]",
                "}");

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
   bson_destroy (&foo);
   bson_destroy (&bar);
   bson_destroy (&third);
}


static void
test_concat (void)
{
   bson_t bcon, expected, child, child2;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&child);
   bson_init (&child2);

   bson_append_utf8 (&child, "hello", -1, "world", -1);
   bson_append_document (&expected, "foo", -1, &child);

   BCON_APPEND (&bcon, "foo", "{", BCON (&child), "}");

   bson_eq_bson (&bcon, &expected);

   bson_reinit (&bcon);
   bson_reinit (&expected);
   bson_reinit (&child);

   bson_append_utf8 (&child, "0", -1, "bar", -1);
   bson_append_utf8 (&child, "1", -1, "baz", -1);
   bson_append_array (&expected, "foo", -1, &child);

   bson_append_utf8 (&child2, "0", -1, "baz", -1);
   BCON_APPEND (&bcon, "foo", "[", "bar", BCON (&child2), "]");

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&child);
   bson_destroy (&child2);
   bson_destroy (&expected);
}


static void
test_iter (void)
{
   bson_t bcon, expected;
   bson_iter_t iter;

   bson_init (&bcon);
   bson_init (&expected);

   bson_append_int32 (&expected, "foo", -1, 100);
   bson_iter_init_find (&iter, &expected, "foo");

   BCON_APPEND (&bcon, "foo", BCON_ITER (&iter));

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
}


static void
test_bcon_new (void)
{
   bson_t expected;
   bson_t *bcon;

   bson_init (&expected);

   bson_append_utf8 (&expected, "hello", -1, "world", -1);
   bcon = BCON_NEW ("hello", "world");

   bson_eq_bson (bcon, &expected);

   bson_destroy (bcon);
   bson_destroy (&expected);
}


static void
test_append_ctx_helper (bson_t *bson, ...)
{
   va_list ap;
   bcon_append_ctx_t ctx;
   bcon_append_ctx_init (&ctx);

   va_start (ap, bson);

   bcon_append_ctx_va (bson, &ctx, &ap);

   va_arg (ap, char *);

   BCON_APPEND_CTX (bson, &ctx, "c", "d");

   bcon_append_ctx_va (bson, &ctx, &ap);

   va_end (ap);
}

static void
test_append_ctx (void)
{
   bson_t bcon, expected, child;

   bson_init (&bcon);
   bson_init (&expected);
   bson_init (&child);

   bson_append_utf8 (&child, "c", -1, "d", -1);
   bson_append_utf8 (&child, "e", -1, "f", -1);

   bson_append_document (&expected, "a", -1, &child);

   test_append_ctx_helper (
      &bcon, "a", "{", NULL, "add magic", "e", "f", "}", NULL);

   bson_eq_bson (&bcon, &expected);

   bson_destroy (&bcon);
   bson_destroy (&expected);
   bson_destroy (&child);
}


void
test_bcon_basic_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/bcon/test_utf8", test_utf8);
   TestSuite_Add (suite, "/bson/bcon/test_double", test_double);
   TestSuite_Add (suite, "/bson/bcon/test_binary", test_binary);
   TestSuite_Add (suite, "/bson/bcon/test_undefined", test_undefined);
   TestSuite_Add (suite, "/bson/bcon/test_oid", test_oid);
   TestSuite_Add (suite, "/bson/bcon/test_bool", test_bool);
   TestSuite_Add (suite, "/bson/bcon/test_date_time", test_date_time);
   TestSuite_Add (suite, "/bson/bcon/test_null", test_null);
   TestSuite_Add (suite, "/bson/bcon/test_regex", test_regex);
   TestSuite_Add (suite, "/bson/bcon/test_dbpointer", test_dbpointer);
   TestSuite_Add (suite, "/bson/bcon/test_code", test_code);
   TestSuite_Add (suite, "/bson/bcon/test_symbol", test_symbol);
   TestSuite_Add (suite, "/bson/bcon/test_codewscope", test_codewscope);
   TestSuite_Add (suite, "/bson/bcon/test_int32", test_int32);
   TestSuite_Add (suite, "/bson/bcon/test_timestamp", test_timestamp);
   TestSuite_Add (suite, "/bson/bcon/test_int64", test_int64);
   TestSuite_Add (suite, "/bson/bcon/test_decimal128", test_decimal128);
   TestSuite_Add (suite, "/bson/bcon/test_maxkey", test_maxkey);
   TestSuite_Add (suite, "/bson/bcon/test_minkey", test_minkey);
   TestSuite_Add (suite, "/bson/bcon/test_bson_document", test_bson_document);
   TestSuite_Add (suite, "/bson/bcon/test_bson_array", test_bson_array);
   TestSuite_Add (suite, "/bson/bcon/test_inline_array", test_inline_array);
   TestSuite_Add (suite, "/bson/bcon/test_inline_doc", test_inline_doc);
   TestSuite_Add (suite, "/bson/bcon/test_inline_nested", test_inline_nested);
   TestSuite_Add (suite, "/bson/bcon/test_concat", test_concat);
   TestSuite_Add (suite, "/bson/bcon/test_iter", test_iter);
   TestSuite_Add (suite, "/bson/bcon/test_bcon_new", test_bcon_new);
   TestSuite_Add (suite, "/bson/bcon/test_append_ctx", test_append_ctx);
}
