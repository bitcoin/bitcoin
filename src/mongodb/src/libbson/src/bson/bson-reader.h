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


#ifndef BSON_READER_H
#define BSON_READER_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#include "bson-compat.h"
#include "bson-oid.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


#define BSON_ERROR_READER_BADFD 1


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_read_func_t --
 *
 *       This function is a callback used by bson_reader_t to read the
 *       next chunk of data from the underlying opaque file descriptor.
 *
 *       This function is meant to operate similar to the read() function
 *       as part of libc on UNIX-like systems.
 *
 * Parameters:
 *       @handle: The handle to read from.
 *       @buf: The buffer to read into.
 *       @count: The number of bytes to read.
 *
 * Returns:
 *       0 for end of stream.
 *       -1 for read failure.
 *       Greater than zero for number of bytes read into @buf.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

typedef ssize_t (*bson_reader_read_func_t) (void *handle,  /* IN */
                                            void *buf,     /* IN */
                                            size_t count); /* IN */


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_destroy_func_t --
 *
 *       Destroy callback to release any resources associated with the
 *       opaque handle.
 *
 * Parameters:
 *       @handle: the handle provided to bson_reader_new_from_handle().
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

typedef void (*bson_reader_destroy_func_t) (void *handle); /* IN */


BSON_EXPORT (bson_reader_t *)
bson_reader_new_from_handle (void *handle,
                             bson_reader_read_func_t rf,
                             bson_reader_destroy_func_t df);
BSON_EXPORT (bson_reader_t *)
bson_reader_new_from_fd (int fd, bool close_on_destroy);
BSON_EXPORT (bson_reader_t *)
bson_reader_new_from_file (const char *path, bson_error_t *error);
BSON_EXPORT (bson_reader_t *)
bson_reader_new_from_data (const uint8_t *data, size_t length);
BSON_EXPORT (void)
bson_reader_destroy (bson_reader_t *reader);
BSON_EXPORT (void)
bson_reader_set_read_func (bson_reader_t *reader, bson_reader_read_func_t func);
BSON_EXPORT (void)
bson_reader_set_destroy_func (bson_reader_t *reader,
                              bson_reader_destroy_func_t func);
BSON_EXPORT (const bson_t *)
bson_reader_read (bson_reader_t *reader, bool *reached_eof);
BSON_EXPORT (off_t)
bson_reader_tell (bson_reader_t *reader);
BSON_EXPORT (void)
bson_reader_reset (bson_reader_t *reader);

BSON_END_DECLS


#endif /* BSON_READER_H */
