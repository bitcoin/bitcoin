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


#include "mongoc-bulk-operation.h"
#include "mongoc-bulk-operation-private.h"
#include "mongoc-client-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-write-concern-private.h"
#include "mongoc-util-private.h"


/*
 * This is the implementation of both write commands and bulk write commands.
 * They are all implemented as one contiguous set since we'd like to cut down
 * on code duplication here.
 *
 * This implementation is currently naive.
 *
 * Some interesting optimizations might be:
 *
 *   - If unordered mode, send operations as we get them instead of waiting
 *     for execute() to be called. This could save us memcpy()'s too.
 *   - If there is no acknowledgement desired, keep a count of how many
 *     replies we need and ask the socket layer to skip that many bytes
 *     when reading.
 *   - Try to use iovec to send write commands with subdocuments rather than
 *     copying them into the write command document.
 */


mongoc_bulk_operation_t *
mongoc_bulk_operation_new (bool ordered)
{
   mongoc_bulk_operation_t *bulk;

   bulk = (mongoc_bulk_operation_t *) bson_malloc0 (sizeof *bulk);
   bulk->flags.bypass_document_validation =
      MONGOC_BYPASS_DOCUMENT_VALIDATION_DEFAULT;
   bulk->flags.ordered = ordered;
   bulk->server_id = 0;

   _mongoc_array_init (&bulk->commands, sizeof (mongoc_write_command_t));
   _mongoc_write_result_init (&bulk->result);

   return bulk;
}


mongoc_bulk_operation_t *
_mongoc_bulk_operation_new (
   mongoc_client_t *client,                     /* IN */
   const char *database,                        /* IN */
   const char *collection,                      /* IN */
   mongoc_bulk_write_flags_t flags,             /* IN */
   const mongoc_write_concern_t *write_concern) /* IN */
{
   mongoc_bulk_operation_t *bulk;

   BSON_ASSERT (client);
   BSON_ASSERT (collection);

   bulk = mongoc_bulk_operation_new (flags.ordered);
   bulk->client = client;
   bulk->database = bson_strdup (database);
   bulk->collection = bson_strdup (collection);
   bulk->write_concern = mongoc_write_concern_copy (write_concern);
   bulk->executed = false;
   bulk->flags = flags;
   bulk->operation_id = ++client->cluster.operation_id;

   return bulk;
}


void
mongoc_bulk_operation_destroy (mongoc_bulk_operation_t *bulk) /* IN */
{
   mongoc_write_command_t *command;
   int i;

   if (bulk) {
      for (i = 0; i < bulk->commands.len; i++) {
         command =
            &_mongoc_array_index (&bulk->commands, mongoc_write_command_t, i);
         _mongoc_write_command_destroy (command);
      }

      bson_free (bulk->database);
      bson_free (bulk->collection);
      mongoc_write_concern_destroy (bulk->write_concern);
      _mongoc_array_destroy (&bulk->commands);

      if (bulk->executed) {
         _mongoc_write_result_destroy (&bulk->result);
      }

      bson_free (bulk);
   }
}


/* for speed, pre-split batch every 1000 docs. a future server's
 * maxWriteBatchSize may grow larger than the default, then we'll revise. */
#define SHOULD_APPEND(_write_cmd, _write_cmd_type) \
   (((_write_cmd->type) == (_write_cmd_type)) &&   \
    (_write_cmd)->n_documents < MONGOC_DEFAULT_WRITE_BATCH_SIZE)

/* already failed, e.g. a bad call to mongoc_bulk_operation_insert? */
#define BULK_EXIT_IF_PRIOR_ERROR       \
   do {                                \
      if (bulk->result.error.domain) { \
         EXIT;                         \
      }                                \
   } while (0)

#define BULK_RETURN_IF_PRIOR_ERROR                                            \
   do {                                                                       \
      if (bulk->result.error.domain) {                                        \
         if (error != &bulk->result.error) {                                  \
            bson_set_error (error,                                            \
                            MONGOC_ERROR_COMMAND,                             \
                            MONGOC_ERROR_COMMAND_INVALID_ARG,                 \
                            "Bulk operation is invalid from prior error: %s", \
                            bulk->result.error.message);                      \
         };                                                                   \
         return false;                                                        \
      };                                                                      \
   } while (0)


bool
_mongoc_bulk_operation_remove_with_opts (mongoc_bulk_operation_t *bulk,
                                         const bson_t *selector,
                                         const bson_t *opts,
                                         bson_error_t *error) /* OUT */
{
   mongoc_write_command_t command = {0};
   mongoc_write_command_t *last;

   ENTRY;

   BSON_ASSERT (bulk);
   BSON_ASSERT (selector);

   BULK_RETURN_IF_PRIOR_ERROR;

   if (bulk->commands.len) {
      last = &_mongoc_array_index (
         &bulk->commands, mongoc_write_command_t, bulk->commands.len - 1);
      if (SHOULD_APPEND (last, MONGOC_WRITE_COMMAND_DELETE)) {
         _mongoc_write_command_delete_append (last, selector, opts);
         RETURN (true);
      }
   }

   _mongoc_write_command_init_delete (
      &command, selector, opts, bulk->flags, bulk->operation_id);

   _mongoc_array_append_val (&bulk->commands, command);

   RETURN (true);
}

bool
mongoc_bulk_operation_remove_one_with_opts (mongoc_bulk_operation_t *bulk,
                                            const bson_t *selector,
                                            const bson_t *opts,
                                            bson_error_t *error) /* OUT */
{
   bool retval;
   bson_t opts_dup;
   bson_iter_t iter;

   ENTRY;

   BULK_RETURN_IF_PRIOR_ERROR;

   if (opts && bson_iter_init_find (&iter, opts, "limit")) {
      if ((!BSON_ITER_HOLDS_INT (&iter)) || !bson_iter_as_int64 (&iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "%s expects the 'limit' option to be 1",
                         BSON_FUNC);
         RETURN (false);
      }

      return _mongoc_bulk_operation_remove_with_opts (
         bulk, selector, opts, error);
   }

   bson_init (&opts_dup);
   BSON_APPEND_INT32 (&opts_dup, "limit", 1);
   if (opts) {
      bson_concat (&opts_dup, opts);
   }
   retval = _mongoc_bulk_operation_remove_with_opts (
      bulk, selector, &opts_dup, error);
   bson_destroy (&opts_dup);

   RETURN (retval);
}

bool
mongoc_bulk_operation_remove_many_with_opts (mongoc_bulk_operation_t *bulk,
                                             const bson_t *selector,
                                             const bson_t *opts,
                                             bson_error_t *error) /* OUT */
{
   bool retval;
   bson_t opts_dup;
   bson_iter_t iter;

   ENTRY;

   BULK_RETURN_IF_PRIOR_ERROR;

   if (opts && bson_iter_init_find (&iter, opts, "limit")) {
      if ((!BSON_ITER_HOLDS_INT (&iter)) || bson_iter_as_int64 (&iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "%s expects the 'limit' option to be 0",
                         BSON_FUNC);
         RETURN (false);
      }

      RETURN (
         _mongoc_bulk_operation_remove_with_opts (bulk, selector, opts, error));
   }

   bson_init (&opts_dup);
   BSON_APPEND_INT32 (&opts_dup, "limit", 0);
   if (opts) {
      bson_concat (&opts_dup, opts);
   }
   retval = _mongoc_bulk_operation_remove_with_opts (
      bulk, selector, &opts_dup, error);
   bson_destroy (&opts_dup);

   RETURN (retval);
}


void
mongoc_bulk_operation_remove (mongoc_bulk_operation_t *bulk, /* IN */
                              const bson_t *selector)        /* IN */
{
   bson_t opts;
   bson_error_t *error = &bulk->result.error;

   ENTRY;

   BULK_EXIT_IF_PRIOR_ERROR;

   bson_init (&opts);
   BSON_APPEND_INT32 (&opts, "limit", 0);

   mongoc_bulk_operation_remove_many_with_opts (bulk, selector, &opts, error);

   bson_destroy (&opts);

   if (error->domain) {
      MONGOC_WARNING ("%s", error->message);
   }

   EXIT;
}


void
mongoc_bulk_operation_remove_one (mongoc_bulk_operation_t *bulk, /* IN */
                                  const bson_t *selector)        /* IN */
{
   bson_t opts;
   bson_error_t *error = &bulk->result.error;

   ENTRY;

   BULK_EXIT_IF_PRIOR_ERROR;

   bson_init (&opts);
   BSON_APPEND_INT32 (&opts, "limit", 1);

   mongoc_bulk_operation_remove_one_with_opts (bulk, selector, &opts, error);
   bson_destroy (&opts);

   if (error->domain) {
      MONGOC_WARNING ("%s", error->message);
   }

   EXIT;
}

void
mongoc_bulk_operation_delete (mongoc_bulk_operation_t *bulk,
                              const bson_t *selector)
{
   ENTRY;

   mongoc_bulk_operation_remove (bulk, selector);

   EXIT;
}

void
mongoc_bulk_operation_delete_one (mongoc_bulk_operation_t *bulk,
                                  const bson_t *selector)
{
   ENTRY;

   mongoc_bulk_operation_remove_one (bulk, selector);

   EXIT;
}

void
mongoc_bulk_operation_insert (mongoc_bulk_operation_t *bulk,
                              const bson_t *document)
{
   ENTRY;

   BSON_ASSERT (bulk);
   BSON_ASSERT (document);

   if (!mongoc_bulk_operation_insert_with_opts (
          bulk, document, NULL /* opts */, &bulk->result.error)) {
      MONGOC_WARNING ("%s", bulk->result.error.message);
   }

   EXIT;
}

bool
mongoc_bulk_operation_insert_with_opts (mongoc_bulk_operation_t *bulk,
                                        const bson_t *document,
                                        const bson_t *opts,
                                        bson_error_t *error)
{
   mongoc_write_command_t command = {0};
   mongoc_write_command_t *last;
   bson_iter_t iter;

   ENTRY;

   BSON_ASSERT (bulk);
   BSON_ASSERT (document);

   BULK_RETURN_IF_PRIOR_ERROR;

   if (opts && bson_iter_init_find_case (&iter, opts, "legacyIndex") &&
       bson_iter_as_bool (&iter)) {
      if (!_mongoc_validate_legacy_index (document, error)) {
         return false;
      }
   } else if (!_mongoc_validate_new_document (document, error)) {
      return false;
   }

   if (bulk->commands.len) {
      last = &_mongoc_array_index (
         &bulk->commands, mongoc_write_command_t, bulk->commands.len - 1);

      if (SHOULD_APPEND (last, MONGOC_WRITE_COMMAND_INSERT)) {
         _mongoc_write_command_insert_append (last, document);
         return true;
      }
   }

   _mongoc_write_command_init_insert (
      &command,
      document,
      bulk->flags,
      bulk->operation_id,
      !mongoc_write_concern_is_acknowledged (bulk->write_concern));

   _mongoc_array_append_val (&bulk->commands, command);

   return true;
}

bool
_mongoc_bulk_operation_replace_one_with_opts (mongoc_bulk_operation_t *bulk,
                                              const bson_t *selector,
                                              const bson_t *document,
                                              const bson_t *opts,
                                              bson_error_t *error) /* OUT */
{
   mongoc_write_command_t command = {0};
   mongoc_write_command_t *last;

   ENTRY;

   BULK_RETURN_IF_PRIOR_ERROR;

   BSON_ASSERT (bulk);
   BSON_ASSERT (selector);
   BSON_ASSERT (document);

   if (!_mongoc_validate_replace (document, error)) {
      RETURN (false);
   }

   if (bulk->commands.len) {
      last = &_mongoc_array_index (
         &bulk->commands, mongoc_write_command_t, bulk->commands.len - 1);
      if (SHOULD_APPEND (last, MONGOC_WRITE_COMMAND_UPDATE)) {
         _mongoc_write_command_update_append (last, selector, document, opts);
         RETURN (true);
      }
   }

   _mongoc_write_command_init_update (
      &command, selector, document, opts, bulk->flags, bulk->operation_id);
   _mongoc_array_append_val (&bulk->commands, command);

   RETURN (true);
}

void
mongoc_bulk_operation_replace_one (mongoc_bulk_operation_t *bulk,
                                   const bson_t *selector,
                                   const bson_t *document,
                                   bool upsert)
{
   bson_t opts;
   bson_error_t *error = &bulk->result.error;

   ENTRY;

   bson_init (&opts);
   BSON_APPEND_BOOL (&opts, "upsert", upsert);
   BSON_APPEND_BOOL (&opts, "multi", false);

   _mongoc_bulk_operation_replace_one_with_opts (
      bulk, selector, document, &opts, error);
   bson_destroy (&opts);

   if (error->domain) {
      MONGOC_WARNING ("%s", error->message);
   }

   EXIT;
}

bool
mongoc_bulk_operation_replace_one_with_opts (mongoc_bulk_operation_t *bulk,
                                             const bson_t *selector,
                                             const bson_t *document,
                                             const bson_t *opts,
                                             bson_error_t *error) /* OUT */
{
   bson_iter_t iter;
   bson_t opts_dup;
   bool retval;

   ENTRY;

   BSON_ASSERT (bulk);
   BSON_ASSERT (selector);
   BSON_ASSERT (document);

   if (opts && bson_iter_init_find (&iter, opts, "multi")) {
      if (!BSON_ITER_HOLDS_BOOL (&iter) || bson_iter_bool (&iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "%s expects the 'multi' option to be false",
                         BSON_FUNC);
         RETURN (false);
      }
      retval = _mongoc_bulk_operation_replace_one_with_opts (
         bulk, selector, document, opts, error);
   } else {
      bson_init (&opts_dup);
      BSON_APPEND_BOOL (&opts_dup, "multi", false);

      if (opts) {
         bson_concat (&opts_dup, opts);
      }
      retval = _mongoc_bulk_operation_replace_one_with_opts (
         bulk, selector, document, &opts_dup, error);
      bson_destroy (&opts_dup);
   }

   RETURN (retval);
}

bool
_mongoc_bulk_operation_update_with_opts (mongoc_bulk_operation_t *bulk,
                                         const bson_t *selector,
                                         const bson_t *document,
                                         const bson_t *opts,
                                         bson_error_t *error) /* OUT */
{
   mongoc_write_command_t command = {0};
   mongoc_write_command_t *last;

   ENTRY;

   BSON_ASSERT (bulk);
   BSON_ASSERT (selector);
   BSON_ASSERT (document);

   BULK_RETURN_IF_PRIOR_ERROR;

   if (!_mongoc_validate_update (document, error)) {
      RETURN (false);
   }

   if (bulk->commands.len) {
      last = &_mongoc_array_index (
         &bulk->commands, mongoc_write_command_t, bulk->commands.len - 1);
      if (SHOULD_APPEND (last, MONGOC_WRITE_COMMAND_UPDATE)) {
         _mongoc_write_command_update_append (last, selector, document, opts);
         RETURN (true);
      }
   }

   _mongoc_write_command_init_update (
      &command, selector, document, opts, bulk->flags, bulk->operation_id);
   _mongoc_array_append_val (&bulk->commands, command);

   RETURN (true);
}

bool
mongoc_bulk_operation_update_one_with_opts (mongoc_bulk_operation_t *bulk,
                                            const bson_t *selector,
                                            const bson_t *document,
                                            const bson_t *opts,
                                            bson_error_t *error) /* OUT */
{
   bool retval;
   bson_t opts_dup;
   bson_iter_t iter;

   ENTRY;

   if (opts && bson_iter_init_find (&iter, opts, "multi")) {
      if (!BSON_ITER_HOLDS_BOOL (&iter) || bson_iter_bool (&iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "%s expects the 'multi' option to be false",
                         BSON_FUNC);
         RETURN (false);
      }

      RETURN (_mongoc_bulk_operation_update_with_opts (
         bulk, selector, document, opts, error));
   }

   bson_init (&opts_dup);
   BSON_APPEND_BOOL (&opts_dup, "multi", false);
   if (opts) {
      bson_concat (&opts_dup, opts);
   }
   retval = _mongoc_bulk_operation_update_with_opts (
      bulk, selector, document, &opts_dup, error);
   bson_destroy (&opts_dup);

   RETURN (retval);
}

bool
mongoc_bulk_operation_update_many_with_opts (mongoc_bulk_operation_t *bulk,
                                             const bson_t *selector,
                                             const bson_t *document,
                                             const bson_t *opts,
                                             bson_error_t *error) /* OUT */
{
   bool retval;
   bson_t opts_dup;
   bson_iter_t iter;

   ENTRY;

   if (opts && bson_iter_init_find (&iter, opts, "multi")) {
      if (!BSON_ITER_HOLDS_BOOL (&iter) || !bson_iter_bool (&iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "%s expects the 'multi' option to be true",
                         BSON_FUNC);
         RETURN (false);
      }

      return _mongoc_bulk_operation_update_with_opts (
         bulk, selector, document, opts, error);
   }

   bson_init (&opts_dup);
   BSON_APPEND_BOOL (&opts_dup, "multi", true);
   if (opts) {
      bson_concat (&opts_dup, opts);
   }
   retval = _mongoc_bulk_operation_update_with_opts (
      bulk, selector, document, &opts_dup, error);
   bson_destroy (&opts_dup);

   RETURN (retval);
}

void
mongoc_bulk_operation_update (mongoc_bulk_operation_t *bulk,
                              const bson_t *selector,
                              const bson_t *document,
                              bool upsert)
{
   bson_t opts;
   bson_error_t *error = &bulk->result.error;

   ENTRY;

   BULK_EXIT_IF_PRIOR_ERROR;

   bson_init (&opts);
   BSON_APPEND_BOOL (&opts, "upsert", upsert);
   BSON_APPEND_BOOL (&opts, "multi", true);

   _mongoc_bulk_operation_update_with_opts (
      bulk, selector, document, &opts, error);

   bson_destroy (&opts);

   if (error->domain) {
      MONGOC_WARNING ("%s", error->message);
   }

   EXIT;
}

void
mongoc_bulk_operation_update_one (mongoc_bulk_operation_t *bulk,
                                  const bson_t *selector,
                                  const bson_t *document,
                                  bool upsert)
{
   bson_t opts;
   bson_error_t *error = &bulk->result.error;

   ENTRY;

   BULK_EXIT_IF_PRIOR_ERROR;

   bson_init (&opts);
   BSON_APPEND_BOOL (&opts, "upsert", upsert);
   BSON_APPEND_BOOL (&opts, "multi", false);

   _mongoc_bulk_operation_update_with_opts (
      bulk, selector, document, &opts, error);

   bson_destroy (&opts);

   if (error->domain) {
      MONGOC_WARNING ("%s", error->message);
   }

   EXIT;
}

uint32_t
mongoc_bulk_operation_execute (mongoc_bulk_operation_t *bulk, /* IN */
                               bson_t *reply,                 /* OUT */
                               bson_error_t *error)           /* OUT */
{
   mongoc_cluster_t *cluster;
   mongoc_write_command_t *command;
   mongoc_server_stream_t *server_stream;
   bool ret;
   uint32_t offset = 0;
   int i;

   ENTRY;

   BSON_ASSERT (bulk);

   if (!bulk->client) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "mongoc_bulk_operation_execute() requires a client "
                      "and one has not been set.");
      RETURN (false);
   }
   cluster = &bulk->client->cluster;

   if (bulk->executed) {
      _mongoc_write_result_destroy (&bulk->result);
   }

   bulk->executed = true;

   if (reply) {
      bson_init (reply);
   }

   if (!bulk->database) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "mongoc_bulk_operation_execute() requires a database "
                      "and one has not been set.");
      RETURN (false);
   } else if (!bulk->collection) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "mongoc_bulk_operation_execute() requires a collection "
                      "and one has not been set.");
      RETURN (false);
   }

   /* error stored by functions like mongoc_bulk_operation_insert that
    * can't report errors immediately */
   if (bulk->result.error.domain) {
      if (error) {
         memcpy (error, &bulk->result.error, sizeof (bson_error_t));
      }

      RETURN (false);
   }

   if (!bulk->commands.len) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Cannot do an empty bulk write");
      RETURN (false);
   }

   if (bulk->server_id) {
      server_stream = mongoc_cluster_stream_for_server (
         cluster, bulk->server_id, true /* reconnect_ok */, error);
   } else {
      server_stream = mongoc_cluster_stream_for_writes (cluster, error);
   }

   if (!server_stream) {
      RETURN (false);
   }

   for (i = 0; i < bulk->commands.len; i++) {
      command =
         &_mongoc_array_index (&bulk->commands, mongoc_write_command_t, i);

      _mongoc_write_command_execute (command,
                                     bulk->client,
                                     server_stream,
                                     bulk->database,
                                     bulk->collection,
                                     bulk->write_concern,
                                     offset,
                                     &bulk->result);

      bulk->server_id = server_stream->sd->id;

      if (bulk->result.failed &&
          (bulk->flags.ordered || bulk->result.must_stop)) {
         GOTO (cleanup);
      }

      offset += command->n_documents;
   }

cleanup:
   ret = _mongoc_write_result_complete (&bulk->result,
                                        bulk->client->error_api_version,
                                        bulk->write_concern,
                                        MONGOC_ERROR_COMMAND /* err domain */,
                                        reply,
                                        error);
   mongoc_server_stream_cleanup (server_stream);

   RETURN (ret ? bulk->server_id : 0);
}

void
mongoc_bulk_operation_set_write_concern (
   mongoc_bulk_operation_t *bulk, const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (bulk);

   if (bulk->write_concern) {
      mongoc_write_concern_destroy (bulk->write_concern);
   }

   if (write_concern) {
      bulk->write_concern = mongoc_write_concern_copy (write_concern);
   } else {
      bulk->write_concern = mongoc_write_concern_new ();
   }
}

const mongoc_write_concern_t *
mongoc_bulk_operation_get_write_concern (const mongoc_bulk_operation_t *bulk)
{
   BSON_ASSERT (bulk);

   return bulk->write_concern;
}


void
mongoc_bulk_operation_set_database (mongoc_bulk_operation_t *bulk,
                                    const char *database)
{
   BSON_ASSERT (bulk);

   if (bulk->database) {
      bson_free (bulk->database);
   }

   bulk->database = bson_strdup (database);
}


void
mongoc_bulk_operation_set_collection (mongoc_bulk_operation_t *bulk,
                                      const char *collection)
{
   BSON_ASSERT (bulk);

   if (bulk->collection) {
      bson_free (bulk->collection);
   }

   bulk->collection = bson_strdup (collection);
}


void
mongoc_bulk_operation_set_client (mongoc_bulk_operation_t *bulk, void *client)
{
   BSON_ASSERT (bulk);

   bulk->client = (mongoc_client_t *) client;

   /* if you call set_client, bulk was likely made by mongoc_bulk_operation_new,
    * not mongoc_collection_create_bulk_operation(), so operation_id is 0. */
   if (!bulk->operation_id) {
      bulk->operation_id = ++bulk->client->cluster.operation_id;
   }
}


uint32_t
mongoc_bulk_operation_get_hint (const mongoc_bulk_operation_t *bulk)
{
   BSON_ASSERT (bulk);

   return bulk->server_id;
}


void
mongoc_bulk_operation_set_hint (mongoc_bulk_operation_t *bulk,
                                uint32_t server_id)
{
   BSON_ASSERT (bulk);

   bulk->server_id = server_id;
}


void
mongoc_bulk_operation_set_bypass_document_validation (
   mongoc_bulk_operation_t *bulk, bool bypass)
{
   BSON_ASSERT (bulk);

   bulk->flags.bypass_document_validation =
      bypass ? MONGOC_BYPASS_DOCUMENT_VALIDATION_TRUE
             : MONGOC_BYPASS_DOCUMENT_VALIDATION_FALSE;
}
