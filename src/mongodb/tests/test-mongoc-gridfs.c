#include <mongoc.h>
#define MONGOC_INSIDE
#include <mongoc-gridfs-file-private.h>
#include <mongoc-client-private.h>

#undef MONGOC_INSIDE

#include "test-libmongoc.h"
#include "TestSuite.h"
#include "test-conveniences.h"
#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"


static mongoc_gridfs_t *
get_test_gridfs (mongoc_client_t *client, const char *name, bson_error_t *error)
{
   char *gen;
   char n[48];
   mongoc_database_t *db;

   gen = gen_collection_name ("fs");
   bson_snprintf (n, sizeof n, "%s_%s", gen, name);
   bson_free (gen);

   db = mongoc_client_get_database (client, "test");
   mongoc_database_drop (db, NULL);
   mongoc_database_destroy (db);

   return mongoc_client_get_gridfs (client, "test", NULL, error);
}


bool
drop_collections (mongoc_gridfs_t *gridfs, bson_error_t *error)
{
   return (mongoc_collection_drop (mongoc_gridfs_get_files (gridfs), error) &&
           mongoc_collection_drop (mongoc_gridfs_get_chunks (gridfs), error));
}


static void
_check_index (mongoc_collection_t *collection, const char *index_json)
{
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *info;
   const char *index_name;
   bson_t index_key;
   int n;

   cursor = mongoc_collection_find_indexes (collection, &error);
   ASSERT_OR_PRINT (cursor, error);

   n = 0;

   while (mongoc_cursor_next (cursor, &info)) {
      index_name = bson_lookup_utf8 (info, "name");

      /* if this is NOT the "_id" index */
      if (strcmp (index_name, "_id_")) {
         bson_lookup_doc (info, "key", &index_key);
         ASSERT_MATCH (&index_key, index_json);
      }

      n++;
   }

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   /* _id index plus the expected index */
   ASSERT_CMPINT (n, ==, 2);

   mongoc_cursor_destroy (cursor);
}


static mongoc_gridfs_t *
_get_gridfs (mock_server_t *server, mongoc_client_t *client)
{
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_gridfs_t *gridfs;

   /* gridfs ensures two indexes */
   future = future_client_get_gridfs (client, "db", NULL, &error);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_NONE, "{'createIndexes': 'fs.chunks'}");

   mock_server_replies_ok_and_destroys (request);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_NONE, "{'createIndexes': 'fs.files'}");

   mock_server_replies_ok_and_destroys (request);

   gridfs = future_get_mongoc_gridfs_ptr (future);
   ASSERT (gridfs);

   future_destroy (future);

   return gridfs;
}


static void
test_create (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_collection_t *files;
   mongoc_collection_t *chunks;

   client = test_framework_client_new ();
   ASSERT (client);

   files = mongoc_client_get_collection (client, "test", "foo.files");
   chunks = mongoc_client_get_collection (client, "test", "foo.chunks");
   mongoc_collection_drop (files, NULL);
   mongoc_collection_drop (chunks, NULL);

   ASSERT_OR_PRINT (
      (gridfs = mongoc_client_get_gridfs (client, "test", "foo", &error)),
      error);

   file = mongoc_gridfs_create_file (gridfs, NULL);

   _check_index (files, "{'filename': 1, 'uploadDate': 1}");
   _check_index (chunks, "{'files_id': 1, 'n': 1}");

   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));

   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_collection_destroy (chunks);
   mongoc_collection_destroy (files);
   mongoc_client_destroy (client);
}


static void
test_remove (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_gridfs_file_opt_t opts = {0};
   mongoc_client_t *client;
   bson_error_t error;
   char name[32];

   client = test_framework_client_new ();
   ASSERT (client);

   ASSERT_OR_PRINT (
      gridfs = mongoc_client_get_gridfs (client, "test", "foo", &error), error);

   mongoc_gridfs_drop (gridfs, &error);

   bson_snprintf (name, sizeof name, "test-remove.%u", rand ());
   opts.filename = name;

   file = mongoc_gridfs_create_file (gridfs, &opts);
   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));

   ASSERT_OR_PRINT (mongoc_gridfs_file_remove (file, &error), error);

   mongoc_gridfs_file_destroy (file);

   file = mongoc_gridfs_find_one_by_filename (gridfs, name, &error);
   ASSERT (!file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_client_destroy (client);
}


static void
prep_files (mongoc_gridfs_t *gridfs)
{
   mongoc_gridfs_file_t *file;
   mongoc_gridfs_file_opt_t opt = {0};
   char buf[100];
   int i = 0;

   for (i = 0; i < 3; i++) {
      bson_snprintf (buf, sizeof buf, "file.%d", i);
      opt.filename = buf;
      file = mongoc_gridfs_create_file (gridfs, &opt);
      ASSERT (file);
      ASSERT (mongoc_gridfs_file_save (file));
      mongoc_gridfs_file_destroy (file);
   }
}


static void
test_list (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_gridfs_file_list_t *list;
   bson_t query, child;
   char buf[100];
   int i = 0;

   client = test_framework_client_new ();
   ASSERT (client);

   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "list", &error), error);

   prep_files (gridfs);

   bson_init (&query);
   bson_append_document_begin (&query, "$orderby", -1, &child);
   bson_append_int32 (&child, "filename", -1, 1);
   bson_append_document_end (&query, &child);
   bson_append_document_begin (&query, "$query", -1, &child);
   bson_append_document_end (&query, &child);

   list = mongoc_gridfs_find (gridfs, &query);

   bson_destroy (&query);

   i = 0;
   while ((file = mongoc_gridfs_file_list_next (list))) {
      bson_snprintf (buf, sizeof buf, "file.%d", i++);

      ASSERT_CMPINT (
         strcmp (mongoc_gridfs_file_get_filename (file), buf), ==, 0);

      mongoc_gridfs_file_destroy (file);
   }
   ASSERT_CMPINT (i, ==, 3);
   mongoc_gridfs_file_list_destroy (list);

   bson_init (&query);
   bson_append_utf8 (&query, "filename", -1, "file.1", -1);
   ASSERT_OR_PRINT (file = mongoc_gridfs_find_one (gridfs, &query, &error),
                    error);

   ASSERT_CMPINT (
      strcmp (mongoc_gridfs_file_get_filename (file), "file.1"), ==, 0);
   mongoc_gridfs_file_destroy (file);

   ASSERT_OR_PRINT (
      file = mongoc_gridfs_find_one_by_filename (gridfs, "file.1", &error),
      error);

   ASSERT_CMPINT (
      strcmp (mongoc_gridfs_file_get_filename (file), "file.1"), ==, 0);
   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_client_destroy (client);
}

static void
test_find_with_opts (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_gridfs_file_list_t *list;

   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   gridfs = get_test_gridfs (client, "test_find_with_opts", &error);
   ASSERT_OR_PRINT (gridfs, error);

   prep_files (gridfs);

   list = mongoc_gridfs_find_with_opts (
      gridfs,
      tmp_bson ("{'filename': {'$ne': 'file.1'}}"),
      tmp_bson ("{'sort': {'filename': -1}}"));

   file = mongoc_gridfs_file_list_next (list);
   ASSERT (file);
   ASSERT_CMPSTR ("file.2", mongoc_gridfs_file_get_filename (file));
   mongoc_gridfs_file_destroy (file);

   file = mongoc_gridfs_file_list_next (list);
   ASSERT (file);
   ASSERT_CMPSTR ("file.0", mongoc_gridfs_file_get_filename (file));
   mongoc_gridfs_file_destroy (file);

   file = mongoc_gridfs_file_list_next (list);
   ASSERT (!file); /* done */
   ASSERT (!mongoc_gridfs_file_list_error (list, &error));
   mongoc_gridfs_file_list_destroy (list);

   file = mongoc_gridfs_find_one_with_opts (
      gridfs, tmp_bson (NULL), tmp_bson ("{'sort': {'filename': -1}}"), &error);

   ASSERT_OR_PRINT (file, error);
   /* file.2 is first, according to sort order */
   ASSERT_CMPSTR ("file.2", mongoc_gridfs_file_get_filename (file));
   mongoc_gridfs_file_destroy (file);

   file = mongoc_gridfs_find_one_with_opts (
      gridfs, tmp_bson ("{'x': {'$bad_operator': 1}}"), NULL, &error);

   ASSERT (!file);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER);

   /* ensure "error" is cleared if we successfully find no file */
   file = mongoc_gridfs_find_one_with_opts (
      gridfs, tmp_bson ("{'x': 'doesntexist'}"), NULL, &error);

   ASSERT (!file);
   ASSERT_CMPINT (error.domain, ==, 0);
   ASSERT_CMPINT (error.code, ==, 0);
   ASSERT_CMPSTR (error.message, "");

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


/* mongoc_gridfs_find_one_with_opts uses limit 1, no matter what's in "opts" */
static void
test_find_one_with_opts_limit (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   bson_error_t error;
   future_t *future;
   request_t *request;

   server = mock_server_with_autoismaster (WIRE_VERSION_FIND_CMD);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   mongoc_client_set_error_api (client, 2);

   gridfs = _get_gridfs (server, client);

   future =
      future_gridfs_find_one_with_opts (gridfs, tmp_bson ("{}"), NULL, &error);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'find': 'fs.files', 'filter': {}, 'limit': 1}");

   mock_server_replies_to_find (request,
                                MONGOC_QUERY_SLAVE_OK,
                                0 /* cursor_id */,
                                1 /* num returned */,
                                "db.fs.files",
                                "{'_id': 1, 'length': 1, 'chunkSize': 1}",
                                true /* is_command */);

   file = future_get_mongoc_gridfs_file_ptr (future);
   ASSERT (file);

   mongoc_gridfs_file_destroy (file);
   future_destroy (future);
   request_destroy (request);

   future = future_gridfs_find_one_with_opts (
      gridfs, tmp_bson ("{}"), tmp_bson ("{'limit': 2}"), &error);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'find': 'fs.files', 'filter': {}, 'limit': 1}");

   mock_server_replies_to_find (request,
                                MONGOC_QUERY_SLAVE_OK,
                                0 /* cursor_id */,
                                1 /* num returned */,
                                "db.fs.files",
                                "{'_id': 1, 'length': 1, 'chunkSize': 1}",
                                true /* is_command */);

   file = future_get_mongoc_gridfs_file_ptr (future);
   ASSERT (file);

   mongoc_gridfs_file_destroy (file);
   future_destroy (future);
   request_destroy (request);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_properties (void)
{
   mongoc_client_t *client;
   mongoc_gridfs_t *gridfs;
   bson_error_t error;
   bson_t *doc_in;
   mongoc_gridfs_file_t *file;
   mongoc_gridfs_file_list_t *list;
   bson_t query = BSON_INITIALIZER;
   const bson_value_t *file_id;
   const char *alias0, *alias1;

   client = test_framework_client_new ();

   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "list", &error), error);

   mongoc_gridfs_drop (gridfs, &error);

   /* the C Driver sets _id to an ObjectId, but other drivers can do anything */
   doc_in = BCON_NEW ("_id",
                      BCON_INT32 (1),
                      "md5",
                      BCON_UTF8 ("md5"),
                      "filename",
                      BCON_UTF8 ("filename"),
                      "contentType",
                      BCON_UTF8 ("content_type"),
                      "aliases",
                      "[",
                      BCON_UTF8 ("alias0"),
                      BCON_UTF8 ("alias1"),
                      "]",
                      "metadata",
                      "{",
                      "key",
                      BCON_UTF8 ("value"),
                      "}",
                      "chunkSize",
                      BCON_INT32 (100));

   ASSERT (mongoc_collection_insert (mongoc_gridfs_get_files (gridfs),
                                     MONGOC_INSERT_NONE,
                                     doc_in,
                                     NULL,
                                     NULL));

   list = mongoc_gridfs_find (gridfs, &query);
   file = mongoc_gridfs_file_list_next (list);
   file_id = mongoc_gridfs_file_get_id (file);
   ASSERT (file_id);
   ASSERT_CMPINT (BSON_TYPE_INT32, ==, file_id->value_type);
   ASSERT_CMPINT (1, ==, file_id->value.v_int32);
   ASSERT_CMPSTR ("md5", mongoc_gridfs_file_get_md5 (file));
   ASSERT_CMPSTR ("filename", mongoc_gridfs_file_get_filename (file));
   ASSERT_CMPSTR ("content_type", mongoc_gridfs_file_get_content_type (file));
   ASSERT (BCON_EXTRACT ((bson_t *) mongoc_gridfs_file_get_aliases (file),
                         "0",
                         BCONE_UTF8 (alias0),
                         "1",
                         BCONE_UTF8 (alias1)));

   ASSERT_CMPSTR ("alias0", alias0);
   ASSERT_CMPSTR ("alias1", alias1);

   drop_collections (gridfs, &error);
   mongoc_gridfs_file_destroy (file);
   mongoc_gridfs_file_list_destroy (list);
   bson_destroy (doc_in);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


static void
test_create_from_stream (void)
{
   int64_t start;
   int64_t now;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   bson_t *filter;
   mongoc_gridfs_file_t *file2;
   mongoc_stream_t *stream;
   mongoc_client_t *client;
   bson_error_t error;

   client = test_framework_client_new ();
   ASSERT (client);

   ASSERT_OR_PRINT ((gridfs = get_test_gridfs (client, "from_stream", &error)),
                    error);

   mongoc_gridfs_drop (gridfs, &error);

   start = ((int64_t) time (NULL)) * 1000;
   stream =
      mongoc_stream_file_new_for_path (BINARY_DIR "/gridfs.dat", O_RDONLY, 0);
   ASSERT_OR_PRINT_ERRNO (stream, errno);
   ASSERT (stream);

   file = mongoc_gridfs_create_file_from_stream (gridfs, stream, NULL);
   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));

   now = ((int64_t) time (NULL)) * 1000;

   filter = tmp_bson (NULL);
   BSON_APPEND_VALUE (filter, "_id", mongoc_gridfs_file_get_id (file));
   file2 = mongoc_gridfs_find_one_with_opts (gridfs, filter, NULL, &error);
   ASSERT_OR_PRINT (file2, error);
   ASSERT_CMPINT64 (start, <=, mongoc_gridfs_file_get_upload_date (file2));
   ASSERT_CMPINT64 (now, >=, mongoc_gridfs_file_get_upload_date (file2));

   mongoc_gridfs_file_destroy (file2);
   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


static void
test_seek (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_stream_t *stream;
   mongoc_client_t *client;
   bson_error_t error;

   client = test_framework_client_new ();

   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "seek", &error), error);

   mongoc_gridfs_drop (gridfs, &error);

   stream = mongoc_stream_file_new_for_path (
      BINARY_DIR "/gridfs-large.dat", O_RDONLY, 0);

   file = mongoc_gridfs_create_file_from_stream (gridfs, stream, NULL);
   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_SET), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) 0);

   ASSERT_CMPINT (
      mongoc_gridfs_file_seek (file, file->chunk_size + 1, SEEK_CUR), ==, 0);
   ASSERT_CMPINT64 (
      mongoc_gridfs_file_tell (file), ==, (uint64_t) (file->chunk_size + 1));

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_END), ==, 0);
   ASSERT_CMPINT64 (
      mongoc_gridfs_file_tell (file), ==, mongoc_gridfs_file_get_length (file));

   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_client_destroy (client);
}


static void
test_read (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_stream_t *stream;
   mongoc_client_t *client;
   bson_error_t error;
   ssize_t r;
   char buf[10], buf2[10];
   mongoc_iovec_t iov[2];
   int previous_errno;
   ssize_t twenty = 20L;

   iov[0].iov_base = buf;
   iov[0].iov_len = 10;

   iov[1].iov_base = buf2;
   iov[1].iov_len = 10;

   client = test_framework_client_new ();
   ASSERT (client);

   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "read", &error), error);

   mongoc_gridfs_drop (gridfs, &error);

   stream = mongoc_stream_file_new_for_path (
      BINARY_DIR "/gridfs-large.dat", O_RDONLY, 0);

   file = mongoc_gridfs_create_file_from_stream (gridfs, stream, NULL);
   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));

   r = mongoc_gridfs_file_readv (file, iov, 2, 20, 0);
   ASSERT_CMPSSIZE_T (r, ==, twenty);
   ASSERT_MEMCMP (iov[0].iov_base, "Bacon ipsu", 10);
   ASSERT_MEMCMP (iov[1].iov_base, "m dolor si", 10);

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 1, SEEK_SET), ==, 0);
   r = mongoc_gridfs_file_readv (file, iov, 2, 20, 0);

   ASSERT_CMPSSIZE_T (r, ==, twenty);
   ASSERT_MEMCMP (iov[0].iov_base, "acon ipsum", 10);
   ASSERT_MEMCMP (iov[1].iov_base, " dolor sit", 10);

   ASSERT_CMPINT (
      mongoc_gridfs_file_seek (file, file->chunk_size - 1, SEEK_SET), ==, 0);
   r = mongoc_gridfs_file_readv (file, iov, 2, 20, 0);

   ASSERT_CMPSSIZE_T (r, ==, twenty);
   ASSERT_CMPINT64 (
      mongoc_gridfs_file_tell (file), ==, (uint64_t) (file->chunk_size + 19));
   ASSERT_MEMCMP (iov[0].iov_base, "turducken ", 10);
   ASSERT_MEMCMP (iov[1].iov_base, "spare ribs", 10);

   BSON_ASSERT (mongoc_gridfs_file_seek (file, 20, SEEK_END) == 0);
   previous_errno = errno;
   r = mongoc_gridfs_file_readv (file, iov, 2, 20, 0);

   BSON_ASSERT (errno == previous_errno);
   BSON_ASSERT (r == 0);
   BSON_ASSERT (mongoc_gridfs_file_tell (file) == file->length + 20);

   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_client_destroy (client);
}


static void
_check_chunk_count (mongoc_gridfs_t *gridfs, int64_t len, int64_t chunk_size)
{
   int64_t expected_chunks;
   int64_t cnt;
   bson_error_t error;

   /* division, rounding up */
   expected_chunks = (len + chunk_size - 1) / chunk_size;
   cnt = mongoc_collection_count (mongoc_gridfs_get_chunks (gridfs),
                                  MONGOC_QUERY_NONE,
                                  tmp_bson (NULL),
                                  0,
                                  0,
                                  NULL,
                                  &error);

   ASSERT_CMPINT64 (expected_chunks, ==, cnt);
}

static void
_test_write (bool at_boundary)
{
   ssize_t seek_len = at_boundary ? 5 : 6;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_client_t *client;
   bson_error_t error;
   ssize_t r;
   char buf[] = "foo bar";
   char buf2[] = " baz";
   char buf3[1000];
   char expected[1000] = {0};
   mongoc_gridfs_file_opt_t opt = {0};
   mongoc_iovec_t iov[2];
   mongoc_iovec_t riov;
   ssize_t len = sizeof buf + sizeof buf2 - 2;

   iov[0].iov_base = buf;
   iov[0].iov_len = sizeof (buf) - 1;
   iov[1].iov_base = buf2;
   iov[1].iov_len = sizeof (buf2) - 1;

   riov.iov_base = buf3;
   riov.iov_len = sizeof (buf3);

   opt.chunk_size = 2;

   client = test_framework_client_new ();
   ASSERT (client);

   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "write", &error), error);

   mongoc_gridfs_drop (gridfs, &error);

   file = mongoc_gridfs_create_file (gridfs, &opt);
   ASSERT (file);

   /* Test a write across many pages */
   r = mongoc_gridfs_file_writev (file, iov, 2, 0);
   ASSERT_CMPSSIZE_T (r, ==, len);

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_SET), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) 0);

   r = mongoc_gridfs_file_readv (file, &riov, 1, len, 0);
   ASSERT_CMPSSIZE_T (r, ==, len);
   ASSERT_CMPINT (memcmp (buf3, "foo bar baz", len), ==, 0);

   ASSERT_CMPINT (
      mongoc_gridfs_file_seek (file, file->chunk_size, SEEK_SET), ==, 0);
   ASSERT_CMPUINT64 (
      mongoc_gridfs_file_tell (file), ==, (uint64_t) (file->chunk_size));

   r = mongoc_gridfs_file_writev (file, iov + 1, 1, 0);
   ASSERT_CMPSSIZE_T (r, ==, iov[1].iov_len);

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_SET), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) 0);

   r = mongoc_gridfs_file_readv (file, &riov, 1, len, 0);
   ASSERT_CMPSSIZE_T (r, ==, len);
   ASSERT_CMPINT (memcmp (buf3, "fo bazr baz", len), ==, 0);
   _check_chunk_count (gridfs, len, file->chunk_size);

   /* Test writing beyond the end of the file */
   BSON_ASSERT (mongoc_gridfs_file_seek (file, seek_len, SEEK_END) == 0);
   BSON_ASSERT (mongoc_gridfs_file_tell (file) == file->length + seek_len);

   r = mongoc_gridfs_file_writev (file, iov, 2, 0);
   BSON_ASSERT (r == len);
   BSON_ASSERT (mongoc_gridfs_file_tell (file) == 2 * len + seek_len);
   BSON_ASSERT (file->length == 2 * len + seek_len);
   BSON_ASSERT (mongoc_gridfs_file_save (file));
   _check_chunk_count (gridfs, 2 * len + seek_len, file->chunk_size);

   BSON_ASSERT (mongoc_gridfs_file_seek (file, 0, SEEK_SET) == 0);
   BSON_ASSERT (mongoc_gridfs_file_tell (file) == 0);

   r = mongoc_gridfs_file_readv (file, &riov, 1, 2 * len + seek_len, 0);
   BSON_ASSERT (r == 2 * len + seek_len);

   /* expect file to be like "fo bazr baz\0\0\0\0\0\0foo bar baz" */
   bson_snprintf (expected, strlen ("fo bazr baz") + 1, "fo bazr baz");
   bson_snprintf (expected + strlen ("fo bazr baz") + seek_len,
                  strlen ("foo bar baz") + 1,
                  "foo bar baz");

   BSON_ASSERT (memcmp (buf3, expected, (size_t) (2 * len + seek_len)) == 0);
   BSON_ASSERT (mongoc_gridfs_file_save (file));

   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_client_destroy (client);
}


static void
test_write (void)
{
   _test_write (false /* at_boundary */);
}


/* Test a write starting and ending exactly on chunk boundaries */
static void
test_write_at_boundary (void)
{
   _test_write (true /* at_boundary */);
}


static void
test_write_past_end (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_client_t *client;
   bson_error_t error;
   ssize_t r;
   char buf[] = "foo";
   char read_buf[2000];
   mongoc_gridfs_file_opt_t opt = {0};
   mongoc_iovec_t iov[1];
   mongoc_iovec_t riov;
   const size_t len = sizeof (buf) - 1;
   const int64_t delta = 35;
   const uint32_t chunk_sz = 10;
   /* division, rounding up */
   const int64_t expected_chunks = ((delta + len) + (chunk_sz - 1)) / chunk_sz;
   int64_t cnt;

   iov[0].iov_base = buf;
   iov[0].iov_len = sizeof (buf) - 1;

   riov.iov_base = read_buf;
   riov.iov_len = sizeof (read_buf);

   opt.chunk_size = chunk_sz;
   opt.filename = "foo";

   client = test_framework_client_new ();
   ASSERT (client);

   gridfs = get_test_gridfs (client, "write_past_end", &error);
   ASSERT_OR_PRINT (gridfs, error);

   file = mongoc_gridfs_create_file (gridfs, &opt);
   ASSERT (file);

   r = mongoc_gridfs_file_writev (file, iov, 1, 0);
   ASSERT_CMPSSIZE_T (r, ==, len);

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, delta, SEEK_SET), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) delta);

   r = mongoc_gridfs_file_writev (file, iov, 1, 0);
   ASSERT_CMPSSIZE_T (r, ==, len);
   mongoc_gridfs_file_save (file);

   cnt = mongoc_collection_count (mongoc_gridfs_get_chunks (gridfs),
                                  MONGOC_QUERY_NONE,
                                  tmp_bson (NULL),
                                  0,
                                  0,
                                  NULL,
                                  &error);

   ASSERT_OR_PRINT (cnt != -1, error);
   ASSERT_CMPINT64 (expected_chunks, ==, cnt);

   mongoc_gridfs_file_destroy (file);
   file = mongoc_gridfs_find_one (gridfs, tmp_bson (NULL), &error);
   ASSERT_OR_PRINT (file, error);

   r = mongoc_gridfs_file_readv (file, &riov, 1, delta + len, 0);
   ASSERT_CMPSSIZE_T (r, ==, (ssize_t) (delta + len));

   mongoc_gridfs_file_destroy (file);
   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


static void
test_empty (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_stream_t *stream;
   mongoc_client_t *client;
   bson_error_t error;
   ssize_t r;
   char buf[2] = {'h', 'i'};
   mongoc_iovec_t iov[1];
   ssize_t two = 2L;

   iov[0].iov_base = buf;
   iov[0].iov_len = 2;

   client = test_framework_client_new ();

   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "empty", &error), error);

   stream =
      mongoc_stream_file_new_for_path (BINARY_DIR "/empty.dat", O_RDONLY, 0);
   ASSERT_OR_PRINT_ERRNO (stream, errno);

   file = mongoc_gridfs_create_file_from_stream (gridfs, stream, NULL);
   ASSERT (file);

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_SET), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) 0);

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_CUR), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) 0);

   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_END), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) 0);

   r = mongoc_gridfs_file_writev (file, iov, 1, 0);

   ASSERT_CMPSSIZE_T (r, ==, two);
   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_SET), ==, 0);
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file), ==, (uint64_t) 0);

   r = mongoc_gridfs_file_readv (file, iov, 1, 2, 0);

   ASSERT_CMPSSIZE_T (r, ==, two);
   ASSERT_CMPINT (strncmp (buf, "hi", 2), ==, 0);

   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_client_destroy (client);
}


static void
test_stream (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_client_t *client;
   mongoc_stream_t *stream;
   mongoc_stream_t *in_stream;
   bson_error_t error;
   ssize_t r;
   char buf[4096];
   mongoc_iovec_t iov;

   iov.iov_base = buf;
   iov.iov_len = sizeof buf;

   client = test_framework_client_new ();
   ASSERT (client);

   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "fs", &error), error);

   mongoc_gridfs_drop (gridfs, &error);

   in_stream =
      mongoc_stream_file_new_for_path (BINARY_DIR "/gridfs.dat", O_RDONLY, 0);
   ASSERT_OR_PRINT_ERRNO (in_stream, errno);

   file = mongoc_gridfs_create_file_from_stream (gridfs, in_stream, NULL);
   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));

   stream = mongoc_stream_gridfs_new (file);

   r = mongoc_stream_readv (stream, &iov, 1, file->length, 0);
   ASSERT_CMPINT64 ((int64_t) r, ==, file->length);

   /* cleanup */
   mongoc_stream_destroy (stream);

   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


#define ASSERT_TELL(file_, position_) \
   ASSERT_CMPUINT64 (mongoc_gridfs_file_tell (file_), ==, position_)


static void
test_long_seek (void *ctx)
{
   const uint64_t four_mb = 4 * 1024 * 1024;

   mongoc_client_t *client;
   bson_error_t error;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   ssize_t r;
   mongoc_gridfs_file_opt_t opt = {0, "filename"};
   mongoc_iovec_t iov;
   char buf[16 * 1024]; /* nothing special about 16k, just a buffer */
   const ssize_t buflen = sizeof (buf);
   ssize_t written;
   int64_t cursor_id;
   int i;

   iov.iov_base = buf;
   iov.iov_len = sizeof (buf);

   client = test_framework_client_new ();
   gridfs = get_test_gridfs (client, "long_seek", &error);
   ASSERT_OR_PRINT (gridfs, error);
   file = mongoc_gridfs_create_file (gridfs, &opt);
   ASSERT (file);

   /* Write 20MB, enough to ensure we need many batches, below */
   written = 0;
   while (written < 20 * 1024 * 1024) {
      r = mongoc_gridfs_file_writev (file, &iov, 1, 0);
      ASSERT_CMPSSIZE_T (r, ==, buflen);
      written += r;
   }

   /* new file handle */
   mongoc_gridfs_file_save (file);
   mongoc_gridfs_file_destroy (file);
   file = mongoc_gridfs_find_one (
      gridfs, tmp_bson ("{'filename': 'filename'}"), &error);

   ASSERT_OR_PRINT (file, error);

   /* read the start of the file */
   r = mongoc_gridfs_file_readv (file, &iov, 1, sizeof (buf), 0);
   ASSERT_CMPSSIZE_T (r, ==, buflen);
   ASSERT_TELL (file, (uint64_t) buflen);
   cursor_id = mongoc_cursor_get_id (file->cursor);
   ASSERT_CMPINT64 ((int64_t) 0, !=, cursor_id);

   /* seek forward into next batch and read, gridfs advances cursor */
   i = mongoc_gridfs_file_seek (file, four_mb, SEEK_CUR);
   ASSERT_CMPINT (i, ==, 0);
   r = mongoc_gridfs_file_readv (file, &iov, 1, sizeof (buf), 0);
   ASSERT_CMPSSIZE_T (r, ==, buflen);
   ASSERT_TELL (file, four_mb + 2 * buflen);

   /* same as the cursor we started with */
   ASSERT_CMPINT64 ((int64_t) 0, !=, mongoc_cursor_get_id (file->cursor));
   ASSERT_CMPINT64 (cursor_id, ==, mongoc_cursor_get_id (file->cursor));

   /* seek more than a batch forward, gridfs discards cursor */
   i = mongoc_gridfs_file_seek (file, 3 * four_mb, SEEK_CUR);
   ASSERT_CMPINT (i, ==, 0);
   ASSERT_TELL (file, 4 * four_mb + 2 * buflen);
   r = mongoc_gridfs_file_readv (file, &iov, 1, sizeof (buf), 0);
   ASSERT_CMPSSIZE_T (r, ==, buflen);
   ASSERT_TELL (file, 4 * four_mb + 3 * buflen);

   /* new cursor, not the one we started with */
   ASSERT_CMPINT64 (cursor_id, !=, mongoc_cursor_get_id (file->cursor));

   mongoc_gridfs_file_destroy (file);
   ASSERT_OR_PRINT (drop_collections (gridfs, &error), error);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


static void
test_remove_by_filename (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_gridfs_file_opt_t opt = {0};
   mongoc_client_t *client;
   bson_error_t error;

   client = test_framework_client_new ();
   ASSERT (client);

   ASSERT_OR_PRINT (
      gridfs = get_test_gridfs (client, "fs_remove_by_filename", &error),
      error);

   mongoc_gridfs_drop (gridfs, &error);

   opt.filename = "foo_file_1.txt";
   file = mongoc_gridfs_create_file (gridfs, &opt);
   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));
   mongoc_gridfs_file_destroy (file);

   opt.filename = "foo_file_2.txt";
   file = mongoc_gridfs_create_file (gridfs, &opt);
   ASSERT (file);
   ASSERT (mongoc_gridfs_file_save (file));

   ASSERT_OR_PRINT (
      mongoc_gridfs_remove_by_filename (gridfs, "foo_file_1.txt", &error),
      error);
   mongoc_gridfs_file_destroy (file);

   file = mongoc_gridfs_find_one_by_filename (gridfs, "foo_file_1.txt", &error);
   ASSERT (!file);

   file = mongoc_gridfs_find_one_by_filename (gridfs, "foo_file_2.txt", &error);
   ASSERT (file);
   mongoc_gridfs_file_destroy (file);

   drop_collections (gridfs, &error);
   mongoc_gridfs_destroy (gridfs);

   mongoc_client_destroy (client);
}

static void
test_missing_chunk (void *ctx)
{
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_collection_t *chunks;
   ssize_t r;
   mongoc_gridfs_file_opt_t opt = {0, "filename"};
   mongoc_iovec_t iov;
   char buf[16 * 1024]; /* nothing special about 16k, just a buffer */
   const ssize_t buflen = sizeof (buf);
   ssize_t written;
   bool ret;

   iov.iov_base = buf;
   iov.iov_len = sizeof (buf);

   client = test_framework_client_new ();
   gridfs = get_test_gridfs (client, "long_seek", &error);
   ASSERT_OR_PRINT (gridfs, error);
   mongoc_gridfs_drop (gridfs, NULL);
   file = mongoc_gridfs_create_file (gridfs, &opt);
   ASSERT (file);

   /* 700k, enough to need three 255k chunks */
   written = 0;
   while (written < 700 * 1024) {
      r = mongoc_gridfs_file_writev (file, &iov, 1, 0);
      ASSERT_CMPSSIZE_T (r, ==, buflen);
      written += r;
   }

   /* new file handle */
   mongoc_gridfs_file_save (file);
   mongoc_gridfs_file_destroy (file);
   file = mongoc_gridfs_find_one_by_filename (gridfs, "filename", &error);
   ASSERT_OR_PRINT (file, error);

   /* chunks have n=0, 1, 2; remove the middle one */
   chunks = mongoc_gridfs_get_chunks (gridfs);
   ret = mongoc_collection_remove (
      chunks, MONGOC_REMOVE_NONE, tmp_bson ("{'n': 1}"), NULL, &error);

   ASSERT_OR_PRINT (ret, error);

   /* read the file */
   for (;;) {
      r = mongoc_gridfs_file_readv (file, &iov, 1, sizeof (buf), 0);
      if (r > 0) {
         ASSERT_CMPSSIZE_T (r, ==, buflen);
      } else {
         ASSERT (mongoc_gridfs_file_error (file, &error));
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_GRIDFS,
                                MONGOC_ERROR_GRIDFS_CHUNK_MISSING,
                                "missing chunk number 1");

         break;
      }
   }

   mongoc_gridfs_file_destroy (file);
   ASSERT_OR_PRINT (drop_collections (gridfs, &error), error);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}

static void
test_missing_file (void)
{
   mongoc_client_t *client;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   bson_error_t error;
   char buf[] = "contents contents";
   mongoc_iovec_t iov;

   iov.iov_base = buf;
   iov.iov_len = sizeof buf;

   client = test_framework_client_new ();
   gridfs =
      mongoc_client_get_gridfs (client, "test_missing_file", NULL, &error);
   ASSERT_OR_PRINT (gridfs, error);

   file = mongoc_gridfs_create_file (gridfs, NULL);
   ASSERT_CMPSSIZE_T (
      mongoc_gridfs_file_writev (file, &iov, 1, 0), ==, (ssize_t) sizeof buf);
   ASSERT_CMPINT (mongoc_gridfs_file_seek (file, 0, SEEK_SET), ==, 0);
   BSON_ASSERT (mongoc_gridfs_file_save (file));

   /* remove the file */
   BSON_ASSERT (mongoc_gridfs_file_remove (file, &error));

   /* readv fails */
   ASSERT_CMPSSIZE_T (mongoc_gridfs_file_readv (file, &iov, 1, sizeof buf, 0),
                      ==,
                      (ssize_t) -1);
   BSON_ASSERT (mongoc_gridfs_file_error (file, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_GRIDFS,
                          MONGOC_ERROR_GRIDFS_CHUNK_MISSING,
                          "missing chunk number 0");

   memset (&error, 0, sizeof error);

   /* writev fails */
   ASSERT_CMPSSIZE_T (
      mongoc_gridfs_file_writev (file, &iov, 1, 0), ==, (ssize_t) -1);
   BSON_ASSERT (mongoc_gridfs_file_error (file, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_GRIDFS,
                          MONGOC_ERROR_GRIDFS_CHUNK_MISSING,
                          "missing chunk number 0");

   mongoc_gridfs_file_destroy (file);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


/* check that user can specify _id of any type for file */
static void
test_set_id (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_client_t *client;
   bson_error_t error;
   bson_value_t id;
   bson_t *query;
   mongoc_gridfs_file_t *file;
   mongoc_gridfs_file_t *result;
   mongoc_gridfs_file_opt_t opt = {0};

   /* create new client and grab gridfs handle */
   client = test_framework_client_new ();
   ASSERT (client);
   gridfs = mongoc_client_get_gridfs (client, "test", "fs", &error);
   ASSERT_OR_PRINT (gridfs, error);

   /* create bson */
   id.value_type = BSON_TYPE_INT32;
   id.value.v_int32 = 1;

   /* query for finding file */
   query = tmp_bson ("{'_id': 1}");

   /* create new file */
   opt.filename = "test";
   file = mongoc_gridfs_create_file (gridfs, &opt);
   ASSERT (file);

   /* if we find a file with new id, then file_set_id worked */
   ASSERT_OR_PRINT (mongoc_gridfs_file_set_id (file, &id, &error), error);
   ASSERT (mongoc_gridfs_file_save (file));
   result = mongoc_gridfs_find_one (gridfs, query, &error);
   ASSERT_OR_PRINT (result, error);

   mongoc_gridfs_file_destroy (result);
   mongoc_gridfs_file_destroy (file);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}

/* check gridfs inherits read / write concern, read prefs from the client */
static void
test_inherit_client_config (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_write_concern_t *write_concern;
   mongoc_read_concern_t *read_concern;
   mongoc_read_prefs_t *secondary_pref;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;

   /* mock mongos: easiest way to test that read preference is configured */
   server = mock_mongos_new (4);
   mock_server_run (server);

   /* configure read / write concern and read prefs on client */
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));

   write_concern = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (write_concern, 2);
   mongoc_client_set_write_concern (client, write_concern);

   read_concern = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (read_concern,
                                  MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   mongoc_client_set_read_concern (client, read_concern);

   secondary_pref = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_client_set_read_prefs (client, secondary_pref);

   gridfs = _get_gridfs (server, client);

   /* test read prefs and read concern */
   future = future_gridfs_find_one (gridfs, tmp_bson ("{}"), &error);
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'find': 'fs.files', 'readConcern': {'level': 'majority'}},"
      " '$readPreference': {'mode': 'secondary'}}");

   mock_server_replies_simple (
      request,
      "{'ok': 1, 'cursor': {'ns': 'fs.files', 'firstBatch': [{'_id': 1}]}}");

   file = future_get_mongoc_gridfs_file_ptr (future);
   ASSERT (file);

   request_destroy (request);
   future_destroy (future);

   /* test write concern */
   future = future_gridfs_file_remove (file, &error);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_NONE,
      "{'delete': 'fs.files', 'writeConcern': {'w': 2}}");

   mock_server_replies_ok_and_destroys (request);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_NONE,
      "{'delete': 'fs.chunks', 'writeConcern': {'w': 2}}");

   mock_server_replies_ok_and_destroys (request);
   ASSERT (future_get_bool (future));
   future_destroy (future);

   mongoc_gridfs_file_destroy (file);
   mongoc_gridfs_destroy (gridfs);
   mongoc_write_concern_destroy (write_concern);
   mongoc_read_concern_destroy (read_concern);
   mongoc_read_prefs_destroy (secondary_pref);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

static void
test_find_one_empty (void)
{
   mongoc_gridfs_t *gridfs;
   mongoc_client_t *client;
   bson_error_t error = {1, 2, "hello"};

   client = test_framework_client_new ();
   ASSERT_OR_PRINT (gridfs = get_test_gridfs (client, "list", &error), error);
   ASSERT (!mongoc_gridfs_find_one (
      gridfs, tmp_bson ("{'x': 'doesntexist'}"), &error));

   /* ensure "error" is cleared if we successfully find no file */
   ASSERT_CMPINT (error.domain, ==, 0);
   ASSERT_CMPINT (error.code, ==, 0);
   ASSERT_CMPSTR (error.message, "");

   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}

void
test_gridfs_install (TestSuite *suite)
{
   TestSuite_AddLive (suite, "/GridFS/create", test_create);
   TestSuite_AddLive (
      suite, "/GridFS/create_from_stream", test_create_from_stream);
   TestSuite_AddLive (suite, "/GridFS/list", test_list);
   TestSuite_AddLive (suite, "/GridFS/find_one_empty", test_find_one_empty);
   TestSuite_AddLive (suite, "/GridFS/find_with_opts", test_find_with_opts);
   TestSuite_AddMockServerTest (
      suite, "/GridFS/find_one_with_opts/limit", test_find_one_with_opts_limit);
   TestSuite_AddLive (suite, "/GridFS/properties", test_properties);
   TestSuite_AddLive (suite, "/GridFS/empty", test_empty);
   TestSuite_AddLive (suite, "/GridFS/read", test_read);
   TestSuite_AddLive (suite, "/GridFS/seek", test_seek);
   TestSuite_AddLive (suite, "/GridFS/stream", test_stream);
   TestSuite_AddLive (suite, "/GridFS/remove", test_remove);
   TestSuite_AddLive (suite, "/GridFS/write", test_write);
   TestSuite_AddLive (
      suite, "/GridFS/write_at_boundary", test_write_at_boundary);
   TestSuite_AddLive (suite, "/GridFS/write_past_end", test_write_past_end);
   TestSuite_AddFull (suite,
                      "/GridFS/test_long_seek",
                      test_long_seek,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (
      suite, "/GridFS/remove_by_filename", test_remove_by_filename);
   TestSuite_AddFull (suite,
                      "/GridFS/missing_chunk",
                      test_missing_chunk,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (suite, "/GridFS/missing_file", test_missing_file);
   TestSuite_AddLive (suite, "/GridFS/file_set_id", test_set_id);
   TestSuite_AddMockServerTest (
      suite, "/GridFS/inherit_client_config", test_inherit_client_config);
}
