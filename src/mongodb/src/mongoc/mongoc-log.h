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

#ifndef MONGOC_LOG_H
#define MONGOC_LOG_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"

BSON_BEGIN_DECLS


#ifndef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "mongoc"
#endif


#define MONGOC_ERROR(...) \
   mongoc_log (MONGOC_LOG_LEVEL_ERROR, MONGOC_LOG_DOMAIN, __VA_ARGS__)
#define MONGOC_CRITICAL(...) \
   mongoc_log (MONGOC_LOG_LEVEL_CRITICAL, MONGOC_LOG_DOMAIN, __VA_ARGS__)
#define MONGOC_WARNING(...) \
   mongoc_log (MONGOC_LOG_LEVEL_WARNING, MONGOC_LOG_DOMAIN, __VA_ARGS__)
#define MONGOC_MESSAGE(...) \
   mongoc_log (MONGOC_LOG_LEVEL_MESSAGE, MONGOC_LOG_DOMAIN, __VA_ARGS__)
#define MONGOC_INFO(...) \
   mongoc_log (MONGOC_LOG_LEVEL_INFO, MONGOC_LOG_DOMAIN, __VA_ARGS__)
#define MONGOC_DEBUG(...) \
   mongoc_log (MONGOC_LOG_LEVEL_DEBUG, MONGOC_LOG_DOMAIN, __VA_ARGS__)


typedef enum {
   MONGOC_LOG_LEVEL_ERROR,
   MONGOC_LOG_LEVEL_CRITICAL,
   MONGOC_LOG_LEVEL_WARNING,
   MONGOC_LOG_LEVEL_MESSAGE,
   MONGOC_LOG_LEVEL_INFO,
   MONGOC_LOG_LEVEL_DEBUG,
   MONGOC_LOG_LEVEL_TRACE,
} mongoc_log_level_t;


/**
 * mongoc_log_func_t:
 * @log_level: The level of the log message.
 * @log_domain: The domain of the log message, such as "client".
 * @message: The message generated.
 * @user_data: User data provided to mongoc_log_set_handler().
 *
 * This function prototype can be used to set a custom log handler for the
 * libmongoc library. This is useful if you would like to show them in a
 * user interface or alternate storage.
 */
typedef void (*mongoc_log_func_t) (mongoc_log_level_t log_level,
                                   const char *log_domain,
                                   const char *message,
                                   void *user_data);


/**
 * mongoc_log_set_handler:
 * @log_func: A function to handle log messages.
 * @user_data: User data for @log_func.
 *
 * Sets the function to be called to handle logging.
 */
MONGOC_EXPORT (void)
mongoc_log_set_handler (mongoc_log_func_t log_func, void *user_data);


/**
 * mongoc_log:
 * @log_level: The log level.
 * @log_domain: The log domain (such as "client").
 * @format: The format string for the log message.
 *
 * Logs a message using the currently configured logger.
 *
 * This method will hold a logging lock to prevent concurrent calls to the
 * logging infrastructure. It is important that your configured log function
 * does not re-enter the logging system or deadlock will occur.
 *
 */
MONGOC_EXPORT (void)
mongoc_log (mongoc_log_level_t log_level,
            const char *log_domain,
            const char *format,
            ...) BSON_GNUC_PRINTF (3, 4);


MONGOC_EXPORT (void)
mongoc_log_default_handler (mongoc_log_level_t log_level,
                            const char *log_domain,
                            const char *message,
                            void *user_data);


/**
 * mongoc_log_level_str:
 * @log_level: The log level.
 *
 * Returns: The string representation of log_level
 */
MONGOC_EXPORT (const char *)
mongoc_log_level_str (mongoc_log_level_t log_level);


/**
 * mongoc_log_trace_enable:
 *
 * Enables tracing at runtime (if it has been enabled at compile time).
 */
MONGOC_EXPORT (void)
mongoc_log_trace_enable (void);


/**
 * mongoc_log_trace_disable:
 *
 * Disables tracing at runtime (if it has been enabled at compile time).
 */
MONGOC_EXPORT (void)
mongoc_log_trace_disable (void);


BSON_END_DECLS


#endif /* MONGOC_LOG_H */
