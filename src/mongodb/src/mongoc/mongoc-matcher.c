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


#include <stdlib.h>

#include "mongoc-error.h"
#include "mongoc-matcher.h"
#include "mongoc-matcher-private.h"
#include "mongoc-matcher-op-private.h"


static mongoc_matcher_op_t *
_mongoc_matcher_parse_logical (mongoc_matcher_opcode_t opcode,
                               bson_iter_t *iter,
                               bool is_root,
                               bson_error_t *error);


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_parse_compare --
 *
 *       Parse a compare spec such as $gt or $in.
 *
 *       See the following link for more information.
 *
 *          http://docs.mongodb.org/manual/reference/operator/query/
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t if successful; otherwise
 *       NULL and @error is set.
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

static mongoc_matcher_op_t *
_mongoc_matcher_parse_compare (bson_iter_t *iter,   /* IN */
                               const char *path,    /* IN */
                               bson_error_t *error) /* OUT */
{
   const char *key;
   mongoc_matcher_op_t *op = NULL, *op_child;
   bson_iter_t child;

   BSON_ASSERT (iter);
   BSON_ASSERT (path);

   if (bson_iter_type (iter) == BSON_TYPE_DOCUMENT) {
      if (!bson_iter_recurse (iter, &child) || !bson_iter_next (&child)) {
         bson_set_error (error,
                         MONGOC_ERROR_MATCHER,
                         MONGOC_ERROR_MATCHER_INVALID,
                         "Document contains no operations.");
         return NULL;
      }

      key = bson_iter_key (&child);

      if (key[0] != '$') {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_EQ, path, iter);
      } else if (strcmp (key, "$not") == 0) {
         if (!(op_child =
                  _mongoc_matcher_parse_compare (&child, path, error))) {
            return NULL;
         }
         op = _mongoc_matcher_op_not_new (path, op_child);
      } else if (strcmp (key, "$gt") == 0) {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_GT, path, &child);
      } else if (strcmp (key, "$gte") == 0) {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_GTE, path, &child);
      } else if (strcmp (key, "$in") == 0) {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_IN, path, &child);
      } else if (strcmp (key, "$lt") == 0) {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_LT, path, &child);
      } else if (strcmp (key, "$lte") == 0) {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_LTE, path, &child);
      } else if (strcmp (key, "$ne") == 0) {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_NE, path, &child);
      } else if (strcmp (key, "$nin") == 0) {
         op = _mongoc_matcher_op_compare_new (
            MONGOC_MATCHER_OPCODE_NIN, path, &child);
      } else if (strcmp (key, "$exists") == 0) {
         op = _mongoc_matcher_op_exists_new (path, bson_iter_bool (&child));
      } else if (strcmp (key, "$type") == 0) {
         op = _mongoc_matcher_op_type_new (path, bson_iter_type (&child));
      } else {
         bson_set_error (error,
                         MONGOC_ERROR_MATCHER,
                         MONGOC_ERROR_MATCHER_INVALID,
                         "Invalid operator \"%s\"",
                         key);
         return NULL;
      }
   } else {
      op =
         _mongoc_matcher_op_compare_new (MONGOC_MATCHER_OPCODE_EQ, path, iter);
   }

   BSON_ASSERT (op);

   return op;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_parse --
 *
 *       Parse a query spec observed by the current key of @iter.
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t if successful; otherwise
 *       NULL an @error is set.
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

static mongoc_matcher_op_t *
_mongoc_matcher_parse (bson_iter_t *iter,   /* IN */
                       bson_error_t *error) /* OUT */
{
   bson_iter_t child;
   const char *key;

   BSON_ASSERT (iter);

   key = bson_iter_key (iter);

   if (*key != '$') {
      return _mongoc_matcher_parse_compare (iter, key, error);
   } else {
      BSON_ASSERT (bson_iter_type (iter) == BSON_TYPE_ARRAY);

      if (!bson_iter_recurse (iter, &child)) {
         bson_set_error (error,
                         MONGOC_ERROR_MATCHER,
                         MONGOC_ERROR_MATCHER_INVALID,
                         "Invalid value for operator \"%s\"",
                         key);
         return NULL;
      }

      if (strcmp (key, "$or") == 0) {
         return _mongoc_matcher_parse_logical (
            MONGOC_MATCHER_OPCODE_OR, &child, false, error);
      } else if (strcmp (key, "$and") == 0) {
         return _mongoc_matcher_parse_logical (
            MONGOC_MATCHER_OPCODE_AND, &child, false, error);
      } else if (strcmp (key, "$nor") == 0) {
         return _mongoc_matcher_parse_logical (
            MONGOC_MATCHER_OPCODE_NOR, &child, false, error);
      }
   }

   bson_set_error (error,
                   MONGOC_ERROR_MATCHER,
                   MONGOC_ERROR_MATCHER_INVALID,
                   "Invalid operator \"%s\"",
                   key);

   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_parse_logical --
 *
 *       Parse a query spec containing a logical operator such as
 *       $or, $and, $not, and $nor.
 *
 *       See the following link for more information.
 *
 *       http://docs.mongodb.org/manual/reference/operator/query/
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t if successful; otherwise
 *       NULL and @error is set.
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

static mongoc_matcher_op_t *
_mongoc_matcher_parse_logical (mongoc_matcher_opcode_t opcode, /* IN */
                               bson_iter_t *iter,              /* IN */
                               bool is_root,                   /* IN */
                               bson_error_t *error)            /* OUT */
{
   mongoc_matcher_op_t *left;
   mongoc_matcher_op_t *right;
   mongoc_matcher_op_t *more;
   mongoc_matcher_op_t *more_wrap;
   bson_iter_t child;

   BSON_ASSERT (opcode);
   BSON_ASSERT (iter);
   BSON_ASSERT (iter);

   if (!bson_iter_next (iter)) {
      bson_set_error (error,
                      MONGOC_ERROR_MATCHER,
                      MONGOC_ERROR_MATCHER_INVALID,
                      "Invalid logical operator.");
      return NULL;
   }

   if (is_root) {
      if (!(left = _mongoc_matcher_parse (iter, error))) {
         return NULL;
      }
   } else {
      if (!BSON_ITER_HOLDS_DOCUMENT (iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_MATCHER,
                         MONGOC_ERROR_MATCHER_INVALID,
                         "Expected document in value.");
         return NULL;
      }

      bson_iter_recurse (iter, &child);
      bson_iter_next (&child);

      if (!(left = _mongoc_matcher_parse (&child, error))) {
         return NULL;
      }
   }

   if (!bson_iter_next (iter)) {
      return left;
   }

   if (is_root) {
      if (!(right = _mongoc_matcher_parse (iter, error))) {
         return NULL;
      }
   } else {
      if (!BSON_ITER_HOLDS_DOCUMENT (iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_MATCHER,
                         MONGOC_ERROR_MATCHER_INVALID,
                         "Expected document in value.");
         return NULL;
      }

      bson_iter_recurse (iter, &child);
      bson_iter_next (&child);

      if (!(right = _mongoc_matcher_parse (&child, error))) {
         return NULL;
      }
   }

   more = _mongoc_matcher_parse_logical (opcode, iter, is_root, error);

   if (more) {
      more_wrap = _mongoc_matcher_op_logical_new (opcode, right, more);
      return _mongoc_matcher_op_logical_new (opcode, left, more_wrap);
   }

   return _mongoc_matcher_op_logical_new (opcode, left, right);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_matcher_new --
 *
 *       Create a new mongoc_matcher_t using the query specification
 *       provided in @query.
 *
 *       This will build an operation tree that can be applied to arbitrary
 *       bson documents using mongoc_matcher_match().
 *
 * Returns:
 *       A newly allocated mongoc_matcher_t if successful; otherwise NULL
 *       and @error is set.
 *
 *       The mongoc_matcher_t should be freed with
 *       mongoc_matcher_destroy().
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

mongoc_matcher_t *
mongoc_matcher_new (const bson_t *query, /* IN */
                    bson_error_t *error) /* OUT */
{
   mongoc_matcher_op_t *op;
   mongoc_matcher_t *matcher;
   bson_iter_t iter;

   BSON_ASSERT (query);

   matcher = (mongoc_matcher_t *) bson_malloc0 (sizeof *matcher);
   bson_copy_to (query, &matcher->query);

   if (!bson_iter_init (&iter, &matcher->query)) {
      goto failure;
   }

   if (!(op = _mongoc_matcher_parse_logical (
            MONGOC_MATCHER_OPCODE_AND, &iter, true, error))) {
      goto failure;
   }

   matcher->optree = op;

   return matcher;

failure:
   bson_destroy (&matcher->query);
   bson_free (matcher);
   return NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_matcher_match --
 *
 *       Checks to see if @bson matches the query specified when creating
 *       @matcher.
 *
 * Returns:
 *       TRUE if @bson matched the query, otherwise FALSE.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_matcher_match (const mongoc_matcher_t *matcher, /* IN */
                      const bson_t *document)          /* IN */
{
   BSON_ASSERT (matcher);
   BSON_ASSERT (matcher->optree);
   BSON_ASSERT (document);

   return _mongoc_matcher_op_match (matcher->optree, document);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_matcher_destroy --
 *
 *       Release all resources associated with @matcher.
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
mongoc_matcher_destroy (mongoc_matcher_t *matcher) /* IN */
{
   BSON_ASSERT (matcher);

   _mongoc_matcher_op_destroy (matcher->optree);
   bson_destroy (&matcher->query);
   bson_free (matcher);
}
