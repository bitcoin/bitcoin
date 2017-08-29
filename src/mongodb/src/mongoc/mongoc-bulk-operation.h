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


#ifndef MONGOC_BULK_OPERATION_H
#define MONGOC_BULK_OPERATION_H


#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-write-concern.h"

#define MONGOC_BULK_WRITE_FLAGS_INIT                     \
   {                                                     \
      true, MONGOC_BYPASS_DOCUMENT_VALIDATION_DEFAULT, 0 \
   }

BSON_BEGIN_DECLS


typedef struct _mongoc_bulk_operation_t mongoc_bulk_operation_t;
typedef struct _mongoc_bulk_write_flags_t mongoc_bulk_write_flags_t;


MONGOC_EXPORT (void)
mongoc_bulk_operation_destroy (mongoc_bulk_operation_t *bulk);
MONGOC_EXPORT (uint32_t)
mongoc_bulk_operation_execute (mongoc_bulk_operation_t *bulk,
                               bson_t *reply,
                               bson_error_t *error);
MONGOC_EXPORT (void)
mongoc_bulk_operation_delete (mongoc_bulk_operation_t *bulk,
                              const bson_t *selector)
   BSON_GNUC_DEPRECATED_FOR (mongoc_bulk_operation_remove);
MONGOC_EXPORT (void)
mongoc_bulk_operation_delete_one (mongoc_bulk_operation_t *bulk,
                                  const bson_t *selector)
   BSON_GNUC_DEPRECATED_FOR (mongoc_bulk_operation_remove_one);
MONGOC_EXPORT (void)
mongoc_bulk_operation_insert (mongoc_bulk_operation_t *bulk,
                              const bson_t *document);
MONGOC_EXPORT (bool)
mongoc_bulk_operation_insert_with_opts (mongoc_bulk_operation_t *bulk,
                                        const bson_t *document,
                                        const bson_t *opts,
                                        bson_error_t *error); /* OUT */
MONGOC_EXPORT (void)
mongoc_bulk_operation_remove (mongoc_bulk_operation_t *bulk,
                              const bson_t *selector);
MONGOC_EXPORT (bool)
mongoc_bulk_operation_remove_many_with_opts (mongoc_bulk_operation_t *bulk,
                                             const bson_t *selector,
                                             const bson_t *opts,
                                             bson_error_t *error); /* OUT */
MONGOC_EXPORT (void)
mongoc_bulk_operation_remove_one (mongoc_bulk_operation_t *bulk,
                                  const bson_t *selector);
MONGOC_EXPORT (bool)
mongoc_bulk_operation_remove_one_with_opts (mongoc_bulk_operation_t *bulk,
                                            const bson_t *selector,
                                            const bson_t *opts,
                                            bson_error_t *error); /* OUT */
MONGOC_EXPORT (void)
mongoc_bulk_operation_replace_one (mongoc_bulk_operation_t *bulk,
                                   const bson_t *selector,
                                   const bson_t *document,
                                   bool upsert);
MONGOC_EXPORT (bool)
mongoc_bulk_operation_replace_one_with_opts (mongoc_bulk_operation_t *bulk,
                                             const bson_t *selector,
                                             const bson_t *document,
                                             const bson_t *opts,
                                             bson_error_t *error); /* OUT */
MONGOC_EXPORT (void)
mongoc_bulk_operation_update (mongoc_bulk_operation_t *bulk,
                              const bson_t *selector,
                              const bson_t *document,
                              bool upsert);
MONGOC_EXPORT (bool)
mongoc_bulk_operation_update_many_with_opts (mongoc_bulk_operation_t *bulk,
                                             const bson_t *selector,
                                             const bson_t *document,
                                             const bson_t *opts,
                                             bson_error_t *error); /* OUT */
MONGOC_EXPORT (void)
mongoc_bulk_operation_update_one (mongoc_bulk_operation_t *bulk,
                                  const bson_t *selector,
                                  const bson_t *document,
                                  bool upsert);
MONGOC_EXPORT (bool)
mongoc_bulk_operation_update_one_with_opts (mongoc_bulk_operation_t *bulk,
                                            const bson_t *selector,
                                            const bson_t *document,
                                            const bson_t *opts,
                                            bson_error_t *error); /* OUT */
MONGOC_EXPORT (void)
mongoc_bulk_operation_set_bypass_document_validation (
   mongoc_bulk_operation_t *bulk, bool bypass);


/*
 * The following functions are really only useful by language bindings and
 * those wanting to replay a bulk operation to a number of clients or
 * collections.
 */
MONGOC_EXPORT (mongoc_bulk_operation_t *)
mongoc_bulk_operation_new (bool ordered);
MONGOC_EXPORT (void)
mongoc_bulk_operation_set_write_concern (
   mongoc_bulk_operation_t *bulk, const mongoc_write_concern_t *write_concern);
MONGOC_EXPORT (void)
mongoc_bulk_operation_set_database (mongoc_bulk_operation_t *bulk,
                                    const char *database);
MONGOC_EXPORT (void)
mongoc_bulk_operation_set_collection (mongoc_bulk_operation_t *bulk,
                                      const char *collection);
MONGOC_EXPORT (void)
mongoc_bulk_operation_set_client (mongoc_bulk_operation_t *bulk, void *client);
/* These names include the term "hint" for backward compatibility, should be
 * mongoc_bulk_operation_get_server_id, mongoc_bulk_operation_set_server_id. */
MONGOC_EXPORT (void)
mongoc_bulk_operation_set_hint (mongoc_bulk_operation_t *bulk,
                                uint32_t server_id);
MONGOC_EXPORT (uint32_t)
mongoc_bulk_operation_get_hint (const mongoc_bulk_operation_t *bulk);
MONGOC_EXPORT (const mongoc_write_concern_t *)
mongoc_bulk_operation_get_write_concern (const mongoc_bulk_operation_t *bulk);
BSON_END_DECLS


#endif /* MONGOC_BULK_OPERATION_H */
