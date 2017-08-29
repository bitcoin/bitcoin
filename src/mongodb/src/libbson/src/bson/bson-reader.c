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

#include "bson.h"

#include <errno.h>
#include <fcntl.h>
#ifdef BSON_OS_WIN32
#include <io.h>
#include <share.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bson-reader.h"
#include "bson-memory.h"


typedef enum {
   BSON_READER_HANDLE = 1,
   BSON_READER_DATA = 2,
} bson_reader_type_t;


typedef struct {
   bson_reader_type_t type;
   void *handle;
   bool done : 1;
   bool failed : 1;
   size_t end;
   size_t len;
   size_t offset;
   size_t bytes_read;
   bson_t inline_bson;
   uint8_t *data;
   bson_reader_read_func_t read_func;
   bson_reader_destroy_func_t destroy_func;
} bson_reader_handle_t;


typedef struct {
   int fd;
   bool do_close;
} bson_reader_handle_fd_t;


typedef struct {
   bson_reader_type_t type;
   const uint8_t *data;
   size_t length;
   size_t offset;
   bson_t inline_bson;
} bson_reader_data_t;


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_handle_fill_buffer --
 *
 *       Attempt to read as much as possible until the underlying buffer
 *       in @reader is filled or we have reached end-of-stream or
 *       read failure.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_reader_handle_fill_buffer (bson_reader_handle_t *reader) /* IN */
{
   ssize_t ret;

   /*
    * Handle first read specially.
    */
   if ((!reader->done) && (!reader->offset) && (!reader->end)) {
      ret = reader->read_func (reader->handle, &reader->data[0], reader->len);

      if (ret <= 0) {
         reader->done = true;
         return;
      }
      reader->bytes_read += ret;

      reader->end = ret;
      return;
   }

   /*
    * Move valid data to head.
    */
   memmove (&reader->data[0],
            &reader->data[reader->offset],
            reader->end - reader->offset);
   reader->end = reader->end - reader->offset;
   reader->offset = 0;

   /*
    * Read in data to fill the buffer.
    */
   ret = reader->read_func (
      reader->handle, &reader->data[reader->end], reader->len - reader->end);

   if (ret <= 0) {
      reader->done = true;
      reader->failed = (ret < 0);
   } else {
      reader->bytes_read += ret;
      reader->end += ret;
   }

   BSON_ASSERT (reader->offset == 0);
   BSON_ASSERT (reader->end <= reader->len);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_new_from_handle --
 *
 *       Allocates and initializes a new bson_reader_t using the opaque
 *       handle provided.
 *
 * Parameters:
 *       @handle: an opaque handle to use to read data.
 *       @rf: a function to perform reads on @handle.
 *       @df: a function to release @handle, or NULL.
 *
 * Returns:
 *       A newly allocated bson_reader_t if successful, otherwise NULL.
 *       Free the successful result with bson_reader_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_reader_t *
bson_reader_new_from_handle (void *handle,
                             bson_reader_read_func_t rf,
                             bson_reader_destroy_func_t df)
{
   bson_reader_handle_t *real;

   BSON_ASSERT (handle);
   BSON_ASSERT (rf);

   real = bson_malloc0 (sizeof *real);
   real->type = BSON_READER_HANDLE;
   real->data = bson_malloc0 (1024);
   real->handle = handle;
   real->len = 1024;
   real->offset = 0;

   bson_reader_set_read_func ((bson_reader_t *) real, rf);

   if (df) {
      bson_reader_set_destroy_func ((bson_reader_t *) real, df);
   }

   _bson_reader_handle_fill_buffer (real);

   return (bson_reader_t *) real;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_handle_fd_destroy --
 *
 *       Cleanup allocations associated with state created in
 *       bson_reader_new_from_fd().
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_reader_handle_fd_destroy (void *handle) /* IN */
{
   bson_reader_handle_fd_t *fd = handle;

   if (fd) {
      if ((fd->fd != -1) && fd->do_close) {
#ifdef _WIN32
         _close (fd->fd);
#else
         close (fd->fd);
#endif
      }
      bson_free (fd);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_handle_fd_read --
 *
 *       Perform read on opaque handle created in
 *       bson_reader_new_from_fd().
 *
 *       The underlying file descriptor is read from the current position
 *       using the bson_reader_handle_fd_t allocated.
 *
 * Returns:
 *       -1 on failure.
 *       0 on end of stream.
 *       Greater than zero on success.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static ssize_t
_bson_reader_handle_fd_read (void *handle, /* IN */
                             void *buf,    /* IN */
                             size_t len)   /* IN */
{
   bson_reader_handle_fd_t *fd = handle;
   ssize_t ret = -1;

   if (fd && (fd->fd != -1)) {
   again:
#ifdef BSON_OS_WIN32
      ret = _read (fd->fd, buf, (unsigned int) len);
#else
      ret = read (fd->fd, buf, len);
#endif
      if ((ret == -1) && (errno == EAGAIN)) {
         goto again;
      }
   }

   return ret;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_new_from_fd --
 *
 *       Create a new bson_reader_t using the file-descriptor provided.
 *
 * Parameters:
 *       @fd: a libc style file-descriptor.
 *       @close_on_destroy: if close() should be called on @fd when
 *          bson_reader_destroy() is called.
 *
 * Returns:
 *       A newly allocated bson_reader_t on success; otherwise NULL.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_reader_t *
bson_reader_new_from_fd (int fd,                /* IN */
                         bool close_on_destroy) /* IN */
{
   bson_reader_handle_fd_t *handle;

   BSON_ASSERT (fd != -1);

   handle = bson_malloc0 (sizeof *handle);
   handle->fd = fd;
   handle->do_close = close_on_destroy;

   return bson_reader_new_from_handle (
      handle, _bson_reader_handle_fd_read, _bson_reader_handle_fd_destroy);
}


/**
 * bson_reader_set_read_func:
 * @reader: A bson_reader_t.
 *
 * Note that @reader must be initialized by bson_reader_init_from_handle(), or
 * data
 * will be destroyed.
 */
/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_set_read_func --
 *
 *       Set the read func to be provided for @reader.
 *
 *       You probably want to use bson_reader_new_from_handle() or
 *       bson_reader_new_from_fd() instead.
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
bson_reader_set_read_func (bson_reader_t *reader,        /* IN */
                           bson_reader_read_func_t func) /* IN */
{
   bson_reader_handle_t *real = (bson_reader_handle_t *) reader;

   BSON_ASSERT (reader->type == BSON_READER_HANDLE);

   real->read_func = func;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_set_destroy_func --
 *
 *       Set the function to cleanup state when @reader is destroyed.
 *
 *       You probably want bson_reader_new_from_fd() or
 *       bson_reader_new_from_handle() instead.
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
bson_reader_set_destroy_func (bson_reader_t *reader,           /* IN */
                              bson_reader_destroy_func_t func) /* IN */
{
   bson_reader_handle_t *real = (bson_reader_handle_t *) reader;

   BSON_ASSERT (reader->type == BSON_READER_HANDLE);

   real->destroy_func = func;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_handle_grow_buffer --
 *
 *       Grow the buffer to the next power of two.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_reader_handle_grow_buffer (bson_reader_handle_t *reader) /* IN */
{
   size_t size;

   size = reader->len * 2;
   reader->data = bson_realloc (reader->data, size);
   reader->len = size;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_handle_tell --
 *
 *       Tell the current position within the underlying file-descriptor.
 *
 * Returns:
 *       An off_t containing the current offset.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static off_t
_bson_reader_handle_tell (bson_reader_handle_t *reader) /* IN */
{
   off_t off;

   off = (off_t) reader->bytes_read;
   off -= (off_t) reader->end;
   off += (off_t) reader->offset;

   return off;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_handle_read --
 *
 *       Read the next chunk of data from the underlying file descriptor
 *       and return a bson_t which should not be modified.
 *
 *       There was a failure if NULL is returned and @reached_eof is
 *       not set to true.
 *
 * Returns:
 *       NULL on failure or end of stream.
 *
 * Side effects:
 *       @reached_eof is set if non-NULL.
 *
 *--------------------------------------------------------------------------
 */

static const bson_t *
_bson_reader_handle_read (bson_reader_handle_t *reader, /* IN */
                          bool *reached_eof)            /* IN */
{
   int32_t blen;

   if (reached_eof) {
      *reached_eof = false;
   }

   while (!reader->done) {
      if ((reader->end - reader->offset) < 4) {
         _bson_reader_handle_fill_buffer (reader);
         continue;
      }

      memcpy (&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE (blen);

      if (blen < 5) {
         return NULL;
      }

      if (blen > (int32_t) (reader->end - reader->offset)) {
         if (blen > (int32_t) reader->len) {
            _bson_reader_handle_grow_buffer (reader);
         }

         _bson_reader_handle_fill_buffer (reader);
         continue;
      }

      if (!bson_init_static (&reader->inline_bson,
                             &reader->data[reader->offset],
                             (uint32_t) blen)) {
         return NULL;
      }

      reader->offset += blen;

      return &reader->inline_bson;
   }

   if (reached_eof) {
      *reached_eof = reader->done && !reader->failed;
   }

   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_new_from_data --
 *
 *       Allocates and initializes a new bson_reader_t that reads the memory
 *       provided as a stream of BSON documents.
 *
 * Parameters:
 *       @data: A buffer to read BSON documents from.
 *       @length: The length of @data.
 *
 * Returns:
 *       A newly allocated bson_reader_t that should be freed with
 *       bson_reader_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_reader_t *
bson_reader_new_from_data (const uint8_t *data, /* IN */
                           size_t length)       /* IN */
{
   bson_reader_data_t *real;

   BSON_ASSERT (data);

   real = (bson_reader_data_t *) bson_malloc0 (sizeof *real);
   real->type = BSON_READER_DATA;
   real->data = data;
   real->length = length;
   real->offset = 0;

   return (bson_reader_t *) real;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_data_read --
 *
 *       Read the next document from the underlying buffer.
 *
 * Returns:
 *       NULL on failure or end of stream.
 *       a bson_t which should not be modified.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static const bson_t *
_bson_reader_data_read (bson_reader_data_t *reader, /* IN */
                        bool *reached_eof)          /* IN */
{
   int32_t blen;

   if (reached_eof) {
      *reached_eof = false;
   }

   if ((reader->offset + 4) < reader->length) {
      memcpy (&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE (blen);

      if (blen < 5) {
         return NULL;
      }

      if (blen > (int32_t) (reader->length - reader->offset)) {
         return NULL;
      }

      if (!bson_init_static (&reader->inline_bson,
                             &reader->data[reader->offset],
                             (uint32_t) blen)) {
         return NULL;
      }

      reader->offset += blen;

      return &reader->inline_bson;
   }

   if (reached_eof) {
      *reached_eof = (reader->offset == reader->length);
   }

   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_reader_data_tell --
 *
 *       Tell the current position in the underlying buffer.
 *
 * Returns:
 *       An off_t of the current offset.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static off_t
_bson_reader_data_tell (bson_reader_data_t *reader) /* IN */
{
   return (off_t) reader->offset;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_destroy --
 *
 *       Release a bson_reader_t created with bson_reader_new_from_data(),
 *       bson_reader_new_from_fd(), or bson_reader_new_from_handle().
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
bson_reader_destroy (bson_reader_t *reader) /* IN */
{
   BSON_ASSERT (reader);

   switch (reader->type) {
   case 0:
      break;
   case BSON_READER_HANDLE: {
      bson_reader_handle_t *handle = (bson_reader_handle_t *) reader;

      if (handle->destroy_func) {
         handle->destroy_func (handle->handle);
      }

      bson_free (handle->data);
   } break;
   case BSON_READER_DATA:
      break;
   default:
      fprintf (stderr, "No such reader type: %02x\n", reader->type);
      break;
   }

   reader->type = 0;

   bson_free (reader);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_read --
 *
 *       Reads the next bson_t in the underlying memory or storage.  The
 *       resulting bson_t should not be modified or freed. You may copy it
 *       and iterate over it.  Functions that take a const bson_t* are safe
 *       to use.
 *
 *       This structure does not survive calls to bson_reader_read() or
 *       bson_reader_destroy() as it uses memory allocated by the reader or
 *       underlying storage/memory.
 *
 *       If NULL is returned then @reached_eof will be set to true if the
 *       end of the file or buffer was reached. This indicates if there was
 *       an error parsing the document stream.
 *
 * Returns:
 *       A const bson_t that should not be modified or freed.
 *       NULL on failure or end of stream.
 *
 * Side effects:
 *       @reached_eof is set if non-NULL.
 *
 *--------------------------------------------------------------------------
 */

const bson_t *
bson_reader_read (bson_reader_t *reader, /* IN */
                  bool *reached_eof)     /* OUT */
{
   BSON_ASSERT (reader);

   switch (reader->type) {
   case BSON_READER_HANDLE:
      return _bson_reader_handle_read ((bson_reader_handle_t *) reader,
                                       reached_eof);

   case BSON_READER_DATA:
      return _bson_reader_data_read ((bson_reader_data_t *) reader,
                                     reached_eof);

   default:
      fprintf (stderr, "No such reader type: %02x\n", reader->type);
      break;
   }

   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_tell --
 *
 *       Return the current position in the underlying reader. This will
 *       always be at the beginning of a bson document or end of file.
 *
 * Returns:
 *       An off_t containing the current offset.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

off_t
bson_reader_tell (bson_reader_t *reader) /* IN */
{
   BSON_ASSERT (reader);

   switch (reader->type) {
   case BSON_READER_HANDLE:
      return _bson_reader_handle_tell ((bson_reader_handle_t *) reader);

   case BSON_READER_DATA:
      return _bson_reader_data_tell ((bson_reader_data_t *) reader);

   default:
      fprintf (stderr, "No such reader type: %02x\n", reader->type);
      return -1;
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_new_from_file --
 *
 *       A convenience function to open a file containing sequential
 *       bson documents and read them using bson_reader_t.
 *
 * Returns:
 *       A new bson_reader_t if successful, otherwise NULL and
 *       @error is set. Free the non-NULL result with
 *       bson_reader_destroy().
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

bson_reader_t *
bson_reader_new_from_file (const char *path,    /* IN */
                           bson_error_t *error) /* OUT */
{
   char errmsg_buf[BSON_ERROR_BUFFER_SIZE];
   char *errmsg;
   int fd;

   BSON_ASSERT (path);

#ifdef BSON_OS_WIN32
   if (_sopen_s (&fd, path, (_O_RDONLY | _O_BINARY), _SH_DENYNO, 0) != 0) {
      fd = -1;
   }
#else
   fd = open (path, O_RDONLY);
#endif

   if (fd == -1) {
      errmsg = bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf);
      bson_set_error (
         error, BSON_ERROR_READER, BSON_ERROR_READER_BADFD, "%s", errmsg);
      return NULL;
   }

   return bson_reader_new_from_fd (fd, true);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_reset --
 *
 *       Restore the reader to its initial state. Valid only for readers
 *       created with bson_reader_new_from_data.
 *
 *--------------------------------------------------------------------------
 */

void
bson_reader_reset (bson_reader_t *reader)
{
   bson_reader_data_t *real = (bson_reader_data_t *) reader;

   if (real->type != BSON_READER_DATA) {
      fprintf (stderr, "Reader type cannot be reset\n");
      return;
   }

   real->offset = 0;
}
