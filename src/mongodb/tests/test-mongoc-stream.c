
#include <fcntl.h>
#include <mongoc.h>
#include <mongoc-stream-private.h>
#include <stdlib.h>

#include "TestSuite.h"


static void
test_buffered_basic (void)
{
   mongoc_stream_t *stream;
   mongoc_stream_t *buffered;
   mongoc_iovec_t iov;
   ssize_t r;
   char buf[16236];

   stream =
      mongoc_stream_file_new_for_path (BINARY_DIR "/reply2.dat", O_RDONLY, 0);
   BSON_ASSERT (stream);

   /* buffered assumes ownership of stream */
   buffered = mongoc_stream_buffered_new (stream, 1024);

   /* try to read large chunk larger than buffer. */
   iov.iov_len = sizeof buf;
   iov.iov_base = buf;
   r = mongoc_stream_readv (buffered, &iov, 1, iov.iov_len, -1);
   if (r != iov.iov_len) {
      char msg[100];

      bson_snprintf (msg,
                     100,
                     "Expected %lld got %llu",
                     (long long) r,
                     (unsigned long long) iov.iov_len);
      ASSERT_CMPSTR (msg, "failed");
   }

   /* cleanup */
   mongoc_stream_destroy (buffered);
}


static void
test_buffered_oversized (void)
{
   mongoc_stream_t *stream;
   mongoc_stream_t *buffered;
   mongoc_iovec_t iov;
   ssize_t r;
   char buf[16236];

   stream =
      mongoc_stream_file_new_for_path (BINARY_DIR "/reply2.dat", O_RDONLY, 0);
   BSON_ASSERT (stream);

   /* buffered assumes ownership of stream */
   buffered = mongoc_stream_buffered_new (stream, 20000);

   /* try to read large chunk larger than buffer. */
   iov.iov_len = sizeof buf;
   iov.iov_base = buf;
   r = mongoc_stream_readv (buffered, &iov, 1, iov.iov_len, -1);
   if (r != iov.iov_len) {
      char msg[100];

      bson_snprintf (msg,
                     100,
                     "Expected %lld got %llu",
                     (long long) r,
                     (unsigned long long) iov.iov_len);
      ASSERT_CMPSTR (msg, "failed");
   }

   /* cleanup */
   mongoc_stream_destroy (buffered);
}


typedef struct {
   mongoc_stream_t vtable;
   ssize_t rval;
} failing_stream_t;

static ssize_t
failing_stream_writev (mongoc_stream_t *stream,
                       mongoc_iovec_t *iov,
                       size_t iovcnt,
                       int32_t timeout_msec)
{
   failing_stream_t *fstream = (failing_stream_t *) stream;

   return fstream->rval;
}

void
failing_stream_destroy (mongoc_stream_t *stream)
{
   bson_free (stream);
}

static mongoc_stream_t *
failing_stream_new (ssize_t rval)
{
   failing_stream_t *stream;

   stream = bson_malloc0 (sizeof *stream);
   stream->vtable.type = 999;
   stream->vtable.writev = failing_stream_writev;
   stream->vtable.destroy = failing_stream_destroy;
   stream->rval = rval;

   return (mongoc_stream_t *) stream;
}


static void
test_stream_writev_full (void)
{
   mongoc_stream_t *error_stream = failing_stream_new (-1);
   mongoc_stream_t *short_stream = failing_stream_new (10);
   mongoc_stream_t *success_stream = failing_stream_new (100);
   char bufa[20];
   char bufb[80];
   bool r;
   mongoc_iovec_t iov[2];
   bson_error_t error = {0};
   const char *error_message = "Failure during socket delivery: ";
   const char *short_message = "Failure to send all requested bytes (only "
                               "sent: 10/100 in 100ms) during socket delivery";

   iov[0].iov_base = bufa;
   iov[0].iov_len = sizeof (bufa);
   iov[1].iov_base = bufb;
   iov[1].iov_len = sizeof (bufb);

   errno = EINVAL;
   r = _mongoc_stream_writev_full (error_stream, iov, 2, 100, &error);

   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_STREAM);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_STREAM_SOCKET);
   ASSERT_STARTSWITH (error.message, error_message);

   errno = 0;
   r = _mongoc_stream_writev_full (short_stream, iov, 2, 100, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_STREAM);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_STREAM_SOCKET);
   ASSERT_CMPSTR (error.message, short_message);

   errno = 0;
   r = _mongoc_stream_writev_full (success_stream, iov, 2, 100, &error);
   BSON_ASSERT (r);

   mongoc_stream_destroy (error_stream);
   mongoc_stream_destroy (short_stream);
   mongoc_stream_destroy (success_stream);
}


void
test_stream_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Stream/buffered/basic", test_buffered_basic);
   TestSuite_Add (suite, "/Stream/buffered/oversized", test_buffered_oversized);
   TestSuite_Add (suite, "/Stream/writev_full", test_stream_writev_full);
}
