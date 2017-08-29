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


#ifndef MONGOC_TRACE_PRIVATE_H
#define MONGOC_TRACE_PRIVATE_H


#include <bson.h>
#include <ctype.h>

#include "mongoc-log.h"
#include "mongoc-log-private.h"


BSON_BEGIN_DECLS


#ifdef MONGOC_TRACE
#define TRACE(msg, ...)                   \
   do {                                   \
      mongoc_log (MONGOC_LOG_LEVEL_TRACE, \
                  MONGOC_LOG_DOMAIN,      \
                  "TRACE: %s():%d " msg,  \
                  BSON_FUNC,              \
                  __LINE__,               \
                  __VA_ARGS__);           \
   } while (0)
#define ENTRY                             \
   do {                                   \
      mongoc_log (MONGOC_LOG_LEVEL_TRACE, \
                  MONGOC_LOG_DOMAIN,      \
                  "ENTRY: %s():%d",       \
                  BSON_FUNC,              \
                  __LINE__);              \
   } while (0)
#define EXIT                              \
   do {                                   \
      mongoc_log (MONGOC_LOG_LEVEL_TRACE, \
                  MONGOC_LOG_DOMAIN,      \
                  " EXIT: %s():%d",       \
                  BSON_FUNC,              \
                  __LINE__);              \
      return;                             \
   } while (0)
#define RETURN(ret)                       \
   do {                                   \
      mongoc_log (MONGOC_LOG_LEVEL_TRACE, \
                  MONGOC_LOG_DOMAIN,      \
                  " EXIT: %s():%d",       \
                  BSON_FUNC,              \
                  __LINE__);              \
      return ret;                         \
   } while (0)
#define GOTO(label)                       \
   do {                                   \
      mongoc_log (MONGOC_LOG_LEVEL_TRACE, \
                  MONGOC_LOG_DOMAIN,      \
                  " GOTO: %s():%d %s",    \
                  BSON_FUNC,              \
                  __LINE__,               \
                  #label);                \
      goto label;                         \
   } while (0)
#define DUMP_BYTES(_n, _b, _l)                            \
   do {                                                   \
      mongoc_log (MONGOC_LOG_LEVEL_TRACE,                 \
                  MONGOC_LOG_DOMAIN,                      \
                  "TRACE: %s():%d %s = %p [%d]",          \
                  BSON_FUNC,                              \
                  __LINE__,                               \
                  #_n,                                    \
                  _b,                                     \
                  (int) _l);                              \
      mongoc_log_trace_bytes (MONGOC_LOG_DOMAIN, _b, _l); \
   } while (0)
#define DUMP_IOVEC(_n, _iov, _iovcnt)                            \
   do {                                                          \
      mongoc_log (MONGOC_LOG_LEVEL_TRACE,                        \
                  MONGOC_LOG_DOMAIN,                             \
                  "TRACE: %s():%d %s = %p [%d]",                 \
                  BSON_FUNC,                                     \
                  __LINE__,                                      \
                  #_n,                                           \
                  _iov,                                          \
                  (int) _iovcnt);                                \
      mongoc_log_trace_iovec (MONGOC_LOG_DOMAIN, _iov, _iovcnt); \
   } while (0)
#else
#define TRACE(msg, ...)
#define ENTRY
#define EXIT return
#define RETURN(ret) return ret
#define GOTO(label) goto label
#define DUMP_BYTES(_n, _b, _l)
#define DUMP_IOVEC(_n, _iov, _iovcnt)
#endif


BSON_END_DECLS


#endif /* MONGOC_TRACE_PRIVATE_H */
