#include <fcntl.h>
#include <mongoc.h>
#include <mongoc-array-private.h>
#include <mongoc-rpc-private.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TestSuite.h"
#include "mongoc-cluster-private.h"


static uint8_t *
get_test_file (const char *filename, size_t *length)
{
   ssize_t len;
   uint8_t *buf;
   char real_filename[256];
   int fd;

   bson_snprintf (
      real_filename, sizeof real_filename, BINARY_DIR "/%s", filename);

#ifdef _WIN32
   fd = _open (real_filename, O_RDONLY | _O_BINARY);
#else
   fd = open (real_filename, O_RDONLY);
#endif

   if (fd == -1) {
      fprintf (stderr, "Failed to open: %s\n", real_filename);
      abort ();
   }

   len = 40960;
   buf = (uint8_t *) bson_malloc0 (len);
#ifdef _WIN32
   len = _read (fd, buf, (uint32_t) len);
#else
   len = read (fd, buf, (uint32_t) len);
#endif
   ASSERT (len > 0);

   *length = len;
   return buf;
}


/*
 * This function expects that @rpc is in HOST ENDIAN format.
 */
static void
assert_rpc_equal (const char *filename, mongoc_rpc_t *rpc)
{
   mongoc_array_t ar;
   uint8_t *data;
   mongoc_iovec_t *iov;
   size_t length;
   off_t off = 0;
   int r;
   int i;

   data = get_test_file (filename, &length);
   _mongoc_array_init (&ar, sizeof (mongoc_iovec_t));

   /*
    * Gather our RPC into a series of iovec that can be compared
    * to the buffer from the RCP snapshot file.
    */
   _mongoc_rpc_gather (rpc, &ar);

#if 0
   fprintf(stderr, "Before swabbing\n");
   fprintf(stderr, "=========================\n");
   mongoc_rpc_printf(rpc);
#endif

   _mongoc_rpc_swab_to_le (rpc);

#if 0
   fprintf(stderr, "After swabbing\n");
   fprintf(stderr, "=========================\n");
   mongoc_rpc_printf(rpc);
#endif

   for (i = 0; i < ar.len; i++) {
      iov = &_mongoc_array_index (&ar, mongoc_iovec_t, i);
      ASSERT (iov->iov_len <= (length - off));
      r = memcmp (&data[off], iov->iov_base, iov->iov_len);
      if (r) {
         fprintf (stderr, "\nError iovec: %u\n", i);
      }
      ASSERT (r == 0);
      off += iov->iov_len;
   }

   _mongoc_array_destroy (&ar);
   bson_free (data);
}


static void
test_mongoc_rpc_delete_gather (void)
{
   mongoc_rpc_t rpc;
   bson_t sel;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   bson_init (&sel);

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_DELETE;
   rpc.delete_.zero = 0;
   rpc.delete_.collection = "test.test";
   rpc.delete_.flags = MONGOC_DELETE_SINGLE_REMOVE;
   rpc.delete_.selector = bson_get_data (&sel);

   assert_rpc_equal ("delete1.dat", &rpc);
}


static void
test_mongoc_rpc_delete_scatter (void)
{
   uint8_t *data;
   mongoc_rpc_t rpc;
   bool r;
   bson_t sel;
   size_t length;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   bson_init (&sel);

   data = get_test_file ("delete1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT_CMPINT (rpc.header.msg_len, ==, 39);
   ASSERT_CMPINT (rpc.header.request_id, ==, 1234);
   ASSERT_CMPINT (rpc.header.response_to, ==, -1);
   ASSERT_CMPINT (rpc.header.opcode, ==, MONGOC_OPCODE_DELETE);
   ASSERT_CMPINT (rpc.delete_.zero, ==, 0);
   ASSERT (!strcmp ("test.test", rpc.delete_.collection));
   ASSERT_CMPINT (rpc.delete_.flags, ==, MONGOC_DELETE_SINGLE_REMOVE);
   ASSERT (!memcmp (rpc.delete_.selector, bson_get_data (&sel), sel.len));

   assert_rpc_equal ("delete1.dat", &rpc);
   bson_free (data);
}


static void
test_mongoc_rpc_get_more_gather (void)
{
   mongoc_rpc_t rpc;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_GET_MORE;
   rpc.get_more.zero = 0;
   rpc.get_more.collection = "test.test";
   rpc.get_more.n_return = 5;
   rpc.get_more.cursor_id = 12345678L;

   assert_rpc_equal ("get_more1.dat", &rpc);
}


static void
test_mongoc_rpc_get_more_scatter (void)
{
   uint8_t *data;
   mongoc_rpc_t rpc;
   bool r;
   size_t length;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   data = get_test_file ("get_more1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT (rpc.header.msg_len == 42);
   ASSERT (rpc.header.request_id == 1234);
   ASSERT (rpc.header.response_to == -1);
   ASSERT (rpc.header.opcode == MONGOC_OPCODE_GET_MORE);
   ASSERT (rpc.get_more.zero == 0);
   ASSERT (!strcmp ("test.test", rpc.get_more.collection));
   ASSERT (rpc.get_more.n_return == 5);
   ASSERT (rpc.get_more.cursor_id == 12345678);

   assert_rpc_equal ("get_more1.dat", &rpc);
   bson_free (data);
}


static void
test_mongoc_rpc_insert_gather (void)
{
   mongoc_rpc_t rpc;
   mongoc_iovec_t iov[20];
   bson_t b;
   int i;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   bson_init (&b);

   for (i = 0; i < 20; i++) {
      iov[i].iov_base = (void *) bson_get_data (&b);
      iov[i].iov_len = b.len;
   }

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_INSERT;
   rpc.insert.flags = MONGOC_INSERT_CONTINUE_ON_ERROR;
   rpc.insert.collection = "test.test";
   rpc.insert.documents = iov;
   rpc.insert.n_documents = 20;

   assert_rpc_equal ("insert1.dat", &rpc);
   bson_destroy (&b);
}


static void
test_mongoc_rpc_insert_scatter (void)
{
   bson_reader_t *reader;
   uint8_t *data;
   const bson_t *b;
   mongoc_rpc_t rpc;
   bool r;
   bool eof = false;
   size_t length;
   bson_t empty;
   int count = 0;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   bson_init (&empty);

   data = get_test_file ("insert1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT_CMPINT (rpc.header.msg_len, ==, 130);
   ASSERT_CMPINT (rpc.header.request_id, ==, 1234);
   ASSERT_CMPINT (rpc.header.response_to, ==, (uint32_t) -1);
   ASSERT_CMPINT (rpc.header.opcode, ==, MONGOC_OPCODE_INSERT);
   ASSERT_CMPINT (rpc.insert.flags, ==, MONGOC_INSERT_CONTINUE_ON_ERROR);
   ASSERT (!strcmp ("test.test", rpc.insert.collection));
   reader =
      bson_reader_new_from_data ((uint8_t *) rpc.insert.documents[0].iov_base,
                                 rpc.insert.documents[0].iov_len);
   while ((b = bson_reader_read (reader, &eof))) {
      r = bson_equal (b, &empty);
      ASSERT (r);
      count++;
   }
   ASSERT (eof == true);
   ASSERT (count == 20);

   assert_rpc_equal ("insert1.dat", &rpc);
   bson_free (data);
   bson_reader_destroy (reader);
   bson_destroy (&empty);
}


static void
test_mongoc_rpc_kill_cursors_gather (void)
{
   mongoc_rpc_t rpc;
   int64_t cursors[] = {1, 2, 3, 4, 5};

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_KILL_CURSORS;
   rpc.kill_cursors.zero = 0;
   rpc.kill_cursors.n_cursors = 5;
   rpc.kill_cursors.cursors = cursors;

   assert_rpc_equal ("kill_cursors1.dat", &rpc);
}


static void
test_mongoc_rpc_kill_cursors_scatter (void)
{
   uint8_t *data;
   const int64_t cursors[] = {1, 2, 3, 4, 5};
   mongoc_rpc_t rpc;
   bool r;
   size_t length;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   data = get_test_file ("kill_cursors1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT_CMPINT (rpc.header.msg_len, ==, 64);
   ASSERT_CMPINT (rpc.header.request_id, ==, 1234);
   ASSERT_CMPINT (rpc.header.response_to, ==, -1);
   ASSERT_CMPINT (rpc.header.opcode, ==, MONGOC_OPCODE_KILL_CURSORS);
   ASSERT_CMPINT (rpc.kill_cursors.zero, ==, 0);
   ASSERT_CMPINT (rpc.kill_cursors.n_cursors, ==, 5);
   ASSERT (!memcmp (rpc.kill_cursors.cursors, cursors, 5 * 8));

   assert_rpc_equal ("kill_cursors1.dat", &rpc);
   bson_free (data);
}


static void
test_mongoc_rpc_msg_gather (void)
{
   mongoc_rpc_t rpc;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_MSG;
   rpc.msg.msg = "this is a test message.";

   assert_rpc_equal ("msg1.dat", &rpc);
}


static void
test_mongoc_rpc_msg_scatter (void)
{
   uint8_t *data;
   mongoc_rpc_t rpc;
   bool r;
   size_t length;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   data = get_test_file ("msg1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT (rpc.header.msg_len == 40);
   ASSERT (rpc.header.request_id == 1234);
   ASSERT (rpc.header.response_to == -1);
   ASSERT (rpc.header.opcode == MONGOC_OPCODE_MSG);
   ASSERT (!strcmp (rpc.msg.msg, "this is a test message."));

   assert_rpc_equal ("msg1.dat", &rpc);
   bson_free (data);
}


static void
test_mongoc_rpc_query_gather (void)
{
   mongoc_rpc_t rpc;
   bson_t b;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   bson_init (&b);

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_QUERY;
   rpc.query.flags = MONGOC_QUERY_SLAVE_OK;
   rpc.query.collection = "test.test";
   rpc.query.skip = 5;
   rpc.query.n_return = 1;
   rpc.query.query = bson_get_data (&b);
   rpc.query.fields = bson_get_data (&b);

   assert_rpc_equal ("query1.dat", &rpc);
}


static void
test_mongoc_rpc_query_scatter (void)
{
   uint8_t *data;
   mongoc_rpc_t rpc;
   bool r;
   bson_t empty;
   size_t length;

   bson_init (&empty);

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   data = get_test_file ("query1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT (rpc.header.msg_len == 48);
   ASSERT (rpc.header.request_id == 1234);
   ASSERT (rpc.header.response_to == (uint32_t) -1);
   ASSERT (rpc.header.opcode == MONGOC_OPCODE_QUERY);
   ASSERT (rpc.query.flags == MONGOC_QUERY_SLAVE_OK);
   ASSERT (!strcmp (rpc.query.collection, "test.test"));
   ASSERT (rpc.query.skip == 5);
   ASSERT (rpc.query.n_return == 1);
   ASSERT (!memcmp (rpc.query.query, bson_get_data (&empty), 5));
   ASSERT (!memcmp (rpc.query.fields, bson_get_data (&empty), 5));

   assert_rpc_equal ("query1.dat", &rpc);
   bson_free (data);
}


static void
test_mongoc_rpc_reply_gather (void)
{
   bson_writer_t *writer;
   mongoc_rpc_t rpc;
   uint8_t *buf = NULL;
   size_t len = 0;
   bson_t *b;
   int i;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   writer = bson_writer_new (&buf, &len, 0, bson_realloc_ctx, NULL);
   for (i = 0; i < 100; i++) {
      bson_writer_begin (writer, &b);
      bson_writer_end (writer);
   }

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_REPLY;
   rpc.reply.flags = MONGOC_REPLY_AWAIT_CAPABLE;
   rpc.reply.cursor_id = 12345678;
   rpc.reply.start_from = 50;
   rpc.reply.n_returned = 100;
   rpc.reply.documents = buf;
   rpc.reply.documents_len = (int32_t) bson_writer_get_length (writer);

   assert_rpc_equal ("reply1.dat", &rpc);
   bson_writer_destroy (writer);
   bson_free (buf);
}


static void
test_mongoc_rpc_reply_scatter (void)
{
   bson_reader_t *reader;
   uint8_t *data;
   mongoc_rpc_t rpc;
   const bson_t *b;
   bool r;
   bool eof = false;
   bson_t empty;
   size_t length;
   int count = 0;

   bson_init (&empty);

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   data = get_test_file ("reply1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT_CMPINT (rpc.header.msg_len, ==, 536);
   ASSERT_CMPINT (rpc.header.request_id, ==, 1234);
   ASSERT_CMPINT (rpc.header.response_to, ==, -1);
   ASSERT_CMPINT (rpc.header.opcode, ==, MONGOC_OPCODE_REPLY);
   ASSERT_CMPINT (rpc.reply.flags, ==, MONGOC_REPLY_AWAIT_CAPABLE);
   ASSERT (rpc.reply.cursor_id == 12345678LL);
   ASSERT_CMPINT (rpc.reply.start_from, ==, 50);
   ASSERT_CMPINT (rpc.reply.n_returned, ==, 100);
   ASSERT_CMPINT (rpc.reply.documents_len, ==, 500);
   reader =
      bson_reader_new_from_data (rpc.reply.documents, rpc.reply.documents_len);
   while ((b = bson_reader_read (reader, &eof))) {
      r = bson_equal (b, &empty);
      ASSERT (r);
      count++;
   }
   ASSERT (eof == true);
   ASSERT (count == 100);

   assert_rpc_equal ("reply1.dat", &rpc);
   bson_reader_destroy (reader);
   bson_free (data);
}


static void
test_mongoc_rpc_reply_scatter2 (void)
{
   bson_reader_t *reader;
   uint8_t *data;
   mongoc_rpc_t rpc;
   const bson_t *b;
   bool r;
   bool eof = false;
   bson_t empty;
   size_t length;
   int count = 0;

   bson_init (&empty);

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   data = get_test_file ("reply2.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT (rpc.header.msg_len == 16236);
   ASSERT (rpc.header.request_id == 0);
   ASSERT (rpc.header.response_to == 1234);
   ASSERT (rpc.header.opcode == MONGOC_OPCODE_REPLY);
   ASSERT (rpc.reply.flags == 0);
   ASSERT (rpc.reply.cursor_id == 12345678);
   ASSERT (rpc.reply.start_from == 0);
   ASSERT (rpc.reply.n_returned == 100);
   ASSERT (rpc.reply.documents_len == 16200);
   reader =
      bson_reader_new_from_data (rpc.reply.documents, rpc.reply.documents_len);
   while ((b = bson_reader_read (reader, &eof))) {
      count++;
   }
   ASSERT (eof == true);
   ASSERT (count == 100);

   assert_rpc_equal ("reply2.dat", &rpc);
   bson_reader_destroy (reader);
   bson_free (data);
}


static void
test_mongoc_rpc_update_gather (void)
{
   mongoc_rpc_t rpc;
   bson_t sel;
   bson_t up;

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   bson_init (&sel);
   bson_init (&up);

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_UPDATE;
   rpc.update.zero = 0;
   rpc.update.collection = "test.test";
   rpc.update.flags = MONGOC_UPDATE_MULTI_UPDATE;
   rpc.update.selector = bson_get_data (&sel);
   rpc.update.update = bson_get_data (&up);

   assert_rpc_equal ("update1.dat", &rpc);
}


static void
test_mongoc_rpc_update_scatter (void)
{
   uint8_t *data;
   mongoc_rpc_t rpc;
   bool r;
   bson_t b;
   bson_t empty;
   size_t length;
   int32_t len;

   bson_init (&empty);

   memset (&rpc, 0xFFFFFFFF, sizeof rpc);

   data = get_test_file ("update1.dat", &length);
   r = _mongoc_rpc_scatter (&rpc, data, length);
   ASSERT (r);
   _mongoc_rpc_swab_from_le (&rpc);

   ASSERT_CMPINT (rpc.header.msg_len, ==, 44);
   ASSERT_CMPINT (rpc.header.request_id, ==, 1234);
   ASSERT_CMPINT (rpc.header.response_to, ==, -1);
   ASSERT_CMPINT (rpc.header.opcode, ==, MONGOC_OPCODE_UPDATE);
   ASSERT_CMPINT (rpc.update.flags, ==, MONGOC_UPDATE_MULTI_UPDATE);
   ASSERT (!strcmp (rpc.update.collection, "test.test"));

   memcpy (&len, rpc.update.selector, 4);
   len = BSON_UINT32_FROM_LE (len);
   ASSERT (len > 4);
   r = bson_init_static (&b, rpc.update.selector, len);
   ASSERT (r);
   r = bson_equal (&b, &empty);
   ASSERT (r);
   bson_destroy (&b);

   memcpy (&len, rpc.update.update, 4);
   len = BSON_UINT32_FROM_LE (len);
   ASSERT (len > 4);
   r = bson_init_static (&b, rpc.update.update, len);
   ASSERT (r);
   r = bson_equal (&b, &empty);
   ASSERT (r);
   bson_destroy (&b);

   assert_rpc_equal ("update1.dat", &rpc);
   bson_free (data);
}


static void
test_mongoc_rpc_buffer_iov (void)
{
   mongoc_array_t ar;
   mongoc_rpc_t rpc;
   bson_t b;
   size_t allocate;
   char *no_header, *full_opcode;
   char *matching_opcode;
   int size;
   mongoc_iovec_t iov;

   bson_init (&b);
   _mongoc_array_init (&ar, sizeof (mongoc_iovec_t));

   rpc.header.msg_len = 0;
   rpc.header.request_id = 1234;
   rpc.header.response_to = -1;
   rpc.header.opcode = MONGOC_OPCODE_QUERY;
   rpc.query.flags = MONGOC_QUERY_SLAVE_OK;
   rpc.query.collection = "test.test";
   rpc.query.skip = 5;
   rpc.query.n_return = 1;
   rpc.query.query = bson_get_data (&b);
   rpc.query.fields = bson_get_data (&b);

   _mongoc_rpc_gather (&rpc, &ar);

   allocate = rpc.header.msg_len - 16;

   BSON_ASSERT (allocate > 0);
   full_opcode = bson_malloc0 (allocate + 16);
   size = _mongoc_cluster_buffer_iovec (
      (mongoc_iovec_t *) ar.data, ar.len, 0, full_opcode);
   ASSERT_CMPINT (size, ==, 48);

   iov.iov_len = size;
   iov.iov_base = full_opcode;
   no_header = bson_malloc0 (allocate);
   size = _mongoc_cluster_buffer_iovec (&iov, 1, 16, no_header);

   ASSERT_CMPINT (size, ==, 32);

   matching_opcode = bson_malloc0 (rpc.header.msg_len);
   memcpy (matching_opcode, full_opcode, 16);
   memcpy (matching_opcode + 16, no_header, 32);

   ASSERT_MEMCMP (full_opcode + 16, no_header, 32);
   ASSERT_MEMCMP (matching_opcode, full_opcode, 48);

   bson_free (no_header);
   bson_free (full_opcode);
   bson_free (matching_opcode);

   _mongoc_array_destroy (&ar);
}


void
test_rpc_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Rpc/delete/gather", test_mongoc_rpc_delete_gather);
   TestSuite_Add (suite, "/Rpc/delete/scatter", test_mongoc_rpc_delete_scatter);
   TestSuite_Add (
      suite, "/Rpc/get_more/gather", test_mongoc_rpc_get_more_gather);
   TestSuite_Add (
      suite, "/Rpc/get_more/scatter", test_mongoc_rpc_get_more_scatter);
   TestSuite_Add (suite, "/Rpc/insert/gather", test_mongoc_rpc_insert_gather);
   TestSuite_Add (suite, "/Rpc/insert/scatter", test_mongoc_rpc_insert_scatter);
   TestSuite_Add (
      suite, "/Rpc/kill_cursors/gather", test_mongoc_rpc_kill_cursors_gather);
   TestSuite_Add (
      suite, "/Rpc/kill_cursors/scatter", test_mongoc_rpc_kill_cursors_scatter);
   TestSuite_Add (suite, "/Rpc/msg/gather", test_mongoc_rpc_msg_gather);
   TestSuite_Add (suite, "/Rpc/msg/scatter", test_mongoc_rpc_msg_scatter);
   TestSuite_Add (suite, "/Rpc/query/gather", test_mongoc_rpc_query_gather);
   TestSuite_Add (suite, "/Rpc/query/scatter", test_mongoc_rpc_query_scatter);
   TestSuite_Add (suite, "/Rpc/reply/gather", test_mongoc_rpc_reply_gather);
   TestSuite_Add (suite, "/Rpc/reply/scatter", test_mongoc_rpc_reply_scatter);
   TestSuite_Add (suite, "/Rpc/reply/scatter2", test_mongoc_rpc_reply_scatter2);
   TestSuite_Add (suite, "/Rpc/update/gather", test_mongoc_rpc_update_gather);
   TestSuite_Add (suite, "/Rpc/update/scatter", test_mongoc_rpc_update_scatter);
   TestSuite_Add (suite, "/Rpc/buffer/iov", test_mongoc_rpc_buffer_iov);
}
