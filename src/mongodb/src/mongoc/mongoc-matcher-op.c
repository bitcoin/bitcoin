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


#include "mongoc-log.h"
#include "mongoc-matcher-op-private.h"
#include "mongoc-util-private.h"

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_exists_new --
 *
 *       Create a new op for checking {$exists: bool}.
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t that should be freed with
 *       _mongoc_matcher_op_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_matcher_op_t *
_mongoc_matcher_op_exists_new (const char *path, /* IN */
                               bool exists)      /* IN */
{
   mongoc_matcher_op_t *op;

   BSON_ASSERT (path);

   op = (mongoc_matcher_op_t *) bson_malloc0 (sizeof *op);
   op->exists.base.opcode = MONGOC_MATCHER_OPCODE_EXISTS;
   op->exists.path = bson_strdup (path);
   op->exists.exists = exists;

   return op;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_type_new --
 *
 *       Create a new op for checking {$type: int}.
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t that should be freed with
 *       _mongoc_matcher_op_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_matcher_op_t *
_mongoc_matcher_op_type_new (const char *path, /* IN */
                             bson_type_t type) /* IN */
{
   mongoc_matcher_op_t *op;

   BSON_ASSERT (path);
   BSON_ASSERT (type);

   op = (mongoc_matcher_op_t *) bson_malloc0 (sizeof *op);
   op->type.base.opcode = MONGOC_MATCHER_OPCODE_TYPE;
   op->type.path = bson_strdup (path);
   op->type.type = type;

   return op;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_logical_new --
 *
 *       Create a new op for checking any of:
 *
 *          {$or: []}
 *          {$nor: []}
 *          {$and: []}
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t that should be freed with
 *       _mongoc_matcher_op_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_matcher_op_t *
_mongoc_matcher_op_logical_new (mongoc_matcher_opcode_t opcode, /* IN */
                                mongoc_matcher_op_t *left,      /* IN */
                                mongoc_matcher_op_t *right)     /* IN */
{
   mongoc_matcher_op_t *op;

   BSON_ASSERT (left);
   BSON_ASSERT ((opcode >= MONGOC_MATCHER_OPCODE_OR) &&
                (opcode <= MONGOC_MATCHER_OPCODE_NOR));

   op = (mongoc_matcher_op_t *) bson_malloc0 (sizeof *op);
   op->logical.base.opcode = opcode;
   op->logical.left = left;
   op->logical.right = right;

   return op;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_compare_new --
 *
 *       Create a new op for checking any of:
 *
 *          {"abc": "def"}
 *          {$gt: {...}
 *          {$gte: {...}
 *          {$lt: {...}
 *          {$lte: {...}
 *          {$ne: {...}
 *          {$in: [...]}
 *          {$nin: [...]}
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t that should be freed with
 *       _mongoc_matcher_op_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_matcher_op_t *
_mongoc_matcher_op_compare_new (mongoc_matcher_opcode_t opcode, /* IN */
                                const char *path,               /* IN */
                                const bson_iter_t *iter)        /* IN */
{
   mongoc_matcher_op_t *op;

   BSON_ASSERT (path);
   BSON_ASSERT (iter);

   op = (mongoc_matcher_op_t *) bson_malloc0 (sizeof *op);
   op->compare.base.opcode = opcode;
   op->compare.path = bson_strdup (path);
   memcpy (&op->compare.iter, iter, sizeof *iter);

   return op;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_not_new --
 *
 *       Create a new op for checking {$not: {...}}
 *
 * Returns:
 *       A newly allocated mongoc_matcher_op_t that should be freed with
 *       _mongoc_matcher_op_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_matcher_op_t *
_mongoc_matcher_op_not_new (const char *path,           /* IN */
                            mongoc_matcher_op_t *child) /* IN */
{
   mongoc_matcher_op_t *op;

   BSON_ASSERT (path);
   BSON_ASSERT (child);

   op = (mongoc_matcher_op_t *) bson_malloc0 (sizeof *op);
   op->not_.base.opcode = MONGOC_MATCHER_OPCODE_NOT;
   op->not_.path = bson_strdup (path);
   op->not_.child = child;

   return op;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_destroy --
 *
 *       Free a mongoc_matcher_op_t structure and all children structures.
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
_mongoc_matcher_op_destroy (mongoc_matcher_op_t *op) /* IN */
{
   BSON_ASSERT (op);

   switch (op->base.opcode) {
   case MONGOC_MATCHER_OPCODE_EQ:
   case MONGOC_MATCHER_OPCODE_GT:
   case MONGOC_MATCHER_OPCODE_GTE:
   case MONGOC_MATCHER_OPCODE_IN:
   case MONGOC_MATCHER_OPCODE_LT:
   case MONGOC_MATCHER_OPCODE_LTE:
   case MONGOC_MATCHER_OPCODE_NE:
   case MONGOC_MATCHER_OPCODE_NIN:
      bson_free (op->compare.path);
      break;
   case MONGOC_MATCHER_OPCODE_OR:
   case MONGOC_MATCHER_OPCODE_AND:
   case MONGOC_MATCHER_OPCODE_NOR:
      if (op->logical.left)
         _mongoc_matcher_op_destroy (op->logical.left);
      if (op->logical.right)
         _mongoc_matcher_op_destroy (op->logical.right);
      break;
   case MONGOC_MATCHER_OPCODE_NOT:
      _mongoc_matcher_op_destroy (op->not_.child);
      bson_free (op->not_.path);
      break;
   case MONGOC_MATCHER_OPCODE_EXISTS:
      bson_free (op->exists.path);
      break;
   case MONGOC_MATCHER_OPCODE_TYPE:
      bson_free (op->type.path);
      break;
   default:
      break;
   }

   bson_free (op);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_exists_match --
 *
 *       Checks to see if @bson matches @exists requirements. The
 *       {$exists: bool} query can be either true or fase so we must
 *       handle false as "not exists".
 *
 * Returns:
 *       true if the field exists and the spec expected it.
 *       true if the field does not exist and the spec expected it to not
 *       exist.
 *       Otherwise, false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_exists_match (mongoc_matcher_op_exists_t *exists, /* IN */
                                 const bson_t *bson)                 /* IN */
{
   bson_iter_t iter;
   bson_iter_t desc;
   bool found;

   BSON_ASSERT (exists);
   BSON_ASSERT (bson);

   found = (bson_iter_init (&iter, bson) &&
            bson_iter_find_descendant (&iter, exists->path, &desc));

   return (found == exists->exists);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_type_match --
 *
 *       Checks if @bson matches the {$type: ...} op.
 *
 * Returns:
 *       true if the requested field was found and the type matched
 *       the requested type.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_type_match (mongoc_matcher_op_type_t *type, /* IN */
                               const bson_t *bson)             /* IN */
{
   bson_iter_t iter;
   bson_iter_t desc;

   BSON_ASSERT (type);
   BSON_ASSERT (bson);

   if (bson_iter_init (&iter, bson) &&
       bson_iter_find_descendant (&iter, type->path, &desc)) {
      return (bson_iter_type (&iter) == type->type);
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_not_match --
 *
 *       Checks if the {$not: ...} expression matches by negating the
 *       child expression.
 *
 * Returns:
 *       true if the child expression returned false.
 *       Otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_not_match (mongoc_matcher_op_not_t *not_, /* IN */
                              const bson_t *bson)            /* IN */
{
   BSON_ASSERT (not_);
   BSON_ASSERT (bson);

   return !_mongoc_matcher_op_match (not_->child, bson);
}


#define _TYPE_CODE(l, r) ((((int) (l)) << 8) | ((int) (r)))
#define _NATIVE_COMPARE(op, t1, t2) \
   (bson_iter##t2 (iter) op bson_iter##t1 (compare_iter))
#define _EQ_COMPARE(t1, t2) _NATIVE_COMPARE (==, t1, t2)
#define _NE_COMPARE(t1, t2) _NATIVE_COMPARE (!=, t1, t2)
#define _GT_COMPARE(t1, t2) _NATIVE_COMPARE (>, t1, t2)
#define _GTE_COMPARE(t1, t2) _NATIVE_COMPARE (>=, t1, t2)
#define _LT_COMPARE(t1, t2) _NATIVE_COMPARE (<, t1, t2)
#define _LTE_COMPARE(t1, t2) _NATIVE_COMPARE (<=, t1, t2)


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_iter_eq_match --
 *
 *       Performs equality match for all types on either left or right
 *       side of the equation.
 *
 *       We try to default to what the compiler would do for comparing
 *       things like integers. Therefore, we just have MACRO'tized
 *       everything so that the compiler sees the native values. (Such
 *       as (double == int64).
 *
 *       The _TYPE_CODE() stuff allows us to shove the type of the left
 *       and the right into a single integer and then do a jump table
 *       with a switch/case for all our supported types.
 *
 *       I imagine a bunch more of these will need to be added, so feel
 *       free to submit patches.
 *
 * Returns:
 *       true if the equality match succeeded.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_iter_eq_match (bson_iter_t *compare_iter, /* IN */
                               bson_iter_t *iter)         /* IN */
{
   int code;

   BSON_ASSERT (compare_iter);
   BSON_ASSERT (iter);

   code = _TYPE_CODE (bson_iter_type (compare_iter), bson_iter_type (iter));

   switch (code) {
   /* Double on Left Side */
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_DOUBLE):
      return _EQ_COMPARE (_double, _double);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_BOOL):
      return _EQ_COMPARE (_double, _bool);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT32):
      return _EQ_COMPARE (_double, _int32);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT64):
      return _EQ_COMPARE (_double, _int64);

   /* UTF8 on Left Side */
   case _TYPE_CODE (BSON_TYPE_UTF8, BSON_TYPE_UTF8): {
      uint32_t llen;
      uint32_t rlen;
      const char *lstr;
      const char *rstr;

      lstr = bson_iter_utf8 (compare_iter, &llen);
      rstr = bson_iter_utf8 (iter, &rlen);

      return ((llen == rlen) && (0 == memcmp (lstr, rstr, llen)));
   }

   /* Int32 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_DOUBLE):
      return _EQ_COMPARE (_int32, _double);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_BOOL):
      return _EQ_COMPARE (_int32, _bool);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT32):
      return _EQ_COMPARE (_int32, _int32);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT64):
      return _EQ_COMPARE (_int32, _int64);

   /* Int64 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_DOUBLE):
      return _EQ_COMPARE (_int64, _double);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_BOOL):
      return _EQ_COMPARE (_int64, _bool);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT32):
      return _EQ_COMPARE (_int64, _int32);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT64):
      return _EQ_COMPARE (_int64, _int64);

   /* Null on Left Side */
   case _TYPE_CODE (BSON_TYPE_NULL, BSON_TYPE_NULL):
   case _TYPE_CODE (BSON_TYPE_NULL, BSON_TYPE_UNDEFINED):
      return true;

   case _TYPE_CODE (BSON_TYPE_ARRAY, BSON_TYPE_ARRAY): {
      bson_iter_t left_array;
      bson_iter_t right_array;
      bson_iter_recurse (compare_iter, &left_array);
      bson_iter_recurse (iter, &right_array);

      while (true) {
         bool left_has_next = bson_iter_next (&left_array);
         bool right_has_next = bson_iter_next (&right_array);

         if (left_has_next != right_has_next) {
            /* different lengths */
            return false;
         }

         if (!left_has_next) {
            /* finished */
            return true;
         }

         if (!_mongoc_matcher_iter_eq_match (&left_array, &right_array)) {
            return false;
         }
      }
   }

   case _TYPE_CODE (BSON_TYPE_DOCUMENT, BSON_TYPE_DOCUMENT): {
      uint32_t llen;
      uint32_t rlen;
      const uint8_t *ldoc;
      const uint8_t *rdoc;

      bson_iter_document (compare_iter, &llen, &ldoc);
      bson_iter_document (iter, &rlen, &rdoc);

      return ((llen == rlen) && (0 == memcmp (ldoc, rdoc, llen)));
   }

   default:
      return false;
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_eq_match --
 *
 *       Performs equality match for all types on either left or right
 *       side of the equation.
 *
 * Returns:
 *       true if the equality match succeeded.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_eq_match (mongoc_matcher_op_compare_t *compare, /* IN */
                             bson_iter_t *iter)                    /* IN */
{
   BSON_ASSERT (compare);
   BSON_ASSERT (iter);

   return _mongoc_matcher_iter_eq_match (&compare->iter, iter);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_gt_match --
 *
 *       Perform {$gt: ...} match using @compare.
 *
 *       In general, we try to default to what the compiler would do
 *       for comparison between different types.
 *
 * Returns:
 *       true if the document field was > the spec value.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_gt_match (mongoc_matcher_op_compare_t *compare, /* IN */
                             bson_iter_t *iter)                    /* IN */
{
   int code;
   bson_iter_t *compare_iter = &compare->iter;

   BSON_ASSERT (compare);
   BSON_ASSERT (iter);

   code = _TYPE_CODE (bson_iter_type (compare_iter), bson_iter_type (iter));

   switch (code) {
   /* Double on Left Side */
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_DOUBLE):
      return _GT_COMPARE (_double, _double);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_BOOL):
      return _GT_COMPARE (_double, _bool);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT32):
      return _GT_COMPARE (_double, _int32);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT64):
      return _GT_COMPARE (_double, _int64);

   /* Int32 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_DOUBLE):
      return _GT_COMPARE (_int32, _double);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_BOOL):
      return _GT_COMPARE (_int32, _bool);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT32):
      return _GT_COMPARE (_int32, _int32);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT64):
      return _GT_COMPARE (_int32, _int64);

   /* Int64 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_DOUBLE):
      return _GT_COMPARE (_int64, _double);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_BOOL):
      return _GT_COMPARE (_int64, _bool);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT32):
      return _GT_COMPARE (_int64, _int32);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT64):
      return _GT_COMPARE (_int64, _int64);

   default:
      MONGOC_WARNING ("Implement for (Type(%d) > Type(%d))",
                      bson_iter_type (compare_iter),
                      bson_iter_type (iter));
      break;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_gte_match --
 *
 *       Perform a match of {"path": {"$gte": value}}.
 *
 * Returns:
 *       true if the the spec matches, otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_gte_match (mongoc_matcher_op_compare_t *compare, /* IN */
                              bson_iter_t *iter)                    /* IN */
{
   bson_iter_t *compare_iter;
   int code;

   BSON_ASSERT (compare);
   BSON_ASSERT (iter);

   compare_iter = &compare->iter;
   code = _TYPE_CODE (bson_iter_type (compare_iter), bson_iter_type (iter));

   switch (code) {
   /* Double on Left Side */
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_DOUBLE):
      return _GTE_COMPARE (_double, _double);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_BOOL):
      return _GTE_COMPARE (_double, _bool);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT32):
      return _GTE_COMPARE (_double, _int32);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT64):
      return _GTE_COMPARE (_double, _int64);

   /* Int32 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_DOUBLE):
      return _GTE_COMPARE (_int32, _double);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_BOOL):
      return _GTE_COMPARE (_int32, _bool);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT32):
      return _GTE_COMPARE (_int32, _int32);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT64):
      return _GTE_COMPARE (_int32, _int64);

   /* Int64 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_DOUBLE):
      return _GTE_COMPARE (_int64, _double);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_BOOL):
      return _GTE_COMPARE (_int64, _bool);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT32):
      return _GTE_COMPARE (_int64, _int32);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT64):
      return _GTE_COMPARE (_int64, _int64);

   default:
      MONGOC_WARNING ("Implement for (Type(%d) >= Type(%d))",
                      bson_iter_type (compare_iter),
                      bson_iter_type (iter));
      break;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_in_match --
 *
 *       Checks the spec {"path": {"$in": [value1, value2, ...]}}.
 *
 * Returns:
 *       true if the spec matched, otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_in_match (mongoc_matcher_op_compare_t *compare, /* IN */
                             bson_iter_t *iter)                    /* IN */
{
   mongoc_matcher_op_compare_t op;

   op.base.opcode = MONGOC_MATCHER_OPCODE_EQ;
   op.path = compare->path;

   if (!BSON_ITER_HOLDS_ARRAY (&compare->iter) ||
       !bson_iter_recurse (&compare->iter, &op.iter)) {
      return false;
   }

   while (bson_iter_next (&op.iter)) {
      if (_mongoc_matcher_op_eq_match (&op, iter)) {
         return true;
      }
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_lt_match --
 *
 *       Perform a {"path": "$lt": {value}} match.
 *
 * Returns:
 *       true if the spec matched, otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_lt_match (mongoc_matcher_op_compare_t *compare, /* IN */
                             bson_iter_t *iter)                    /* IN */
{
   bson_iter_t *compare_iter;
   int code;

   BSON_ASSERT (compare);
   BSON_ASSERT (iter);

   compare_iter = &compare->iter;
   code = _TYPE_CODE (bson_iter_type (compare_iter), bson_iter_type (iter));

   switch (code) {
   /* Double on Left Side */
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_DOUBLE):
      return _LT_COMPARE (_double, _double);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_BOOL):
      return _LT_COMPARE (_double, _bool);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT32):
      return _LT_COMPARE (_double, _int32);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT64):
      return _LT_COMPARE (_double, _int64);

   /* Int32 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_DOUBLE):
      return _LT_COMPARE (_int32, _double);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_BOOL):
      return _LT_COMPARE (_int32, _bool);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT32):
      return _LT_COMPARE (_int32, _int32);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT64):
      return _LT_COMPARE (_int32, _int64);

   /* Int64 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_DOUBLE):
      return _LT_COMPARE (_int64, _double);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_BOOL):
      return _LT_COMPARE (_int64, _bool);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT32):
      return _LT_COMPARE (_int64, _int32);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT64):
      return _LT_COMPARE (_int64, _int64);

   default:
      MONGOC_WARNING ("Implement for (Type(%d) < Type(%d))",
                      bson_iter_type (compare_iter),
                      bson_iter_type (iter));
      break;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_lte_match --
 *
 *       Perform a {"$path": {"$lte": value}} match.
 *
 * Returns:
 *       true if the spec matched, otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_lte_match (mongoc_matcher_op_compare_t *compare, /* IN */
                              bson_iter_t *iter)                    /* IN */
{
   bson_iter_t *compare_iter;
   int code;

   BSON_ASSERT (compare);
   BSON_ASSERT (iter);

   compare_iter = &compare->iter;
   code = _TYPE_CODE (bson_iter_type (compare_iter), bson_iter_type (iter));

   switch (code) {
   /* Double on Left Side */
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_DOUBLE):
      return _LTE_COMPARE (_double, _double);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_BOOL):
      return _LTE_COMPARE (_double, _bool);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT32):
      return _LTE_COMPARE (_double, _int32);
   case _TYPE_CODE (BSON_TYPE_DOUBLE, BSON_TYPE_INT64):
      return _LTE_COMPARE (_double, _int64);

   /* Int32 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_DOUBLE):
      return _LTE_COMPARE (_int32, _double);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_BOOL):
      return _LTE_COMPARE (_int32, _bool);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT32):
      return _LTE_COMPARE (_int32, _int32);
   case _TYPE_CODE (BSON_TYPE_INT32, BSON_TYPE_INT64):
      return _LTE_COMPARE (_int32, _int64);

   /* Int64 on Left Side */
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_DOUBLE):
      return _LTE_COMPARE (_int64, _double);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_BOOL):
      return _LTE_COMPARE (_int64, _bool);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT32):
      return _LTE_COMPARE (_int64, _int32);
   case _TYPE_CODE (BSON_TYPE_INT64, BSON_TYPE_INT64):
      return _LTE_COMPARE (_int64, _int64);

   default:
      MONGOC_WARNING ("Implement for (Type(%d) <= Type(%d))",
                      bson_iter_type (compare_iter),
                      bson_iter_type (iter));
      break;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_ne_match --
 *
 *       Perform a {"path": {"$ne": value}} match.
 *
 * Returns:
 *       true if the field "path" was not found or the value is not-equal
 *       to value.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_ne_match (mongoc_matcher_op_compare_t *compare, /* IN */
                             bson_iter_t *iter)                    /* IN */
{
   return !_mongoc_matcher_op_eq_match (compare, iter);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_nin_match --
 *
 *       Perform a {"path": {"$nin": value}} match.
 *
 * Returns:
 *       true if value was not found in the array at "path".
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_nin_match (mongoc_matcher_op_compare_t *compare, /* IN */
                              bson_iter_t *iter)                    /* IN */
{
   return !_mongoc_matcher_op_in_match (compare, iter);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_compare_match --
 *
 *       Dispatch function for mongoc_matcher_op_compare_t operations
 *       to perform a match.
 *
 * Returns:
 *       Opcode dependent.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_compare_match (mongoc_matcher_op_compare_t *compare, /* IN */
                                  const bson_t *bson)                   /* IN */
{
   bson_iter_t tmp;
   bson_iter_t iter;

   BSON_ASSERT (compare);
   BSON_ASSERT (bson);

   if (strchr (compare->path, '.')) {
      if (!bson_iter_init (&tmp, bson) ||
          !bson_iter_find_descendant (&tmp, compare->path, &iter)) {
         return false;
      }
   } else if (!bson_iter_init_find (&iter, bson, compare->path)) {
      return false;
   }

   switch ((int) compare->base.opcode) {
   case MONGOC_MATCHER_OPCODE_EQ:
      return _mongoc_matcher_op_eq_match (compare, &iter);
   case MONGOC_MATCHER_OPCODE_GT:
      return _mongoc_matcher_op_gt_match (compare, &iter);
   case MONGOC_MATCHER_OPCODE_GTE:
      return _mongoc_matcher_op_gte_match (compare, &iter);
   case MONGOC_MATCHER_OPCODE_IN:
      return _mongoc_matcher_op_in_match (compare, &iter);
   case MONGOC_MATCHER_OPCODE_LT:
      return _mongoc_matcher_op_lt_match (compare, &iter);
   case MONGOC_MATCHER_OPCODE_LTE:
      return _mongoc_matcher_op_lte_match (compare, &iter);
   case MONGOC_MATCHER_OPCODE_NE:
      return _mongoc_matcher_op_ne_match (compare, &iter);
   case MONGOC_MATCHER_OPCODE_NIN:
      return _mongoc_matcher_op_nin_match (compare, &iter);
   default:
      BSON_ASSERT (false);
      break;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_logical_match --
 *
 *       Dispatch function for mongoc_matcher_op_logical_t operations
 *       to perform a match.
 *
 * Returns:
 *       Opcode specific.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_matcher_op_logical_match (mongoc_matcher_op_logical_t *logical, /* IN */
                                  const bson_t *bson)                   /* IN */
{
   BSON_ASSERT (logical);
   BSON_ASSERT (bson);

   switch ((int) logical->base.opcode) {
   case MONGOC_MATCHER_OPCODE_OR:
      return (_mongoc_matcher_op_match (logical->left, bson) ||
              _mongoc_matcher_op_match (logical->right, bson));
   case MONGOC_MATCHER_OPCODE_AND:
      return (_mongoc_matcher_op_match (logical->left, bson) &&
              _mongoc_matcher_op_match (logical->right, bson));
   case MONGOC_MATCHER_OPCODE_NOR:
      return !(_mongoc_matcher_op_match (logical->left, bson) ||
               _mongoc_matcher_op_match (logical->right, bson));
   default:
      BSON_ASSERT (false);
      break;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_match --
 *
 *       Dispatch function for all operation types to perform a match.
 *
 * Returns:
 *       Opcode specific.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_matcher_op_match (mongoc_matcher_op_t *op, /* IN */
                          const bson_t *bson)      /* IN */
{
   BSON_ASSERT (op);
   BSON_ASSERT (bson);

   switch (op->base.opcode) {
   case MONGOC_MATCHER_OPCODE_EQ:
   case MONGOC_MATCHER_OPCODE_GT:
   case MONGOC_MATCHER_OPCODE_GTE:
   case MONGOC_MATCHER_OPCODE_IN:
   case MONGOC_MATCHER_OPCODE_LT:
   case MONGOC_MATCHER_OPCODE_LTE:
   case MONGOC_MATCHER_OPCODE_NE:
   case MONGOC_MATCHER_OPCODE_NIN:
      return _mongoc_matcher_op_compare_match (&op->compare, bson);
   case MONGOC_MATCHER_OPCODE_OR:
   case MONGOC_MATCHER_OPCODE_AND:
   case MONGOC_MATCHER_OPCODE_NOR:
      return _mongoc_matcher_op_logical_match (&op->logical, bson);
   case MONGOC_MATCHER_OPCODE_NOT:
      return _mongoc_matcher_op_not_match (&op->not_, bson);
   case MONGOC_MATCHER_OPCODE_EXISTS:
      return _mongoc_matcher_op_exists_match (&op->exists, bson);
   case MONGOC_MATCHER_OPCODE_TYPE:
      return _mongoc_matcher_op_type_match (&op->type, bson);
   default:
      break;
   }

   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_matcher_op_to_bson --
 *
 *       Convert the optree specified by @op to a bson document similar
 *       to what the query would have been. This is not perfectly the
 *       same, and so should not be used as such.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @bson is appended to, and therefore must be initialized before
 *       calling this function.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_matcher_op_to_bson (mongoc_matcher_op_t *op, /* IN */
                            bson_t *bson)            /* IN */
{
   const char *str;
   bson_t child;
   bson_t child2;

   BSON_ASSERT (op);
   BSON_ASSERT (bson);

   switch (op->base.opcode) {
   case MONGOC_MATCHER_OPCODE_EQ:
      _ignore_value (
         bson_append_iter (bson, op->compare.path, -1, &op->compare.iter));
      break;
   case MONGOC_MATCHER_OPCODE_GT:
   case MONGOC_MATCHER_OPCODE_GTE:
   case MONGOC_MATCHER_OPCODE_IN:
   case MONGOC_MATCHER_OPCODE_LT:
   case MONGOC_MATCHER_OPCODE_LTE:
   case MONGOC_MATCHER_OPCODE_NE:
   case MONGOC_MATCHER_OPCODE_NIN:
      switch ((int) op->base.opcode) {
      case MONGOC_MATCHER_OPCODE_GT:
         str = "$gt";
         break;
      case MONGOC_MATCHER_OPCODE_GTE:
         str = "$gte";
         break;
      case MONGOC_MATCHER_OPCODE_IN:
         str = "$in";
         break;
      case MONGOC_MATCHER_OPCODE_LT:
         str = "$lt";
         break;
      case MONGOC_MATCHER_OPCODE_LTE:
         str = "$lte";
         break;
      case MONGOC_MATCHER_OPCODE_NE:
         str = "$ne";
         break;
      case MONGOC_MATCHER_OPCODE_NIN:
         str = "$nin";
         break;
      default:
         str = "???";
         break;
      }
      if (bson_append_document_begin (bson, op->compare.path, -1, &child)) {
         _ignore_value (bson_append_iter (&child, str, -1, &op->compare.iter));
         bson_append_document_end (bson, &child);
      }
      break;
   case MONGOC_MATCHER_OPCODE_OR:
   case MONGOC_MATCHER_OPCODE_AND:
   case MONGOC_MATCHER_OPCODE_NOR:
      if (op->base.opcode == MONGOC_MATCHER_OPCODE_OR) {
         str = "$or";
      } else if (op->base.opcode == MONGOC_MATCHER_OPCODE_AND) {
         str = "$and";
      } else if (op->base.opcode == MONGOC_MATCHER_OPCODE_NOR) {
         str = "$nor";
      } else {
         BSON_ASSERT (false);
         str = NULL;
      }
      bson_append_array_begin (bson, str, -1, &child);
      bson_append_document_begin (&child, "0", 1, &child2);
      _mongoc_matcher_op_to_bson (op->logical.left, &child2);
      bson_append_document_end (&child, &child2);
      if (op->logical.right) {
         bson_append_document_begin (&child, "1", 1, &child2);
         _mongoc_matcher_op_to_bson (op->logical.right, &child2);
         bson_append_document_end (&child, &child2);
      }
      bson_append_array_end (bson, &child);
      break;
   case MONGOC_MATCHER_OPCODE_NOT:
      bson_append_document_begin (bson, op->not_.path, -1, &child);
      bson_append_document_begin (&child, "$not", 4, &child2);
      _mongoc_matcher_op_to_bson (op->not_.child, &child2);
      bson_append_document_end (&child, &child2);
      bson_append_document_end (bson, &child);
      break;
   case MONGOC_MATCHER_OPCODE_EXISTS:
      BSON_APPEND_BOOL (bson, "$exists", op->exists.exists);
      break;
   case MONGOC_MATCHER_OPCODE_TYPE:
      BSON_APPEND_INT32 (bson, "$type", (int) op->type.type);
      break;
   default:
      BSON_ASSERT (false);
      break;
   }
}
