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


#ifndef BSON_WRITER_H
#define BSON_WRITER_H


#include "bson.h"


BSON_BEGIN_DECLS


/**
 * bson_writer_t:
 *
 * The bson_writer_t structure is a helper for writing a series of BSON
 * documents to a single malloc() buffer. You can provide a realloc() style
 * function to grow the buffer as you go.
 *
 * This is useful if you want to build a series of BSON documents right into
 * the target buffer for an outgoing packet. The offset parameter allows you to
 * start at an offset of the target buffer.
 */
typedef struct _bson_writer_t bson_writer_t;


BSON_EXPORT (bson_writer_t *)
bson_writer_new (uint8_t **buf,
                 size_t *buflen,
                 size_t offset,
                 bson_realloc_func realloc_func,
                 void *realloc_func_ctx);
BSON_EXPORT (void)
bson_writer_destroy (bson_writer_t *writer);
BSON_EXPORT (size_t)
bson_writer_get_length (bson_writer_t *writer);
BSON_EXPORT (bool)
bson_writer_begin (bson_writer_t *writer, bson_t **bson);
BSON_EXPORT (void)
bson_writer_end (bson_writer_t *writer);
BSON_EXPORT (void)
bson_writer_rollback (bson_writer_t *writer);


BSON_END_DECLS


#endif /* BSON_WRITER_H */
