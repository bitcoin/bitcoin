/*
 * Copyright 2013 MongoDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "gridfs_file_page"

#include "mongoc-gridfs-file-page.h"
#include "mongoc-gridfs-file-page-private.h"

#include "mongoc-trace-private.h"


/** create a new page from a buffer
 *
 * The buffer should stick around for the life of the page
 */
mongoc_gridfs_file_page_t *
_mongoc_gridfs_file_page_new (const uint8_t *data,
                              uint32_t len,
                              uint32_t chunk_size)
{
   mongoc_gridfs_file_page_t *page;

   ENTRY;

   BSON_ASSERT (data);
   BSON_ASSERT (len <= chunk_size);

   page = (mongoc_gridfs_file_page_t *) bson_malloc0 (sizeof *page);

   page->chunk_size = chunk_size;
   page->read_buf = data;
   page->len = len;

   RETURN (page);
}


bool
_mongoc_gridfs_file_page_seek (mongoc_gridfs_file_page_t *page, uint32_t offset)
{
   ENTRY;

   BSON_ASSERT (page);

   page->offset = offset;

   RETURN (1);
}


int32_t
_mongoc_gridfs_file_page_read (mongoc_gridfs_file_page_t *page,
                               void *dst,
                               uint32_t len)
{
   int bytes_read;
   const uint8_t *src;

   ENTRY;

   BSON_ASSERT (page);
   BSON_ASSERT (dst);

   bytes_read = BSON_MIN (len, page->len - page->offset);

   src = page->read_buf ? page->read_buf : page->buf;

   memcpy (dst, src + page->offset, bytes_read);

   page->offset += bytes_read;

   RETURN (bytes_read);
}


/**
 * _mongoc_gridfs_file_page_write:
 *
 * Write to a page.
 *
* Writes are copy-on-write with regards to the buffer that was passed to the
 * mongoc_gridfs_file_page_t during construction. In other words, the first
 * write allocates a large enough buffer for file->chunk_size, which becomes
 * authoritative from then on.
 *
 * A write of zero bytes will trigger the copy-on-write mechanism.
 */
int32_t
_mongoc_gridfs_file_page_write (mongoc_gridfs_file_page_t *page,
                                const void *src,
                                uint32_t len)
{
   int bytes_written;

   ENTRY;

   BSON_ASSERT (page);
   BSON_ASSERT (src);

   bytes_written = BSON_MIN (len, page->chunk_size - page->offset);

   if (!page->buf) {
      page->buf = (uint8_t *) bson_malloc (page->chunk_size);
      memcpy (
         page->buf, page->read_buf, BSON_MIN (page->chunk_size, page->len));
   }

   /* Copy bytes and adjust the page position */
   memcpy (page->buf + page->offset, src, bytes_written);
   page->offset += bytes_written;
   page->len = BSON_MAX (page->offset, page->len);

   /* Don't use the old read buffer, which is no longer current */
   page->read_buf = page->buf;

   RETURN (bytes_written);
}


/**
 * _mongoc_gridfs_file_page_memset0:
 *
 *      Write zeros to a page, starting from the page's current position. Up to
 *      `len` bytes will be set to zero or until the page is full, whichever
 *      comes first.
 *
 *      Like _mongoc_gridfs_file_page_write, operations are copy-on-write with
 *      regards to the page buffer.
 *
 * Returns:
 *      Number of bytes set.
 */
uint32_t
_mongoc_gridfs_file_page_memset0 (mongoc_gridfs_file_page_t *page, uint32_t len)
{
   uint32_t bytes_set;

   ENTRY;

   BSON_ASSERT (page);

   bytes_set = BSON_MIN (page->chunk_size - page->offset, len);

   if (!page->buf) {
      page->buf = (uint8_t *) bson_malloc0 (page->chunk_size);
      memcpy (
         page->buf, page->read_buf, BSON_MIN (page->chunk_size, page->len));
   }

   /* Set bytes and adjust the page position */
   memset (page->buf + page->offset, '\0', bytes_set);
   page->offset += bytes_set;
   page->len = BSON_MAX (page->offset, page->len);

   /* Don't use the old read buffer, which is no longer current */
   page->read_buf = page->buf;

   RETURN (bytes_set);
}


const uint8_t *
_mongoc_gridfs_file_page_get_data (mongoc_gridfs_file_page_t *page)
{
   ENTRY;

   BSON_ASSERT (page);

   RETURN (page->buf ? page->buf : page->read_buf);
}


uint32_t
_mongoc_gridfs_file_page_get_len (mongoc_gridfs_file_page_t *page)
{
   ENTRY;

   BSON_ASSERT (page);

   RETURN (page->len);
}


uint32_t
_mongoc_gridfs_file_page_tell (mongoc_gridfs_file_page_t *page)
{
   ENTRY;

   BSON_ASSERT (page);

   RETURN (page->offset);
}


bool
_mongoc_gridfs_file_page_is_dirty (mongoc_gridfs_file_page_t *page)
{
   ENTRY;

   BSON_ASSERT (page);

   RETURN (page->buf ? 1 : 0);
}


void
_mongoc_gridfs_file_page_destroy (mongoc_gridfs_file_page_t *page)
{
   ENTRY;

   if (page->buf) {
      bson_free (page->buf);
   }

   bson_free (page);

   EXIT;
}
