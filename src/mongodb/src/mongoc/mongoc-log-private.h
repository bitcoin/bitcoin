/*
 * Copyright 2015 MongoDB, Inc.
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

#ifndef MONGOC_LOG_PRIVATE_H
#define MONGOC_LOG_PRIVATE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include "mongoc-iovec.h"

/* just for testing */
void
_mongoc_log_get_handler (mongoc_log_func_t *log_func, void **user_data);

bool
_mongoc_log_trace_is_enabled (void);

void
mongoc_log_trace_bytes (const char *domain, const uint8_t *_b, size_t _l);

void
mongoc_log_trace_iovec (const char *domain,
                        const mongoc_iovec_t *_iov,
                        size_t _iovcnt);

#endif /* MONGOC_LOG_PRIVATE_H */
