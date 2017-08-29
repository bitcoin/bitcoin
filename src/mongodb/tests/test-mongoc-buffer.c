#include <fcntl.h>
#include <mongoc.h>
#include <mongoc-buffer-private.h>

#include "TestSuite.h"


static void
test_mongoc_buffer_basic (void)
{
   mongoc_stream_t *stream;
   mongoc_buffer_t buf;
   bson_error_t error = {0};
   uint8_t *data = (uint8_t *) bson_malloc0 (1024);
   ssize_t r;

   stream =
      mongoc_stream_file_new_for_path (BINARY_DIR "/reply1.dat", O_RDONLY, 0);
   ASSERT (stream);

   _mongoc_buffer_init (&buf, data, 1024, NULL, NULL);

   r = _mongoc_buffer_fill (&buf, stream, 537, 0, &error);
   ASSERT_CMPINT ((int) r, ==, -1);
   r = _mongoc_buffer_fill (&buf, stream, 536, 0, &error);
   ASSERT_CMPINT ((int) r, ==, 536);
   ASSERT (buf.len == 536);

   _mongoc_buffer_destroy (&buf);
   _mongoc_buffer_destroy (&buf);
   _mongoc_buffer_destroy (&buf);
   _mongoc_buffer_destroy (&buf);

   mongoc_stream_destroy (stream);
}


void
test_buffer_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Buffer/Basic", test_mongoc_buffer_basic);
}
