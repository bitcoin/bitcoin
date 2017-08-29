#include <bcon.h>
#include <mongoc.h>
#include <stdio.h>

static void
log_query (const bson_t *doc, const bson_t *query)
{
   char *str1;
   char *str2;

   str1 = bson_as_canonical_extended_json (doc, NULL);
   str2 = bson_as_canonical_extended_json (query, NULL);

   printf ("Matching %s against %s\n", str2, str1);

   bson_free (str1);
   bson_free (str2);
}

static void
check_match (const bson_t *doc, const bson_t *query)
{
   bson_error_t error;
   mongoc_matcher_t *matcher = mongoc_matcher_new (query, &error);
   if (!matcher) {
      fprintf (stderr, "Error: %s\n", error.message);
      return;
   }

   if (mongoc_matcher_match (matcher, doc)) {
      printf ("  Document matched!\n");
   } else {
      printf ("  No match.\n");
   }

   mongoc_matcher_destroy (matcher);
}

static void
example (void)
{
   bson_t *query;
   bson_t *doc;

   doc = BCON_NEW ("hello", "[", "{", "foo", BCON_UTF8 ("bar"), "}", "]");
   query = BCON_NEW ("hello.0.foo", BCON_UTF8 ("bar"));

   log_query (doc, query);
   check_match (doc, query);

   bson_destroy (doc);
   bson_destroy (query);

   /* i is > 1 or i < -1. */
   query = BCON_NEW ("$or",
                     "[",
                     "{",
                     "i",
                     "{",
                     "$gt",
                     BCON_INT32 (1),
                     "}",
                     "}",
                     "{",
                     "i",
                     "{",
                     "$lt",
                     BCON_INT32 (-1),
                     "}",
                     "}",
                     "]");

   doc = BCON_NEW ("i", BCON_INT32 (2));
   log_query (doc, query);
   check_match (doc, query);

   bson_destroy (doc);

   doc = BCON_NEW ("i", BCON_INT32 (0));
   log_query (doc, query);
   check_match (doc, query);

   bson_destroy (doc);
   bson_destroy (query);
}

int
main (int argc, char *argv[])
{
   mongoc_init ();
   example ();
   mongoc_cleanup ();

   return 0;
}
