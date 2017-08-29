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
#define MONGOC_LOG_DOMAIN "gridfs_file"

#include <limits.h>
#include <time.h>
#include <errno.h>

#include "mongoc-cursor.h"
#include "mongoc-cursor-private.h"
#include "mongoc-collection.h"
#include "mongoc-gridfs.h"
#include "mongoc-gridfs-private.h"
#include "mongoc-gridfs-file.h"
#include "mongoc-gridfs-file-private.h"
#include "mongoc-gridfs-file-page.h"
#include "mongoc-gridfs-file-page-private.h"
#include "mongoc-iovec.h"
#include "mongoc-trace-private.h"
#include "mongoc-error.h"

static bool
_mongoc_gridfs_file_refresh_page (mongoc_gridfs_file_t *file);

static bool
_mongoc_gridfs_file_flush_page (mongoc_gridfs_file_t *file);

static ssize_t
_mongoc_gridfs_file_extend (mongoc_gridfs_file_t *file);


/*****************************************************************
* Magic accessor generation
*
* We need some accessors to get and set properties on files, to handle memory
* ownership and to determine dirtiness.  These macros produce the getters and
* setters we need
*****************************************************************/

#define MONGOC_GRIDFS_FILE_STR_ACCESSOR(name)                             \
   const char *mongoc_gridfs_file_get_##name (mongoc_gridfs_file_t *file) \
   {                                                                      \
      return file->name ? file->name : file->bson_##name;                 \
   }                                                                      \
   void mongoc_gridfs_file_set_##name (mongoc_gridfs_file_t *file,        \
                                       const char *str)                   \
   {                                                                      \
      if (file->name) {                                                   \
         bson_free (file->name);                                          \
      }                                                                   \
      file->name = bson_strdup (str);                                     \
      file->is_dirty = 1;                                                 \
   }

#define MONGOC_GRIDFS_FILE_BSON_ACCESSOR(name)                              \
   const bson_t *mongoc_gridfs_file_get_##name (mongoc_gridfs_file_t *file) \
   {                                                                        \
      if (file->name.len) {                                                 \
         return &file->name;                                                \
      } else if (file->bson_##name.len) {                                   \
         return &file->bson_##name;                                         \
      } else {                                                              \
         return NULL;                                                       \
      }                                                                     \
   }                                                                        \
   void mongoc_gridfs_file_set_##name (mongoc_gridfs_file_t *file,          \
                                       const bson_t *bson)                  \
   {                                                                        \
      if (file->name.len) {                                                 \
         bson_destroy (&file->name);                                        \
      }                                                                     \
      bson_copy_to (bson, &(file->name));                                   \
      file->is_dirty = 1;                                                   \
   }

MONGOC_GRIDFS_FILE_STR_ACCESSOR (md5)
MONGOC_GRIDFS_FILE_STR_ACCESSOR (filename)
MONGOC_GRIDFS_FILE_STR_ACCESSOR (content_type)
MONGOC_GRIDFS_FILE_BSON_ACCESSOR (aliases)
MONGOC_GRIDFS_FILE_BSON_ACCESSOR (metadata)

/**
 * mongoc_gridfs_file_set_id:
 *
 * the user can set the files_id to an id of any type. Must be called before
 * mongoc_gridfs_file_save.
 *
 */

bool
mongoc_gridfs_file_set_id (mongoc_gridfs_file_t *file,
                           const bson_value_t *id,
                           bson_error_t *error)
{
   if (!file->is_dirty) {
      bson_set_error (error,
                      MONGOC_ERROR_GRIDFS,
                      MONGOC_ERROR_GRIDFS_PROTOCOL_ERROR,
                      "Cannot set file id after saving file.");
      return false;
   }
   bson_value_copy (id, &file->files_id);
   return true;
}

/** save a gridfs file */
bool
mongoc_gridfs_file_save (mongoc_gridfs_file_t *file)
{
   bson_t *selector, *update, child;
   const char *md5;
   const char *filename;
   const char *content_type;
   const bson_t *aliases;
   const bson_t *metadata;
   bool r;

   ENTRY;

   if (!file->is_dirty) {
      return 1;
   }

   if (file->page && _mongoc_gridfs_file_page_is_dirty (file->page)) {
      _mongoc_gridfs_file_flush_page (file);
   }

   md5 = mongoc_gridfs_file_get_md5 (file);
   filename = mongoc_gridfs_file_get_filename (file);
   content_type = mongoc_gridfs_file_get_content_type (file);
   aliases = mongoc_gridfs_file_get_aliases (file);
   metadata = mongoc_gridfs_file_get_metadata (file);

   selector = bson_new ();
   bson_append_value (selector, "_id", -1, &file->files_id);

   update = bson_new ();
   bson_append_document_begin (update, "$set", -1, &child);
   bson_append_int64 (&child, "length", -1, file->length);
   bson_append_int32 (&child, "chunkSize", -1, file->chunk_size);
   bson_append_date_time (&child, "uploadDate", -1, file->upload_date);

   if (md5) {
      bson_append_utf8 (&child, "md5", -1, md5, -1);
   }

   if (filename) {
      bson_append_utf8 (&child, "filename", -1, filename, -1);
   }

   if (content_type) {
      bson_append_utf8 (&child, "contentType", -1, content_type, -1);
   }

   if (aliases) {
      bson_append_array (&child, "aliases", -1, aliases);
   }

   if (metadata) {
      bson_append_document (&child, "metadata", -1, metadata);
   }

   bson_append_document_end (update, &child);

   r = mongoc_collection_update (file->gridfs->files,
                                 MONGOC_UPDATE_UPSERT,
                                 selector,
                                 update,
                                 NULL,
                                 &file->error);

   bson_destroy (selector);
   bson_destroy (update);

   file->is_dirty = 0;

   RETURN (r);
}


/**
 * _mongoc_gridfs_file_new_from_bson:
 *
 * creates a gridfs file from a bson object
 *
 * This is only really useful for instantiating a gridfs file from a server
 * side object
 */
mongoc_gridfs_file_t *
_mongoc_gridfs_file_new_from_bson (mongoc_gridfs_t *gridfs, const bson_t *data)
{
   mongoc_gridfs_file_t *file;
   const bson_value_t *value;
   const char *key;
   bson_iter_t iter;
   const uint8_t *buf;
   uint32_t buf_len;

   ENTRY;

   BSON_ASSERT (gridfs);
   BSON_ASSERT (data);

   file = (mongoc_gridfs_file_t *) bson_malloc0 (sizeof *file);

   file->gridfs = gridfs;
   bson_copy_to (data, &file->bson);

   bson_iter_init (&iter, &file->bson);

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);

      if (0 == strcmp (key, "_id")) {
         value = bson_iter_value (&iter);
         bson_value_copy (value, &file->files_id);
      } else if (0 == strcmp (key, "length")) {
         if (!BSON_ITER_HOLDS_NUMBER (&iter)) {
            GOTO (failure);
         }
         file->length = bson_iter_as_int64 (&iter);
      } else if (0 == strcmp (key, "chunkSize")) {
         if (!BSON_ITER_HOLDS_NUMBER (&iter)) {
            GOTO (failure);
         }
         if (bson_iter_as_int64 (&iter) > INT32_MAX) {
            GOTO (failure);
         }
         file->chunk_size = (int32_t) bson_iter_as_int64 (&iter);
      } else if (0 == strcmp (key, "uploadDate")) {
         if (!BSON_ITER_HOLDS_DATE_TIME (&iter)) {
            GOTO (failure);
         }
         file->upload_date = bson_iter_date_time (&iter);
      } else if (0 == strcmp (key, "md5")) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            GOTO (failure);
         }
         file->bson_md5 = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp (key, "filename")) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            GOTO (failure);
         }
         file->bson_filename = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp (key, "contentType")) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            GOTO (failure);
         }
         file->bson_content_type = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp (key, "aliases")) {
         if (!BSON_ITER_HOLDS_ARRAY (&iter)) {
            GOTO (failure);
         }
         bson_iter_array (&iter, &buf_len, &buf);
         bson_init_static (&file->bson_aliases, buf, buf_len);
      } else if (0 == strcmp (key, "metadata")) {
         if (!BSON_ITER_HOLDS_DOCUMENT (&iter)) {
            GOTO (failure);
         }
         bson_iter_document (&iter, &buf_len, &buf);
         bson_init_static (&file->bson_metadata, buf, buf_len);
      }
   }

   /* TODO: is there are a minimal object we should be verifying that we
    * actually have here? */

   RETURN (file);

failure:
   bson_destroy (&file->bson);

   RETURN (NULL);
}


/**
 * _mongoc_gridfs_file_new:
 *
 * Create a new empty gridfs file
 */
mongoc_gridfs_file_t *
_mongoc_gridfs_file_new (mongoc_gridfs_t *gridfs, mongoc_gridfs_file_opt_t *opt)
{
   mongoc_gridfs_file_t *file;
   mongoc_gridfs_file_opt_t default_opt = {0};

   ENTRY;

   BSON_ASSERT (gridfs);

   if (!opt) {
      opt = &default_opt;
   }

   file = (mongoc_gridfs_file_t *) bson_malloc0 (sizeof *file);

   file->gridfs = gridfs;
   file->is_dirty = 1;

   if (opt->chunk_size) {
      file->chunk_size = opt->chunk_size;
   } else {
      /*
       * The default chunk size is now 255kb. This used to be 256k but has been
       * reduced to allow for them to fit within power of two sizes in mongod.
       *
       * See CDRIVER-322.
       */
      file->chunk_size = (1 << 18) - 1024;
   }

   file->files_id.value_type = BSON_TYPE_OID;
   bson_oid_init (&file->files_id.value.v_oid, NULL);

   file->upload_date = ((int64_t) time (NULL)) * 1000;

   if (opt->md5) {
      file->md5 = bson_strdup (opt->md5);
   }

   if (opt->filename) {
      file->filename = bson_strdup (opt->filename);
   }

   if (opt->content_type) {
      file->content_type = bson_strdup (opt->content_type);
   }

   if (opt->aliases) {
      bson_copy_to (opt->aliases, &(file->aliases));
   }

   if (opt->metadata) {
      bson_copy_to (opt->metadata, &(file->metadata));
   }

   file->pos = 0;
   file->n = 0;

   RETURN (file);
}

void
mongoc_gridfs_file_destroy (mongoc_gridfs_file_t *file)
{
   ENTRY;

   BSON_ASSERT (file);

   if (file->page) {
      _mongoc_gridfs_file_page_destroy (file->page);
   }

   if (file->bson.len) {
      bson_destroy (&file->bson);
   }

   if (file->cursor) {
      mongoc_cursor_destroy (file->cursor);
   }

   if (file->files_id.value_type) {
      bson_value_destroy (&file->files_id);
   }

   if (file->md5) {
      bson_free (file->md5);
   }

   if (file->filename) {
      bson_free (file->filename);
   }

   if (file->content_type) {
      bson_free (file->content_type);
   }

   if (file->aliases.len) {
      bson_destroy (&file->aliases);
   }

   if (file->bson_aliases.len) {
      bson_destroy (&file->bson_aliases);
   }

   if (file->metadata.len) {
      bson_destroy (&file->metadata);
   }

   if (file->bson_metadata.len) {
      bson_destroy (&file->bson_metadata);
   }

   bson_free (file);

   EXIT;
}


/** readv against a gridfs file
 *  timeout_msec is unused */
ssize_t
mongoc_gridfs_file_readv (mongoc_gridfs_file_t *file,
                          mongoc_iovec_t *iov,
                          size_t iovcnt,
                          size_t min_bytes,
                          uint32_t timeout_msec)
{
   uint32_t bytes_read = 0;
   int32_t r;
   size_t i;
   uint32_t iov_pos;

   ENTRY;

   BSON_ASSERT (file);
   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);

   /* Reading when positioned past the end does nothing */
   if (file->pos >= file->length) {
      return 0;
   }

   /* Try to get the current chunk */
   if (!file->page && !_mongoc_gridfs_file_refresh_page (file)) {
      return -1;
   }

   for (i = 0; i < iovcnt; i++) {
      iov_pos = 0;

      for (;;) {
         r = _mongoc_gridfs_file_page_read (
            file->page,
            (uint8_t *) iov[i].iov_base + iov_pos,
            (uint32_t) (iov[i].iov_len - iov_pos));
         BSON_ASSERT (r >= 0);

         iov_pos += r;
         file->pos += r;
         bytes_read += r;

         if (iov_pos == iov[i].iov_len) {
            /* filled a bucket, keep going */
            break;
         } else if (file->length == file->pos) {
            /* we're at the end of the file.  So we're done */
            RETURN (bytes_read);
         } else if (bytes_read >= min_bytes) {
            /* we need a new page, but we've read enough bytes to stop */
            RETURN (bytes_read);
         } else if (!_mongoc_gridfs_file_refresh_page (file)) {
            return -1;
         }
      }
   }

   RETURN (bytes_read);
}


/** writev against a gridfs file
 *  timeout_msec is unused */
ssize_t
mongoc_gridfs_file_writev (mongoc_gridfs_file_t *file,
                           const mongoc_iovec_t *iov,
                           size_t iovcnt,
                           uint32_t timeout_msec)
{
   uint32_t bytes_written = 0;
   int32_t r;
   size_t i;
   uint32_t iov_pos;

   ENTRY;

   BSON_ASSERT (file);
   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);

   /* Pull in the correct page */
   if (!file->page && !_mongoc_gridfs_file_refresh_page (file)) {
      return -1;
   }

   /* When writing past the end-of-file, fill the gap with zeros */
   if (file->pos > file->length && !_mongoc_gridfs_file_extend (file)) {
      return -1;
   }

   for (i = 0; i < iovcnt; i++) {
      iov_pos = 0;

      for (;;) {
         if (!file->page && !_mongoc_gridfs_file_refresh_page (file)) {
            return -1;
         }

         /* write bytes until an iov is exhausted or the page is full */
         r = _mongoc_gridfs_file_page_write (
            file->page,
            (uint8_t *) iov[i].iov_base + iov_pos,
            (uint32_t) (iov[i].iov_len - iov_pos));
         BSON_ASSERT (r >= 0);

         iov_pos += r;
         file->pos += r;
         bytes_written += r;

         file->length = BSON_MAX (file->length, (int64_t) file->pos);

         if (iov_pos == iov[i].iov_len) {
            /** filled a bucket, keep going */
            break;
         } else {
            /** flush the buffer, the next pass through will bring in a new page
             */
            if (!_mongoc_gridfs_file_flush_page (file)) {
               return -1;
            }
         }
      }
   }

   file->is_dirty = 1;

   RETURN (bytes_written);
}


/**
 * _mongoc_gridfs_file_extend:
 *
 *      Extend a GridFS file to the current position pointer. Zeros will be
 *      appended to the end of the file until file->length is even with
 * file->pos.
 *
 *      If file->length >= file->pos, the function exits successfully with no
 *      operation performed.
 *
 * Parameters:
 *      @file: A mongoc_gridfs_file_t.
 *
 * Returns:
 *      The number of zero bytes written, or -1 on failure.
 */
static ssize_t
_mongoc_gridfs_file_extend (mongoc_gridfs_file_t *file)
{
   int64_t target_length;
   ssize_t diff;

   ENTRY;

   BSON_ASSERT (file);

   if (file->length >= file->pos) {
      RETURN (0);
   }

   diff = (ssize_t) (file->pos - file->length);
   target_length = file->pos;
   mongoc_gridfs_file_seek (file, 0, SEEK_END);

   while (true) {
      if (!file->page && !_mongoc_gridfs_file_refresh_page (file)) {
         RETURN (-1);
      }

      /* Set bytes until we reach the limit or fill a page */
      file->pos += _mongoc_gridfs_file_page_memset0 (file->page,
                                                     target_length - file->pos);

      if (file->pos == target_length) {
         /* We're done */
         break;
      } else if (!_mongoc_gridfs_file_flush_page (file)) {
         /* We tried to flush a full buffer, but an error occurred */
         RETURN (-1);
      }
   }

   file->length = target_length;
   file->is_dirty = true;

   RETURN (diff);
}


/**
 * _mongoc_gridfs_file_flush_page:
 *
 *    Unconditionally flushes the file's current page to the database.
 *    The page to flush is determined by page->n.
 *
 * Side Effects:
 *
 *    On success, file->page is properly destroyed and set to NULL.
 *
 * Returns:
 *
 *    True on success; false otherwise.
 */
static bool
_mongoc_gridfs_file_flush_page (mongoc_gridfs_file_t *file)
{
   bson_t *selector, *update;
   bool r;
   const uint8_t *buf;
   uint32_t len;

   ENTRY;
   BSON_ASSERT (file);
   BSON_ASSERT (file->page);

   buf = _mongoc_gridfs_file_page_get_data (file->page);
   len = _mongoc_gridfs_file_page_get_len (file->page);

   selector = bson_new ();

   bson_append_value (selector, "files_id", -1, &file->files_id);
   bson_append_int32 (selector, "n", -1, file->n);

   update = bson_sized_new (file->chunk_size + 100);

   bson_append_value (update, "files_id", -1, &file->files_id);
   bson_append_int32 (update, "n", -1, file->n);
   bson_append_binary (update, "data", -1, BSON_SUBTYPE_BINARY, buf, len);

   r = mongoc_collection_update (file->gridfs->chunks,
                                 MONGOC_UPDATE_UPSERT,
                                 selector,
                                 update,
                                 NULL,
                                 &file->error);

   bson_destroy (selector);
   bson_destroy (update);

   if (r) {
      _mongoc_gridfs_file_page_destroy (file->page);
      file->page = NULL;
      r = mongoc_gridfs_file_save (file);
   }

   RETURN (r);
}


/**
 * _mongoc_gridfs_file_keep_cursor:
 *
 *    After a seek, decide if the next read should use the current cursor or
 *    start a new query.
 *
 * Preconditions:
 *
 *    file has a cursor and cursor range.
 *
 * Side Effects:
 *
 *    None.
 */
static bool
_mongoc_gridfs_file_keep_cursor (mongoc_gridfs_file_t *file)
{
   uint32_t chunk_no;
   uint32_t chunks_per_batch;

   if (file->n < 0 || file->chunk_size <= 0) {
      return false;
   }

   chunk_no = (uint32_t) file->n;
   /* server returns roughly 4 MB batches by default */
   chunks_per_batch = (4 * 1024 * 1024) / (uint32_t) file->chunk_size;

   return (
      /* cursor is on or before the desired chunk */
      file->cursor_range[0] <= chunk_no &&
      /* chunk_no is before end of file */
      chunk_no <= file->cursor_range[1] &&
      /* desired chunk is in this batch or next one */
      chunk_no < file->cursor_range[0] + 2 * chunks_per_batch);
}


static int64_t
divide_round_up (int64_t num, int64_t denom)
{
   return (num + denom - 1) / denom;
}


static void
missing_chunk (mongoc_gridfs_file_t *file)
{
   bson_set_error (&file->error,
                   MONGOC_ERROR_GRIDFS,
                   MONGOC_ERROR_GRIDFS_CHUNK_MISSING,
                   "missing chunk number %" PRId32,
                   file->n);

   if (file->cursor) {
      mongoc_cursor_destroy (file->cursor);
      file->cursor = NULL;
   }
}


/**
 * _mongoc_gridfs_file_refresh_page:
 *
 *    Refresh a GridFS file's underlying page. This recalculates the current
 *    page number based on the file's stream position, then fetches that page
 *    from the database.
 *
 *    Note that this fetch is unconditional and the page is queried from the
 *    database even if the current page covers the same theoretical chunk.
 *
 *
 * Side Effects:
 *
 *    file->page is loaded with the appropriate buffer, fetched from the
 *    database. If the file position is at the end of the file and on a new
 *    chunk boundary, a new page is created. If the position is far past the
 *    end of the file, _mongoc_gridfs_file_extend is responsible for creating
 *    chunks to file the gap.
 *
 *    file->n is set based on file->pos. file->error is set on error.
 */
static bool
_mongoc_gridfs_file_refresh_page (mongoc_gridfs_file_t *file)
{
   bson_t query;
   bson_t child;
   bson_t opts;
   const bson_t *chunk;
   const char *key;
   bson_iter_t iter;
   int64_t existing_chunks;
   int64_t required_chunks;

   const uint8_t *data = NULL;
   uint32_t len;

   ENTRY;

   BSON_ASSERT (file);

   file->n = (int32_t) (file->pos / file->chunk_size);

   if (file->page) {
      _mongoc_gridfs_file_page_destroy (file->page);
      file->page = NULL;
   }

   /* if the file pointer is past the end of the current file (i.e. pointing to
    * a new chunk), we'll pass the page constructor a new empty page. */
   existing_chunks = divide_round_up (file->length, file->chunk_size);
   required_chunks = divide_round_up (file->pos + 1, file->chunk_size);
   if (required_chunks > existing_chunks) {
      data = (uint8_t *) "";
      len = 0;
   } else {
      /* if we have a cursor, but the cursor doesn't have the chunk we're going
       * to need, destroy it (we'll grab a new one immediately there after) */
      if (file->cursor && !_mongoc_gridfs_file_keep_cursor (file)) {
         mongoc_cursor_destroy (file->cursor);
         file->cursor = NULL;
      }

      if (!file->cursor) {
         bson_init (&query);
         BSON_APPEND_VALUE (&query, "files_id", &file->files_id);
         BSON_APPEND_DOCUMENT_BEGIN (&query, "n", &child);
         BSON_APPEND_INT32 (&child, "$gte", file->n);
         bson_append_document_end (&query, &child);

         bson_init (&opts);
         BSON_APPEND_DOCUMENT_BEGIN (&opts, "sort", &child);
         BSON_APPEND_INT32 (&child, "n", 1);
         bson_append_document_end (&opts, &child);

         BSON_APPEND_DOCUMENT_BEGIN (&opts, "projection", &child);
         BSON_APPEND_INT32 (&child, "n", 1);
         BSON_APPEND_INT32 (&child, "data", 1);
         BSON_APPEND_INT32 (&child, "_id", 0);
         bson_append_document_end (&opts, &child);

         /* find all chunks greater than or equal to our current file pos */
         file->cursor = mongoc_collection_find_with_opts (
            file->gridfs->chunks, &query, &opts, NULL);

         file->cursor_range[0] = file->n;
         file->cursor_range[1] = (uint32_t) (file->length / file->chunk_size);

         bson_destroy (&query);
         bson_destroy (&opts);

         BSON_ASSERT (file->cursor);
      }

      /* we might have had a cursor before, then seeked ahead past a chunk.
       * iterate until we're on the right chunk */
      while (file->cursor_range[0] <= file->n) {
         if (!mongoc_cursor_next (file->cursor, &chunk)) {
            /* copy cursor error; if there's none, we're missing a chunk */
            if (!mongoc_cursor_error (file->cursor, &file->error)) {
               missing_chunk (file);
            }

            RETURN (0);
         }

         file->cursor_range[0]++;
      }

      bson_iter_init (&iter, chunk);

      /* grab out what we need from the chunk */
      while (bson_iter_next (&iter)) {
         key = bson_iter_key (&iter);

         if (strcmp (key, "n") == 0) {
            if (file->n != bson_iter_int32 (&iter)) {
               missing_chunk (file);
               RETURN (0);
            }
         } else if (strcmp (key, "data") == 0) {
            bson_iter_binary (&iter, NULL, &len, &data);
         } else {
            /* Unexpected key. This should never happen */
            RETURN (0);
         }
      }

      if (file->n != file->pos / file->chunk_size) {
         return 0;
      }
   }

   if (!data) {
      bson_set_error (&file->error,
                      MONGOC_ERROR_GRIDFS,
                      MONGOC_ERROR_GRIDFS_CHUNK_MISSING,
                      "corrupt chunk number %" PRId32,
                      file->n);
      RETURN (0);
   }

   file->page = _mongoc_gridfs_file_page_new (data, len, file->chunk_size);

   /* seek in the page towards wherever we're supposed to be */
   RETURN (
      _mongoc_gridfs_file_page_seek (file->page, file->pos % file->chunk_size));
}


/**
 * mongoc_gridfs_file_seek:
 *
 *    Adjust the file position pointer in `file` by `delta`, starting from the
 *    position `whence`. The `whence` argument is interpreted as in fseek(2):
 *
 *       SEEK_SET    Set the position relative to the start of the file.
 *       SEEK_CUR    Move `delta` from the current file position.
 *       SEEK_END    Move `delta` from the end-of-file.
 *
 * Parameters:
 *
 *    @file: A mongoc_gridfs_file_t.
 *    @delta: The amount to move. May be positive or negative.
 *    @whence: One of SEEK_SET, SEEK_CUR or SEEK_END.
 *
 * Errors:
 *
 *    [EINVAL] `whence` is not one of SEEK_SET, SEEK_CUR or SEEK_END.
 *    [EINVAL] Resulting file position would be negative.
 *
 * Side Effects:
 *
 *    On success, the file's underlying position pointer is set appropriately.
 *    On failure, the file position is NOT changed and errno is set.
 *
 * Returns:
 *
 *    0 on success.
 *    -1 on error, and errno set to indicate the error.
 */
int
mongoc_gridfs_file_seek (mongoc_gridfs_file_t *file, int64_t delta, int whence)
{
   int64_t offset;

   BSON_ASSERT (file);

   switch (whence) {
   case SEEK_SET:
      offset = delta;
      break;
   case SEEK_CUR:
      offset = file->pos + delta;
      break;
   case SEEK_END:
      offset = file->length + delta;
      break;
   default:
      errno = EINVAL;
      return -1;

      break;
   }

   if (offset < 0) {
      errno = EINVAL;
      return -1;
   }

   if (offset / file->chunk_size != file->n) {
      /** no longer on the same page */

      if (file->page) {
         if (_mongoc_gridfs_file_page_is_dirty (file->page)) {
            _mongoc_gridfs_file_flush_page (file);
         } else {
            _mongoc_gridfs_file_page_destroy (file->page);
            file->page = NULL;
         }
      }

      /** we'll pick up the seek when we fetch a page on the next action.  We
       * lazily load */
   } else if (file->page) {
      _mongoc_gridfs_file_page_seek (file->page, offset % file->chunk_size);
   }

   file->pos = offset;
   file->n = file->pos / file->chunk_size;

   return 0;
}

uint64_t
mongoc_gridfs_file_tell (mongoc_gridfs_file_t *file)
{
   BSON_ASSERT (file);

   return file->pos;
}

bool
mongoc_gridfs_file_error (mongoc_gridfs_file_t *file, bson_error_t *error)
{
   BSON_ASSERT (file);
   BSON_ASSERT (error);

   if (BSON_UNLIKELY (file->error.domain)) {
      bson_set_error (error,
                      file->error.domain,
                      file->error.code,
                      "%s",
                      file->error.message);
      RETURN (true);
   }

   RETURN (false);
}

const bson_value_t *
mongoc_gridfs_file_get_id (mongoc_gridfs_file_t *file)
{
   BSON_ASSERT (file);

   return &file->files_id;
}

int64_t
mongoc_gridfs_file_get_length (mongoc_gridfs_file_t *file)
{
   BSON_ASSERT (file);

   return file->length;
}

int32_t
mongoc_gridfs_file_get_chunk_size (mongoc_gridfs_file_t *file)
{
   BSON_ASSERT (file);

   return file->chunk_size;
}

int64_t
mongoc_gridfs_file_get_upload_date (mongoc_gridfs_file_t *file)
{
   BSON_ASSERT (file);

   return file->upload_date;
}

bool
mongoc_gridfs_file_remove (mongoc_gridfs_file_t *file, bson_error_t *error)
{
   bson_t sel = BSON_INITIALIZER;
   bool ret = false;

   BSON_ASSERT (file);

   BSON_APPEND_VALUE (&sel, "_id", &file->files_id);

   if (!mongoc_collection_remove (file->gridfs->files,
                                  MONGOC_REMOVE_SINGLE_REMOVE,
                                  &sel,
                                  NULL,
                                  error)) {
      goto cleanup;
   }

   bson_reinit (&sel);
   BSON_APPEND_VALUE (&sel, "files_id", &file->files_id);

   if (!mongoc_collection_remove (
          file->gridfs->chunks, MONGOC_REMOVE_NONE, &sel, NULL, error)) {
      goto cleanup;
   }

   ret = true;

cleanup:
   bson_destroy (&sel);

   return ret;
}
