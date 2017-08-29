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


#include "mongoc-write-concern.h"
#include "mongoc-write-concern-private.h"
#include "mongoc-find-and-modify.h"
#include "mongoc-find-and-modify-private.h"
#include "mongoc-util-private.h"


/**
 * mongoc_find_and_modify_new:
 *
 * Create a new mongoc_find_and_modify_t.
 *
 * Returns: A newly allocated mongoc_find_and_modify_t. This should be freed
 *    with mongoc_find_and_modify_destroy().
 */
mongoc_find_and_modify_opts_t *
mongoc_find_and_modify_opts_new (void)
{
   mongoc_find_and_modify_opts_t *opts = NULL;

   opts = (mongoc_find_and_modify_opts_t *) bson_malloc0 (sizeof *opts);
   bson_init (&opts->extra);
   opts->bypass_document_validation = MONGOC_BYPASS_DOCUMENT_VALIDATION_DEFAULT;

   return opts;
}

bool
mongoc_find_and_modify_opts_set_sort (mongoc_find_and_modify_opts_t *opts,
                                      const bson_t *sort)
{
   BSON_ASSERT (opts);

   if (sort) {
      _mongoc_bson_destroy_if_set (opts->sort);
      opts->sort = bson_copy (sort);
      return true;
   }

   return false;
}

void
mongoc_find_and_modify_opts_get_sort (const mongoc_find_and_modify_opts_t *opts,
                                      bson_t *sort)
{
   BSON_ASSERT (opts);
   BSON_ASSERT (sort);

   if (opts->sort) {
      bson_copy_to (opts->sort, sort);
   } else {
      bson_init (sort);
   }
}

bool
mongoc_find_and_modify_opts_set_update (mongoc_find_and_modify_opts_t *opts,
                                        const bson_t *update)
{
   BSON_ASSERT (opts);

   if (update) {
      _mongoc_bson_destroy_if_set (opts->update);
      opts->update = bson_copy (update);
      return true;
   }

   return false;
}

void
mongoc_find_and_modify_opts_get_update (
   const mongoc_find_and_modify_opts_t *opts, bson_t *update)
{
   BSON_ASSERT (opts);
   BSON_ASSERT (update);

   if (opts->update) {
      bson_copy_to (opts->update, update);
   } else {
      bson_init (update);
   }
}

bool
mongoc_find_and_modify_opts_set_fields (mongoc_find_and_modify_opts_t *opts,
                                        const bson_t *fields)
{
   BSON_ASSERT (opts);

   if (fields) {
      _mongoc_bson_destroy_if_set (opts->fields);
      opts->fields = bson_copy (fields);
      return true;
   }

   return false;
}

void
mongoc_find_and_modify_opts_get_fields (
   const mongoc_find_and_modify_opts_t *opts, bson_t *fields)
{
   BSON_ASSERT (opts);
   BSON_ASSERT (fields);

   if (opts->fields) {
      bson_copy_to (opts->fields, fields);
   } else {
      bson_init (fields);
   }
}

bool
mongoc_find_and_modify_opts_set_flags (
   mongoc_find_and_modify_opts_t *opts,
   const mongoc_find_and_modify_flags_t flags)
{
   BSON_ASSERT (opts);

   opts->flags = flags;
   return true;
}

mongoc_find_and_modify_flags_t
mongoc_find_and_modify_opts_get_flags (
   const mongoc_find_and_modify_opts_t *opts)
{
   BSON_ASSERT (opts);

   return opts->flags;
}

bool
mongoc_find_and_modify_opts_set_bypass_document_validation (
   mongoc_find_and_modify_opts_t *opts, bool bypass)
{
   BSON_ASSERT (opts);

   opts->bypass_document_validation =
      bypass ? MONGOC_BYPASS_DOCUMENT_VALIDATION_TRUE
             : MONGOC_BYPASS_DOCUMENT_VALIDATION_FALSE;
   return true;
}

bool
mongoc_find_and_modify_opts_get_bypass_document_validation (
   const mongoc_find_and_modify_opts_t *opts)
{
   BSON_ASSERT (opts);

   return opts->bypass_document_validation ==
          MONGOC_BYPASS_DOCUMENT_VALIDATION_TRUE;
}

bool
mongoc_find_and_modify_opts_set_max_time_ms (
   mongoc_find_and_modify_opts_t *opts, uint32_t max_time_ms)
{
   BSON_ASSERT (opts);

   opts->max_time_ms = max_time_ms;
   return true;
}

uint32_t
mongoc_find_and_modify_opts_get_max_time_ms (
   const mongoc_find_and_modify_opts_t *opts)
{
   BSON_ASSERT (opts);

   return opts->max_time_ms;
}

bool
mongoc_find_and_modify_opts_append (mongoc_find_and_modify_opts_t *opts,
                                    const bson_t *extra)
{
   BSON_ASSERT (opts);
   BSON_ASSERT (extra);

   return bson_concat (&opts->extra, extra);
}

void
mongoc_find_and_modify_opts_get_extra (
   const mongoc_find_and_modify_opts_t *opts, bson_t *extra)
{
   BSON_ASSERT (opts);
   BSON_ASSERT (extra);

   bson_copy_to (&opts->extra, extra);
}

/**
 * mongoc_find_and_modify_opts_destroy:
 * @opts: A mongoc_find_and_modify_opts_t.
 *
 * Releases a mongoc_find_and_modify_opts_t and all associated memory.
 */
void
mongoc_find_and_modify_opts_destroy (mongoc_find_and_modify_opts_t *opts)
{
   if (opts) {
      _mongoc_bson_destroy_if_set (opts->sort);
      _mongoc_bson_destroy_if_set (opts->update);
      _mongoc_bson_destroy_if_set (opts->fields);
      bson_destroy (&opts->extra);
      bson_free (opts);
   }
}
