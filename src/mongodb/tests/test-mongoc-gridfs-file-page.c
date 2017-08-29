#include <mongoc.h>
#include <mongoc-gridfs-file-page-private.h>

#include "TestSuite.h"

static void
test_create (void)
{
   uint8_t fox[] = "the quick brown fox jumped over the laxy dog";
   uint32_t len = sizeof fox;

   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (fox, len, 4096);

   ASSERT (page);

   _mongoc_gridfs_file_page_destroy (page);
}


static void
test_is_dirty (void)
{
   uint8_t buf[] = "abcde";
   uint32_t len = sizeof buf;
   int32_t r;

   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (buf, len, 10);
   ASSERT (page);

   r = _mongoc_gridfs_file_page_is_dirty (page);
   ASSERT (!r);

   r = _mongoc_gridfs_file_page_write (page, "foo", 3);
   ASSERT (r == 3);

   r = _mongoc_gridfs_file_page_is_dirty (page);
   ASSERT (r);

   _mongoc_gridfs_file_page_destroy (page);
}


static void
test_get_data (void)
{
   uint8_t buf[] = "abcde";
   uint32_t len = sizeof buf;
   const uint8_t *ptr;
   int32_t r;

   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (buf, len, 10);
   ASSERT (page);

   ptr = _mongoc_gridfs_file_page_get_data (page);
   ASSERT (ptr == buf);

   r = _mongoc_gridfs_file_page_write (page, "foo", 3);
   ASSERT (r == 3);

   ptr = _mongoc_gridfs_file_page_get_data (page);
   ASSERT (ptr != buf);

   _mongoc_gridfs_file_page_destroy (page);
}


static void
test_get_len (void)
{
   uint8_t buf[] = "abcde";
   uint32_t len = sizeof buf;
   int32_t r;

   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (buf, len, 10);
   ASSERT (page);

   r = _mongoc_gridfs_file_page_get_len (page);
   ASSERT (r == 6);

   _mongoc_gridfs_file_page_destroy (page);
}


static void
test_read (void)
{
   uint8_t fox[] = "the quick brown fox jumped over the laxy dog";
   uint32_t len = sizeof fox;
   int32_t r;
   char buf[100];

   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (fox, len, 4096);

   ASSERT (page);

   r = _mongoc_gridfs_file_page_read (page, buf, 3);
   ASSERT (r == 3);
   ASSERT (memcmp ("the", buf, 3) == 0);
   ASSERT (page->offset == 3);

   r = _mongoc_gridfs_file_page_read (page, buf, 50);
   ASSERT (r == len - 3);
   ASSERT (memcmp (fox + 3, buf, len - 3) == 0);

   _mongoc_gridfs_file_page_destroy (page);
}


static void
test_seek (void)
{
   uint8_t fox[] = "the quick brown fox jumped over the laxy dog";
   uint32_t len = sizeof fox;
   int32_t r;

   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (fox, len, 4096);

   ASSERT (page);

   r = _mongoc_gridfs_file_page_seek (page, 4);
   ASSERT (r);
   ASSERT (page->offset == 4);

   r = _mongoc_gridfs_file_page_tell (page);
   ASSERT (r == 4);

   _mongoc_gridfs_file_page_destroy (page);
}


static void
test_write (void)
{
   uint8_t buf[] = "abcde";
   uint32_t len = sizeof buf;
   int32_t r;

   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (buf, len, 10);

   ASSERT (page);
   ASSERT (page->len == len);
   ASSERT (!page->buf);

   r = _mongoc_gridfs_file_page_write (page, "1", 1);
   ASSERT (r == 1);
   ASSERT (page->buf);
   ASSERT (memcmp (page->buf, "1bcde", len) == 0);
   ASSERT (page->offset == 1);
   ASSERT (page->len == len);

   r = _mongoc_gridfs_file_page_write (page, "234567", 6);
   ASSERT (r == 6);
   ASSERT (memcmp (page->buf, "1234567", 7) == 0);
   ASSERT (page->offset == 7);
   ASSERT (page->len == 7);

   r = _mongoc_gridfs_file_page_write (page, "8910", 4);
   ASSERT (r == 3);
   ASSERT (memcmp (page->buf, "1234567891", 10) == 0);
   ASSERT (page->offset == 10);
   ASSERT (page->len == 10);

   r = _mongoc_gridfs_file_page_write (page, "foo", 3);
   ASSERT (r == 0);

   _mongoc_gridfs_file_page_destroy (page);
}


static void
test_memset0 (void)
{
   uint8_t buf[] = "wxyz";
   uint32_t len = sizeof buf;
   mongoc_gridfs_file_page_t *page;

   page = _mongoc_gridfs_file_page_new (buf, len, 5);

   ASSERT (page);
   ASSERT (page->len == len);
   ASSERT (!page->buf);

   ASSERT_CMPUINT32 (1, ==, _mongoc_gridfs_file_page_memset0 (page, 1));
   ASSERT (page->buf);
   ASSERT (memcmp (page->buf, "\0xyz", 4) == 0);
   ASSERT (page->offset == 1);

   ASSERT_CMPUINT32 (4, ==, _mongoc_gridfs_file_page_memset0 (page, 10));
   ASSERT (page->buf);
   ASSERT (memcmp (page->buf, "\0\0\0\0\0", 5) == 0);
   ASSERT (page->offset == 5);

   /* file position is already at the end */
   ASSERT_CMPUINT32 (0, ==, _mongoc_gridfs_file_page_memset0 (page, 10));

   _mongoc_gridfs_file_page_destroy (page);
}


void
test_gridfs_file_page_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/GridFS/File/Page/create", test_create);
   TestSuite_Add (suite, "/GridFS/File/Page/get_data", test_get_data);
   TestSuite_Add (suite, "/GridFS/File/Page/get_len", test_get_len);
   TestSuite_Add (suite, "/GridFS/File/Page/is_dirty", test_is_dirty);
   TestSuite_Add (suite, "/GridFS/File/Page/read", test_read);
   TestSuite_Add (suite, "/GridFS/File/Page/seek", test_seek);
   TestSuite_Add (suite, "/GridFS/File/Page/write", test_write);
   TestSuite_Add (suite, "/GridFS/File/Page/memset0", test_memset0);
}
