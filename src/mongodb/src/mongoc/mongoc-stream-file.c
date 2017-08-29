/*
 * Copyright 2014 MongoDB, Inc.
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


#ifdef _WIN32
#include <io.h>
#include <share.h>
#endif

#include "mongoc-stream-private.h"
#include "mongoc-stream-file.h"
#include "mongoc-trace-private.h"


/*
 * TODO: This does not respect timeouts or set O_NONBLOCK.
 *       But that should be fine until it isn't :-)
 */


struct _mongoc_stream_file_t {
   mongoc_stream_t vtable;
   int fd;
};


static int
_mongoc_stream_file_close (mongoc_stream_t *stream)
{
   mongoc_stream_file_t *file = (mongoc_stream_file_t *) stream;
   int ret;

   ENTRY;

   BSON_ASSERT (file);

   if (file->fd != -1) {
#ifdef _WIN32
      ret = _close (file->fd);
#else
      ret = close (file->fd);
#endif
      file->fd = -1;
      RETURN (ret);
   }

   RETURN (0);
}


static void
_mongoc_stream_file_destroy (mongoc_stream_t *stream)
{
   mongoc_stream_file_t *file = (mongoc_stream_file_t *) stream;

   ENTRY;

   BSON_ASSERT (file);

   if (file->fd) {
      _mongoc_stream_file_close (stream);
   }

   bson_free (file);

   EXIT;
}


static void
_mongoc_stream_file_failed (mongoc_stream_t *stream)
{
   ENTRY;

   _mongoc_stream_file_destroy (stream);

   EXIT;
}


static int
_mongoc_stream_file_flush (mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_file_t *file = (mongoc_stream_file_t *) stream;

   BSON_ASSERT (file);

   if (file->fd != -1) {
#ifdef _WIN32
      return _commit (file->fd);
#else
      return fsync (file->fd);
#endif
   }

   return 0;
}


static ssize_t
_mongoc_stream_file_readv (mongoc_stream_t *stream, /* IN */
                           mongoc_iovec_t *iov,     /* IN */
                           size_t iovcnt,           /* IN */
                           size_t min_bytes,        /* IN */
                           int32_t timeout_msec)    /* IN */
{
   mongoc_stream_file_t *file = (mongoc_stream_file_t *) stream;
   ssize_t ret = 0;

#ifdef _WIN32
   ssize_t nread;
   size_t i;

   ENTRY;

   for (i = 0; i < iovcnt; i++) {
      nread = _read (file->fd, iov[i].iov_base, iov[i].iov_len);
      if (nread < 0) {
         RETURN (ret ? ret : -1);
      } else if (nread == 0) {
         RETURN (ret ? ret : 0);
      } else {
         ret += nread;
         if (nread != iov[i].iov_len) {
            RETURN (ret ? ret : -1);
         }
      }
   }

   RETURN (ret);
#else
   ENTRY;
   ret = readv (file->fd, iov, (int) iovcnt);
   RETURN (ret);
#endif
}


static ssize_t
_mongoc_stream_file_writev (mongoc_stream_t *stream, /* IN */
                            mongoc_iovec_t *iov,     /* IN */
                            size_t iovcnt,           /* IN */
                            int32_t timeout_msec)    /* IN */
{
   mongoc_stream_file_t *file = (mongoc_stream_file_t *) stream;

#ifdef _WIN32
   ssize_t ret = 0;
   ssize_t nwrite;
   size_t i;

   for (i = 0; i < iovcnt; i++) {
      nwrite = _write (file->fd, iov[i].iov_base, iov[i].iov_len);
      if (nwrite != iov[i].iov_len) {
         return ret ? ret : -1;
      }
      ret += nwrite;
   }

   return ret;
#else
   return writev (file->fd, iov, (int) iovcnt);
#endif
}


static bool
_mongoc_stream_file_check_closed (mongoc_stream_t *stream) /* IN */
{
   return false;
}


mongoc_stream_t *
mongoc_stream_file_new (int fd) /* IN */
{
   mongoc_stream_file_t *stream;

   BSON_ASSERT (fd != -1);

   stream = (mongoc_stream_file_t *) bson_malloc0 (sizeof *stream);
   stream->vtable.type = MONGOC_STREAM_FILE;
   stream->vtable.close = _mongoc_stream_file_close;
   stream->vtable.destroy = _mongoc_stream_file_destroy;
   stream->vtable.failed = _mongoc_stream_file_failed;
   stream->vtable.flush = _mongoc_stream_file_flush;
   stream->vtable.readv = _mongoc_stream_file_readv;
   stream->vtable.writev = _mongoc_stream_file_writev;
   stream->vtable.check_closed = _mongoc_stream_file_check_closed;
   stream->fd = fd;

   return (mongoc_stream_t *) stream;
}


mongoc_stream_t *
mongoc_stream_file_new_for_path (const char *path, /* IN */
                                 int flags,        /* IN */
                                 int mode)         /* IN */
{
   int fd = -1;

   BSON_ASSERT (path);

#ifdef _WIN32
   if (_sopen_s (&fd, path, (flags | _O_BINARY), _SH_DENYNO, mode) != 0) {
      fd = -1;
   }
#else
   fd = open (path, flags, mode);
#endif

   if (fd == -1) {
      return NULL;
   }

   return mongoc_stream_file_new (fd);
}


int
mongoc_stream_file_get_fd (mongoc_stream_file_t *stream)
{
   BSON_ASSERT (stream);

   return stream->fd;
}
