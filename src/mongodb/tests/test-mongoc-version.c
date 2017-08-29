#include <mongoc.h>

#include "TestSuite.h"

static void
test_mongoc_version (void)
{
   ASSERT_CMPINT (mongoc_get_major_version (), ==, MONGOC_MAJOR_VERSION);
   ASSERT_CMPINT (mongoc_get_minor_version (), ==, MONGOC_MINOR_VERSION);
   ASSERT_CMPINT (mongoc_get_micro_version (), ==, MONGOC_MICRO_VERSION);
   ASSERT_CMPSTR (mongoc_get_version (), MONGOC_VERSION_S);

   ASSERT (mongoc_check_version (
      MONGOC_MAJOR_VERSION, MONGOC_MINOR_VERSION, MONGOC_MICRO_VERSION));

   ASSERT (!mongoc_check_version (
      MONGOC_MAJOR_VERSION, MONGOC_MINOR_VERSION + 1, MONGOC_MICRO_VERSION));
}

void
test_version_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Version", test_mongoc_version);
}
