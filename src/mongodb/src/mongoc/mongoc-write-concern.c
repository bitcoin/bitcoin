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


#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-write-concern.h"
#include "mongoc-write-concern-private.h"


static BSON_INLINE bool
_mongoc_write_concern_warn_frozen (mongoc_write_concern_t *write_concern)
{
   if (write_concern->frozen) {
      MONGOC_WARNING ("Cannot modify a frozen write-concern.");
   }

   return write_concern->frozen;
}

static void
_mongoc_write_concern_freeze (mongoc_write_concern_t *write_concern);


/**
 * mongoc_write_concern_new:
 *
 * Create a new mongoc_write_concern_t.
 *
 * Returns: A newly allocated mongoc_write_concern_t. This should be freed
 *    with mongoc_write_concern_destroy().
 */
mongoc_write_concern_t *
mongoc_write_concern_new (void)
{
   mongoc_write_concern_t *write_concern;

   write_concern =
      (mongoc_write_concern_t *) bson_malloc0 (sizeof *write_concern);
   write_concern->w = MONGOC_WRITE_CONCERN_W_DEFAULT;
   write_concern->fsync_ = MONGOC_WRITE_CONCERN_FSYNC_DEFAULT;
   write_concern->journal = MONGOC_WRITE_CONCERN_JOURNAL_DEFAULT;
   write_concern->is_default = true;

   return write_concern;
}


mongoc_write_concern_t *
mongoc_write_concern_copy (const mongoc_write_concern_t *write_concern)
{
   mongoc_write_concern_t *ret = NULL;

   if (write_concern) {
      ret = mongoc_write_concern_new ();
      ret->fsync_ = write_concern->fsync_;
      ret->journal = write_concern->journal;
      ret->w = write_concern->w;
      ret->wtimeout = write_concern->wtimeout;
      ret->frozen = false;
      ret->wtag = bson_strdup (write_concern->wtag);
      ret->is_default = write_concern->is_default;
   }

   return ret;
}


/**
 * mongoc_write_concern_destroy:
 * @write_concern: A mongoc_write_concern_t.
 *
 * Releases a mongoc_write_concern_t and all associated memory.
 */
void
mongoc_write_concern_destroy (mongoc_write_concern_t *write_concern)
{
   if (write_concern) {
      if (write_concern->compiled.len) {
         bson_destroy (&write_concern->compiled);
         bson_destroy (&write_concern->compiled_gle);
      }

      bson_free (write_concern->wtag);
      bson_free (write_concern);
   }
}


bool
mongoc_write_concern_get_fsync (const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (write_concern);
   return (write_concern->fsync_ == true);
}


/**
 * mongoc_write_concern_set_fsync:
 * @write_concern: A mongoc_write_concern_t.
 * @fsync_: If the write concern requires fsync() by the server.
 *
 * Set if fsync() should be called on the server before acknowledging a
 * write request.
 */
void
mongoc_write_concern_set_fsync (mongoc_write_concern_t *write_concern,
                                bool fsync_)
{
   BSON_ASSERT (write_concern);

   if (!_mongoc_write_concern_warn_frozen (write_concern)) {
      write_concern->fsync_ = !!fsync_;
      write_concern->is_default = false;
   }
}


bool
mongoc_write_concern_get_journal (const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (write_concern);
   return (write_concern->journal == true);
}


bool
mongoc_write_concern_journal_is_set (
   const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (write_concern);
   return (write_concern->journal != MONGOC_WRITE_CONCERN_JOURNAL_DEFAULT);
}


/**
 * mongoc_write_concern_set_journal:
 * @write_concern: A mongoc_write_concern_t.
 * @journal: If the write should be journaled.
 *
 * Set if the write request should be journaled before acknowledging the
 * write request.
 */
void
mongoc_write_concern_set_journal (mongoc_write_concern_t *write_concern,
                                  bool journal)
{
   BSON_ASSERT (write_concern);

   if (!_mongoc_write_concern_warn_frozen (write_concern)) {
      write_concern->journal = !!journal;
      write_concern->is_default = false;
   }
}


int32_t
mongoc_write_concern_get_w (const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (write_concern);
   return write_concern->w;
}


/**
 * mongoc_write_concern_set_w:
 * @w: The number of nodes for write or MONGOC_WRITE_CONCERN_W_MAJORITY
 * for "majority".
 *
 * Sets the number of nodes that must acknowledge the write request before
 * acknowledging the write request to the client.
 *
 * You may specifiy @w as MONGOC_WRITE_CONCERN_W_MAJORITY to request that
 * a "majority" of nodes acknowledge the request.
 */
void
mongoc_write_concern_set_w (mongoc_write_concern_t *write_concern, int32_t w)
{
   BSON_ASSERT (write_concern);
   BSON_ASSERT (w >= -3);

   if (!_mongoc_write_concern_warn_frozen (write_concern)) {
      write_concern->w = w;
      if (w != MONGOC_WRITE_CONCERN_W_DEFAULT) {
         write_concern->is_default = false;
      }
   }
}


int32_t
mongoc_write_concern_get_wtimeout (const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (write_concern);
   return write_concern->wtimeout;
}


/**
 * mongoc_write_concern_set_wtimeout:
 * @write_concern: A mongoc_write_concern_t.
 * @wtimeout_msec: Number of milliseconds before timeout.
 *
 * Sets the number of milliseconds to wait before considering a write
 * request as failed. A value of 0 indicates no write timeout.
 *
 * The @wtimeout_msec parameter must be positive or zero. Negative values will
 * be ignored.
 */
void
mongoc_write_concern_set_wtimeout (mongoc_write_concern_t *write_concern,
                                   int32_t wtimeout_msec)
{
   BSON_ASSERT (write_concern);

   if (wtimeout_msec < 0) {
      return;
   }

   if (!_mongoc_write_concern_warn_frozen (write_concern)) {
      write_concern->wtimeout = wtimeout_msec;
      write_concern->is_default = false;
   }
}


bool
mongoc_write_concern_get_wmajority (const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (write_concern);
   return (write_concern->w == MONGOC_WRITE_CONCERN_W_MAJORITY);
}


/**
 * mongoc_write_concern_set_wmajority:
 * @write_concern: A mongoc_write_concern_t.
 * @wtimeout_msec: Number of milliseconds before timeout.
 *
 * Sets the "w" of a write concern to "majority". It is suggested that
 * you provide a reasonable @wtimeout_msec to wait before considering the
 * write request failed. A @wtimeout_msec value of 0 indicates no write timeout.
 *
 * The @wtimeout_msec parameter must be positive or zero. Negative values will
 * be ignored.
 */
void
mongoc_write_concern_set_wmajority (mongoc_write_concern_t *write_concern,
                                    int32_t wtimeout_msec)
{
   BSON_ASSERT (write_concern);

   if (!_mongoc_write_concern_warn_frozen (write_concern)) {
      write_concern->w = MONGOC_WRITE_CONCERN_W_MAJORITY;
      write_concern->is_default = false;

      if (wtimeout_msec >= 0) {
         write_concern->wtimeout = wtimeout_msec;
      }
   }
}


const char *
mongoc_write_concern_get_wtag (const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT (write_concern);

   if (write_concern->w == MONGOC_WRITE_CONCERN_W_TAG) {
      return write_concern->wtag;
   }

   return NULL;
}


void
mongoc_write_concern_set_wtag (mongoc_write_concern_t *write_concern,
                               const char *wtag)
{
   BSON_ASSERT (write_concern);

   if (!_mongoc_write_concern_warn_frozen (write_concern)) {
      bson_free (write_concern->wtag);
      write_concern->wtag = bson_strdup (wtag);
      write_concern->w = MONGOC_WRITE_CONCERN_W_TAG;
      write_concern->is_default = false;
   }
}

/**
 * mongoc_write_concern_get_bson:
 * @write_concern: A mongoc_write_concern_t.
 *
 * This is an internal function.
 *
 * Freeze the write concern if necessary and retrieve the encoded bson_t
 * representing the write concern.
 *
 * You may not modify the write concern further after calling this function.
 *
 * Returns: A bson_t that should not be modified or freed as it is owned by
 *    the mongoc_write_concern_t instance.
 */
const bson_t *
_mongoc_write_concern_get_bson (mongoc_write_concern_t *write_concern)
{
   if (!write_concern->frozen) {
      _mongoc_write_concern_freeze (write_concern);
   }

   return &write_concern->compiled;
}

/**
 * mongoc_write_concern_get_gle:
 * @write_concern: A mongoc_write_concern_t.
 *
 * This is an internal function.
 *
 * Freeze the write concern if necessary and retrieve the encoded bson_t
 * representing the write concern as a get last error command.
 *
 * You may not modify the write concern further after calling this function.
 *
 * Returns: A bson_t that should not be modified or freed as it is owned by
 *    the mongoc_write_concern_t instance.
 */
const bson_t *
_mongoc_write_concern_get_gle (mongoc_write_concern_t *write_concern)
{
   if (!write_concern->frozen) {
      _mongoc_write_concern_freeze (write_concern);
   }

   return &write_concern->compiled_gle;
}


/**
 * mongoc_write_concern_is_default:
 * @write_concern: A mongoc_write_concern_t.
 *
 * Returns is_default, which is true when write_concern has not been modified.
 *
 */
bool
mongoc_write_concern_is_default (const mongoc_write_concern_t *write_concern)
{
   return !write_concern || write_concern->is_default;
}


/**
 * mongoc_write_concern_freeze:
 * @write_concern: A mongoc_write_concern_t.
 *
 * This is an internal function.
 *
 * Freeze the write concern if necessary and encode it into a bson_ts which
 * represent the raw bson form and the get last error command form.
 *
 * You may not modify the write concern further after calling this function.
 */
static void
_mongoc_write_concern_freeze (mongoc_write_concern_t *write_concern)
{
   bson_t *compiled;
   bson_t *compiled_gle;

   BSON_ASSERT (write_concern);

   compiled = &write_concern->compiled;
   compiled_gle = &write_concern->compiled_gle;

   write_concern->frozen = true;

   bson_init (compiled);
   bson_init (compiled_gle);

   if (write_concern->w == MONGOC_WRITE_CONCERN_W_TAG) {
      BSON_ASSERT (write_concern->wtag);
      BSON_APPEND_UTF8 (compiled, "w", write_concern->wtag);
   } else if (write_concern->w == MONGOC_WRITE_CONCERN_W_MAJORITY) {
      BSON_APPEND_UTF8 (compiled, "w", "majority");
   } else if (write_concern->w == MONGOC_WRITE_CONCERN_W_DEFAULT) {
      /* Do Nothing */
   } else {
      BSON_APPEND_INT32 (compiled, "w", write_concern->w);
   }

   if (write_concern->fsync_ != MONGOC_WRITE_CONCERN_FSYNC_DEFAULT) {
      bson_append_bool (compiled, "fsync", 5, !!write_concern->fsync_);
   }

   if (write_concern->journal != MONGOC_WRITE_CONCERN_JOURNAL_DEFAULT) {
      bson_append_bool (compiled, "j", 1, !!write_concern->journal);
   }

   if (write_concern->wtimeout) {
      bson_append_int32 (compiled, "wtimeout", 8, write_concern->wtimeout);
   }

   BSON_APPEND_INT32 (compiled_gle, "getlasterror", 1);
   bson_concat (compiled_gle, compiled);
}


/**
 * mongoc_write_concern_is_acknowledged:
 * @concern: (in): A mongoc_write_concern_t.
 *
 * Checks to see if @write_concern requests that a getlasterror command is to
 * be delivered to the MongoDB server.
 *
 * Returns: true if a getlasterror command should be sent.
 */
bool
mongoc_write_concern_is_acknowledged (
   const mongoc_write_concern_t *write_concern)
{
   if (write_concern) {
      return (((write_concern->w != MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED) &&
               (write_concern->w != MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED)) ||
              write_concern->fsync_ == true ||
              mongoc_write_concern_get_journal (write_concern));
   }
   return true;
}


/**
 * mongoc_write_concern_is_valid:
 * @write_concern: (in): A mongoc_write_concern_t.
 *
 * Checks to see if @write_concern is valid and does not contain conflicting
 * options.
 *
 * Returns: true if the write concern is valid; otherwise false.
 */
bool
mongoc_write_concern_is_valid (const mongoc_write_concern_t *write_concern)
{
   if (!write_concern) {
      return false;
   }

   /* Journal or fsync should require acknowledgement.  */
   if ((write_concern->fsync_ == true ||
        mongoc_write_concern_get_journal (write_concern)) &&
       (write_concern->w == MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED ||
        write_concern->w == MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED)) {
      return false;
   }

   if (write_concern->wtimeout < 0) {
      return false;
   }

   return true;
}


bool
_mongoc_write_concern_validate (const mongoc_write_concern_t *write_concern,
                                bson_error_t *error)
{
   if (write_concern && !mongoc_write_concern_is_valid (write_concern)) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Invalid mongoc_write_concern_t");
      return false;
   }
   return true;
}


/**
 * _mongoc_parse_wc_err:
 * @doc: (in): A bson document.
 * @error: (out): A bson_error_t.
 *
 * Parses a document, usually a server reply,
 * looking for a writeConcernError. Returns true if
 * there is a writeConcernError, false otherwise.
 */
bool
_mongoc_parse_wc_err (const bson_t *doc, bson_error_t *error)
{
   bson_iter_t iter;
   bson_iter_t inner;

   if (bson_iter_init_find (&iter, doc, "writeConcernError") &&
       BSON_ITER_HOLDS_DOCUMENT (&iter)) {
      const char *errmsg = NULL;
      int32_t code = 0;
      bson_iter_recurse (&iter, &inner);
      while (bson_iter_next (&inner)) {
         if (BSON_ITER_IS_KEY (&inner, "code")) {
            code = bson_iter_int32 (&inner);
         } else if (BSON_ITER_IS_KEY (&inner, "errmsg")) {
            errmsg = bson_iter_utf8 (&inner, NULL);
         }
      }
      bson_set_error (error,
                      MONGOC_ERROR_WRITE_CONCERN,
                      code,
                      "Write Concern error: %s",
                      errmsg);
      return true;
   }
   return false;
}


/**
 * mongoc_write_concern_append:
 * @write_concern: (in): A mongoc_write_concern_t.
 * @command: (out): A pointer to a bson document.
 *
 * Appends a write_concern document to a command, to send to
 * a server.
 *
 * Returns true on success, false on failure.
 *
 */
bool
mongoc_write_concern_append (mongoc_write_concern_t *write_concern,
                             bson_t *command)
{
   if (!mongoc_write_concern_is_valid (write_concern)) {
      MONGOC_ERROR ("Invalid writeConcern passed into "
                    "mongoc_write_concern_append.");
      return false;
   }
   if (!bson_append_document (command,
                              "writeConcern",
                              12,
                              _mongoc_write_concern_get_bson (write_concern))) {
      MONGOC_ERROR ("Could not append writeConcern to command.");
      return false;
   }
   return true;
}

/**
 * _mongoc_write_concern_new_from_iter:
 *
 * Create a new mongoc_write_concern_t from an iterator positioned on
 * a "writeConcern" document.
 *
 * Returns: A newly allocated mongoc_write_concern_t. This should be freed
 *    with mongoc_write_concern_destroy().
 */
mongoc_write_concern_t *
_mongoc_write_concern_new_from_iter (bson_iter_t *iter)
{
   bson_iter_t inner;
   mongoc_write_concern_t *write_concern;

   BSON_ASSERT (iter);
   write_concern =
      (mongoc_write_concern_t *) bson_malloc0 (sizeof *write_concern);
   write_concern->w = MONGOC_WRITE_CONCERN_W_DEFAULT;
   write_concern->fsync_ = MONGOC_WRITE_CONCERN_FSYNC_DEFAULT;
   write_concern->journal = MONGOC_WRITE_CONCERN_JOURNAL_DEFAULT;

   BSON_ASSERT (bson_iter_recurse (iter, &inner));
   while (bson_iter_next (&inner)) {
      if (BSON_ITER_IS_KEY (&inner, "w")) {
         if (BSON_ITER_HOLDS_INT32 (&inner)) {
            write_concern->w = bson_iter_int32 (&inner);
         } else if (BSON_ITER_HOLDS_UTF8 (&inner)) {
            if (!strcmp (bson_iter_utf8 (&inner, NULL), "majority")) {
               write_concern->w = MONGOC_WRITE_CONCERN_W_MAJORITY;
            } else {
               write_concern->w = MONGOC_WRITE_CONCERN_W_TAG;
               write_concern->wtag = bson_iter_dup_utf8 (&inner, NULL);
            }
         }
      } else if (BSON_ITER_IS_KEY (&inner, "fsync") &&
                 BSON_ITER_HOLDS_BOOL (&inner)) {
         write_concern->fsync_ = bson_iter_bool (&inner);
      } else if (BSON_ITER_IS_KEY (&inner, "j") &&
                 BSON_ITER_HOLDS_BOOL (&inner)) {
         write_concern->journal = bson_iter_bool (&inner);
      } else if (BSON_ITER_IS_KEY (&inner, "wtimeout") &&
                 BSON_ITER_HOLDS_INT32 (&inner)) {
         write_concern->wtimeout = bson_iter_bool (&inner);
      }
   }

   return write_concern;
}

/**
 * _mongoc_write_concern_iter_is_valid:
 * @iter: (in): A bson_iter_t positioned on a "writeConcern" BSON document.
 *
 * Checks to see if @write_concern is valid and does not contain conflicting
 * options.
 *
 * Returns: true if the write concern is valid; otherwise false.
 */
bool
_mongoc_write_concern_iter_is_valid (bson_iter_t *iter)
{
   bson_iter_t inner;
   bool has_fsync = false;
   bool w0 = false;
   bool j = false;

   BSON_ASSERT (iter);
   BSON_ASSERT (bson_iter_recurse (iter, &inner));
   while (bson_iter_next (&inner)) {
      if (BSON_ITER_IS_KEY (&inner, "fsync")) {
         if (!BSON_ITER_HOLDS_BOOL (&inner)) {
            return false;
         }
         has_fsync = bson_iter_bool (&inner);
      } else if (BSON_ITER_IS_KEY (&inner, "w")) {
         if (BSON_ITER_HOLDS_INT32 (&inner)) {
            if (bson_iter_int32 (&inner) ==
                   MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED ||
                bson_iter_int32 (&inner) ==
                   MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED) {
               w0 = true;
            }
         } else if (!(BSON_ITER_HOLDS_UTF8 (&inner))) {
            return false;
         }
      } else if (BSON_ITER_IS_KEY (&inner, "j")) {
         if (!BSON_ITER_HOLDS_BOOL (&inner)) {
            return false;
         }
         j = bson_iter_bool (&inner);
      } else if (BSON_ITER_IS_KEY (&inner, "wtimeout")) {
         if (!BSON_ITER_HOLDS_INT32 (&inner) || bson_iter_int32 (&inner) < 0) {
            return false;
         }
      }
   }

   if ((has_fsync || j) && w0) {
      return false;
   }

   return true;
}
