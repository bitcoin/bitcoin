/*
 * Copyright 2013 MongoDB, Inc.
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


#include "bson-private.h"
#include "bson-writer.h"


struct _bson_writer_t {
   bool ready;
   uint8_t **buf;
   size_t *buflen;
   size_t offset;
   bson_realloc_func realloc_func;
   void *realloc_func_ctx;
   bson_t b;
};


/*
 *--------------------------------------------------------------------------
 *
 * bson_writer_new --
 *
 *       Creates a new instance of bson_writer_t using the buffer, length,
 *       offset, and realloc() function supplied.
 *
 *       The caller is expected to clean up the structure when finished
 *       using bson_writer_destroy().
 *
 * Parameters:
 *       @buf: (inout): A pointer to a target buffer.
 *       @buflen: (inout): A pointer to the buffer length.
 *       @offset: The offset in the target buffer to start from.
 *       @realloc_func: A realloc() style function or NULL.
 *
 * Returns:
 *       A newly allocated bson_writer_t that should be freed with
 *       bson_writer_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_writer_t *
bson_writer_new (uint8_t **buf,                  /* IN */
                 size_t *buflen,                 /* IN */
                 size_t offset,                  /* IN */
                 bson_realloc_func realloc_func, /* IN */
                 void *realloc_func_ctx)         /* IN */
{
   bson_writer_t *writer;

   writer = bson_malloc0 (sizeof *writer);
   writer->buf = buf;
   writer->buflen = buflen;
   writer->offset = offset;
   writer->realloc_func = realloc_func;
   writer->realloc_func_ctx = realloc_func_ctx;
   writer->ready = true;

   return writer;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_writer_destroy --
 *
 *       Cleanup after @writer and release any allocated memory. Note that
 *       the buffer supplied to bson_writer_new() is NOT freed from this
 *       method.  The caller is responsible for that.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_writer_destroy (bson_writer_t *writer) /* IN */
{
   bson_free (writer);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_writer_get_length --
 *
 *       Fetches the current length of the content written by the buffer
 *       (including the initial offset). This includes a partly written
 *       document currently being written.
 *
 *       This is useful if you want to check to see if you've passed a given
 *       memory boundry that cannot be sent in a packet. See
 *       bson_writer_rollback() to abort the current document being written.
 *
 * Returns:
 *       The number of bytes written plus initial offset.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

size_t
bson_writer_get_length (bson_writer_t *writer) /* IN */
{
   return writer->offset + writer->b.len;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_writer_begin --
 *
 *       Begins writing a new document. The caller may use the bson
 *       structure to write out a new BSON document. When completed, the
 *       caller must call either bson_writer_end() or
 *       bson_writer_rollback().
 *
 * Parameters:
 *       @writer: A bson_writer_t.
 *       @bson: (out): A location for a bson_t*.
 *
 * Returns:
 *       true if the underlying realloc was successful; otherwise false.
 *
 * Side effects:
 *       @bson is initialized if true is returned.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_writer_begin (bson_writer_t *writer, /* IN */
                   bson_t **bson)         /* OUT */
{
   bson_impl_alloc_t *b;
   bool grown = false;

   BSON_ASSERT (writer);
   BSON_ASSERT (writer->ready);
   BSON_ASSERT (bson);

   writer->ready = false;

   memset (&writer->b, 0, sizeof (bson_t));

   b = (bson_impl_alloc_t *) &writer->b;
   b->flags = BSON_FLAG_STATIC | BSON_FLAG_NO_FREE;
   b->len = 5;
   b->parent = NULL;
   b->buf = writer->buf;
   b->buflen = writer->buflen;
   b->offset = writer->offset;
   b->alloc = NULL;
   b->alloclen = 0;
   b->realloc = writer->realloc_func;
   b->realloc_func_ctx = writer->realloc_func_ctx;

   while ((writer->offset + writer->b.len) > *writer->buflen) {
      if (!writer->realloc_func) {
         memset (&writer->b, 0, sizeof (bson_t));
         writer->ready = true;
         return false;
      }
      grown = true;

      if (!*writer->buflen) {
         *writer->buflen = 64;
      } else {
         (*writer->buflen) *= 2;
      }
   }

   if (grown) {
      *writer->buf = writer->realloc_func (
         *writer->buf, *writer->buflen, writer->realloc_func_ctx);
   }

   memset ((*writer->buf) + writer->offset + 1, 0, 5);
   (*writer->buf)[writer->offset] = 5;

   *bson = &writer->b;

   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_writer_end --
 *
 *       Complete writing of a bson_writer_t to the buffer supplied.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_writer_end (bson_writer_t *writer) /* IN */
{
   BSON_ASSERT (writer);
   BSON_ASSERT (!writer->ready);

   writer->offset += writer->b.len;
   memset (&writer->b, 0, sizeof (bson_t));
   writer->ready = true;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_writer_rollback --
 *
 *       Abort the appending of the current bson_t to the memory region
 *       managed by @writer.  This is useful if you detected that you went
 *       past a particular memory limit.  For example, MongoDB has 48MB
 *       message limits.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_writer_rollback (bson_writer_t *writer) /* IN */
{
   BSON_ASSERT (writer);

   if (writer->b.len) {
      memset (&writer->b, 0, sizeof (bson_t));
   }

   writer->ready = true;
}
