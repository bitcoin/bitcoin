
#include <bcon.h>
#include <mongoc.h>
#include <mongoc-matcher-private.h>
#include <mongoc-util-private.h>

#include "TestSuite.h"

BEGIN_IGNORE_DEPRECATIONS;

static void
test_mongoc_matcher_basic (void)
{
   bson_t matcher_query;
   bson_t *query;
   bson_t *to_match;
   bson_t *should_fail;
   bson_error_t error;
   mongoc_matcher_t *matcher;

   bson_init (&matcher_query);

   query = BCON_NEW ("city",
                     "New York",
                     "state",
                     "New York",
                     "favorite color",
                     "blue",
                     "name",
                     "{",
                     "$not",
                     "invalid",
                     "}",
                     /*      "zip", "{", "$in", "[", BCON_INT32(11201), */
                     /*      BCON_INT32(90210), "]", "}", */
                     "$or",
                     "[",
                     "{",
                     "age",
                     "{",
                     "$lt",
                     BCON_INT32 (18),
                     "}",
                     "}",
                     "{",
                     "age",
                     "{",
                     "$gt",
                     BCON_INT32 (45),
                     "}",
                     "}",
                     "]");

   matcher = mongoc_matcher_new (query, &error);

   BSON_ASSERT (matcher);

   _mongoc_matcher_op_to_bson (matcher->optree, &matcher_query);

#if 0
   {
      char *out = bson_as_canonical_extended_json(&matcher_query, NULL);
      fprintf(stderr, "bson: %s\n", out);
      free(out);
   }
#endif

   to_match = BCON_NEW ("city",
                        "New York",
                        "state",
                        "New York",
                        "favorite color",
                        "blue",
                        "zip",
                        BCON_INT32 (11201),
                        "age",
                        BCON_INT32 (65));

   BSON_ASSERT (mongoc_matcher_match (matcher, to_match));

   should_fail = BCON_NEW ("city",
                           "New York",
                           "state",
                           "New York",
                           "favorite color",
                           "blue",
                           "zip",
                           BCON_INT32 (99999),
                           "age",
                           BCON_INT32 (30));

   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));

   bson_destroy (query);
   bson_destroy (to_match);
   bson_destroy (should_fail);
   bson_destroy (&matcher_query);

   mongoc_matcher_destroy (matcher);
}

static void
test_mongoc_matcher_array (void)
{
   bson_t *query;
   bson_t *to_match;
   bson_t *should_fail;
   bson_error_t error;
   mongoc_matcher_t *matcher;

   query = BCON_NEW ("a", "[", BCON_INT32 (1), BCON_INT32 (2), "]");
   matcher = mongoc_matcher_new (query, &error);
   BSON_ASSERT (matcher);

   /* query matches itself */
   BSON_ASSERT (mongoc_matcher_match (matcher, query));

   to_match =
      BCON_NEW ("a", "[", BCON_INT32 (1), BCON_INT32 (2), "]", "b", "whatever");
   BSON_ASSERT (mongoc_matcher_match (matcher, to_match));

   /* query {a: [1, 2]} doesn't match {a: 1} */
   should_fail = BCON_NEW ("a", BCON_INT32 (1));
   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));
   bson_destroy (should_fail);

   /* query {a: [1, 2]} doesn't match {a: [2, 1]} */
   should_fail = BCON_NEW ("a", "[", BCON_INT32 (2), BCON_INT32 (1), "]");
   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));
   bson_destroy (should_fail);

   /* query {a: [1, 2]} doesn't match {a: [1, 2, 3]} */
   should_fail =
      BCON_NEW ("a", "[", BCON_INT32 (1), BCON_INT32 (2), BCON_INT32 (3), "]");
   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));
   bson_destroy (should_fail);

   /* query {a: [1, 2]} doesn't match {a: [1]} */
   should_fail = BCON_NEW ("a", "[", BCON_INT32 (1), "]");
   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));

   bson_destroy (to_match);
   mongoc_matcher_destroy (matcher);
   bson_destroy (query);

   /* empty array */
   query = BCON_NEW ("a", "[", "]");

   /* {a: []} matches itself */
   matcher = mongoc_matcher_new (query, &error);
   BSON_ASSERT (matcher);

   /* query {a: []} matches {a: [], b: "whatever"} */
   to_match = BCON_NEW ("a", "[", "]", "b", "whatever");
   BSON_ASSERT (mongoc_matcher_match (matcher, query));
   BSON_ASSERT (mongoc_matcher_match (matcher, to_match));

   /* query {a: []} doesn't match {a: [1]} */
   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));
   bson_destroy (should_fail);

   /* query {a: []} doesn't match empty document */
   should_fail = bson_new ();
   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));

   bson_destroy (should_fail);

   /* query {a: []} doesn't match {a: null} */
   should_fail = BCON_NEW ("a", BCON_NULL);
   BSON_ASSERT (!mongoc_matcher_match (matcher, should_fail));

   bson_destroy (should_fail);
   bson_destroy (query);
   bson_destroy (to_match);
   mongoc_matcher_destroy (matcher);
}


typedef struct compare_check {
   const char *op;
   int32_t doc;
   int32_t query;
   bool expected;
} compare_check;


static void
test_mongoc_matcher_compare (void)
{
   mongoc_matcher_t *matcher;
   compare_check checks[] = {{"$gt", 2, 2, false},
                             {"$gte", 2, 2, true},
                             {"$lt", 2, 2, false},
                             {"$lte", 2, 2, true},
                             {"$ne", 2, 2, false},
                             {NULL}};
   bson_t *doc;
   bson_t *q;
   int i;

   for (i = 0; checks[i].op; i++) {
      doc = BCON_NEW ("a", BCON_INT32 (checks[i].doc));
      q = BCON_NEW ("a", "{", checks[i].op, BCON_INT32 (checks[i].query), "}");
      matcher = mongoc_matcher_new (q, NULL);
      BSON_ASSERT (matcher);
      BSON_ASSERT (mongoc_matcher_match (matcher, doc) == checks[i].expected);
      bson_destroy (q);
      bson_destroy (doc);
      mongoc_matcher_destroy (matcher);
   }
}


typedef struct {
   const char *spec;
   const char *doc;
   bool match;
} logic_op_test_t;


static void
test_mongoc_matcher_logic_ops (void)
{
   logic_op_test_t tests[] = {
      {"{\"$or\": [{\"a\": 1}, {\"b\": 2}]}", "{\"a\": 1}", true},
      {"{\"$or\": [{\"a\": 1}, {\"b\": 2}]}", "{\"b\": 2}", true},
      {"{\"$or\": [{\"a\": 1}, {\"b\": 2}]}", "{\"a\": 3}", false},
      {"{\"$or\": [{\"a\": {\"$gt\": 1}}, {\"a\": {\"$lt\": -1}}]}",
       "{\"a\": 3}",
       true},
      {"{\"$or\": [{\"a\": {\"$gt\": 1}}, {\"a\": {\"$lt\": -1}}]}",
       "{\"a\": -2}",
       true},
      {"{\"$or\": [{\"a\": {\"$gt\": 1}}, {\"a\": {\"$lt\": -1}}]}",
       "{\"a\": 0}",
       false},
      {"{\"$and\": [{\"a\": 1}, {\"b\": 2}]}", "{\"a\": 1, \"b\": 2}", true},
      {"{\"$and\": [{\"a\": 1}, {\"b\": 2}]}", "{\"a\": 1, \"b\": 1}", false},
      {"{\"$and\": [{\"a\": 1}, {\"b\": 2}]}", "{\"a\": 1}", false},
      {"{\"$and\": [{\"a\": 1}, {\"b\": 2}]}", "{\"b\": 2}", false},
      {"{\"$and\": [{\"a\": {\"$gt\": -1}}, {\"a\": {\"$lt\": 1}}]}",
       "{\"a\": 0}",
       true},
      {"{\"$and\": [{\"a\": {\"$gt\": -1}}, {\"a\": {\"$lt\": 1}}]}",
       "{\"a\": -2}",
       false},
      {"{\"$and\": [{\"a\": {\"$gt\": -1}}, {\"a\": {\"$lt\": 1}}]}",
       "{\"a\": 1}",
       false},
   };

   int n_tests = sizeof tests / sizeof (logic_op_test_t);
   int i;
   logic_op_test_t test;
   bson_t *spec;
   bson_error_t error;
   mongoc_matcher_t *matcher;
   bson_t *doc;
   bool r;

   for (i = 0; i < n_tests; i++) {
      test = tests[i];
      spec = bson_new_from_json ((uint8_t *) test.spec, -1, &error);
      if (!spec) {
         fprintf (stderr,
                  "couldn't parse JSON query:\n\n%s\n\n%s\n",
                  test.spec,
                  error.message);
         abort ();
      }

      matcher = mongoc_matcher_new (spec, &error);
      BSON_ASSERT (matcher);

      doc = bson_new_from_json ((uint8_t *) test.doc, -1, &error);
      if (!doc) {
         fprintf (stderr,
                  "couldn't parse JSON document:\n\n%s\n\n%s\n",
                  test.doc,
                  error.message);
         abort ();
      }

      r = mongoc_matcher_match (matcher, doc);
      if (test.match != r) {
         fprintf (stderr,
                  "query:\n\n%s\n\nshould %shave matched:\n\n%s\n",
                  test.match ? "" : "not ",
                  test.spec,
                  test.doc);
         abort ();
      }

      mongoc_matcher_destroy (matcher);
      bson_destroy (doc);
      bson_destroy (spec);
   }
}


static void
test_mongoc_matcher_bad_spec (void)
{
   bson_t *spec;
   bson_error_t error;
   mongoc_matcher_t *matcher;

   spec = BCON_NEW ("name", "{", "$abc", "invalid", "}");
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (!matcher);
   BSON_ASSERT (error.domain == MONGOC_ERROR_MATCHER);
   BSON_ASSERT (error.code == MONGOC_ERROR_MATCHER_INVALID);
   bson_destroy (spec);

   spec = BCON_NEW ("name", "{", "$or", "", "}");
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (!matcher);
   BSON_ASSERT (error.domain == MONGOC_ERROR_MATCHER);
   BSON_ASSERT (error.code == MONGOC_ERROR_MATCHER_INVALID);
   bson_destroy (spec);
}


static void
test_mongoc_matcher_eq_utf8 (void)
{
   bson_t *doc;
   bson_t *spec;
   bson_error_t error;
   mongoc_matcher_t *matcher;
   bool r;

   spec = BCON_NEW ("hello", "world");
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, spec);
   BSON_ASSERT (r);
   bson_destroy (spec);
   mongoc_matcher_destroy (matcher);

   spec = BCON_NEW ("hello", "world");
   doc = BCON_NEW ("hello", BCON_NULL);
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, doc);
   BSON_ASSERT (!r);
   bson_destroy (spec);
   bson_destroy (doc);
   mongoc_matcher_destroy (matcher);

   spec = BCON_NEW ("hello", "world");
   doc = BCON_NEW ("hello", BCON_UNDEFINED);
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, doc);
   BSON_ASSERT (!r);
   bson_destroy (spec);
   bson_destroy (doc);
   mongoc_matcher_destroy (matcher);
}


static void
test_mongoc_matcher_eq_int32 (void)
{
   bson_t *spec;
   bson_t *doc;
   bson_error_t error;
   mongoc_matcher_t *matcher;
   bool r;

   spec = BCON_NEW ("hello", BCON_INT32 (1234));
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, spec);
   BSON_ASSERT (r);
   bson_destroy (spec);
   mongoc_matcher_destroy (matcher);

   spec = BCON_NEW ("hello", BCON_INT32 (1234));
   doc = BCON_NEW ("hello", BCON_INT64 (1234));
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, doc);
   BSON_ASSERT (r);
   bson_destroy (spec);
   bson_destroy (doc);
   mongoc_matcher_destroy (matcher);

   spec = BCON_NEW ("hello", BCON_INT32 (1234));
   doc = BCON_NEW ("hello", BCON_INT64 (4321));
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, doc);
   BSON_ASSERT (!r);
   bson_destroy (spec);
   bson_destroy (doc);
   mongoc_matcher_destroy (matcher);
}


static void
test_mongoc_matcher_eq_int64 (void)
{
   bson_t *spec;
   bson_t *doc;
   bson_error_t error;
   mongoc_matcher_t *matcher;
   bool r;

   spec = BCON_NEW ("hello", BCON_INT64 (1234));
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, spec);
   BSON_ASSERT (r);
   bson_destroy (spec);
   mongoc_matcher_destroy (matcher);

   spec = BCON_NEW ("hello", BCON_INT64 (1234));
   doc = BCON_NEW ("hello", BCON_INT64 (1234));
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, doc);
   BSON_ASSERT (r);
   bson_destroy (spec);
   bson_destroy (doc);
   mongoc_matcher_destroy (matcher);

   spec = BCON_NEW ("hello", BCON_INT64 (1234));
   doc = BCON_NEW ("hello", BCON_INT32 (4321));
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   r = mongoc_matcher_match (matcher, doc);
   BSON_ASSERT (!r);
   bson_destroy (spec);
   bson_destroy (doc);
   mongoc_matcher_destroy (matcher);
}


static void
test_mongoc_matcher_eq_doc (void)
{
   bson_t *spec;
   bson_t *doc;
   bson_error_t error;
   mongoc_matcher_t *matcher;

   /* {doc: {a: 1}} matches itself */
   spec = BCON_NEW ("doc", "{", "a", BCON_INT32 (1), "}");
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   BSON_ASSERT (mongoc_matcher_match (matcher, spec));

   /* {doc: {a: 1}} matches {doc: {a: 1}, foo: "whatever"} */
   doc = BCON_NEW (
      "doc", "{", "a", BCON_INT32 (1), "}", "foo", BCON_UTF8 ("whatever"));
   BSON_ASSERT (mongoc_matcher_match (matcher, doc));
   bson_destroy (doc);

   /* {doc: {a: 1}} doesn't match {doc: 1} */
   doc = BCON_NEW ("doc", BCON_INT32 (1));
   BSON_ASSERT (!mongoc_matcher_match (matcher, doc));
   bson_destroy (doc);

   /* {doc: {a: 1}} doesn't match {doc: {}} */
   doc = BCON_NEW ("doc", "{", "}");
   BSON_ASSERT (!mongoc_matcher_match (matcher, doc));
   bson_destroy (doc);

   /* {doc: {a: 1}} doesn't match {doc: {a: 2}} */
   doc = BCON_NEW ("doc", "{", "a", BCON_INT32 (2), "}");
   BSON_ASSERT (!mongoc_matcher_match (matcher, doc));
   bson_destroy (doc);

   /* {doc: {a: 1}} doesn't match {doc: {b: 1}} */
   doc = BCON_NEW ("doc", "{", "b", BCON_INT32 (1), "}");
   BSON_ASSERT (!mongoc_matcher_match (matcher, doc));
   bson_destroy (doc);

   /* {doc: {a: 1}} doesn't match {doc: {a: 1, b: 1}} */
   doc = BCON_NEW ("doc", "{", "a", BCON_INT32 (1), "b", BCON_INT32 (1), "}");
   BSON_ASSERT (!mongoc_matcher_match (matcher, doc));
   bson_destroy (doc);

   /* {doc: {a: 1, b:1}} matches itself */
   bson_destroy (spec);
   mongoc_matcher_destroy (matcher);
   spec = BCON_NEW ("doc", "{", "a", BCON_INT32 (1), "b", BCON_INT32 (1), "}");
   matcher = mongoc_matcher_new (spec, &error);
   BSON_ASSERT (matcher);
   BSON_ASSERT (mongoc_matcher_match (matcher, spec));

   /* {doc: {a: 1, b:1}} doesn't match {doc: {a: 1}} */
   doc = BCON_NEW ("doc", "{", "a", BCON_INT32 (1), "}");
   BSON_ASSERT (!mongoc_matcher_match (matcher, doc));
   bson_destroy (spec);
   bson_destroy (doc);
   mongoc_matcher_destroy (matcher);
}


static void
test_mongoc_matcher_in_basic (void)
{
   mongoc_matcher_t *matcher;
   bson_error_t error;
   bool r;
   bson_t *spec;
   bson_t doc = BSON_INITIALIZER;

   spec = BCON_NEW ("key",
                    "{",
                    "$in",
                    "[",
                    BCON_INT32 (1),
                    BCON_INT32 (2),
                    BCON_INT32 (3),
                    "]",
                    "}");

   matcher = mongoc_matcher_new (spec, &error);
   r = mongoc_matcher_match (matcher, &doc);
   ASSERT (!r);

   bson_reinit (&doc);
   bson_append_int32 (&doc, "key", 3, 1);
   r = mongoc_matcher_match (matcher, &doc);
   ASSERT (r);

   bson_reinit (&doc);
   bson_append_int32 (&doc, "key", 3, 2);
   r = mongoc_matcher_match (matcher, &doc);
   ASSERT (r);

   bson_reinit (&doc);
   bson_append_int32 (&doc, "key", 3, 3);
   r = mongoc_matcher_match (matcher, &doc);
   ASSERT (r);

   bson_reinit (&doc);
   bson_append_int32 (&doc, "key", 3, 4);
   r = mongoc_matcher_match (matcher, &doc);
   ASSERT (!r);

   bson_destroy (&doc);
   bson_destroy (spec);
   mongoc_matcher_destroy (matcher);
}

END_IGNORE_DEPRECATIONS;

void
test_matcher_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Matcher/basic", test_mongoc_matcher_basic);
   TestSuite_Add (suite, "/Matcher/array", test_mongoc_matcher_array);
   TestSuite_Add (suite, "/Matcher/compare", test_mongoc_matcher_compare);
   TestSuite_Add (suite, "/Matcher/logic", test_mongoc_matcher_logic_ops);
   TestSuite_Add (suite, "/Matcher/bad_spec", test_mongoc_matcher_bad_spec);
   TestSuite_Add (suite, "/Matcher/eq/utf8", test_mongoc_matcher_eq_utf8);
   TestSuite_Add (suite, "/Matcher/eq/int32", test_mongoc_matcher_eq_int32);
   TestSuite_Add (suite, "/Matcher/eq/int64", test_mongoc_matcher_eq_int64);
   TestSuite_Add (suite, "/Matcher/eq/doc", test_mongoc_matcher_eq_doc);
   TestSuite_Add (suite, "/Matcher/in/basic", test_mongoc_matcher_in_basic);
}
