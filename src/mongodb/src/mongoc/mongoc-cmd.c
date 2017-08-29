/*
 * Copyright 2017 MongoDB, Inc.
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


#include "mongoc-cmd-private.h"
#include "mongoc-read-prefs-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-client-private.h"
#include "mongoc-write-concern-private.h"


void
mongoc_cmd_parts_init (mongoc_cmd_parts_t *parts,
                       const char *db_name,
                       mongoc_query_flags_t user_query_flags,
                       const bson_t *command_body)
{
   parts->body = command_body;
   parts->user_query_flags = user_query_flags;
   parts->read_prefs = NULL;
   parts->is_write_command = false;
   bson_init (&parts->extra);
   bson_init (&parts->assembled_body);

   parts->assembled.db_name = db_name;
   parts->assembled.command = NULL;
   parts->assembled.query_flags = MONGOC_QUERY_NONE;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_append_opts --
 *
 *       Take an iterator over user-supplied options document and append the
 *       options to @parts->command_extra, taking the selected server's max
 *       wire version into account.
 *
 * Return:
 *       True if the options were successfully applied. If any options are
 *       invalid, returns false and fills out @error. In that case @parts is
 *       invalid and must not be used.
 *
 * Side effects:
 *       May partly apply options before returning an error.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cmd_parts_append_opts (mongoc_cmd_parts_t *parts,
                              bson_iter_t *iter,
                              int max_wire_version,
                              bson_error_t *error)
{
   ENTRY;

   /* not yet assembled */
   BSON_ASSERT (!parts->assembled.command);

   while (bson_iter_next (iter)) {
      if (BSON_ITER_IS_KEY (iter, "collation")) {
         if (max_wire_version < WIRE_VERSION_COLLATION) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                            "The selected server does not support collation");
            RETURN (false);
         }

      } else if (BSON_ITER_IS_KEY (iter, "writeConcern")) {
         if (!_mongoc_write_concern_iter_is_valid (iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_COMMAND_INVALID_ARG,
                            "Invalid writeConcern");
            RETURN (false);
         }

         if (max_wire_version < WIRE_VERSION_CMD_WRITE_CONCERN) {
            continue;
         }

      } else if (BSON_ITER_IS_KEY (iter, "readConcern")) {
         if (max_wire_version < WIRE_VERSION_READ_CONCERN) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                            "The selected server does not support readConcern");
            RETURN (false);
         }
      } else if (BSON_ITER_IS_KEY (iter, "serverId")) {
         continue;
      }

      bson_append_iter (&parts->extra, bson_iter_key (iter), -1, iter);
   }

   RETURN (true);
}


/* Update result with the read prefs, following Server Selection Spec.
 * The driver must have discovered the server is a mongos.
 */
static void
_cmd_parts_apply_read_preferences_mongos (mongoc_cmd_parts_t *parts)
{
   mongoc_read_mode_t mode;
   const bson_t *tags = NULL;
   bson_t child;
   const char *mode_str;
   int64_t stale;

   mode = mongoc_read_prefs_get_mode (parts->read_prefs);
   if (parts->read_prefs) {
      tags = mongoc_read_prefs_get_tags (parts->read_prefs);
   }

   /* Server Selection Spec says:
    *
    * For mode 'primary', drivers MUST NOT set the slaveOK wire protocol flag
    *   and MUST NOT use $readPreference
    *
    * For mode 'secondary', drivers MUST set the slaveOK wire protocol flag and
    *   MUST also use $readPreference
    *
    * For mode 'primaryPreferred', drivers MUST set the slaveOK wire protocol
    *   flag and MUST also use $readPreference
    *
    * For mode 'secondaryPreferred', drivers MUST set the slaveOK wire protocol
    *   flag. If the read preference contains a non-empty tag_sets parameter,
    *   drivers MUST use $readPreference; otherwise, drivers MUST NOT use
    *   $readPreference
    *
    * For mode 'nearest', drivers MUST set the slaveOK wire protocol flag and
    *   MUST also use $readPreference
    */
   if (mode == MONGOC_READ_SECONDARY_PREFERRED && bson_empty0 (tags)) {
      parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;
   } else if (mode != MONGOC_READ_PRIMARY) {
      parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;

      /* Server Selection Spec: "When any $ modifier is used, including the
       * $readPreference modifier, the query MUST be provided using the $query
       * modifier".
       *
       * This applies to commands, too.
       */

      if (bson_has_field (parts->body, "$query")) {
         bson_concat (&parts->assembled_body, parts->body);
      } else {
         bson_append_document (
            &parts->assembled_body, "$query", 6, parts->body);
      }

      bson_append_document_begin (
         &parts->assembled_body, "$readPreference", 15, &child);

      mode_str = _mongoc_read_mode_as_str (mode);
      bson_append_utf8 (&child, "mode", 4, mode_str, -1);
      if (!bson_empty0 (tags)) {
         bson_append_array (&child, "tags", 4, tags);
      }

      stale = mongoc_read_prefs_get_max_staleness_seconds (parts->read_prefs);
      if (stale != MONGOC_NO_MAX_STALENESS) {
         bson_append_int64 (&child, "maxStalenessSeconds", 19, stale);
      }

      bson_append_document_end (&parts->assembled_body, &child);
      parts->assembled.command = &parts->assembled_body;
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_assemble --
 *
 *       Assemble the command body, options, and read preference into one
 *       command.
 *
 * Side effects:
 *       Sets @parts->command_ptr and @parts->query_flags. Concatenates
 *       @parts->body and @parts->command_extra into @parts->assembled if
 *       needed.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cmd_parts_assemble (mongoc_cmd_parts_t *parts,
                           const mongoc_server_stream_t *server_stream)
{
   mongoc_server_description_type_t server_type;

   ENTRY;

   BSON_ASSERT (parts);
   BSON_ASSERT (server_stream);

   server_type = server_stream->sd->type;

   /* must not be assembled already */
   BSON_ASSERT (!parts->assembled.command);
   BSON_ASSERT (bson_empty (&parts->assembled_body));

   /* begin with raw flags/cmd as assembled flags/cmd, might change below */
   parts->assembled.command = parts->body;
   parts->assembled.query_flags = parts->user_query_flags;
   parts->assembled.server_id = server_stream->sd->id;

   if (!parts->is_write_command) {
      switch (server_stream->topology_type) {
      case MONGOC_TOPOLOGY_SINGLE:
         if (server_type == MONGOC_SERVER_MONGOS) {
            _cmd_parts_apply_read_preferences_mongos (parts);
         } else {
            /* Server Selection Spec: for topology type single and server types
             * besides mongos, "clients MUST always set the slaveOK wire
             * protocol flag on reads to ensure that any server type can handle
             * the request."
             */
            parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;
         }

         break;

      case MONGOC_TOPOLOGY_RS_NO_PRIMARY:
      case MONGOC_TOPOLOGY_RS_WITH_PRIMARY:
         /* Server Selection Spec: for RS topology types, "For all read
          * preferences modes except primary, clients MUST set the slaveOK wire
          * protocol flag to ensure that any suitable server can handle the
          * request. Clients MUST  NOT set the slaveOK wire protocol flag if the
          * read preference mode is primary.
          */
         if (parts->read_prefs &&
             parts->read_prefs->mode != MONGOC_READ_PRIMARY) {
            parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;
         }

         break;

      case MONGOC_TOPOLOGY_SHARDED:
         _cmd_parts_apply_read_preferences_mongos (parts);
         break;

      case MONGOC_TOPOLOGY_UNKNOWN:
      case MONGOC_TOPOLOGY_DESCRIPTION_TYPES:
      default:
         /* must not call mongoc_cmd_parts_assemble w/ unknown topology type */
         BSON_ASSERT (false);
      }
   } /* if (!parts->is_write_command) */

   if (!bson_empty (&parts->extra)) {
      /* Did we already copy the command body? */
      if (parts->assembled.command == parts->body) {
         bson_concat (&parts->assembled_body, parts->body);
         bson_concat (&parts->assembled_body, &parts->extra);
         parts->assembled.command = &parts->assembled_body;
      }
   }

   EXIT;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_assemble_simple --
 *
 *       Sets @parts->assembled.command and @parts->query_flags, without
 *       applying any server-specific logic.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cmd_parts_assemble_simple (mongoc_cmd_parts_t *parts, uint32_t server_id)
{
   /* must not be assembled already, must have no options set */
   BSON_ASSERT (!parts->assembled.command);
   BSON_ASSERT (bson_empty (&parts->assembled_body));
   BSON_ASSERT (bson_empty (&parts->extra));

   parts->assembled.query_flags = parts->user_query_flags;
   parts->assembled.command = parts->body;
   parts->assembled.server_id = server_id;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_cleanup --
 *
 *       Free memory associated with a stack-allocated mongoc_cmd_parts_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cmd_parts_cleanup (mongoc_cmd_parts_t *parts)
{
   bson_destroy (&parts->extra);
   bson_destroy (&parts->assembled_body);
}
