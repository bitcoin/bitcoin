#include "mongoc-thread-private.h"

#include "TestSuite.h"


static void
test_cond_wait (void)
{
   int64_t start, duration_usec;
   mongoc_mutex_t mutex;
   mongoc_cond_t cond;

   mongoc_mutex_init (&mutex);
   mongoc_cond_init (&cond);

   mongoc_mutex_lock (&mutex);
   start = bson_get_monotonic_time ();
   mongoc_cond_timedwait (&cond, &mutex, 100);
   duration_usec = bson_get_monotonic_time () - start;
   mongoc_mutex_unlock (&mutex);

   if (!((50 * 1000 < duration_usec) && (150 * 1000 > duration_usec))) {
      fprintf (
         stderr, "expected to wait 100ms, waited %" PRId64 "\n", duration_usec);
   }

   mongoc_cond_destroy (&cond);
   mongoc_mutex_destroy (&mutex);
}


void
test_thread_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Thread/cond_wait", test_cond_wait);
}
