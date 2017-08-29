#include <mongoc.h>

#include "mongoc-set-private.h"

#include "TestSuite.h"

static void
test_set_dtor (void *item_, void *ctx_)
{
   int *destroyed = (int *) ctx_;

   (*destroyed)++;
}

static bool
test_set_visit_cb (void *item_, void *ctx_)
{
   int *visited = (int *) ctx_;

   (*visited)++;

   return true;
}

static bool
test_set_stop_after_cb (void *item_, void *ctx_)
{
   int *stop_after = (int *) ctx_;

   (*stop_after)--;

   return *stop_after > 0;
}

static void
test_set_new (void)
{
   void *items[10];
   int i;
   int destroyed = 0;
   int visited = 0;
   int stop_after = 3;

   mongoc_set_t *set = mongoc_set_new (2, &test_set_dtor, &destroyed);

   for (i = 0; i < 5; i++) {
      mongoc_set_add (set, i, items + i);
   }

   for (i = 0; i < 5; i++) {
      BSON_ASSERT (mongoc_set_get (set, i) == items + i);
   }

   mongoc_set_rm (set, 0);

   BSON_ASSERT (destroyed == 1);

   for (i = 5; i < 10; i++) {
      mongoc_set_add (set, i, items + i);
   }

   for (i = 5; i < 10; i++) {
      BSON_ASSERT (mongoc_set_get (set, i) == items + i);
   }

   mongoc_set_rm (set, 9);
   BSON_ASSERT (destroyed == 2);
   mongoc_set_rm (set, 5);
   BSON_ASSERT (destroyed == 3);

   BSON_ASSERT (mongoc_set_get (set, 1) == items + 1);
   BSON_ASSERT (mongoc_set_get (set, 7) == items + 7);
   BSON_ASSERT (!mongoc_set_get (set, 5));

   mongoc_set_add (set, 5, items + 5);
   BSON_ASSERT (mongoc_set_get (set, 5) == items + 5);

   mongoc_set_for_each (set, test_set_visit_cb, &visited);
   BSON_ASSERT (visited == 8);

   mongoc_set_for_each (set, test_set_stop_after_cb, &stop_after);
   BSON_ASSERT (stop_after == 0);

   mongoc_set_destroy (set);
}


void
test_set_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Set/new", test_set_new);
}
