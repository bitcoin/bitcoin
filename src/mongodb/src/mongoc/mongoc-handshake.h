/*
 * Copyright 2016 MongoDB, Inc.
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


#ifndef MONGOC_HANDSHAKE_H
#define MONGOC_HANDSHAKE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"

BSON_BEGIN_DECLS

#define MONGOC_HANDSHAKE_APPNAME_MAX 128

/**
 * mongoc_handshake_data_append:
 *
 * This function is intended for use by drivers which wrap the C Driver.
 * Calling this function will store the given strings as handshake data about
 * the system and driver by appending them to the handshake data for the
 * underlying C Driver. These values, along with other handshake data collected
 * during mongoc_init will be sent to the server as part of the initial
 * connection handshake in the "client" document. This function cannot be
 * called more than once, or after a handshake has been initiated.
 *
 * The passed in strings are copied, and don't have to remain valid after the
 * call to mongoc_handshake_data_append(). The driver may store truncated
 * versions of the passed in strings.
 *
 * Note:
 *   This function allocates memory, and therefore caution should be used when
 *   using this in conjunction with bson_mem_set_vtable. If this function is
 *   called before bson_mem_set_vtable, then bson_mem_restore_vtable must be
 *   called before calling mongoc_cleanup. Failure to do so will result in
 *   memory being freed by the wrong allocator.
 *
 *
 * @driver_name: An optional string storing the name of the wrapping driver
 * @driver_version: An optional string storing the version of the wrapping
 * driver.
 * @platform: An optional string storing any information about the current
 *            platform, for example configure options or compile flags.
 *
 *
 * Returns true if the given fields are set successfully. Otherwise, it returns
 * false and logs an error.
 *
 * The default handshake data the driver sends with "isMaster" looks something
 * like:
 *  client: {
 *    driver: {
 *      name: "mongoc",
 *      version: "1.5.0"
 *    },
 *    os: {...},
 *    platform: "CC=gcc CFLAGS=-Wall -pedantic"
 *  }
 *
 * If we call
 *   mongoc_handshake_data_append ("phongo", "1.1.8", "CC=clang")
 * and it returns true, the driver sends handshake data like:
 *  client: {
 *    driver: {
 *      name: "mongoc / phongo",
 *      version: "1.5.0 / 1.1.8"
 *    },
 *    os: {...},
 *    platform: "CC=gcc CFLAGS=-Wall -pedantic / CC=clang"
 *  }
 *
 */
MONGOC_EXPORT (bool)
mongoc_handshake_data_append (const char *driver_name,
                              const char *driver_version,
                              const char *platform);

BSON_END_DECLS

#endif
