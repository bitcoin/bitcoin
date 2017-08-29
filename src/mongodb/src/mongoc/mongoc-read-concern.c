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


#include "mongoc-log.h"
#include "mongoc-read-concern.h"
#include "mongoc-read-concern-private.h"


static void
_mongoc_read_concern_freeze (mongoc_read_concern_t *read_concern);


/**
 * mongoc_read_concern_new:
 *
 * Create a new mongoc_read_concern_t.
 *
 * Returns: A newly allocated mongoc_read_concern_t. This should be freed
 *    with mongoc_read_concern_destroy().
 */
mongoc_read_concern_t *
mongoc_read_concern_new (void)
{
   mongoc_read_concern_t *read_concern;

   read_concern = (mongoc_read_concern_t *) bson_malloc0 (sizeof *read_concern);

   return read_concern;
}


mongoc_read_concern_t *
mongoc_read_concern_copy (const mongoc_read_concern_t *read_concern)
{
   mongoc_read_concern_t *ret = NULL;

   if (read_concern) {
      ret = mongoc_read_concern_new ();
      ret->level = bson_strdup (read_concern->level);
   }

   return ret;
}


/**
 * mongoc_read_concern_destroy:
 * @read_concern: A mongoc_read_concern_t.
 *
 * Releases a mongoc_read_concern_t and all associated memory.
 */
void
mongoc_read_concern_destroy (mongoc_read_concern_t *read_concern)
{
   if (read_concern) {
      if (read_concern->compiled.len) {
         bson_destroy (&read_concern->compiled);
      }

      bson_free (read_concern->level);
      bson_free (read_concern);
   }
}


const char *
mongoc_read_concern_get_level (const mongoc_read_concern_t *read_concern)
{
   BSON_ASSERT (read_concern);

   return read_concern->level;
}


/**
 * mongoc_read_concern_set_level:
 * @read_concern: A mongoc_read_concern_t.
 * @level: The read concern level
 *
 * Sets the read concern level. Any string is supported for future compatibility
 * but MongoDB 3.2 only accepts "local" and "majority", aka:
 *  - MONGOC_READ_CONCERN_LEVEL_LOCAL
 *  - MONGOC_READ_CONCERN_LEVEL_MAJORITY
 * MongoDB 3.4 added
 *  - MONGOC_READ_CONCERN_LEVEL_LINEARIZABLE
 *
 *  If the @read_concern has already been frozen, calling this function will not
 *  alter the read concern level.
 *
 * See the MongoDB docs for more information on readConcernLevel
 */
bool
mongoc_read_concern_set_level (mongoc_read_concern_t *read_concern,
                               const char *level)
{
   BSON_ASSERT (read_concern);

   if (read_concern->frozen) {
      return false;
   }

   bson_free (read_concern->level);
   read_concern->level = bson_strdup (level);
   return true;
}

/**
 * mongoc_read_concern_append:
 * @read_concern: (in): A mongoc_read_concern_t.
 * @opts: (out): A pointer to a bson document.
 *
 * Appends a read_concern document to command options to send to
 * a server.
 *
 * Returns true on success, false on failure.
 *
 */
bool
mongoc_read_concern_append (mongoc_read_concern_t *read_concern,
                            bson_t *command)
{
   BSON_ASSERT (read_concern);

   if (!read_concern->level) {
      return true;
   }

   if (!bson_append_document (command,
                              "readConcern",
                              11,
                              _mongoc_read_concern_get_bson (read_concern))) {
      MONGOC_ERROR ("Could not append readConcern to command.");
      return false;
   }

   return true;
}


/**
 * mongoc_read_concern_is_default:
 * @read_concern: A const mongoc_read_concern_t.
 *
 * Returns true when read_concern has not been modified.
 */
bool
mongoc_read_concern_is_default (const mongoc_read_concern_t *read_concern)
{
   return !read_concern || !read_concern->level;
}


/**
 * mongoc_read_concern_get_bson:
 * @read_concern: A mongoc_read_concern_t.
 *
 * This is an internal function.
 *
 * Freeze the read concern if necessary and retrieve the encoded bson_t
 * representing the read concern.
 *
 * You may not modify the read concern further after calling this function.
 *
 * Returns: A bson_t that should not be modified or freed as it is owned by
 *    the mongoc_read_concern_t instance.
 */
const bson_t *
_mongoc_read_concern_get_bson (mongoc_read_concern_t *read_concern)
{
   if (!read_concern->frozen) {
      _mongoc_read_concern_freeze (read_concern);
   }

   return &read_concern->compiled;
}

/**
 * mongoc_read_concern_freeze:
 * @read_concern: A mongoc_read_concern_t.
 *
 * This is an internal function.
 *
 * Freeze the read concern if necessary and encode it into a bson_ts which
 * represent the raw bson form and the get last error command form.
 *
 * You may not modify the read concern further after calling this function.
 */
static void
_mongoc_read_concern_freeze (mongoc_read_concern_t *read_concern)
{
   bson_t *compiled;

   BSON_ASSERT (read_concern);

   compiled = &read_concern->compiled;

   read_concern->frozen = true;

   bson_init (compiled);

   BSON_ASSERT (read_concern->level);
   BSON_APPEND_UTF8 (compiled, "level", read_concern->level);
}
