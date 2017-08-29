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


#include "mongoc-client-private.h"
#include "mongoc-collection.h"
#include "mongoc-collection-private.h"
#include "mongoc-cursor.h"
#include "mongoc-cursor-array-private.h"
#include "mongoc-cursor-cursorid-private.h"
#include "mongoc-cursor-transform-private.h"
#include "mongoc-cursor-private.h"
#include "mongoc-database.h"
#include "mongoc-database-private.h"
#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-trace-private.h"
#include "mongoc-util-private.h"
#include "mongoc-write-concern-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "database"


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_database_new --
 *
 *       Create a new instance of mongoc_database_t for @client.
 *
 *       @client must stay valid for the life of the resulting
 *       database structure.
 *
 * Returns:
 *       A newly allocated mongoc_database_t that should be freed with
 *       mongoc_database_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_database_t *
_mongoc_database_new (mongoc_client_t *client,
                      const char *name,
                      const mongoc_read_prefs_t *read_prefs,
                      const mongoc_read_concern_t *read_concern,
                      const mongoc_write_concern_t *write_concern)
{
   mongoc_database_t *db;

   ENTRY;

   BSON_ASSERT (client);
   BSON_ASSERT (name);

   db = (mongoc_database_t *) bson_malloc0 (sizeof *db);
   db->client = client;
   db->write_concern = write_concern ? mongoc_write_concern_copy (write_concern)
                                     : mongoc_write_concern_new ();
   db->read_concern = read_concern ? mongoc_read_concern_copy (read_concern)
                                   : mongoc_read_concern_new ();
   db->read_prefs = read_prefs ? mongoc_read_prefs_copy (read_prefs)
                               : mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   bson_strncpy (db->name, name, sizeof db->name);

   RETURN (db);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_destroy --
 *
 *       Releases resources associated with @database.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Everything.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_database_destroy (mongoc_database_t *database)
{
   ENTRY;

   BSON_ASSERT (database);

   if (database->read_prefs) {
      mongoc_read_prefs_destroy (database->read_prefs);
      database->read_prefs = NULL;
   }

   if (database->read_concern) {
      mongoc_read_concern_destroy (database->read_concern);
      database->read_concern = NULL;
   }

   if (database->write_concern) {
      mongoc_write_concern_destroy (database->write_concern);
      database->write_concern = NULL;
   }

   bson_free (database);

   EXIT;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_copy --
 *
 *       Returns a copy of @database that needs to be freed by calling
 *       mongoc_database_destroy.
 *
 * Returns:
 *       A copy of this database.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_database_t *
mongoc_database_copy (mongoc_database_t *database)
{
   ENTRY;

   BSON_ASSERT (database);

   RETURN (_mongoc_database_new (database->client,
                                 database->name,
                                 database->read_prefs,
                                 database->read_concern,
                                 database->write_concern));
}

mongoc_cursor_t *
mongoc_database_command (mongoc_database_t *database,
                         mongoc_query_flags_t flags,
                         uint32_t skip,
                         uint32_t limit,
                         uint32_t batch_size,
                         const bson_t *command,
                         const bson_t *fields,
                         const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (database);
   BSON_ASSERT (command);

   /* Server Selection Spec: "The generic command method has a default read
    * preference of mode 'primary'. The generic command method MUST ignore any
    * default read preference from client, database or collection
    * configuration. The generic command method SHOULD allow an optional read
    * preference argument."
    */
   return mongoc_client_command (database->client,
                                 database->name,
                                 flags,
                                 skip,
                                 limit,
                                 batch_size,
                                 command,
                                 fields,
                                 read_prefs);
}


bool
mongoc_database_command_simple (mongoc_database_t *database,
                                const bson_t *command,
                                const mongoc_read_prefs_t *read_prefs,
                                bson_t *reply,
                                bson_error_t *error)
{
   BSON_ASSERT (database);
   BSON_ASSERT (command);

   /* Server Selection Spec: "The generic command method has a default read
    * preference of mode 'primary'. The generic command method MUST ignore any
    * default read preference from client, database or collection
    * configuration. The generic command method SHOULD allow an optional read
    * preference argument."
    */
   return mongoc_client_command_simple (
      database->client, database->name, command, read_prefs, reply, error);
}


bool
mongoc_database_read_command_with_opts (mongoc_database_t *database,
                                        const bson_t *command,
                                        const mongoc_read_prefs_t *read_prefs,
                                        const bson_t *opts,
                                        bson_t *reply,
                                        bson_error_t *error)
{
   return _mongoc_client_command_with_opts (
      database->client,
      database->name,
      command,
      MONGOC_CMD_READ,
      opts,
      MONGOC_QUERY_NONE,
      COALESCE (read_prefs, database->read_prefs),
      database->read_concern,
      database->write_concern,
      reply,
      error);
}


bool
mongoc_database_write_command_with_opts (mongoc_database_t *database,
                                         const bson_t *command,
                                         const bson_t *opts,
                                         bson_t *reply,
                                         bson_error_t *error)
{
   return _mongoc_client_command_with_opts (database->client,
                                            database->name,
                                            command,
                                            MONGOC_CMD_WRITE,
                                            opts,
                                            MONGOC_QUERY_NONE,
                                            database->read_prefs,
                                            database->read_concern,
                                            database->write_concern,
                                            reply,
                                            error);
}


bool
mongoc_database_read_write_command_with_opts (
   mongoc_database_t *database,
   const bson_t *command,
   const mongoc_read_prefs_t *read_prefs /* IGNORED */,
   const bson_t *opts,
   bson_t *reply,
   bson_error_t *error)
{
   return _mongoc_client_command_with_opts (
      database->client,
      database->name,
      command,
      MONGOC_CMD_RW,
      opts,
      MONGOC_QUERY_NONE,
      COALESCE (read_prefs, database->read_prefs),
      database->read_concern,
      database->write_concern,
      reply,
      error);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_drop --
 *
 *       Requests that the MongoDB server drops @database, including all
 *       collections and indexes associated with @database.
 *
 *       Make sure this is really what you want!
 *
 * Returns:
 *       true if @database was dropped.
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_database_drop (mongoc_database_t *database, bson_error_t *error)
{
   return mongoc_database_drop_with_opts (database, NULL, error);
}


bool
mongoc_database_drop_with_opts (mongoc_database_t *database,
                                const bson_t *opts,
                                bson_error_t *error)
{
   bool ret;
   bson_t cmd;

   BSON_ASSERT (database);

   bson_init (&cmd);
   bson_append_int32 (&cmd, "dropDatabase", 12, 1);

   ret = _mongoc_client_command_with_opts (database->client,
                                           database->name,
                                           &cmd,
                                           MONGOC_CMD_WRITE,
                                           opts,
                                           MONGOC_QUERY_NONE,
                                           database->read_prefs,
                                           database->read_concern,
                                           database->write_concern,
                                           NULL, /* reply */
                                           error);
   bson_destroy (&cmd);

   return ret;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_add_user_legacy --
 *
 *       A helper to add a user or update their password on @database.
 *       This uses the legacy protocol by inserting into system.users.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

static bool
mongoc_database_add_user_legacy (mongoc_database_t *database,
                                 const char *username,
                                 const char *password,
                                 bson_error_t *error)
{
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor = NULL;
   const bson_t *doc;
   bool ret = false;
   bson_t query;
   bson_t opts;
   bson_t user;
   char *input;
   char *pwd = NULL;

   ENTRY;

   BSON_ASSERT (database);
   BSON_ASSERT (username);
   BSON_ASSERT (password);

   /*
    * Users are stored in the <dbname>.system.users virtual collection.
    */
   collection = mongoc_client_get_collection (
      database->client, database->name, "system.users");
   BSON_ASSERT (collection);

   /*
    * Hash the users password.
    */
   input = bson_strdup_printf ("%s:mongo:%s", username, password);
   pwd = _mongoc_hex_md5 (input);
   bson_free (input);

   /*
    * Check to see if the user exists. If so, we will update the
    * password instead of inserting a new user.
    */
   bson_init (&query);
   bson_append_utf8 (&query, "user", 4, username, -1);

   bson_init (&opts);
   bson_append_int64 (&opts, "limit", 5, 1);
   bson_append_bool (&opts, "singleBatch", 11, true);

   cursor = mongoc_collection_find_with_opts (collection, &query, &opts, NULL);
   if (!mongoc_cursor_next (cursor, &doc)) {
      if (mongoc_cursor_error (cursor, error)) {
         GOTO (failure);
      }
      bson_init (&user);
      bson_append_utf8 (&user, "user", 4, username, -1);
      bson_append_bool (&user, "readOnly", 8, false);
      bson_append_utf8 (&user, "pwd", 3, pwd, -1);
   } else {
      bson_init (&user);
      bson_copy_to_excluding_noinit (doc, &user, "pwd", (char *) NULL);
      bson_append_utf8 (&user, "pwd", 3, pwd, -1);
   }

   if (!mongoc_collection_insert (
          collection, MONGOC_INSERT_NONE, &user, NULL, error)) {
      GOTO (failure_with_user);
   }

   ret = true;

failure_with_user:
   bson_destroy (&user);

failure:
   if (cursor) {
      mongoc_cursor_destroy (cursor);
   }
   mongoc_collection_destroy (collection);
   bson_destroy (&query);
   bson_destroy (&opts);
   bson_free (pwd);

   RETURN (ret);
}


bool
mongoc_database_remove_user (mongoc_database_t *database,
                             const char *username,
                             bson_error_t *error)
{
   mongoc_collection_t *col;
   bson_error_t lerror;
   bson_t cmd;
   bool ret;

   ENTRY;

   BSON_ASSERT (database);
   BSON_ASSERT (username);

   bson_init (&cmd);
   BSON_APPEND_UTF8 (&cmd, "dropUser", username);
   ret = mongoc_database_command_simple (database, &cmd, NULL, NULL, &lerror);
   bson_destroy (&cmd);

   if (!ret && (lerror.code == MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND)) {
      bson_init (&cmd);
      BSON_APPEND_UTF8 (&cmd, "user", username);

      col = mongoc_client_get_collection (
         database->client, database->name, "system.users");
      BSON_ASSERT (col);

      ret = mongoc_collection_remove (
         col, MONGOC_REMOVE_SINGLE_REMOVE, &cmd, NULL, error);

      bson_destroy (&cmd);
      mongoc_collection_destroy (col);
   } else if (error) {
      memcpy (error, &lerror, sizeof *error);
   }

   RETURN (ret);
}


bool
mongoc_database_remove_all_users (mongoc_database_t *database,
                                  bson_error_t *error)
{
   mongoc_collection_t *col;
   bson_error_t lerror;
   bson_t cmd;
   bool ret;

   ENTRY;

   BSON_ASSERT (database);

   bson_init (&cmd);
   BSON_APPEND_INT32 (&cmd, "dropAllUsersFromDatabase", 1);
   ret = mongoc_database_command_simple (database, &cmd, NULL, NULL, &lerror);
   bson_destroy (&cmd);

   if (!ret && (lerror.code == MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND)) {
      bson_init (&cmd);

      col = mongoc_client_get_collection (
         database->client, database->name, "system.users");
      BSON_ASSERT (col);

      ret =
         mongoc_collection_remove (col, MONGOC_REMOVE_NONE, &cmd, NULL, error);

      bson_destroy (&cmd);
      mongoc_collection_destroy (col);
   } else if (error) {
      memcpy (error, &lerror, sizeof *error);
   }

   RETURN (ret);
}


/**
 * mongoc_database_add_user:
 * @database: A #mongoc_database_t.
 * @username: A string containing the username.
 * @password: (allow-none): A string containing password, or NULL.
 * @roles: (allow-none): An optional bson_t of roles.
 * @custom_data: (allow-none): An optional bson_t of data to store.
 * @error: (out) (allow-none): A location for a bson_error_t or %NULL.
 *
 * Creates a new user with access to @database.
 *
 * Returns: None.
 * Side effects: None.
 */
bool
mongoc_database_add_user (mongoc_database_t *database,
                          const char *username,
                          const char *password,
                          const bson_t *roles,
                          const bson_t *custom_data,
                          bson_error_t *error)
{
   bson_error_t lerror;
   bson_t cmd;
   bson_t ar;
   char *input;
   char *hashed_password;
   bool ret = false;

   ENTRY;

   BSON_ASSERT (database);
   BSON_ASSERT (username);

   /*
    * CDRIVER-232:
    *
    * Perform a (slow and tedious) round trip to mongod to determine if
    * we can safely call createUser. Otherwise, we will fallback and
    * perform legacy insertion into users collection.
    */
   bson_init (&cmd);
   BSON_APPEND_UTF8 (&cmd, "usersInfo", username);
   ret = mongoc_database_command_simple (database, &cmd, NULL, NULL, &lerror);
   bson_destroy (&cmd);

   if (!ret && (lerror.code == MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND)) {
      ret =
         mongoc_database_add_user_legacy (database, username, password, error);
   } else if (ret || (lerror.code == 13)) {
      /* usersInfo succeeded or failed with auth err, we're on modern mongod */
      input = bson_strdup_printf ("%s:mongo:%s", username, password);
      hashed_password = _mongoc_hex_md5 (input);
      bson_free (input);

      bson_init (&cmd);
      BSON_APPEND_UTF8 (&cmd, "createUser", username);
      BSON_APPEND_UTF8 (&cmd, "pwd", hashed_password);
      BSON_APPEND_BOOL (&cmd, "digestPassword", false);
      if (custom_data) {
         BSON_APPEND_DOCUMENT (&cmd, "customData", custom_data);
      }
      if (roles) {
         BSON_APPEND_ARRAY (&cmd, "roles", roles);
      } else {
         bson_append_array_begin (&cmd, "roles", 5, &ar);
         bson_append_array_end (&cmd, &ar);
      }

      ret = mongoc_database_command_simple (database, &cmd, NULL, NULL, error);

      bson_free (hashed_password);
      bson_destroy (&cmd);
   } else if (error) {
      memcpy (error, &lerror, sizeof *error);
   }

   RETURN (ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_get_read_prefs --
 *
 *       Fetch the read preferences for @database.
 *
 * Returns:
 *       A mongoc_read_prefs_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_read_prefs_t *
mongoc_database_get_read_prefs (const mongoc_database_t *database) /* IN */
{
   BSON_ASSERT (database);
   return database->read_prefs;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_set_read_prefs --
 *
 *       Sets the default read preferences for @database.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_database_set_read_prefs (mongoc_database_t *database,
                                const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (database);

   if (database->read_prefs) {
      mongoc_read_prefs_destroy (database->read_prefs);
      database->read_prefs = NULL;
   }

   if (read_prefs) {
      database->read_prefs = mongoc_read_prefs_copy (read_prefs);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_get_read_concern --
 *
 *       Fetches the read concern for @database.
 *
 * Returns:
 *       A mongoc_read_concern_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_read_concern_t *
mongoc_database_get_read_concern (const mongoc_database_t *database)
{
   BSON_ASSERT (database);

   return database->read_concern;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_set_read_concern --
 *
 *       Set the default read concern for @database.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_database_set_read_concern (mongoc_database_t *database,
                                  const mongoc_read_concern_t *read_concern)
{
   BSON_ASSERT (database);

   if (database->read_concern) {
      mongoc_read_concern_destroy (database->read_concern);
      database->read_concern = NULL;
   }

   if (read_concern) {
      database->read_concern = mongoc_read_concern_copy (read_concern);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_get_write_concern --
 *
 *       Fetches the write concern for @database.
 *
 * Returns:
 *       A mongoc_write_concern_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_write_concern_t *
mongoc_database_get_write_concern (const mongoc_database_t *database)
{
   BSON_ASSERT (database);

   return database->write_concern;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_database_set_write_concern --
 *
 *       Set the default write concern for @database.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_database_set_write_concern (mongoc_database_t *database,
                                   const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (database);

   if (database->write_concern) {
      mongoc_write_concern_destroy (database->write_concern);
      database->write_concern = NULL;
   }

   if (write_concern) {
      database->write_concern = mongoc_write_concern_copy (write_concern);
   }
}


/**
 * mongoc_database_has_collection:
 * @database: (in): A #mongoc_database_t.
 * @name: (in): The name of the collection to check for.
 * @error: (out) (allow-none): A location for a #bson_error_t, or %NULL.
 *
 * Checks to see if a collection exists within the database on the MongoDB
 * server.
 *
 * This will return %false if their was an error communicating with the
 * server, or if the collection does not exist.
 *
 * If @error is provided, it will first be zeroed. Upon error, error.domain
 * will be set.
 *
 * Returns: %true if @name exists, otherwise %false. @error may be set.
 */
bool
mongoc_database_has_collection (mongoc_database_t *database,
                                const char *name,
                                bson_error_t *error)
{
   bson_iter_t col_iter;
   bool ret = false;
   const char *cur_name;
   bson_t filter = BSON_INITIALIZER;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   ENTRY;

   BSON_ASSERT (database);
   BSON_ASSERT (name);

   if (error) {
      memset (error, 0, sizeof *error);
   }

   BSON_APPEND_UTF8 (&filter, "name", name);

   cursor = mongoc_database_find_collections (database, &filter, error);

   if (!cursor) {
      return ret;
   }

   if (error && ((error->domain != 0) || (error->code != 0))) {
      GOTO (cleanup);
   }

   while (mongoc_cursor_next (cursor, &doc)) {
      if (bson_iter_init (&col_iter, doc) &&
          bson_iter_find (&col_iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&col_iter) &&
          (cur_name = bson_iter_utf8 (&col_iter, NULL))) {
         if (!strcmp (cur_name, name)) {
            ret = true;
            GOTO (cleanup);
         }
      }
   }

cleanup:
   mongoc_cursor_destroy (cursor);

   RETURN (ret);
}

typedef struct {
   const char *dbname;
   size_t dbname_len;
   const char *name;
} mongoc_database_find_collections_legacy_ctx_t;

static mongoc_cursor_transform_mode_t
_mongoc_database_find_collections_legacy_filter (const bson_t *bson, void *ctx_)
{
   bson_iter_t iter;
   mongoc_database_find_collections_legacy_ctx_t *ctx;

   ctx = (mongoc_database_find_collections_legacy_ctx_t *) ctx_;

   if (bson_iter_init_find (&iter, bson, "name") &&
       BSON_ITER_HOLDS_UTF8 (&iter) &&
       (ctx->name = bson_iter_utf8 (&iter, NULL)) && !strchr (ctx->name, '$') &&
       (0 == strncmp (ctx->name, ctx->dbname, ctx->dbname_len))) {
      return MONGO_CURSOR_TRANSFORM_MUTATE;
   } else {
      return MONGO_CURSOR_TRANSFORM_DROP;
   }
}

static void
_mongoc_database_find_collections_legacy_mutate (const bson_t *bson,
                                                 bson_t *out,
                                                 void *ctx_)
{
   mongoc_database_find_collections_legacy_ctx_t *ctx;

   ctx = (mongoc_database_find_collections_legacy_ctx_t *) ctx_;

   bson_copy_to_excluding_noinit (bson, out, "name", NULL);
   BSON_APPEND_UTF8 (
      out, "name", ctx->name + (ctx->dbname_len + 1)); /* +1 for the '.' */
}

/* Uses old way of querying system.namespaces. */
mongoc_cursor_t *
_mongoc_database_find_collections_legacy (mongoc_database_t *database,
                                          const bson_t *filter,
                                          bson_error_t *error)
{
   mongoc_collection_t *col;
   mongoc_cursor_t *cursor = NULL;
   mongoc_read_prefs_t *read_prefs;
   uint32_t dbname_len;
   bson_t legacy_filter;
   bson_iter_t iter;
   const char *col_filter;
   bson_t q = BSON_INITIALIZER;
   mongoc_database_find_collections_legacy_ctx_t *ctx;

   BSON_ASSERT (database);

   col = mongoc_client_get_collection (
      database->client, database->name, "system.namespaces");

   BSON_ASSERT (col);

   dbname_len = (uint32_t) strlen (database->name);

   ctx = (mongoc_database_find_collections_legacy_ctx_t *) bson_malloc (
      sizeof (*ctx));

   ctx->dbname = database->name;
   ctx->dbname_len = dbname_len;

   /* Filtering on name needs to be handled differently for old servers. */
   if (filter && bson_iter_init_find (&iter, filter, "name")) {
      bson_string_t *buf;
      /* on legacy servers, this must be a string (i.e. not a regex) */
      if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
         bson_set_error (
            error,
            MONGOC_ERROR_NAMESPACE,
            MONGOC_ERROR_NAMESPACE_INVALID_FILTER_TYPE,
            "On legacy servers, a filter on name can only be a string.");
         bson_free (ctx);
         goto cleanup_filter;
      }
      BSON_ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      col_filter = bson_iter_utf8 (&iter, NULL);
      bson_init (&legacy_filter);
      bson_copy_to_excluding_noinit (filter, &legacy_filter, "name", NULL);
      /* We must db-qualify filters on name. */
      buf = bson_string_new (database->name);
      bson_string_append_c (buf, '.');
      bson_string_append (buf, col_filter);
      BSON_APPEND_UTF8 (&legacy_filter, "name", buf->str);
      bson_string_free (buf, true);
      filter = &legacy_filter;
   }

   /* Enumerate Collections Spec: "run listCollections on the primary node in
    * replicaset mode" */
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   cursor = mongoc_collection_find_with_opts (
      col, filter ? filter : &q, NULL, read_prefs);

   _mongoc_cursor_transform_init (
      cursor,
      _mongoc_database_find_collections_legacy_filter,
      _mongoc_database_find_collections_legacy_mutate,
      &bson_free,
      ctx);

   mongoc_read_prefs_destroy (read_prefs);

cleanup_filter:
   mongoc_collection_destroy (col);

   return cursor;
}

mongoc_cursor_t *
mongoc_database_find_collections (mongoc_database_t *database,
                                  const bson_t *filter,
                                  bson_error_t *error)
{
   mongoc_cursor_t *cursor;
   bson_t cmd = BSON_INITIALIZER;
   bson_t child;
   bson_error_t lerror;

   BSON_ASSERT (database);

   BSON_APPEND_INT32 (&cmd, "listCollections", 1);

   if (filter) {
      BSON_APPEND_DOCUMENT (&cmd, "filter", filter);
      BSON_APPEND_DOCUMENT_BEGIN (&cmd, "cursor", &child);
      bson_append_document_end (&cmd, &child);
   }

   /* Enumerate Collections Spec: "run listCollections on the primary node in
    * replicaset mode" */
   cursor = _mongoc_cursor_new_with_opts (database->client,
                                          database->name,
                                          true /* is_command */,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);

   _mongoc_cursor_cursorid_init (cursor, &cmd);

   if (_mongoc_cursor_cursorid_prime (cursor)) {
      /* intentionally empty */
   } else {
      if (mongoc_cursor_error (cursor, &lerror)) {
         if (lerror.code == MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND) {
            /* We are talking to a server that doesn' support listCollections.
             */
            /* clear out the error. */
            memset (&lerror, 0, sizeof lerror);
            /* try again with using system.namespaces */
            mongoc_cursor_destroy (cursor);
            cursor = _mongoc_database_find_collections_legacy (
               database, filter, error);
         } else if (error) {
            memcpy (error, &lerror, sizeof *error);
         }
      }
   }

   bson_destroy (&cmd);

   return cursor;
}

char **
mongoc_database_get_collection_names (mongoc_database_t *database,
                                      bson_error_t *error)
{
   bson_iter_t col;
   const char *name;
   char *namecopy;
   mongoc_array_t strv_buf;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   char **ret;

   BSON_ASSERT (database);

   cursor = mongoc_database_find_collections (database, NULL, error);

   if (!cursor) {
      return NULL;
   }

   _mongoc_array_init (&strv_buf, sizeof (char *));

   while (mongoc_cursor_next (cursor, &doc)) {
      if (bson_iter_init (&col, doc) && bson_iter_find (&col, "name") &&
          BSON_ITER_HOLDS_UTF8 (&col) && (name = bson_iter_utf8 (&col, NULL))) {
         namecopy = bson_strdup (name);
         _mongoc_array_append_val (&strv_buf, namecopy);
      }
   }

   /* append a null pointer for the last value. also handles the case
    * of no values. */
   namecopy = NULL;
   _mongoc_array_append_val (&strv_buf, namecopy);

   if (mongoc_cursor_error (cursor, error)) {
      _mongoc_array_destroy (&strv_buf);
      ret = NULL;
   } else {
      ret = (char **) strv_buf.data;
   }

   mongoc_cursor_destroy (cursor);

   return ret;
}


mongoc_collection_t *
mongoc_database_create_collection (mongoc_database_t *database,
                                   const char *name,
                                   const bson_t *opts,
                                   bson_error_t *error)
{
   mongoc_collection_t *collection = NULL;
   bson_iter_t iter;
   bson_t cmd;
   bool capped = false;

   BSON_ASSERT (database);
   BSON_ASSERT (name);

   if (strchr (name, '$')) {
      bson_set_error (error,
                      MONGOC_ERROR_NAMESPACE,
                      MONGOC_ERROR_NAMESPACE_INVALID,
                      "The namespace \"%s\" is invalid.",
                      name);
      return NULL;
   }

   if (opts) {
      if (bson_iter_init_find (&iter, opts, "capped")) {
         if (!BSON_ITER_HOLDS_BOOL (&iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_COMMAND_INVALID_ARG,
                            "The argument \"capped\" must be a boolean.");
            return NULL;
         }
         capped = bson_iter_bool (&iter);
      }

      if (bson_iter_init_find (&iter, opts, "size")) {
         if (!BSON_ITER_HOLDS_INT (&iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_COMMAND_INVALID_ARG,
                            "The argument \"size\" must be an integer.");
            return NULL;
         }
         if (!capped) {
            bson_set_error (
               error,
               MONGOC_ERROR_COMMAND,
               MONGOC_ERROR_COMMAND_INVALID_ARG,
               "The \"size\" parameter requires {\"capped\": true}");
            return NULL;
         }
      }

      if (bson_iter_init_find (&iter, opts, "max")) {
         if (!BSON_ITER_HOLDS_INT (&iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_COMMAND_INVALID_ARG,
                            "The argument \"max\" must be an integer.");
            return NULL;
         }
         if (!capped) {
            bson_set_error (
               error,
               MONGOC_ERROR_COMMAND,
               MONGOC_ERROR_COMMAND_INVALID_ARG,
               "The \"max\" parameter requires {\"capped\": true}");
            return NULL;
         }
      }

      if (bson_iter_init_find (&iter, opts, "storageEngine")) {
         if (!BSON_ITER_HOLDS_DOCUMENT (&iter)) {
            bson_set_error (
               error,
               MONGOC_ERROR_COMMAND,
               MONGOC_ERROR_COMMAND_INVALID_ARG,
               "The \"storageEngine\" parameter must be a document");

            return NULL;
         }

         if (bson_iter_find (&iter, "wiredTiger")) {
            if (!BSON_ITER_HOLDS_DOCUMENT (&iter)) {
               bson_set_error (error,
                               MONGOC_ERROR_COMMAND,
                               MONGOC_ERROR_COMMAND_INVALID_ARG,
                               "The \"wiredTiger\" option must take a document "
                               "argument with a \"configString\" field");
               return NULL;
            }

            if (bson_iter_find (&iter, "configString")) {
               if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
                  bson_set_error (
                     error,
                     MONGOC_ERROR_COMMAND,
                     MONGOC_ERROR_COMMAND_INVALID_ARG,
                     "The \"configString\" parameter must be a string");
                  return NULL;
               }
            } else {
               bson_set_error (error,
                               MONGOC_ERROR_COMMAND,
                               MONGOC_ERROR_COMMAND_INVALID_ARG,
                               "The \"wiredTiger\" option must take a document "
                               "argument with a \"configString\" field");
               return NULL;
            }
         }
      }
   }


   bson_init (&cmd);
   BSON_APPEND_UTF8 (&cmd, "create", name);

   if (_mongoc_client_command_with_opts (database->client,
                                         database->name,
                                         &cmd,
                                         MONGOC_CMD_WRITE,
                                         opts,
                                         MONGOC_QUERY_NONE,
                                         database->read_prefs,
                                         database->read_concern,
                                         database->write_concern,
                                         NULL, /* reply */
                                         error)) {
      collection = _mongoc_collection_new (database->client,
                                           database->name,
                                           name,
                                           database->read_prefs,
                                           database->read_concern,
                                           database->write_concern);
   }

   bson_destroy (&cmd);

   return collection;
}


mongoc_collection_t *
mongoc_database_get_collection (mongoc_database_t *database,
                                const char *collection)
{
   BSON_ASSERT (database);
   BSON_ASSERT (collection);

   return _mongoc_collection_new (database->client,
                                  database->name,
                                  collection,
                                  database->read_prefs,
                                  database->read_concern,
                                  database->write_concern);
}


const char *
mongoc_database_get_name (mongoc_database_t *database)
{
   BSON_ASSERT (database);

   return database->name;
}
