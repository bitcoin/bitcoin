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

#include <bson.h>

#include "mongoc-client-private.h"
#include "mongoc-error.h"
#include "mongoc-trace-private.h"
#include "mongoc-write-command-private.h"
#include "mongoc-write-concern-private.h"
#include "mongoc-util-private.h"


/*
 * TODO:
 *
 *    - Remove error parameter to ops, favor result->error.
 */

#define WRITE_CONCERN_DOC(wc)                                                \
   (wc) ? (_mongoc_write_concern_get_bson ((mongoc_write_concern_t *) (wc))) \
        : (&gEmptyWriteConcern)

typedef void (*mongoc_write_op_t) (mongoc_write_command_t *command,
                                   mongoc_client_t *client,
                                   mongoc_server_stream_t *server_stream,
                                   const char *database,
                                   const char *collection,
                                   const mongoc_write_concern_t *write_concern,
                                   uint32_t offset,
                                   mongoc_write_result_t *result,
                                   bson_error_t *error);


static bson_t gEmptyWriteConcern = BSON_INITIALIZER;

/* indexed by MONGOC_WRITE_COMMAND_DELETE, INSERT, UPDATE */
static const char *gCommandNames[] = {"delete", "insert", "update"};
static const char *gCommandFields[] = {"deletes", "documents", "updates"};
static const uint32_t gCommandFieldLens[] = {7, 9, 7};

static int32_t
_mongoc_write_result_merge_arrays (uint32_t offset,
                                   mongoc_write_result_t *result,
                                   bson_t *dest,
                                   bson_iter_t *iter);

static bool
_is_duplicate_key_error (int32_t code)
{
   return code == 11000 || code == 16460 || /* see SERVER-11493 */
          code == 11001 || /* duplicate key for updates before 2.6 */
          code == 12582;   /* mongos before 2.6 */
}


void
_mongoc_write_command_insert_append (mongoc_write_command_t *command,
                                     const bson_t *document)
{
   const char *key;
   bson_iter_t iter;
   bson_oid_t oid;
   bson_t tmp;
   char keydata[16];

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (command->type == MONGOC_WRITE_COMMAND_INSERT);
   BSON_ASSERT (document);
   BSON_ASSERT (document->len >= 5);

   key = NULL;
   bson_uint32_to_string (command->n_documents, &key, keydata, sizeof keydata);

   BSON_ASSERT (key);

   /*
    * If the document does not contain an "_id" field, we need to generate
    * a new oid for "_id".
    */
   if (!bson_iter_init_find (&iter, document, "_id")) {
      bson_init (&tmp);
      bson_oid_init (&oid, NULL);
      BSON_APPEND_OID (&tmp, "_id", &oid);
      bson_concat (&tmp, document);
      BSON_APPEND_DOCUMENT (command->documents, key, &tmp);
      bson_destroy (&tmp);
   } else {
      BSON_APPEND_DOCUMENT (command->documents, key, document);
   }

   command->n_documents++;

   EXIT;
}

void
_mongoc_write_command_update_append (mongoc_write_command_t *command,
                                     const bson_t *selector,
                                     const bson_t *update,
                                     const bson_t *opts)
{
   const char *key;
   char keydata[16];
   bson_t doc;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (command->type == MONGOC_WRITE_COMMAND_UPDATE);
   BSON_ASSERT (selector && update);

   bson_init (&doc);
   BSON_APPEND_DOCUMENT (&doc, "q", selector);
   BSON_APPEND_DOCUMENT (&doc, "u", update);
   if (opts) {
      bson_concat (&doc, opts);
      command->flags.has_collation |= bson_has_field (opts, "collation");
   }

   key = NULL;
   bson_uint32_to_string (command->n_documents, &key, keydata, sizeof keydata);
   BSON_ASSERT (key);
   BSON_APPEND_DOCUMENT (command->documents, key, &doc);
   command->n_documents++;

   bson_destroy (&doc);

   EXIT;
}

void
_mongoc_write_command_delete_append (mongoc_write_command_t *command,
                                     const bson_t *selector,
                                     const bson_t *opts)
{
   const char *key;
   char keydata[16];
   bson_t doc;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (command->type == MONGOC_WRITE_COMMAND_DELETE);
   BSON_ASSERT (selector);

   BSON_ASSERT (selector->len >= 5);

   bson_init (&doc);
   BSON_APPEND_DOCUMENT (&doc, "q", selector);
   if (opts) {
      bson_concat (&doc, opts);
      command->flags.has_collation |= bson_has_field (opts, "collation");
   }

   key = NULL;
   bson_uint32_to_string (command->n_documents, &key, keydata, sizeof keydata);
   BSON_ASSERT (key);
   BSON_APPEND_DOCUMENT (command->documents, key, &doc);
   command->n_documents++;

   bson_destroy (&doc);

   EXIT;
}

void
_mongoc_write_command_init_insert (mongoc_write_command_t *command, /* IN */
                                   const bson_t *document,          /* IN */
                                   mongoc_bulk_write_flags_t flags, /* IN */
                                   int64_t operation_id,            /* IN */
                                   bool allow_bulk_op_insert)       /* IN */
{
   ENTRY;

   BSON_ASSERT (command);

   command->type = MONGOC_WRITE_COMMAND_INSERT;
   command->documents = bson_new ();
   command->n_documents = 0;
   command->flags = flags;
   command->u.insert.allow_bulk_op_insert = (uint8_t) allow_bulk_op_insert;
   command->operation_id = operation_id;

   /* must handle NULL document from mongoc_collection_insert_bulk */
   if (document) {
      _mongoc_write_command_insert_append (command, document);
   }

   EXIT;
}


void
_mongoc_write_command_init_delete (mongoc_write_command_t *command, /* IN */
                                   const bson_t *selector,          /* IN */
                                   const bson_t *opts,              /* IN */
                                   mongoc_bulk_write_flags_t flags, /* IN */
                                   int64_t operation_id)            /* IN */
{
   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (selector);

   command->type = MONGOC_WRITE_COMMAND_DELETE;
   command->documents = bson_new ();
   command->n_documents = 0;
   command->flags = flags;
   command->operation_id = operation_id;

   _mongoc_write_command_delete_append (command, selector, opts);

   EXIT;
}


void
_mongoc_write_command_init_update (mongoc_write_command_t *command, /* IN */
                                   const bson_t *selector,          /* IN */
                                   const bson_t *update,            /* IN */
                                   const bson_t *opts,              /* IN */
                                   mongoc_bulk_write_flags_t flags, /* IN */
                                   int64_t operation_id)            /* IN */
{
   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (selector);
   BSON_ASSERT (update);

   command->type = MONGOC_WRITE_COMMAND_UPDATE;
   command->documents = bson_new ();
   command->n_documents = 0;
   command->flags = flags;
   command->operation_id = operation_id;

   _mongoc_write_command_update_append (command, selector, update, opts);

   EXIT;
}


/* takes initialized bson_t *doc and begins formatting a write command */
static void
_mongoc_write_command_init (bson_t *doc,
                            mongoc_write_command_t *command,
                            const char *collection,
                            const mongoc_write_concern_t *write_concern)
{
   bson_iter_t iter;

   ENTRY;

   if (!command->n_documents || !bson_iter_init (&iter, command->documents) ||
       !bson_iter_next (&iter)) {
      EXIT;
   }

   BSON_APPEND_UTF8 (doc, gCommandNames[command->type], collection);
   BSON_APPEND_DOCUMENT (
      doc, "writeConcern", WRITE_CONCERN_DOC (write_concern));
   BSON_APPEND_BOOL (doc, "ordered", command->flags.ordered);

   if (command->flags.bypass_document_validation !=
       MONGOC_BYPASS_DOCUMENT_VALIDATION_DEFAULT) {
      BSON_APPEND_BOOL (doc,
                        "bypassDocumentValidation",
                        !!command->flags.bypass_document_validation);
   }

   EXIT;
}


static void
_mongoc_monitor_legacy_write (mongoc_client_t *client,
                              mongoc_write_command_t *command,
                              const char *db,
                              const char *collection,
                              const mongoc_write_concern_t *write_concern,
                              mongoc_server_stream_t *stream,
                              int64_t request_id)
{
   bson_t doc;
   mongoc_apm_command_started_t event;

   ENTRY;

   if (!client->apm_callbacks.started) {
      EXIT;
   }

   bson_init (&doc);
   _mongoc_write_command_init (&doc, command, collection, write_concern);

   /* copy the whole documents buffer as e.g. "updates": [...] */
   BSON_APPEND_ARRAY (&doc, gCommandFields[command->type], command->documents);

   mongoc_apm_command_started_init (&event,
                                    &doc,
                                    db,
                                    gCommandNames[command->type],
                                    request_id,
                                    command->operation_id,
                                    &stream->sd->host,
                                    stream->sd->id,
                                    client->apm_context);

   client->apm_callbacks.started (&event);

   mongoc_apm_command_started_cleanup (&event);
   bson_destroy (&doc);
}


static void
append_write_err (bson_t *doc,
                  uint32_t code,
                  const char *errmsg,
                  size_t errmsg_len,
                  const bson_t *errinfo)
{
   bson_t array = BSON_INITIALIZER;
   bson_t child;

   BSON_ASSERT (errmsg);

   /* writeErrors: [{index: 0, code: code, errmsg: errmsg, errInfo: {...}}] */
   bson_append_document_begin (&array, "0", 1, &child);
   bson_append_int32 (&child, "index", 5, 0);
   bson_append_int32 (&child, "code", 4, (int32_t) code);
   bson_append_utf8 (&child, "errmsg", 6, errmsg, (int) errmsg_len);
   if (errinfo) {
      bson_append_document (&child, "errInfo", 7, errinfo);
   }

   bson_append_document_end (&array, &child);
   bson_append_array (doc, "writeErrors", 11, &array);

   bson_destroy (&array);
}


static void
append_write_concern_err (bson_t *doc, const char *errmsg, size_t errmsg_len)
{
   bson_t array = BSON_INITIALIZER;
   bson_t child;
   bson_t errinfo;

   BSON_ASSERT (errmsg);

   /* writeConcernErrors: [{code: 64,
    *                       errmsg: errmsg,
    *                       errInfo: {wtimeout: true}}] */
   bson_append_document_begin (&array, "0", 1, &child);
   bson_append_int32 (&child, "code", 4, 64);
   bson_append_utf8 (&child, "errmsg", 6, errmsg, (int) errmsg_len);
   bson_append_document_begin (&child, "errInfo", 7, &errinfo);
   bson_append_bool (&errinfo, "wtimeout", 8, true);
   bson_append_document_end (&child, &errinfo);
   bson_append_document_end (&array, &child);
   bson_append_array (doc, "writeConcernErrors", 18, &array);

   bson_destroy (&array);
}


static bool
get_upserted_id (const bson_t *update, bson_value_t *upserted_id)
{
   bson_iter_t iter;
   bson_iter_t id_iter;

   /* Versions of MongoDB before 2.6 don't return the _id for an upsert if _id
    * is not an ObjectId, so find it in the update document's query "q" or
    * update "u". It must be in one or both: if it were in neither the _id
    * would be server-generated, therefore an ObjectId, therefore returned and
    * we wouldn't call this function. If _id is in both the update document
    * *and* the query spec the update document _id takes precedence.
    */

   bson_iter_init (&iter, update);

   if (bson_iter_find_descendant (&iter, "u._id", &id_iter)) {
      bson_value_copy (bson_iter_value (&id_iter), upserted_id);
      return true;
   } else {
      bson_iter_init (&iter, update);

      if (bson_iter_find_descendant (&iter, "q._id", &id_iter)) {
         bson_value_copy (bson_iter_value (&id_iter), upserted_id);
         return true;
      }
   }

   /* server bug? */
   return false;
}


static void
append_upserted (bson_t *doc, const bson_value_t *upserted_id)
{
   bson_t array = BSON_INITIALIZER;
   bson_t child;

   /* append upserted: [{index: 0, _id: upserted_id}]*/
   bson_append_document_begin (&array, "0", 1, &child);
   bson_append_int32 (&child, "index", 5, 0);
   bson_append_value (&child, "_id", 3, upserted_id);
   bson_append_document_end (&array, &child);

   bson_append_array (doc, "upserted", 8, &array);

   bson_destroy (&array);
}


/* fire command-succeeded event as if we'd used a modern write command.
 * note, cluster.request_id was incremented once for the write, again
 * for the getLastError, so cluster.request_id is no longer valid; used the
 * passed-in request_id instead.
 */
static void
_mongoc_monitor_legacy_write_succeeded (mongoc_client_t *client,
                                        int64_t duration,
                                        mongoc_write_command_t *command,
                                        const bson_t *gle,
                                        mongoc_server_stream_t *stream,
                                        int64_t request_id)
{
   bson_iter_t iter;
   bson_t doc;
   int64_t ok = 1;
   int64_t n = 0;
   uint32_t code = 8;
   bool wtimeout = false;

   /* server error message */
   const char *errmsg = NULL;
   size_t errmsg_len = 0;

   /* server errInfo subdocument */
   bool has_errinfo = false;
   uint32_t len;
   const uint8_t *data;
   bson_t errinfo;

   /* server upsertedId value */
   bool has_upserted_id = false;
   bson_value_t upserted_id;

   /* server updatedExisting value */
   bool has_updated_existing = false;
   bool updated_existing = false;

   mongoc_apm_command_succeeded_t event;

   ENTRY;

   if (!client->apm_callbacks.succeeded) {
      EXIT;
   }

   /* first extract interesting fields from getlasterror response */
   if (gle) {
      bson_iter_init (&iter, gle);
      while (bson_iter_next (&iter)) {
         if (!strcmp (bson_iter_key (&iter), "ok")) {
            ok = bson_iter_as_int64 (&iter);
         } else if (!strcmp (bson_iter_key (&iter), "n")) {
            n = bson_iter_as_int64 (&iter);
         } else if (!strcmp (bson_iter_key (&iter), "code")) {
            code = (uint32_t) bson_iter_as_int64 (&iter);
            if (code == 0) {
               /* server sent non-numeric error code? */
               code = 8;
            }
         } else if (!strcmp (bson_iter_key (&iter), "upserted")) {
            has_upserted_id = true;
            bson_value_copy (bson_iter_value (&iter), &upserted_id);
         } else if (!strcmp (bson_iter_key (&iter), "updatedExisting")) {
            has_updated_existing = true;
            updated_existing = bson_iter_as_bool (&iter);
         } else if ((!strcmp (bson_iter_key (&iter), "err") ||
                     !strcmp (bson_iter_key (&iter), "errmsg")) &&
                    BSON_ITER_HOLDS_UTF8 (&iter)) {
            errmsg = bson_iter_utf8_unsafe (&iter, &errmsg_len);
         } else if (!strcmp (bson_iter_key (&iter), "errInfo") &&
                    BSON_ITER_HOLDS_DOCUMENT (&iter)) {
            bson_iter_document (&iter, &len, &data);
            bson_init_static (&errinfo, data, len);
            has_errinfo = true;
         } else if (!strcmp (bson_iter_key (&iter), "wtimeout")) {
            wtimeout = true;
         }
      }
   }

   /* based on PyMongo's _convert_write_result() */
   bson_init (&doc);
   bson_append_int32 (&doc, "ok", 2, (int32_t) ok);

   if (errmsg && !wtimeout) {
      /* Failure, but pass to the success callback. Command Monitoring Spec:
       * "Commands that executed on the server and return a status of {ok: 1}
       * are considered successful commands and fire CommandSucceededEvent.
       * Commands that have write errors are included since the actual command
       * did succeed, only writes failed." */
      append_write_err (
         &doc, code, errmsg, errmsg_len, has_errinfo ? &errinfo : NULL);
   } else {
      /* Success, perhaps with a writeConcernError. */
      if (errmsg) {
         append_write_concern_err (&doc, errmsg, errmsg_len);
      }

      if (command->type == MONGOC_WRITE_COMMAND_INSERT) {
         /* GLE result for insert is always 0 in most MongoDB versions. */
         n = command->n_documents;
      } else if (command->type == MONGOC_WRITE_COMMAND_UPDATE) {
         if (has_upserted_id) {
            append_upserted (&doc, &upserted_id);
         } else if (has_updated_existing && !updated_existing && n == 1) {
            has_upserted_id =
               get_upserted_id (&command->documents[0], &upserted_id);

            if (has_upserted_id) {
               append_upserted (&doc, &upserted_id);
            }
         }
      }
   }

   bson_append_int32 (&doc, "n", 1, (int32_t) n);

   mongoc_apm_command_succeeded_init (&event,
                                      duration,
                                      &doc,
                                      gCommandNames[command->type],
                                      request_id,
                                      command->operation_id,
                                      &stream->sd->host,
                                      stream->sd->id,
                                      client->apm_context);

   client->apm_callbacks.succeeded (&event);

   mongoc_apm_command_succeeded_cleanup (&event);
   bson_destroy (&doc);

   if (has_upserted_id) {
      bson_value_destroy (&upserted_id);
   }

   EXIT;
}


/*
 *-------------------------------------------------------------------------
 *
 * too_large_error --
 *
 *       Fill a bson_error_t and optional bson_t with error info after
 *       receiving a document for bulk insert, update, or remove that is
 *       larger than max_bson_size.
 *
 *       "err_doc" should be NULL or an empty initialized bson_t.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       "error" and optionally "err_doc" are filled out.
 *
 *-------------------------------------------------------------------------
 */

static void
too_large_error (bson_error_t *error,
                 int32_t idx,
                 int32_t len,
                 int32_t max_bson_size,
                 bson_t *err_doc)
{
   bson_set_error (error,
                   MONGOC_ERROR_BSON,
                   MONGOC_ERROR_BSON_INVALID,
                   "Document %u is too large for the cluster. "
                   "Document is %u bytes, max is %d.",
                   idx,
                   len,
                   max_bson_size);

   if (err_doc) {
      BSON_APPEND_INT32 (err_doc, "index", idx);
      BSON_APPEND_UTF8 (err_doc, "err", error->message);
      BSON_APPEND_INT32 (err_doc, "code", MONGOC_ERROR_BSON_INVALID);
   }
}


static void
_mongoc_write_command_delete_legacy (
   mongoc_write_command_t *command,
   mongoc_client_t *client,
   mongoc_server_stream_t *server_stream,
   const char *database,
   const char *collection,
   const mongoc_write_concern_t *write_concern,
   uint32_t offset,
   mongoc_write_result_t *result,
   bson_error_t *error)
{
   int64_t started;
   int32_t max_bson_obj_size;
   const uint8_t *data;
   mongoc_rpc_t rpc;
   uint32_t request_id;
   bson_iter_t iter;
   bson_iter_t q_iter;
   uint32_t len;
   int64_t limit = 0;
   bson_t *gle = NULL;
   char ns[MONGOC_NAMESPACE_MAX + 1];
   bool r;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (database);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (collection);

   started = bson_get_monotonic_time ();

   max_bson_obj_size = mongoc_server_stream_max_bson_obj_size (server_stream);

   r = bson_iter_init (&iter, command->documents);
   BSON_ASSERT (r);
   if (!command->n_documents || !bson_iter_next (&iter)) {
      bson_set_error (error,
                      MONGOC_ERROR_COLLECTION,
                      MONGOC_ERROR_COLLECTION_DELETE_FAILED,
                      "Cannot do an empty delete.");
      result->failed = true;
      EXIT;
   }

   bson_snprintf (ns, sizeof ns, "%s.%s", database, collection);

   do {
      /* the document is like { "q": { <selector> }, limit: <0 or 1> } */
      r = (bson_iter_recurse (&iter, &q_iter) &&
           bson_iter_find (&q_iter, "q") && BSON_ITER_HOLDS_DOCUMENT (&q_iter));

      BSON_ASSERT (r);
      bson_iter_document (&q_iter, &len, &data);
      BSON_ASSERT (data);
      BSON_ASSERT (len >= 5);
      if (len > max_bson_obj_size) {
         too_large_error (error, 0, len, max_bson_obj_size, NULL);
         result->failed = true;
         EXIT;
      }

      request_id = ++client->cluster.request_id;

      rpc.header.msg_len = 0;
      rpc.header.request_id = request_id;
      rpc.header.response_to = 0;
      rpc.header.opcode = MONGOC_OPCODE_DELETE;
      rpc.delete_.zero = 0;
      rpc.delete_.collection = ns;

      if (bson_iter_find (&q_iter, "limit") &&
          (BSON_ITER_HOLDS_INT (&q_iter))) {
         limit = bson_iter_as_int64 (&q_iter);
      }

      rpc.delete_.flags =
         limit ? MONGOC_DELETE_SINGLE_REMOVE : MONGOC_DELETE_NONE;
      rpc.delete_.selector = data;

      _mongoc_monitor_legacy_write (client,
                                    command,
                                    database,
                                    collection,
                                    write_concern,
                                    server_stream,
                                    request_id);

      if (!mongoc_cluster_sendv_to_server (
             &client->cluster, &rpc, server_stream, write_concern, error)) {
         result->failed = true;
         EXIT;
      }

      if (mongoc_write_concern_is_acknowledged (write_concern)) {
         if (!_mongoc_client_recv_gle (client, server_stream, &gle, error)) {
            result->failed = true;
            EXIT;
         }

         _mongoc_write_result_merge_legacy (
            result,
            command,
            gle,
            client->error_api_version,
            MONGOC_ERROR_COLLECTION_DELETE_FAILED,
            offset);

         offset++;
      }

      _mongoc_monitor_legacy_write_succeeded (client,
                                              bson_get_monotonic_time () -
                                                 started,
                                              command,
                                              gle,
                                              server_stream,
                                              request_id);

      if (gle) {
         bson_destroy (gle);
         gle = NULL;
      }

      started = bson_get_monotonic_time ();
   } while (bson_iter_next (&iter));

   EXIT;
}


static void
_mongoc_write_command_insert_legacy (
   mongoc_write_command_t *command,
   mongoc_client_t *client,
   mongoc_server_stream_t *server_stream,
   const char *database,
   const char *collection,
   const mongoc_write_concern_t *write_concern,
   uint32_t offset,
   mongoc_write_result_t *result,
   bson_error_t *error)
{
   int64_t started;
   uint32_t current_offset;
   mongoc_iovec_t *iov;
   const uint8_t *data;
   mongoc_rpc_t rpc;
   bson_iter_t iter;
   uint32_t len;
   bson_t *gle = NULL;
   uint32_t size = 0;
   bool has_more;
   char ns[MONGOC_NAMESPACE_MAX + 1];
   bool r;
   uint32_t n_docs_in_batch;
   uint32_t request_id = 0;
   uint32_t idx = 0;
   int32_t max_msg_size;
   int32_t max_bson_obj_size;
   bool singly;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (database);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (collection);
   BSON_ASSERT (command->type == MONGOC_WRITE_COMMAND_INSERT);

   started = bson_get_monotonic_time ();
   current_offset = offset;

   max_bson_obj_size = mongoc_server_stream_max_bson_obj_size (server_stream);
   max_msg_size = mongoc_server_stream_max_msg_size (server_stream);

   singly = !command->u.insert.allow_bulk_op_insert;

   r = bson_iter_init (&iter, command->documents);
   BSON_ASSERT (r);

   if (!command->n_documents || !bson_iter_next (&iter)) {
      bson_set_error (error,
                      MONGOC_ERROR_COLLECTION,
                      MONGOC_ERROR_COLLECTION_INSERT_FAILED,
                      "Cannot do an empty insert.");
      result->failed = true;
      EXIT;
   }

   bson_snprintf (ns, sizeof ns, "%s.%s", database, collection);

   iov = (mongoc_iovec_t *) bson_malloc ((sizeof *iov) * command->n_documents);

again:
   has_more = false;
   n_docs_in_batch = 0;
   size = (uint32_t) (sizeof (mongoc_rpc_header_t) + 4 + strlen (database) + 1 +
                      strlen (collection) + 1);

   do {
      BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
      BSON_ASSERT (n_docs_in_batch <= idx);
      BSON_ASSERT (idx < command->n_documents);

      bson_iter_document (&iter, &len, &data);

      BSON_ASSERT (data);
      BSON_ASSERT (len >= 5);

      if (len > max_bson_obj_size) {
         /* document is too large */
         bson_t write_err_doc = BSON_INITIALIZER;

         too_large_error (error, idx, len, max_bson_obj_size, &write_err_doc);

         _mongoc_write_result_merge_legacy (
            result,
            command,
            &write_err_doc,
            client->error_api_version,
            MONGOC_ERROR_COLLECTION_INSERT_FAILED,
            offset + idx);

         bson_destroy (&write_err_doc);

         if (command->flags.ordered) {
            /* send the batch so far (if any) and return the error */
            break;
         }
      } else if ((n_docs_in_batch == 1 && singly) ||
                 size > (max_msg_size - len)) {
         /* batch is full, send it and then start the next batch */
         has_more = true;
         break;
      } else {
         /* add document to batch and continue building the batch */
         iov[n_docs_in_batch].iov_base = (void *) data;
         iov[n_docs_in_batch].iov_len = len;
         size += len;
         n_docs_in_batch++;
      }

      idx++;
   } while (bson_iter_next (&iter));

   if (n_docs_in_batch) {
      request_id = ++client->cluster.request_id;

      rpc.header.msg_len = 0;
      rpc.header.request_id = request_id;
      rpc.header.response_to = 0;
      rpc.header.opcode = MONGOC_OPCODE_INSERT;
      rpc.insert.flags =
         ((command->flags.ordered) ? MONGOC_INSERT_NONE
                                   : MONGOC_INSERT_CONTINUE_ON_ERROR);
      rpc.insert.collection = ns;
      rpc.insert.documents = iov;
      rpc.insert.n_documents = n_docs_in_batch;

      _mongoc_monitor_legacy_write (client,
                                    command,
                                    database,
                                    collection,
                                    write_concern,
                                    server_stream,
                                    request_id);

      if (!mongoc_cluster_sendv_to_server (
             &client->cluster, &rpc, server_stream, write_concern, error)) {
         result->failed = true;
         GOTO (cleanup);
      }

      if (mongoc_write_concern_is_acknowledged (write_concern)) {
         bool err = false;
         bson_iter_t citer;

         if (!_mongoc_client_recv_gle (client, server_stream, &gle, error)) {
            result->failed = true;
            GOTO (cleanup);
         }

         err = (bson_iter_init_find (&citer, gle, "err") &&
                bson_iter_as_bool (&citer));

         /*
          * Overwrite the "n" field since it will be zero. Otherwise, our
          * merge_legacy code will not know how many we tried in this batch.
          */
         if (!err && bson_iter_init_find (&citer, gle, "n") &&
             BSON_ITER_HOLDS_INT32 (&citer) && !bson_iter_int32 (&citer)) {
            bson_iter_overwrite_int32 (&citer, n_docs_in_batch);
         }
      }

      _mongoc_monitor_legacy_write_succeeded (client,
                                              bson_get_monotonic_time () -
                                                 started,
                                              command,
                                              gle,
                                              server_stream,
                                              request_id);

      started = bson_get_monotonic_time ();
   }

cleanup:

   if (gle) {
      _mongoc_write_result_merge_legacy (result,
                                         command,
                                         gle,
                                         client->error_api_version,
                                         MONGOC_ERROR_COLLECTION_INSERT_FAILED,
                                         current_offset);

      current_offset = offset + idx;
      bson_destroy (gle);
      gle = NULL;
   }

   if (has_more) {
      GOTO (again);
   }

   bson_free (iov);

   EXIT;
}


void
_empty_error (mongoc_write_command_t *command, bson_error_t *error)
{
   static const uint32_t codes[] = {MONGOC_ERROR_COLLECTION_DELETE_FAILED,
                                    MONGOC_ERROR_COLLECTION_INSERT_FAILED,
                                    MONGOC_ERROR_COLLECTION_UPDATE_FAILED};

   bson_set_error (error,
                   MONGOC_ERROR_COLLECTION,
                   codes[command->type],
                   "Cannot do an empty %s",
                   gCommandNames[command->type]);
}


bool
_mongoc_write_command_will_overflow (uint32_t len_so_far,
                                     uint32_t document_len,
                                     uint32_t n_documents_written,
                                     int32_t max_bson_size,
                                     int32_t max_write_batch_size)
{
   /* max BSON object size + 16k bytes.
    * server guarantees there is enough room: SERVER-10643
    */
   int32_t max_cmd_size = max_bson_size + 16384;

   BSON_ASSERT (max_bson_size);

   if (len_so_far + document_len > max_cmd_size) {
      return true;
   } else if (max_write_batch_size > 0 &&
              n_documents_written >= max_write_batch_size) {
      return true;
   }

   return false;
}


static void
_mongoc_write_command_update_legacy (
   mongoc_write_command_t *command,
   mongoc_client_t *client,
   mongoc_server_stream_t *server_stream,
   const char *database,
   const char *collection,
   const mongoc_write_concern_t *write_concern,
   uint32_t offset,
   mongoc_write_result_t *result,
   bson_error_t *error)
{
   int64_t started;
   int32_t max_bson_obj_size;
   mongoc_rpc_t rpc;
   uint32_t request_id = 0;
   bson_iter_t iter, subiter, subsubiter;
   bson_t doc;
   bool has_update, has_selector, is_upsert;
   bson_t update, selector;
   bson_t *gle = NULL;
   const uint8_t *data = NULL;
   uint32_t len = 0;
   size_t err_offset;
   bool val = false;
   char ns[MONGOC_NAMESPACE_MAX + 1];
   int32_t affected = 0;
   int vflags = (BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL |
                 BSON_VALIDATE_DOLLAR_KEYS | BSON_VALIDATE_DOT_KEYS);

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (database);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (collection);

   started = bson_get_monotonic_time ();

   max_bson_obj_size = mongoc_server_stream_max_bson_obj_size (server_stream);

   bson_iter_init (&iter, command->documents);
   while (bson_iter_next (&iter)) {
      if (bson_iter_recurse (&iter, &subiter) &&
          bson_iter_find (&subiter, "u") &&
          BSON_ITER_HOLDS_DOCUMENT (&subiter)) {
         bson_iter_document (&subiter, &len, &data);
         bson_init_static (&doc, data, len);

         if (bson_iter_init (&subsubiter, &doc) &&
             bson_iter_next (&subsubiter) &&
             (bson_iter_key (&subsubiter)[0] != '$') &&
             !bson_validate (
                &doc, (bson_validate_flags_t) vflags, &err_offset)) {
            result->failed = true;
            bson_set_error (error,
                            MONGOC_ERROR_BSON,
                            MONGOC_ERROR_BSON_INVALID,
                            "update document is corrupt or contains "
                            "invalid keys including $ or .");
            EXIT;
         }
      } else {
         result->failed = true;
         bson_set_error (error,
                         MONGOC_ERROR_BSON,
                         MONGOC_ERROR_BSON_INVALID,
                         "updates is malformed.");
         EXIT;
      }
   }

   bson_snprintf (ns, sizeof ns, "%s.%s", database, collection);

   bson_iter_init (&iter, command->documents);
   while (bson_iter_next (&iter)) {
      request_id = ++client->cluster.request_id;

      rpc.header.msg_len = 0;
      rpc.header.request_id = request_id;
      rpc.header.response_to = 0;
      rpc.header.opcode = MONGOC_OPCODE_UPDATE;
      rpc.update.zero = 0;
      rpc.update.collection = ns;
      rpc.update.flags = MONGOC_UPDATE_NONE;

      has_update = false;
      has_selector = false;
      is_upsert = false;

      bson_iter_recurse (&iter, &subiter);
      while (bson_iter_next (&subiter)) {
         if (strcmp (bson_iter_key (&subiter), "u") == 0) {
            bson_iter_document (&subiter, &len, &data);
            if (len > max_bson_obj_size) {
               too_large_error (error, 0, len, max_bson_obj_size, NULL);
               result->failed = true;
               EXIT;
            }

            rpc.update.update = data;
            bson_init_static (&update, data, len);
            has_update = true;
         } else if (strcmp (bson_iter_key (&subiter), "q") == 0) {
            bson_iter_document (&subiter, &len, &data);
            if (len > max_bson_obj_size) {
               too_large_error (error, 0, len, max_bson_obj_size, NULL);
               result->failed = true;
               EXIT;
            }

            rpc.update.selector = data;
            bson_init_static (&selector, data, len);
            has_selector = true;
         } else if (strcmp (bson_iter_key (&subiter), "multi") == 0) {
            val = bson_iter_bool (&subiter);
            if (val) {
               rpc.update.flags = (mongoc_update_flags_t) (
                  rpc.update.flags | MONGOC_UPDATE_MULTI_UPDATE);
            }
         } else if (strcmp (bson_iter_key (&subiter), "upsert") == 0) {
            val = bson_iter_bool (&subiter);
            if (val) {
               rpc.update.flags = (mongoc_update_flags_t) (
                  rpc.update.flags | MONGOC_UPDATE_UPSERT);
            }
            is_upsert = true;
         }
      }

      _mongoc_monitor_legacy_write (client,
                                    command,
                                    database,
                                    collection,
                                    write_concern,
                                    server_stream,
                                    request_id);

      if (!mongoc_cluster_sendv_to_server (
             &client->cluster, &rpc, server_stream, write_concern, error)) {
         result->failed = true;
         EXIT;
      }

      if (mongoc_write_concern_is_acknowledged (write_concern)) {
         if (!_mongoc_client_recv_gle (client, server_stream, &gle, error)) {
            result->failed = true;
            EXIT;
         }

         if (bson_iter_init_find (&subiter, gle, "n") &&
             BSON_ITER_HOLDS_INT32 (&subiter)) {
            affected = bson_iter_int32 (&subiter);
         }

         /*
          * CDRIVER-372:
          *
          * Versions of MongoDB before 2.6 don't return the _id for an
          * upsert if _id is not an ObjectId.
          */
         if (is_upsert && affected &&
             !bson_iter_init_find (&subiter, gle, "upserted") &&
             bson_iter_init_find (&subiter, gle, "updatedExisting") &&
             BSON_ITER_HOLDS_BOOL (&subiter) && !bson_iter_bool (&subiter)) {
            if (has_update && bson_iter_init_find (&subiter, &update, "_id")) {
               _ignore_value (bson_append_iter (gle, "upserted", 8, &subiter));
            } else if (has_selector &&
                       bson_iter_init_find (&subiter, &selector, "_id")) {
               _ignore_value (bson_append_iter (gle, "upserted", 8, &subiter));
            }
         }

         _mongoc_write_result_merge_legacy (
            result,
            command,
            gle,
            client->error_api_version,
            MONGOC_ERROR_COLLECTION_UPDATE_FAILED,
            offset);

         offset++;
      }

      _mongoc_monitor_legacy_write_succeeded (client,
                                              bson_get_monotonic_time () -
                                                 started,
                                              command,
                                              gle,
                                              server_stream,
                                              request_id);

      if (gle) {
         bson_destroy (gle);
         gle = NULL;
      }

      started = bson_get_monotonic_time ();
   }
}


static mongoc_write_op_t gLegacyWriteOps[3] = {
   _mongoc_write_command_delete_legacy,
   _mongoc_write_command_insert_legacy,
   _mongoc_write_command_update_legacy};


static void
_mongoc_write_command (mongoc_write_command_t *command,
                       mongoc_client_t *client,
                       mongoc_server_stream_t *server_stream,
                       const char *database,
                       const char *collection,
                       const mongoc_write_concern_t *write_concern,
                       uint32_t offset,
                       mongoc_write_result_t *result,
                       bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   const uint8_t *data;
   bson_iter_t iter;
   const char *key;
   uint32_t len = 0;
   bson_t tmp;
   bson_t ar;
   bson_t cmd;
   bson_t reply;
   char str[16];
   bool has_more;
   bool ret = false;
   uint32_t i;
   int32_t max_bson_obj_size;
   int32_t max_write_batch_size;
   int32_t min_wire_version;
   uint32_t overhead;
   uint32_t key_len;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (database);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (collection);

   bson_init (&cmd);
   max_bson_obj_size = mongoc_server_stream_max_bson_obj_size (server_stream);
   max_write_batch_size =
      mongoc_server_stream_max_write_batch_size (server_stream);

   /*
    * If we have an unacknowledged write and the server supports the legacy
    * opcodes, then submit the legacy opcode so we don't need to wait for
    * a response from the server.
    */

   min_wire_version = server_stream->sd->min_wire_version;
   if ((min_wire_version == 0) &&
       !mongoc_write_concern_is_acknowledged (write_concern)) {
      if (command->flags.bypass_document_validation !=
          MONGOC_BYPASS_DOCUMENT_VALIDATION_DEFAULT) {
         bson_set_error (
            error,
            MONGOC_ERROR_COMMAND,
            MONGOC_ERROR_COMMAND_INVALID_ARG,
            "Cannot set bypassDocumentValidation for unacknowledged writes");
         EXIT;
      }
      if (command->flags.has_collation) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "Cannot set collation for unacknowledged writes");
         EXIT;
      }
      gLegacyWriteOps[command->type](command,
                                     client,
                                     server_stream,
                                     database,
                                     collection,
                                     write_concern,
                                     offset,
                                     result,
                                     error);
      EXIT;
   }
   if (command->flags.has_collation &&
       server_stream->sd->max_wire_version < WIRE_VERSION_COLLATION) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                      "Collation is not supported by the selected server");
      EXIT;
   }

   if (!command->n_documents || !bson_iter_init (&iter, command->documents) ||
       !bson_iter_next (&iter)) {
      _empty_error (command, error);
      result->failed = true;
      EXIT;
   }

again:
   has_more = false;
   i = 0;

   _mongoc_write_command_init (&cmd, command, collection, write_concern);

   /* 1 byte to specify array type, 1 byte for field name's null terminator */
   overhead = cmd.len + 2 + gCommandFieldLens[command->type];

   if (!_mongoc_write_command_will_overflow (overhead,
                                             command->documents->len,
                                             command->n_documents,
                                             max_bson_obj_size,
                                             max_write_batch_size)) {
      /* copy the whole documents buffer as e.g. "updates": [...] */
      bson_append_array (&cmd,
                         gCommandFields[command->type],
                         gCommandFieldLens[command->type],
                         command->documents);
      i = command->n_documents;
   } else {
      bson_append_array_begin (&cmd,
                               gCommandFields[command->type],
                               gCommandFieldLens[command->type],
                               &ar);

      do {
         BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
         bson_iter_document (&iter, &len, &data);

         /* append array element like "0": { ... doc ... } */
         key_len = (uint32_t) bson_uint32_to_string (i, &key, str, sizeof str);

         /* 1 byte to specify document type, 1 byte for key's null terminator */
         if (_mongoc_write_command_will_overflow (overhead,
                                                  key_len + len + 2 + ar.len,
                                                  i,
                                                  max_bson_obj_size,
                                                  max_write_batch_size)) {
            has_more = true;
            break;
         }

         BSON_ASSERT (bson_init_static (&tmp, data, len));
         BSON_APPEND_DOCUMENT (&ar, key, &tmp);

         bson_destroy (&tmp);

         i++;
      } while (bson_iter_next (&iter));

      bson_append_array_end (&cmd, &ar);
   }

   if (!i) {
      too_large_error (error, i, len, max_bson_obj_size, NULL);
      result->failed = true;
      ret = false;

      /* the current document is too large, continue to the next */
      if (!bson_iter_next (&iter)) {
         GOTO (cleanup);
      }
   } else {
      mongoc_cmd_parts_init (&parts, database, MONGOC_QUERY_NONE, &cmd);
      parts.is_write_command = true;
      parts.assembled.operation_id = command->operation_id;
      ret = mongoc_cluster_run_command_monitored (
         &client->cluster, &parts, server_stream, &reply, error);

      if (!ret) {
         result->failed = true;
         if (bson_empty (&reply)) {
            /* The command not only failed,
             * the roundtrip to the server failed and the node was disconnected
             */
            result->must_stop = true;
         }
      }

      _mongoc_write_result_merge (result, command, &reply, offset);
      offset += i;
      bson_destroy (&reply);
      mongoc_cmd_parts_cleanup (&parts);
   }

   if (has_more && (ret || !command->flags.ordered) && !result->must_stop) {
      bson_reinit (&cmd);
      GOTO (again);
   }

cleanup:
   bson_destroy (&cmd);
   EXIT;
}


void
_mongoc_write_command_execute (
   mongoc_write_command_t *command,             /* IN */
   mongoc_client_t *client,                     /* IN */
   mongoc_server_stream_t *server_stream,       /* IN */
   const char *database,                        /* IN */
   const char *collection,                      /* IN */
   const mongoc_write_concern_t *write_concern, /* IN */
   uint32_t offset,                             /* IN */
   mongoc_write_result_t *result)               /* OUT */
{
   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (database);
   BSON_ASSERT (collection);
   BSON_ASSERT (result);

   if (!write_concern) {
      write_concern = client->write_concern;
   }

   if (!mongoc_write_concern_is_valid (write_concern)) {
      bson_set_error (&result->error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "The write concern is invalid.");
      result->failed = true;
      EXIT;
   }

   if (server_stream->sd->max_wire_version >= WIRE_VERSION_WRITE_CMD) {
      _mongoc_write_command (command,
                             client,
                             server_stream,
                             database,
                             collection,
                             write_concern,
                             offset,
                             result,
                             &result->error);
   } else {
      if (command->flags.bypass_document_validation !=
          MONGOC_BYPASS_DOCUMENT_VALIDATION_DEFAULT) {
         bson_set_error (
            &result->error,
            MONGOC_ERROR_COMMAND,
            MONGOC_ERROR_COMMAND_INVALID_ARG,
            "Cannot set bypassDocumentValidation for unacknowledged writes");
         result->failed = true;
         EXIT;
      }
      if (command->flags.has_collation &&
          server_stream->sd->max_wire_version < WIRE_VERSION_COLLATION) {
         bson_set_error (&result->error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "Cannot set collation for unacknowledged writes");
         result->failed = true;
         EXIT;
      }
      gLegacyWriteOps[command->type](command,
                                     client,
                                     server_stream,
                                     database,
                                     collection,
                                     write_concern,
                                     offset,
                                     result,
                                     &result->error);
   }

   EXIT;
}


void
_mongoc_write_command_destroy (mongoc_write_command_t *command)
{
   ENTRY;

   if (command) {
      bson_destroy (command->documents);
   }

   EXIT;
}


void
_mongoc_write_result_init (mongoc_write_result_t *result) /* IN */
{
   ENTRY;

   BSON_ASSERT (result);

   memset (result, 0, sizeof *result);

   bson_init (&result->upserted);
   bson_init (&result->writeConcernErrors);
   bson_init (&result->writeErrors);

   EXIT;
}


void
_mongoc_write_result_destroy (mongoc_write_result_t *result)
{
   ENTRY;

   BSON_ASSERT (result);

   bson_destroy (&result->upserted);
   bson_destroy (&result->writeConcernErrors);
   bson_destroy (&result->writeErrors);

   EXIT;
}


static void
_mongoc_write_result_append_upsert (mongoc_write_result_t *result,
                                    int32_t idx,
                                    const bson_value_t *value)
{
   bson_t child;
   const char *keyptr = NULL;
   char key[12];
   int len;

   BSON_ASSERT (result);
   BSON_ASSERT (value);

   len = (int) bson_uint32_to_string (
      result->upsert_append_count, &keyptr, key, sizeof key);

   bson_append_document_begin (&result->upserted, keyptr, len, &child);
   BSON_APPEND_INT32 (&child, "index", idx);
   BSON_APPEND_VALUE (&child, "_id", value);
   bson_append_document_end (&result->upserted, &child);

   result->upsert_append_count++;
}


static void
_append_write_concern_err_legacy (mongoc_write_result_t *result,
                                  const char *err,
                                  int32_t code)
{
   char str[16];
   const char *key;
   size_t keylen;
   bson_t write_concern_error;

   /* don't set result->failed; record the write concern err and continue */
   keylen = bson_uint32_to_string (
      result->n_writeConcernErrors, &key, str, sizeof str);

   BSON_ASSERT (keylen < INT_MAX);

   bson_append_document_begin (
      &result->writeConcernErrors, key, (int) keylen, &write_concern_error);

   bson_append_int32 (&write_concern_error, "code", 4, code);
   bson_append_utf8 (&write_concern_error, "errmsg", 6, err, -1);
   bson_append_document_end (&result->writeConcernErrors, &write_concern_error);
   result->n_writeConcernErrors++;
}


static void
_append_write_err_legacy (mongoc_write_result_t *result,
                          const char *err,
                          mongoc_error_domain_t domain,
                          int32_t code,
                          uint32_t offset)
{
   bson_t holder, write_errors, child;
   bson_iter_t iter;

   BSON_ASSERT (code > 0);

   if (!result->error.domain) {
      bson_set_error (&result->error, domain, (uint32_t) code, "%s", err);
   }

   /* stop processing, if result->ordered */
   result->failed = true;

   bson_init (&holder);
   bson_append_array_begin (&holder, "0", 1, &write_errors);
   bson_append_document_begin (&write_errors, "0", 1, &child);

   /* set error's "index" to 0; fixed up in _mongoc_write_result_merge_arrays */
   bson_append_int32 (&child, "index", 5, 0);
   bson_append_int32 (&child, "code", 4, code);
   bson_append_utf8 (&child, "errmsg", 6, err, -1);
   bson_append_document_end (&write_errors, &child);
   bson_append_array_end (&holder, &write_errors);
   bson_iter_init (&iter, &holder);
   bson_iter_next (&iter);

   _mongoc_write_result_merge_arrays (
      offset, result, &result->writeErrors, &iter);

   bson_destroy (&holder);
}


void
_mongoc_write_result_merge_legacy (mongoc_write_result_t *result,   /* IN */
                                   mongoc_write_command_t *command, /* IN */
                                   const bson_t *reply,             /* IN */
                                   int32_t error_api_version,
                                   mongoc_error_code_t default_code,
                                   uint32_t offset)
{
   const bson_value_t *value;
   bson_iter_t iter;
   bson_iter_t ar;
   bson_iter_t citer;
   const char *err = NULL;
   int32_t code = 0;
   int32_t n = 0;
   int32_t upsert_idx = 0;
   mongoc_error_domain_t domain;

   ENTRY;

   BSON_ASSERT (result);
   BSON_ASSERT (reply);

   domain = error_api_version >= MONGOC_ERROR_API_VERSION_2
               ? MONGOC_ERROR_SERVER
               : MONGOC_ERROR_COLLECTION;

   if (bson_iter_init_find (&iter, reply, "n") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      n = bson_iter_int32 (&iter);
   }

   if (bson_iter_init_find (&iter, reply, "err") &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      err = bson_iter_utf8 (&iter, NULL);
   }

   if (bson_iter_init_find (&iter, reply, "code") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      code = bson_iter_int32 (&iter);
   }

   if (_is_duplicate_key_error (code)) {
      code = MONGOC_ERROR_DUPLICATE_KEY;
   }

   if (code || err) {
      if (!err) {
         err = "unknown error";
      }

      if (bson_iter_init_find (&iter, reply, "wtimeout") &&
          bson_iter_as_bool (&iter)) {
         if (!code) {
            code = (int32_t) MONGOC_ERROR_WRITE_CONCERN_ERROR;
         }

         _append_write_concern_err_legacy (result, err, code);
      } else {
         if (!code) {
            code = (int32_t) default_code;
         }

         _append_write_err_legacy (result, err, domain, code, offset);
      }
   }

   switch (command->type) {
   case MONGOC_WRITE_COMMAND_INSERT:
      if (n) {
         result->nInserted += n;
      }
      break;
   case MONGOC_WRITE_COMMAND_DELETE:
      result->nRemoved += n;
      break;
   case MONGOC_WRITE_COMMAND_UPDATE:
      if (bson_iter_init_find (&iter, reply, "upserted") &&
          !BSON_ITER_HOLDS_ARRAY (&iter)) {
         result->nUpserted += n;
         value = bson_iter_value (&iter);
         _mongoc_write_result_append_upsert (result, offset, value);
      } else if (bson_iter_init_find (&iter, reply, "upserted") &&
                 BSON_ITER_HOLDS_ARRAY (&iter)) {
         result->nUpserted += n;
         if (bson_iter_recurse (&iter, &ar)) {
            while (bson_iter_next (&ar)) {
               if (BSON_ITER_HOLDS_DOCUMENT (&ar) &&
                   bson_iter_recurse (&ar, &citer) &&
                   bson_iter_find (&citer, "_id")) {
                  value = bson_iter_value (&citer);
                  _mongoc_write_result_append_upsert (
                     result, offset + upsert_idx, value);
                  upsert_idx++;
               }
            }
         }
      } else if ((n == 1) &&
                 bson_iter_init_find (&iter, reply, "updatedExisting") &&
                 BSON_ITER_HOLDS_BOOL (&iter) && !bson_iter_bool (&iter)) {
         result->nUpserted += n;
      } else {
         result->nMatched += n;
      }
      break;
   default:
      break;
   }

   result->omit_nModified = true;

   EXIT;
}


static int32_t
_mongoc_write_result_merge_arrays (uint32_t offset,
                                   mongoc_write_result_t *result, /* IN */
                                   bson_t *dest,                  /* IN */
                                   bson_iter_t *iter)             /* IN */
{
   const bson_value_t *value;
   bson_iter_t ar;
   bson_iter_t citer;
   int32_t idx;
   int32_t count = 0;
   int32_t aridx;
   bson_t child;
   const char *keyptr = NULL;
   char key[12];
   int len;

   ENTRY;

   BSON_ASSERT (result);
   BSON_ASSERT (dest);
   BSON_ASSERT (iter);
   BSON_ASSERT (BSON_ITER_HOLDS_ARRAY (iter));

   aridx = bson_count_keys (dest);

   if (bson_iter_recurse (iter, &ar)) {
      while (bson_iter_next (&ar)) {
         if (BSON_ITER_HOLDS_DOCUMENT (&ar) &&
             bson_iter_recurse (&ar, &citer)) {
            len =
               (int) bson_uint32_to_string (aridx++, &keyptr, key, sizeof key);
            bson_append_document_begin (dest, keyptr, len, &child);
            while (bson_iter_next (&citer)) {
               if (BSON_ITER_IS_KEY (&citer, "index")) {
                  idx = bson_iter_int32 (&citer) + offset;
                  BSON_APPEND_INT32 (&child, "index", idx);
               } else {
                  value = bson_iter_value (&citer);
                  BSON_APPEND_VALUE (&child, bson_iter_key (&citer), value);
               }
            }
            bson_append_document_end (dest, &child);
            count++;
         }
      }
   }

   RETURN (count);
}


void
_mongoc_write_result_merge (mongoc_write_result_t *result,   /* IN */
                            mongoc_write_command_t *command, /* IN */
                            const bson_t *reply,             /* IN */
                            uint32_t offset)
{
   int32_t server_index = 0;
   const bson_value_t *value;
   bson_iter_t iter;
   bson_iter_t citer;
   bson_iter_t ar;
   int32_t n_upserted = 0;
   int32_t affected = 0;

   ENTRY;

   BSON_ASSERT (result);
   BSON_ASSERT (reply);

   if (bson_iter_init_find (&iter, reply, "n") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      affected = bson_iter_int32 (&iter);
   }

   if (bson_iter_init_find (&iter, reply, "writeErrors") &&
       BSON_ITER_HOLDS_ARRAY (&iter) && bson_iter_recurse (&iter, &citer) &&
       bson_iter_next (&citer)) {
      result->failed = true;
   }

   switch (command->type) {
   case MONGOC_WRITE_COMMAND_INSERT:
      result->nInserted += affected;
      break;
   case MONGOC_WRITE_COMMAND_DELETE:
      result->nRemoved += affected;
      break;
   case MONGOC_WRITE_COMMAND_UPDATE:

      /* server returns each upserted _id with its index into this batch
       * look for "upserted": [{"index": 4, "_id": ObjectId()}, ...] */
      if (bson_iter_init_find (&iter, reply, "upserted")) {
         if (BSON_ITER_HOLDS_ARRAY (&iter) &&
             (bson_iter_recurse (&iter, &ar))) {
            while (bson_iter_next (&ar)) {
               if (BSON_ITER_HOLDS_DOCUMENT (&ar) &&
                   bson_iter_recurse (&ar, &citer) &&
                   bson_iter_find (&citer, "index") &&
                   BSON_ITER_HOLDS_INT32 (&citer)) {
                  server_index = bson_iter_int32 (&citer);

                  if (bson_iter_recurse (&ar, &citer) &&
                      bson_iter_find (&citer, "_id")) {
                     value = bson_iter_value (&citer);
                     _mongoc_write_result_append_upsert (
                        result, offset + server_index, value);
                     n_upserted++;
                  }
               }
            }
         }
         result->nUpserted += n_upserted;
         /*
          * XXX: The following addition to nMatched needs some checking.
          *      I'm highly skeptical of it.
          */
         result->nMatched += BSON_MAX (0, (affected - n_upserted));
      } else {
         result->nMatched += affected;
      }
      /*
       * SERVER-13001 - in a mixed sharded cluster a call to update could
       * return nModified (>= 2.6) or not (<= 2.4).  If any call does not
       * return nModified we can't report a valid final count so omit the
       * field completely.
       */
      if (bson_iter_init_find (&iter, reply, "nModified") &&
          BSON_ITER_HOLDS_INT32 (&iter)) {
         result->nModified += bson_iter_int32 (&iter);
      } else {
         /*
          * nModified could be BSON_TYPE_NULL, which should also be omitted.
          */
         result->omit_nModified = true;
      }
      break;
   default:
      BSON_ASSERT (false);
      break;
   }

   if (bson_iter_init_find (&iter, reply, "writeErrors") &&
       BSON_ITER_HOLDS_ARRAY (&iter)) {
      _mongoc_write_result_merge_arrays (
         offset, result, &result->writeErrors, &iter);
   }

   if (bson_iter_init_find (&iter, reply, "writeConcernError") &&
       BSON_ITER_HOLDS_DOCUMENT (&iter)) {
      uint32_t len;
      const uint8_t *data;
      bson_t write_concern_error;
      char str[16];
      const char *key;

      /* writeConcernError is a subdocument in the server response
       * append it to the result->writeConcernErrors array */
      bson_iter_document (&iter, &len, &data);
      bson_init_static (&write_concern_error, data, len);

      bson_uint32_to_string (
         result->n_writeConcernErrors, &key, str, sizeof str);

      bson_append_document (
         &result->writeConcernErrors, key, -1, &write_concern_error);

      result->n_writeConcernErrors++;
   }

   EXIT;
}


/*
 * If error is not set, set code from first document in array like
 * [{"code": 64, "errmsg": "duplicate"}, ...]. Format the error message
 * from all errors in array.
*/
static void
_set_error_from_response (bson_t *bson_array,
                          mongoc_error_domain_t domain,
                          const char *error_type,
                          bson_error_t *error /* OUT */)
{
   bson_iter_t array_iter;
   bson_iter_t doc_iter;
   bson_string_t *compound_err;
   const char *errmsg = NULL;
   int32_t code = 0;
   uint32_t n_keys, i;

   compound_err = bson_string_new (NULL);
   n_keys = bson_count_keys (bson_array);
   if (n_keys > 1) {
      bson_string_append_printf (
         compound_err, "Multiple %s errors: ", error_type);
   }

   if (!bson_empty0 (bson_array) && bson_iter_init (&array_iter, bson_array)) {
      /* get first code and all error messages */
      i = 0;

      while (bson_iter_next (&array_iter)) {
         if (BSON_ITER_HOLDS_DOCUMENT (&array_iter) &&
             bson_iter_recurse (&array_iter, &doc_iter)) {
            /* parse doc, which is like {"code": 64, "errmsg": "duplicate"} */
            while (bson_iter_next (&doc_iter)) {
               /* use the first error code we find */
               if (BSON_ITER_IS_KEY (&doc_iter, "code") && code == 0) {
                  code = bson_iter_int32 (&doc_iter);
               } else if (BSON_ITER_IS_KEY (&doc_iter, "errmsg")) {
                  errmsg = bson_iter_utf8 (&doc_iter, NULL);

                  /* build message like 'Multiple write errors: "foo", "bar"' */
                  if (n_keys > 1) {
                     bson_string_append_printf (compound_err, "\"%s\"", errmsg);
                     if (i < n_keys - 1) {
                        bson_string_append (compound_err, ", ");
                     }
                  } else {
                     /* single error message */
                     bson_string_append (compound_err, errmsg);
                  }
               }
            }

            i++;
         }
      }

      if (code && compound_err->len) {
         bson_set_error (
            error, domain, (uint32_t) code, "%s", compound_err->str);
      }
   }

   bson_string_free (compound_err, true);
}


bool
_mongoc_write_result_complete (
   mongoc_write_result_t *result,             /* IN */
   int32_t error_api_version,                 /* IN */
   const mongoc_write_concern_t *wc,          /* IN */
   mongoc_error_domain_t err_domain_override, /* IN */
   bson_t *bson,                              /* OUT */
   bson_error_t *error)                       /* OUT */
{
   mongoc_error_domain_t domain;

   ENTRY;

   BSON_ASSERT (result);

   if (error_api_version >= MONGOC_ERROR_API_VERSION_2) {
      domain = MONGOC_ERROR_SERVER;
   } else if (err_domain_override) {
      domain = err_domain_override;
   } else if (result->error.domain) {
      domain = (mongoc_error_domain_t) result->error.domain;
   } else {
      domain = MONGOC_ERROR_COLLECTION;
   }

   if (bson && mongoc_write_concern_is_acknowledged (wc)) {
      BSON_APPEND_INT32 (bson, "nInserted", result->nInserted);
      BSON_APPEND_INT32 (bson, "nMatched", result->nMatched);
      if (!result->omit_nModified) {
         BSON_APPEND_INT32 (bson, "nModified", result->nModified);
      }
      BSON_APPEND_INT32 (bson, "nRemoved", result->nRemoved);
      BSON_APPEND_INT32 (bson, "nUpserted", result->nUpserted);
      if (!bson_empty0 (&result->upserted)) {
         BSON_APPEND_ARRAY (bson, "upserted", &result->upserted);
      }
      BSON_APPEND_ARRAY (bson, "writeErrors", &result->writeErrors);
      if (result->n_writeConcernErrors) {
         BSON_APPEND_ARRAY (
            bson, "writeConcernErrors", &result->writeConcernErrors);
      }
   }

   /* set bson_error_t from first write error or write concern error */
   _set_error_from_response (
      &result->writeErrors, domain, "write", &result->error);

   if (!result->error.code) {
      _set_error_from_response (&result->writeConcernErrors,
                                MONGOC_ERROR_WRITE_CONCERN,
                                "write concern",
                                &result->error);
   }

   if (error) {
      memcpy (error, &result->error, sizeof *error);
   }

   RETURN (!result->failed && result->error.code == 0);
}
