#include "mongoc-array-private.h"
#include "TestSuite.h"


static void
test_array (void)
{
   mongoc_array_t ar;
   int i;
   int v;

   _mongoc_array_init (&ar, sizeof i);
   BSON_ASSERT (ar.element_size == sizeof i);
   BSON_ASSERT (ar.len == 0);
   BSON_ASSERT (ar.allocated);
   BSON_ASSERT (ar.data);

   for (i = 0; i < 100; i++) {
      _mongoc_array_append_val (&ar, i);
   }

   for (i = 0; i < 100; i++) {
      v = _mongoc_array_index (&ar, int, i);
      BSON_ASSERT (v == i);
   }

   BSON_ASSERT (ar.len == 100);
   BSON_ASSERT (ar.allocated >= (100 * sizeof i));

   _mongoc_array_clear (&ar);
   BSON_ASSERT (ar.len == 0);
   BSON_ASSERT (ar.allocated);
   BSON_ASSERT (ar.data);
   BSON_ASSERT (ar.element_size);

   _mongoc_array_destroy (&ar);
}


void
test_array_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Array/Basic", test_array);
}
