#include <bson.h>

#include "TestSuite.h"

static void
test_bson_version (void)
{
   ASSERT_CMPINT (bson_get_major_version (), ==, BSON_MAJOR_VERSION);
   ASSERT_CMPINT (bson_get_minor_version (), ==, BSON_MINOR_VERSION);
   ASSERT_CMPINT (bson_get_micro_version (), ==, BSON_MICRO_VERSION);
   ASSERT_CMPSTR (bson_get_version (), BSON_VERSION_S);

   ASSERT (bson_check_version (
      BSON_MAJOR_VERSION, BSON_MINOR_VERSION, BSON_MICRO_VERSION));

   ASSERT (!bson_check_version (
      BSON_MAJOR_VERSION, BSON_MINOR_VERSION + 1, BSON_MICRO_VERSION));
}

void
test_version_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/version", test_bson_version);
}
