#include "TestSuite.h"
#include "bson-config.h"


extern void
test_atomic_install (TestSuite *suite);
extern void
test_bson_corpus_install (TestSuite *suite);
extern void
test_bcon_basic_install (TestSuite *suite);
extern void
test_bcon_extract_install (TestSuite *suite);
extern void
test_bson_install (TestSuite *suite);
extern void
test_clock_install (TestSuite *suite);
extern void
test_decimal128_install (TestSuite *suite);
extern void
test_endian_install (TestSuite *suite);
extern void
test_error_install (TestSuite *suite);
extern void
test_iso8601_install (TestSuite *suite);
extern void
test_iter_install (TestSuite *suite);
extern void
test_json_install (TestSuite *suite);
extern void
test_oid_install (TestSuite *suite);
extern void
test_reader_install (TestSuite *suite);
extern void
test_string_install (TestSuite *suite);
extern void
test_utf8_install (TestSuite *suite);
extern void
test_value_install (TestSuite *suite);
extern void
test_version_install (TestSuite *suite);
extern void
test_writer_install (TestSuite *suite);


int
main (int argc, char *argv[])
{
   TestSuite suite;
   int ret;

   TestSuite_Init (&suite, "", argc, argv);

   test_atomic_install (&suite);
   test_bson_corpus_install (&suite);
   test_bcon_basic_install (&suite);
   test_bcon_extract_install (&suite);
   test_bson_install (&suite);
   test_clock_install (&suite);
   test_error_install (&suite);
   test_endian_install (&suite);
   test_iso8601_install (&suite);
   test_iter_install (&suite);
   test_json_install (&suite);
   test_oid_install (&suite);
   test_reader_install (&suite);
   test_string_install (&suite);
   test_utf8_install (&suite);
   test_value_install (&suite);
   test_version_install (&suite);
   test_writer_install (&suite);
   test_decimal128_install (&suite);

   ret = TestSuite_Run (&suite);

   TestSuite_Destroy (&suite);

   return ret;
}
