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


#ifndef BSON_JSON_H
#define BSON_JSON_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#include "bson.h"


BSON_BEGIN_DECLS


typedef struct _bson_json_reader_t bson_json_reader_t;


typedef enum {
   BSON_JSON_ERROR_READ_CORRUPT_JS = 1,
   BSON_JSON_ERROR_READ_INVALID_PARAM,
   BSON_JSON_ERROR_READ_CB_FAILURE,
} bson_json_error_code_t;


typedef ssize_t (*bson_json_reader_cb) (void *handle,
                                        uint8_t *buf,
                                        size_t count);
typedef void (*bson_json_destroy_cb) (void *handle);


BSON_EXPORT (bson_json_reader_t *)
bson_json_reader_new (void *data,
                      bson_json_reader_cb cb,
                      bson_json_destroy_cb dcb,
                      bool allow_multiple,
                      size_t buf_size);
BSON_EXPORT (bson_json_reader_t *)
bson_json_reader_new_from_fd (int fd, bool close_on_destroy);
BSON_EXPORT (bson_json_reader_t *)
bson_json_reader_new_from_file (const char *filename, bson_error_t *error);
BSON_EXPORT (void)
bson_json_reader_destroy (bson_json_reader_t *reader);
BSON_EXPORT (int)
bson_json_reader_read (bson_json_reader_t *reader,
                       bson_t *bson,
                       bson_error_t *error);
BSON_EXPORT (bson_json_reader_t *)
bson_json_data_reader_new (bool allow_multiple, size_t size);
BSON_EXPORT (void)
bson_json_data_reader_ingest (bson_json_reader_t *reader,
                              const uint8_t *data,
                              size_t len);


BSON_END_DECLS


#endif /* BSON_JSON_H */
