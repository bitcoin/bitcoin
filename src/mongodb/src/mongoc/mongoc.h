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


#ifndef MONGOC_H
#define MONGOC_H


#include <bson.h>

#define MONGOC_INSIDE
#include "mongoc-macros.h"
#include "mongoc-apm.h"
#include "mongoc-bulk-operation.h"
#include "mongoc-client.h"
#include "mongoc-client-pool.h"
#include "mongoc-collection.h"
#include "mongoc-config.h"
#include "mongoc-cursor.h"
#include "mongoc-database.h"
#include "mongoc-index.h"
#include "mongoc-error.h"
#include "mongoc-flags.h"
#include "mongoc-gridfs.h"
#include "mongoc-gridfs-file.h"
#include "mongoc-gridfs-file-list.h"
#include "mongoc-gridfs-file-page.h"
#include "mongoc-host-list.h"
#include "mongoc-init.h"
#include "mongoc-matcher.h"
#include "mongoc-handshake.h"
#include "mongoc-opcode.h"
#include "mongoc-log.h"
#include "mongoc-socket.h"
#include "mongoc-stream.h"
#include "mongoc-stream-buffered.h"
#include "mongoc-stream-file.h"
#include "mongoc-stream-gridfs.h"
#include "mongoc-stream-socket.h"
#include "mongoc-uri.h"
#include "mongoc-write-concern.h"
#include "mongoc-version.h"
#include "mongoc-version-functions.h"
#ifdef MONGOC_ENABLE_SSL
#include "mongoc-rand.h"
#include "mongoc-stream-tls.h"
#include "mongoc-ssl.h"
#endif
#undef MONGOC_INSIDE


#endif /* MONGOC_H */
