#include "mongoc-util-private.h"
#include "TestSuite.h"


static void
test_mongoc_usleep_basic (void)
{
   int64_t start;
   int64_t duration;

   start = bson_get_monotonic_time ();
   _mongoc_usleep (50 * 1000); /* 50 ms */
   duration = bson_get_monotonic_time () - start;
   ASSERT_CMPINT ((int) duration, >, 0);
   ASSERT_CMPTIME ((int) duration, 200 * 1000);
}

void
test_usleep_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Sleep/basic", test_mongoc_usleep_basic);
}
